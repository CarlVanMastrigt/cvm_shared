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

void cvm_vk_create_image_atlas(cvm_vk_image_atlas * ia,VkImage image,VkImageView image_view,uint32_t width,uint32_t height,bool multithreaded)
{
    uint32_t i,j;

    if(width<1<<CVM_VK_BASE_TILE_SIZE_FACTOR||height<1<<CVM_VK_BASE_TILE_SIZE_FACTOR)
    {
        fprintf(stderr,"IMAGE ATLAS TOO SMALL\n");
        exit(-1);
    }

    ia->image=image;
    ia->image_view=image_view;

    if(width & (width-1) || height & (height-1))
    {
        fprintf(stderr,"IMAGE ATLAS MUST BE CREATED AT POWER OF 2 WIDTH AND HEIGHT\n");
        exit(-1);
    }

    for(i=0;width>1<<i;i++);
    ia->num_tile_sizes_h=i-CVM_VK_BASE_TILE_SIZE_FACTOR+1;

    for(i=0;height>1<<i;i++);
    ia->num_tile_sizes_v=i-CVM_VK_BASE_TILE_SIZE_FACTOR+1;

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

        for(j=0;j<ia->num_tile_sizes_v;j++)
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

    t->x_pos=0;
    t->y_pos=0;

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

    first=ia->available_tiles[ia->num_tile_sizes_h-1][ia->num_tile_sizes_v-1].heap[0];///return the last, base tile to the list to be processed (unused tiles)
    first->next_h=ia->first_unused_tile;
    ia->first_unused_tile=first;

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
    uint32_t size_h,size_v,i,j,c,u,d,o,o_mid,o_end;///a lot of these could be recycled...
    cvm_vk_image_atlas_tile *base,*partner,*adjacent;
    uint_fast32_t lock;
    cvm_vk_image_atlas_tile ** heap;

    for(size_h=0;width>1<<(size_h+CVM_VK_BASE_TILE_SIZE_FACTOR);size_h++);
    for(size_v=0;height>1<<(size_v+CVM_VK_BASE_TILE_SIZE_FACTOR);size_v++);

    if(size_h>=ia->num_tile_sizes_h || size_v>=ia->num_tile_sizes_v)return NULL;///i dont expect this to get hit so dont bother doing earlier easier test

    if(ia->multithreaded)do lock=atomic_load(&ia->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&ia->spinlock,&lock,1));

    if(ia->unused_tile_count < ia->num_tile_sizes_h+ ia->num_tile_sizes_v)
    {
        base=malloc(CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT*sizeof(cvm_vk_image_atlas_tile));

        partner=ia->first_unused_tile;
        i=CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT;
        while(i--)
        {
            base[i].next_h=partner;
            base[i].is_base=!i;
            partner=base+i;
        }

        ia->first_unused_tile=base;
        ia->unused_tile_count+=CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT;
    }


    base=NULL;

    for(i=size_h;i<ia->num_tile_sizes_h;i++)
    {
        if(ia->available_tiles_bitmasks[i]>=1<<size_v)
        {
            for(j=size_v;!(ia->available_tiles_bitmasks[i] & 1<<j);j++);///get lowest bitmasked entry
            if(!base || base->size_factor_h+base->size_factor_v > i+j)
            {
                base=ia->available_tiles[i][j].heap[0];
            }
        }
    }

    if(!base)///a tile to split was not found!
    {
        if(ia->multithreaded) atomic_store(&ia->spinlock,0);
        return NULL;///no tile large enough exists
    }

    ///extract from heap
    heap=ia->available_tiles[base->size_factor_h][base->size_factor_v].heap;

    if((c= --ia->available_tiles[base->size_factor_h][base->size_factor_v].count))
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
    else ia->available_tiles_bitmasks[base->size_factor_h]&= ~(1<<base->size_factor_v);///took last tile of this size


    while(base->size_factor_h>size_h || base->size_factor_v>size_v)
    {
        partner=ia->first_unused_tile;
        ia->first_unused_tile=partner->next_h;
        ia->unused_tile_count--;

        ///here if offset(o) isnt equal to relevant adjacent position then adjacent is larger and starts before the relevant (v/h) block that contains the tile we're splitting

        if(base->size_factor_h != size_h && (base->size_factor_v==size_v || base->size_factor_h>base->size_factor_v))///try to keep tiles square where possible, with slight bias towards longer thinner tiles
        {
            ///split vertically
            base->size_factor_h--;

            partner->offset=base->offset;
            partner->x_pos+=1<<base->size_factor_h;

            adjacent=partner->next_h=base->next_h;
            partner->prev_h=base;
            base->next_h=partner;

            ///adjust adjacency info, traverse all edges finding tiles that should link to partner now

            ///traverse right edge
            o=base->y_pos;
            o_end=o+(1<<base->size_factor_v);
            if(adjacent && adjacent->y_pos == o) while(o < o_end)
            {
                adjacent->prev_h=partner;
                o+=1<<adjacent->size_factor_v;
                adjacent=adjacent->next_v;
            }

            ///traverse bottom edge
            adjacent=partner->next_v=base->next_v;
            o=base->x_pos;
            o_mid=o+(1<<base->size_factor_h);
            o_end=o_mid+(1<<base->size_factor_h);
            if(adjacent && adjacent->x_pos == o) while(o < o_end)
            {
                if(o==o_mid)partner->next_v=adjacent;
                if(o>=o_mid)adjacent->prev_v=partner;
                o+=1<<adjacent->size_factor_h;
                adjacent=adjacent->next_h;
            }

            ///traverse top edge
            adjacent=partner->prev_v=base->prev_v;
            o=base->x_pos;
            /// o_mid and o_end unchanged
            if(adjacent && adjacent->x_pos == o) while(o < o_end)
            {
                while(adjacent->next_v!=base) adjacent=adjacent->next_v;
                if(o==o_mid)partner->prev_v=adjacent;
                if(o>=o_mid)adjacent->next_v=partner;
                o+=1<<adjacent->size_factor_h;
                adjacent=adjacent->next_h;
            }
        }
        else
        {
            ///split horizontally
            base->size_factor_v--;

            partner->offset=base->offset;
            partner->y_pos+=1<<base->size_factor_v;

            adjacent=partner->next_v=base->next_v;
            partner->prev_v=base;
            base->next_v=partner;

            ///adjust adjacency info, traverse all edges finding tiles that should link to partner now

            ///traverse bottom edge
            o=base->x_pos;
            o_end=o+(1<<base->size_factor_h);
            if(adjacent && adjacent->x_pos == o) while(o < o_end)
            {
                adjacent->prev_v=partner;
                o+=1<<adjacent->size_factor_h;
                adjacent=adjacent->next_h;
            }

            ///traverse right edge
            adjacent=partner->next_h=base->next_h;
            o=base->y_pos;
            o_mid=o+(1<<base->size_factor_v);
            o_end=o_mid+(1<<base->size_factor_v);
            if(adjacent && adjacent->y_pos == o) while(o < o_end)
            {
                if(o==o_mid)partner->next_h=adjacent;
                if(o>=o_mid)adjacent->prev_h=partner;
                o+=1<<adjacent->size_factor_v;
                adjacent=adjacent->next_v;
            }

            ///traverse left edge
            adjacent=partner->prev_h=base->prev_h;
            o=base->y_pos;
            /// o_mid and o_end unchanged
            if(adjacent && adjacent->y_pos == o) while(o < o_end)
            {
                while(adjacent->next_h!=base) adjacent=adjacent->next_h; /// not the case when t starts before o
                if(o==o_mid)partner->prev_h=adjacent;
                if(o>=o_mid)adjacent->next_h=partner;
                o+=1<<adjacent->size_factor_v;
                adjacent=adjacent->next_v;
            }
        }

        partner->heap_index=0;///must be no tiles already in heap this tile will be put in (otherwise we'd be splitting that one)
        ia->available_tiles[base->size_factor_h][base->size_factor_v].heap[0]=partner;
        ia->available_tiles[base->size_factor_h][base->size_factor_v].count=1;

        ia->available_tiles_bitmasks[base->size_factor_h] |= 1<<base->size_factor_v;

        partner->available=true;
        partner->size_factor_h=base->size_factor_h;
        partner->size_factor_v=base->size_factor_v;
    }

    base->available=false;

    if(ia->multithreaded) atomic_store(&ia->spinlock,0);
    return base;
}

