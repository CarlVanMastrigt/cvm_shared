/**
Copyright 2021 Carl van Mastrigt

This file is part of cvm_shared.

cvm_shared is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvm_shared is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with cvm_shared.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cvm_shared.h"

void cvm_vk_create_image_atlas(cvm_vk_image_atlas * ia,uint32_t width,uint32_t height,bool multithreaded)
{
    uint32_t i,j;

    if(width<1<<CVM_VK_BASE_TILE_SIZE_FACTOR||height<1<<CVM_VK_BASE_TILE_SIZE_FACTOR)
    {
        fprintf(stderr,"IMAGE ATLAS TOO SMALL\n");
        exit(-1);
    }

    for(i=0;width>1<<i;i++);
    ia->num_tile_sizes_h=i-CVM_VK_BASE_TILE_SIZE_FACTOR+1;
    ia->width=1<<i;

    for(i=0;height>1<<i;i++);
    ia->num_tile_sizes_v=i-CVM_VK_BASE_TILE_SIZE_FACTOR+1;
    ia->height=1<<i;

    ia->multithreaded=multithreaded;
    atomic_init(&ia->spinlock,0);

    cvm_vk_image_atlas_tile * tiles=malloc(CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT*sizeof(cvm_vk_image_atlas_tile));

    cvm_vk_image_atlas_tile * next=NULL;
    i=CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT;
    while(i--)
    {
        tiles[i].next_h=next;
        tiles[i].is_base=!i;
        next=tiles+i;
    }

    ia->first_unused_tile=tiles;
    ia->unused_tile_count=CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT;

    ia->available_tiles=malloc(ia->num_tile_sizes_h*sizeof(cvm_vk_available_atlas_tile_heap*));

    for(i=0;i<ia->num_tile_sizes_h;i++)
    {
        ia->available_tiles[i]=malloc(ia->num_tile_sizes_v*sizeof(cvm_vk_available_atlas_tile_heap));

        for(j=0;j<ia->num_tile_sizes_v;i++)
        {
            ia->available_tiles[i][j].heap=malloc(4*sizeof(cvm_vk_image_atlas_tile*));
            ia->available_tiles[i][j].space=4;
            ia->available_tiles[i][j].count=0;
        }
    }

    for(i=0;i<16;i++)ia->available_tiles_bitmasks[i]=0;

    ///starts off as one big tile

    cvm_vk_image_atlas_tile * t=ia->first_unused_tile;
    ia->first_unused_tile=t->next_h;
    ia->unused_tile_count--;

    t->prev_h=NULL;
    t->next_h=NULL;

    t->prev_v=NULL;
    t->next_v=NULL;

    t->offset=0;

    t->heap_index=0;
    t->size_factor_h=ia->num_tile_sizes_h-1;
    t->size_factor_v=ia->num_tile_sizes_v-1;

    t->available=true;

    ia->available_tiles[t->size_factor_h][t->size_factor_v].heap[0]=t;
    ia->available_tiles[t->size_factor_h][t->size_factor_v].count=1;

    ia->available_tiles_bitmasks[t->size_factor_h]=1<<t->size_factor_v;
}

void cvm_vk_destroy_image_atlas(cvm_vk_image_atlas * ia)
{
    uint32_t i,j;
    cvm_vk_image_atlas_tile *first,*last,*current,*next;

    if(ia->available_tiles[ia->num_tile_sizes_h-1][ia->num_tile_sizes_v-1].count!=1)
    {
        fprintf(stderr,"TRYING TO DESTROY AN IMAGE ATLAS WITH ACTIVE TILES\n");
        exit(-1);
        ///could instead just rightmost to offload all active buffers to the unused linked list
    }

    for(i=0;i<ia->num_tile_sizes_h;i++)
    {
        for(j=0;j<ia->num_tile_sizes_v;j++)
        {
            free(ia->available_tiles[i][j].heap);
        }

        free(ia->available_tiles[i]);
    }

    free(ia->available_tiles);


    first=last=NULL;
    ///get all base tiles in their own linked list, discarding all others
    for(current=ia->first_unused_tile;current;current=next)
    {
        next=current->next_h;
        if(current->is_base)
        {
            if(last) last=last->next_h=current;
            else first=last=current;
        }
    }
    last->next_h=NULL;

    for(current=first;current;current=next)
    {
        next=current->next_h;
        free(current);///free all base allocations (to mathch appropriate allocs)
    }
}

///probably worth having this as function, allows calling from branches

//static void split_tile();

cvm_vk_image_atlas_tile * cvm_vk_acquire_image_atlas_tile(cvm_vk_image_atlas * ia,uint32_t width,uint32_t height)
{
    uint32_t size_h,size_v,parent_size_h,parent_size_v,i,j,split_count,c,u,d;
    cvm_vk_image_atlas_tile *p,*n,*parent;
    uint_fast32_t lock;
    cvm_vk_image_atlas_tile ** heap;

    for(size_h=0;width>1<<(size_h+CVM_VK_BASE_TILE_SIZE_FACTOR);size_h++);
    for(size_v=0;height>1<<(size_v+CVM_VK_BASE_TILE_SIZE_FACTOR);size_v++);

    if(size_h>=ia->num_tile_sizes_h || size_v>=ia->num_tile_sizes_v)return NULL;///i dont expect this to get hit so dont bother doing earlier easier test

    if(ia->multithreaded)do lock=atomic_load(&ia->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&ia->spinlock,&lock,1));

    if(ia->unused_tile_count < ia->num_tile_sizes_h+ ia->num_tile_sizes_v)
    {
        p=malloc(CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT*sizeof(cvm_vk_image_atlas_tile));

        n=ia->first_unused_tile;
        i=CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT;
        while(i--)
        {
            p[i].next_h=n;
            p[i].is_base=!i;
            n=p+i;
        }

        ia->first_unused_tile=p;
        ia->unused_tile_count+=CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT;
    }

    ///figure out which tile size to use, squareize first then give prefernce to either v or h
    ///as squarising is preferred (?), start by testing anything is available in h/v presently used

    ///bias towards longer tiles vertically as this will support glyphs better



    //ts_h=s_h;
    //ts_v=s_v;

    //bool test_h=s_h>s_v;///slight bias towards vertical splits when s_v==s_h

    split_count=33;///larger than maximum supported split count (32)

    for(i=size_h;i<ia->num_tile_sizes_h;i++)
    {
        if(ia->available_tiles_bitmasks[i]>=1<<size_v)
        {
            for(j=size_v;!(ia->available_tiles_bitmasks[i] & 1<<j);j++);///get lowest bitmasked entry
            if(i-size_h + j-size_v < split_count)
            {
                split_count=i-size_h + j-size_v;
                parent_size_h=i;
                parent_size_v=j;
            }
        }
    }

    if(split_count==33)///a tile to split was not found!
    {
        if(ia->multithreaded) atomic_store(&ia->spinlock,0);
        return NULL;///no tile large enough exists
    }

    ///extract p from heap
    heap=ia->available_tiles[parent_size_h][parent_size_v].heap;

    p=heap[0];

    if((c= --ia->available_tiles[parent_size_h][parent_size_v].count))
    {
        u=0;

        while((d=(u<<1)+1)<c)
        {
            d+= (d+1<c) && (heap[d+1]->offset < heap[d]->offset);
            if(heap[c]->offset < heap[d]->offset) break;
            heap[d]->heap_index=u;///need to know (new) index in heap
            heap[u]=heap[d];
            u=d;
        }

        heap[c]->heap_index=u;
        heap[u]=heap[c];
    }
    else ia->available_tiles_bitmasks[parent_size_h]&= ~(1<<parent_size_v);

    while(parent_size_h>size_h && parent_size_v>size_v)
    {
        n=ia->first_unused_tile;
        ia->first_unused_tile=n->next_h;
        ia->unused_tile_count--;

        n->available=true;
        n->heap_index=0;///must be no tiles already in heap this tile will be put in (otherwise we'd be splitting that one)

        if(parent_size_h != size_h && (parent_size_v==size_v || parent_size_h>parent_size_v))
        {
            ///split vertically
            n->size_factor_h= --p->size_factor_h;
        }
        else
        {
            ///split horizontally
        }
    }


//    while(ia->available_tiles[ts_h][ts_v].count==0)///bitmask may be unhelpful...
//    {
//        ///instead of JUST preferring squarisation, prefernce fewer splits
//
//        ///mash CAN work well! just needs to be 16 uint array (with size of 16) for hast loads, still use same search algorithm for best fit tile to split
//
//        /// cant just iterate through available stepping through v or h, need to check all available with same req. split count, v/h bias informs search traversal direction?
//
//
//        /// this paradigm will result in double checks of bitmask if only one of ts_h & ts_v change...
////        if(ts_h>ts_v)
////        {
////            if(ia->available_tiles_bitmasks_h[ts_h]>1<<s_v)///prefer h
////            {
////                ///found tile
////            }
////            else if(ia->available_tiles_bitmasks_v[ts_v]>1<<s_h)
////            {
////                ///found tile
////            }
////            else
////            {
////                ///increment
////            }
////        }
////        else
////        {
////        }
//    }



}