static inline void remove_tile_from_availability_heap(cvm_vk_image_atlas * ia,cvm_vk_image_atlas_tile * t,uint32_t s_h,uint32_t s_v)
{
    cvm_vk_image_atlas_tile ** heap;
    uint32_t c,u,d;

    if((c= --ia->available_tiles[s_h][s_v].count))
    {
        heap=ia->available_tiles[s_h][s_v].heap;

        d=t->heap_index;
        while(d && heap[c]->offset < heap[(u=(d-1)>>1)]->offset)///try moving up heap
        {
            heap[u]->heap_index=d;
            heap[d]=heap[u];
            d=u;
        }
        u=d;
        while((d=(u<<1)+1)<c)///try moving down heap
        {
            d+= d+1<c && heap[d+1]->offset < heap[d]->offset;
            if(heap[c]->offset < heap[d]->offset) break;
            heap[d]->heap_index=u;
            heap[u]=heap[d];
            u=d;
        }

        heap[c]->heap_index=u;
        heap[u]=heap[c];
    }
    else ia->available_tiles_bitmasks[s_h]&= ~(1<<s_v);///partner was last available tile of this size


    ///add to list of unused tiles
    t->next_h=ia->first_unused_tile;
    ia->first_unused_tile=t;
    ia->unused_tile_count++;
}

void cvm_vk_relinquish_image_atlas_tile(cvm_vk_image_atlas * ia,cvm_vk_image_atlas_tile * base)
{
    uint32_t u,d,o,o_end;///a lot of these could be recycled...  current_size_h,current_size_v,
    cvm_vk_image_atlas_tile *partner,*adjacent;
    uint_fast32_t lock;
    cvm_vk_image_atlas_tile ** heap;
    bool finished_h,finished_v;

    if(!base)return;

    ///base,partner,test

    if(ia->multithreaded)do lock=atomic_load(&ia->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&ia->spinlock,&lock,1));

    finished_h=finished_v=false;

    do
    {
        ///combine horizontally adjacent
        while(!finished_h && (base->size_factor_h<=base->size_factor_v || finished_v))
        {
            if(base->x_pos & 1<<base->size_factor_h)///base is right of current horizontal tile pair
            {
                if((partner=base->prev_h) && partner->size_factor_h==base->size_factor_h && partner->size_factor_v==base->size_factor_v && partner->available)
                {
                    base->offset=partner->offset;
                    ///traverse left side
                    adjacent=base->prev_h=partner->prev_h;
                    o=partner->y_pos;
                    o_end=o+(1<<base->size_factor_v);
                    if(adjacent && adjacent->y_pos==o) while(o < o_end)
                    {
                        while(adjacent->next_h!=partner)adjacent=adjacent->next_h;
                        adjacent->next_h=base;
                        o+=1<<adjacent->size_factor_v;
                        adjacent=adjacent->next_v;
                    }

                    ///traverse bottom side
                    adjacent=base->next_v=partner->next_v;
                    o=partner->x_pos;
                    o_end=o+(1<<base->size_factor_h);
                    if(adjacent && adjacent->x_pos==o) while(o < o_end)
                    {
                        adjacent->prev_v=base;
                        o+=1<<adjacent->size_factor_h;
                        adjacent=adjacent->next_h;
                    }

                    ///traverse top side
                    adjacent=base->prev_v=partner->prev_v;
                    o=partner->x_pos;
                    ///o_end hasn't changed
                    if(adjacent && adjacent->x_pos==o) while(o < o_end)
                    {
                        while(adjacent->next_v!=partner)adjacent=adjacent->next_v;
                        adjacent->next_v=base;
                        o+=1<<adjacent->size_factor_h;
                        adjacent=adjacent->next_h;
                    }

                    remove_tile_from_availability_heap(ia,partner,base->size_factor_h,base->size_factor_v);

                    base->size_factor_h++;

                    finished_v=false;///new combination potential
                }
                else finished_h=true;
            }
            else///base is left of current horizontal tile pair
            {
                if((partner=base->next_h) && partner->size_factor_h==base->size_factor_h && partner->size_factor_v==base->size_factor_v && partner->available)
                {
                    ///traverse right side
                    adjacent=base->next_h=partner->next_h;
                    o=partner->y_pos;
                    o_end=o+(1<<base->size_factor_v);
                    if(adjacent && adjacent->y_pos==o) while(o < o_end)
                    {
                        adjacent->prev_h=base;
                        o+=1<<adjacent->size_factor_v;
                        adjacent=adjacent->next_v;
                    }

                    ///traverse bottom side
                    adjacent=partner->next_v;
                    o=partner->x_pos;
                    o_end=o+(1<<base->size_factor_h);
                    if(adjacent && adjacent->x_pos==o) while(o < o_end)
                    {
                        adjacent->prev_v=base;
                        o+=1<<adjacent->size_factor_h;
                        adjacent=adjacent->next_h;
                    }

                    ///traverse top side
                    adjacent=partner->prev_v;
                    o=partner->x_pos;
                    ///o_end hasn't changed
                    if(adjacent && adjacent->x_pos==o) while(o < o_end)
                    {
                        while(adjacent->next_v!=partner)adjacent=adjacent->next_v;
                        adjacent->next_v=base;
                        o+=1<<adjacent->size_factor_h;
                        adjacent=adjacent->next_h;
                    }

                    remove_tile_from_availability_heap(ia,partner,base->size_factor_h,base->size_factor_v);

                    base->size_factor_h++;

                    finished_v=false;///new combination potential
                }
                else finished_h=true;
            }
        }

        ///combine vertically adjacent
        while(!finished_v && (base->size_factor_v<=base->size_factor_h || finished_h))
        {
            if(base->y_pos & 1<<base->size_factor_v)///base is bottom of current vertical tile pair
            {
                if((partner=base->prev_v) && partner->size_factor_h==base->size_factor_h && partner->size_factor_v==base->size_factor_v && partner->available)
                {
                    base->offset=partner->offset;
                    ///traverse top side
                    adjacent=base->prev_v=partner->prev_v;
                    o=partner->x_pos;
                    o_end=o+(1<<base->size_factor_h);
                    if(adjacent && adjacent->x_pos==o) while(o < o_end)
                    {
                        while(adjacent->next_v!=partner)adjacent=adjacent->next_v;
                        adjacent->next_v=base;
                        o+=1<<adjacent->size_factor_h;
                        adjacent=adjacent->next_h;
                    }

                    ///traverse left side
                    adjacent=base->prev_h=partner->prev_h;
                    o=partner->y_pos;
                    o_end=o+(1<<base->size_factor_v);
                    if(adjacent && adjacent->y_pos==o) while(o < o_end)
                    {
                        while(adjacent->next_h!=partner)adjacent=adjacent->next_h;
                        adjacent->next_h=base;
                        o+=1<<adjacent->size_factor_v;
                        adjacent=adjacent->next_v;
                    }

                    ///traverse right side
                    adjacent=base->next_h=partner->next_h;
                    o=partner->y_pos;
                    ///o_end hasn't changed
                    if(adjacent && adjacent->y_pos==o) while(o < o_end)
                    {
                        adjacent->prev_h=base;
                        o+=1<<adjacent->size_factor_v;
                        adjacent=adjacent->next_v;
                    }

                    remove_tile_from_availability_heap(ia,partner,base->size_factor_h,base->size_factor_v);

                    base->size_factor_v++;

                    finished_h=false;///new combination potential
                }
                else finished_v=true;
            }
            else///base is top of current vertical tile pair
            {
                if((partner=base->next_v) && partner->size_factor_h==base->size_factor_h && partner->size_factor_v==base->size_factor_v && partner->available)
                {
                    ///traverse bottom side
                    adjacent=base->next_v=partner->next_v;
                    o=partner->x_pos;
                    o_end=o+(1<<base->size_factor_h);
                    if(adjacent && adjacent->x_pos==o) while(o < o_end)
                    {
                        adjacent->prev_v=base;
                        o+=1<<adjacent->size_factor_h;
                        adjacent=adjacent->next_h;
                    }

                    ///traverse left side
                    adjacent=partner->prev_h;
                    o=partner->y_pos;
                    o_end=o+(1<<base->size_factor_v);
                    if(adjacent && adjacent->y_pos==o) while(o < o_end)
                    {
                        while(adjacent->next_h!=partner)adjacent=adjacent->next_h;
                        adjacent->next_h=base;
                        o+=1<<adjacent->size_factor_v;
                        adjacent=adjacent->next_v;
                    }

                    ///traverse right side
                    adjacent=partner->next_h;
                    o=partner->y_pos;
                    ///o_end hasn't changed
                    if(adjacent && adjacent->y_pos==o) while(o < o_end)
                    {
                        adjacent->prev_h=base;
                        o+=1<<adjacent->size_factor_v;
                        adjacent=adjacent->next_v;
                    }

                    remove_tile_from_availability_heap(ia,partner,base->size_factor_h,base->size_factor_v);

                    base->size_factor_v++;

                    finished_h=false;///new combination potential
                }
                else finished_v=true;
            }
        }

    }
    while(!finished_h || !finished_v);


    d=ia->available_tiles[base->size_factor_h][base->size_factor_v].count++;
    heap=ia->available_tiles[base->size_factor_h][base->size_factor_v].heap;

    if(d==ia->available_tiles[base->size_factor_h][base->size_factor_v].space)
    {
        heap=ia->available_tiles[base->size_factor_h][base->size_factor_v].heap=realloc(heap,sizeof(cvm_vk_image_atlas_tile*)*(ia->available_tiles[base->size_factor_h][base->size_factor_v].space*=2));
    }

    while(d && heap[(u=(d-1)>>1)]->offset > base->offset)
    {
        heap[u]->heap_index=d;
        heap[d]=heap[u];
        d=u;
    }

    base->heap_index=d;
    base->available=true;
    heap[d]=base;

    ia->available_tiles_bitmasks[base->size_factor_h]|=1<<base->size_factor_v;///partner was last available tile of this size
}
