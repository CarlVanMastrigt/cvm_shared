/**
Copyright 2021,2022 Carl van Mastrigt

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

void cvm_vk_create_image_atlas(cvm_vk_image_atlas * ia,VkImage image,VkImageView image_view,size_t bytes_per_pixel,uint32_t width,uint32_t height,bool multithreaded)
{
    uint32_t i,j;

    assert(width > 1<<CVM_VK_BASE_TILE_SIZE_FACTOR && height > 1<<CVM_VK_BASE_TILE_SIZE_FACTOR);///IMAGE ATLAS TOO SMALL

    ia->image=image;
    ia->image_view=image_view;
    ia->bytes_per_pixel=bytes_per_pixel;

    assert((width>=(1<<CVM_VK_BASE_TILE_SIZE_FACTOR)) && (height>=(1<<CVM_VK_BASE_TILE_SIZE_FACTOR)));/// IMAGE ATLAS TOO SMALL
    assert(!(width & (width-1)) && !(height & (height-1)));///IMAGE ATLAS MUST BE CREATED AT POWER OF 2 WIDTH AND HEIGHT

    ia->num_tile_sizes_h=cvm_lbs_32(width)+1-CVM_VK_BASE_TILE_SIZE_FACTOR;
    ia->num_tile_sizes_v=cvm_lbs_32(height)+1-CVM_VK_BASE_TILE_SIZE_FACTOR;

    ia->multithreaded=multithreaded;
    atomic_init(&ia->acquire_spinlock,0);

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

    atomic_init(&ia->copy_spinlock,0);
    ia->staging_buffer=NULL;
    ia->pending_copy_actions=malloc(sizeof(VkBufferImageCopy)*16);
    ia->pending_copy_space=16;
    ia->pending_copy_count=0;

    ia->initialised=false;
}

void cvm_vk_destroy_image_atlas(cvm_vk_image_atlas * ia)
{
    uint32_t i,j;
    cvm_vk_image_atlas_tile *first,*last,*current,*next;

    ///one tile of max size is below, represent fully relinquished / empty atlas
    assert(ia->available_tiles[ia->num_tile_sizes_h-1][ia->num_tile_sizes_v-1].count==1);///TRYING TO DESTROY AN IMAGE ATLAS WITH ACTIVE TILES

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

    free(ia->pending_copy_actions);
}

///probably worth having this as function, allows calling from branches

//static void split_tile();

cvm_vk_image_atlas_tile * cvm_vk_acquire_image_atlas_tile(cvm_vk_image_atlas * ia,uint32_t width,uint32_t height)
{
    uint32_t i,desired_size_factor_h,desired_size_factor_v,size_factor_h,size_factor_v,current,up,down,offset,offset_mid,offset_end;
    cvm_vk_image_atlas_tile *base,*partner,*adjacent;
    uint_fast32_t lock;
    cvm_vk_image_atlas_tile ** heap;


    desired_size_factor_h=(width<CVM_VK_BASE_TILE_SIZE)?0:cvm_po2_32_gte(width)-CVM_VK_BASE_TILE_SIZE_FACTOR;
    desired_size_factor_v=(height<CVM_VK_BASE_TILE_SIZE)?0:cvm_po2_32_gte(height)-CVM_VK_BASE_TILE_SIZE_FACTOR;

    if(desired_size_factor_h>=ia->num_tile_sizes_h || desired_size_factor_v>=ia->num_tile_sizes_v)return NULL;///i dont expect this to get hit so dont bother doing earlier easier test

    if(ia->multithreaded)
    {
        do lock=atomic_load(&ia->acquire_spinlock);
        while(lock!=0 || !atomic_compare_exchange_weak(&ia->acquire_spinlock,&lock,1));
    }

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

    for(size_factor_h=desired_size_factor_h;size_factor_h<ia->num_tile_sizes_h;size_factor_h++)
    {
        if(ia->available_tiles_bitmasks[size_factor_h]>=1u<<desired_size_factor_v)
        {
//            size_factor_v=__builtin_ctz(ia->available_tiles_bitmasks[size_factor_h]&~((1<<desired_size_factor_v)-1));
            size_factor_v=__builtin_ctz(ia->available_tiles_bitmasks[size_factor_h]>>desired_size_factor_v)+desired_size_factor_v;

            if(!base || (uint32_t)base->size_factor_h+(uint32_t)base->size_factor_v > size_factor_h+size_factor_v)
            {
                base=ia->available_tiles[size_factor_h][size_factor_v].heap[0];
            }
        }
    }

    if(!base)///a tile to split was not found!
    {
        if(ia->multithreaded) atomic_store(&ia->acquire_spinlock,0);
        return NULL;///no tile large enough exists
    }

    ///extract from heap
    heap=ia->available_tiles[base->size_factor_h][base->size_factor_v].heap;

    if((current= --ia->available_tiles[base->size_factor_h][base->size_factor_v].count))
    {
        up=0;

        while((down=(up<<1)+1)<current)
        {
            down+= (down+1<current) && (heap[down+1]->offset < heap[down]->offset);
            if(heap[current]->offset < heap[down]->offset) break;
            heap[down]->heap_index=up;///need to know (new) index in heap
            heap[up]=heap[down];
            up=down;
        }

        heap[current]->heap_index=up;
        heap[up]=heap[current];
    }
    else ia->available_tiles_bitmasks[base->size_factor_h]&= ~(1<<base->size_factor_v);///took last tile of this size


    while(base->size_factor_h>desired_size_factor_h || base->size_factor_v>desired_size_factor_v)
    {
        partner=ia->first_unused_tile;
        ia->first_unused_tile=partner->next_h;
        ia->unused_tile_count--;

        ///here if offset isnt equal to relevant adjacent position then adjacent is larger and starts before the relevant (v/h) block that contains the tile we're splitting

        if(base->size_factor_h != desired_size_factor_h && (base->size_factor_v==desired_size_factor_v || base->size_factor_h>=base->size_factor_v))///try to keep tiles square where possible, with slight bias towards wider tiles
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
            offset=base->y_pos;
            offset_end=offset+(1<<base->size_factor_v);
            if(adjacent && adjacent->y_pos == offset)
            {
                while(offset < offset_end)
                {
                    adjacent->prev_h=partner;
                    offset+=1<<adjacent->size_factor_v;
                    adjacent=adjacent->next_v;
                }
            }

            ///traverse bottom edge
            adjacent=partner->next_v=base->next_v;
            offset=base->x_pos;
            offset_mid=offset+(1<<base->size_factor_h);
            offset_end=offset_mid+(1<<base->size_factor_h);
            if(adjacent && adjacent->x_pos == offset)
            {
                while(offset < offset_end)
                {
                    if(offset==offset_mid)partner->next_v=adjacent;
                    if(offset>=offset_mid)adjacent->prev_v=partner;
                    offset+=1<<adjacent->size_factor_h;
                    adjacent=adjacent->next_h;
                }
            }

            ///traverse top edge
            adjacent=partner->prev_v=base->prev_v;
            offset=base->x_pos;
            /// o_mid and o_end unchanged
            if(adjacent && adjacent->x_pos == offset)
            {
                while(offset < offset_end)
                {
                    while(adjacent->next_v!=base) adjacent=adjacent->next_v;
                    if(offset==offset_mid)partner->prev_v=adjacent;
                    if(offset>=offset_mid)adjacent->next_v=partner;
                    offset+=1<<adjacent->size_factor_h;
                    adjacent=adjacent->next_h;
                }
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
            offset=base->x_pos;
            offset_end=offset+(1<<base->size_factor_h);
            if(adjacent && adjacent->x_pos == offset)
            {
                while(offset < offset_end)
                {
                    adjacent->prev_v=partner;
                    offset+=1<<adjacent->size_factor_h;
                    adjacent=adjacent->next_h;
                }
            }

            ///traverse right edge
            adjacent=partner->next_h=base->next_h;
            offset=base->y_pos;
            offset_mid=offset+(1<<base->size_factor_v);
            offset_end=offset_mid+(1<<base->size_factor_v);
            if(adjacent && adjacent->y_pos == offset)
            {
                while(offset < offset_end)
                {
                    if(offset==offset_mid)partner->next_h=adjacent;
                    if(offset>=offset_mid)adjacent->prev_h=partner;
                    offset+=1<<adjacent->size_factor_v;
                    adjacent=adjacent->next_v;
                }
            }

            ///traverse left edge
            adjacent=partner->prev_h=base->prev_h;
            offset=base->y_pos;
            /// o_mid and o_end unchanged
            if(adjacent && adjacent->y_pos == offset)
            {
                while(offset < offset_end)
                {
                    while(adjacent->next_h!=base) adjacent=adjacent->next_h;
                    if(offset==offset_mid)partner->prev_h=adjacent;
                    if(offset>=offset_mid)adjacent->next_h=partner;
                    offset+=1<<adjacent->size_factor_v;
                    adjacent=adjacent->next_v;
                }
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

    if(ia->multithreaded) atomic_store(&ia->acquire_spinlock,0);

    return base;
}

static inline void remove_tile_from_availability_heap(cvm_vk_image_atlas * ia,cvm_vk_image_atlas_tile * t,uint32_t size_factor_h,uint32_t size_factor_v)
{
    cvm_vk_image_atlas_tile ** heap;
    uint32_t current,up,down;

    if((current= --ia->available_tiles[size_factor_h][size_factor_v].count))
    {
        heap=ia->available_tiles[size_factor_h][size_factor_v].heap;

        down=t->heap_index;
        while(down && heap[current]->offset < heap[(up=(down-1)>>1)]->offset)///try moving up heap
        {
            heap[up]->heap_index=down;
            heap[down]=heap[up];
            down=up;
        }
        up=down;
        while((down=(up<<1)+1)<current)///try moving down heap
        {
            down+= down+1<current && heap[down+1]->offset < heap[down]->offset;
            if(heap[current]->offset < heap[down]->offset) break;
            heap[down]->heap_index=up;
            heap[up]=heap[down];
            up=down;
        }

        heap[current]->heap_index=up;
        heap[up]=heap[current];
    }
    else ia->available_tiles_bitmasks[size_factor_h]&= ~(1<<size_factor_v);///partner was last available tile of this size


    ///add to list of unused tiles
    t->next_h=ia->first_unused_tile;
    ia->first_unused_tile=t;
    ia->unused_tile_count++;
}

void cvm_vk_relinquish_image_atlas_tile(cvm_vk_image_atlas * ia,cvm_vk_image_atlas_tile * base)
{
    uint32_t up,down,offset,offset_end;
    cvm_vk_image_atlas_tile *partner,*adjacent;
    uint_fast32_t lock;
    cvm_vk_image_atlas_tile ** heap;
    bool finished_h,finished_v;

    if(!base)return;

    ///base,partner,test

    if(ia->multithreaded)
    {
        do lock=atomic_load(&ia->acquire_spinlock);
        while(lock!=0 || !atomic_compare_exchange_weak(&ia->acquire_spinlock,&lock,1));
    }

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
                    offset=partner->y_pos;
                    offset_end=offset+(1<<base->size_factor_v);
                    if(adjacent && adjacent->y_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            while(adjacent->next_h!=partner)adjacent=adjacent->next_h;
                            adjacent->next_h=base;
                            offset+=1<<adjacent->size_factor_v;
                            adjacent=adjacent->next_v;
                        }
                    }

                    ///traverse bottom side
                    adjacent=base->next_v=partner->next_v;
                    offset=partner->x_pos;
                    offset_end=offset+(1<<base->size_factor_h);
                    if(adjacent && adjacent->x_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            adjacent->prev_v=base;
                            offset+=1<<adjacent->size_factor_h;
                            adjacent=adjacent->next_h;
                        }
                    }

                    ///traverse top side
                    adjacent=base->prev_v=partner->prev_v;
                    offset=partner->x_pos;
                    ///o_end hasn't changed
                    if(adjacent && adjacent->x_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            while(adjacent->next_v!=partner)adjacent=adjacent->next_v;
                            adjacent->next_v=base;
                            offset+=1<<adjacent->size_factor_h;
                            adjacent=adjacent->next_h;
                        }
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
                    offset=partner->y_pos;
                    offset_end=offset+(1<<base->size_factor_v);
                    if(adjacent && adjacent->y_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            adjacent->prev_h=base;
                            offset+=1<<adjacent->size_factor_v;
                            adjacent=adjacent->next_v;
                        }
                    }

                    ///traverse bottom side
                    adjacent=partner->next_v;
                    offset=partner->x_pos;
                    offset_end=offset+(1<<base->size_factor_h);
                    if(adjacent && adjacent->x_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            adjacent->prev_v=base;
                            offset+=1<<adjacent->size_factor_h;
                            adjacent=adjacent->next_h;
                        }
                    }

                    ///traverse top side
                    adjacent=partner->prev_v;
                    offset=partner->x_pos;
                    ///o_end hasn't changed
                    if(adjacent && adjacent->x_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            while(adjacent->next_v!=partner)adjacent=adjacent->next_v;
                            adjacent->next_v=base;
                            offset+=1<<adjacent->size_factor_h;
                            adjacent=adjacent->next_h;
                        }
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
                    offset=partner->x_pos;
                    offset_end=offset+(1<<base->size_factor_h);
                    if(adjacent && adjacent->x_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            while(adjacent->next_v!=partner)adjacent=adjacent->next_v;
                            adjacent->next_v=base;
                            offset+=1<<adjacent->size_factor_h;
                            adjacent=adjacent->next_h;
                        }
                    }

                    ///traverse left side
                    adjacent=base->prev_h=partner->prev_h;
                    offset=partner->y_pos;
                    offset_end=offset+(1<<base->size_factor_v);
                    if(adjacent && adjacent->y_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            while(adjacent->next_h!=partner)adjacent=adjacent->next_h;
                            adjacent->next_h=base;
                            offset+=1<<adjacent->size_factor_v;
                            adjacent=adjacent->next_v;
                        }
                    }

                    ///traverse right side
                    adjacent=base->next_h=partner->next_h;
                    offset=partner->y_pos;
                    ///o_end hasn't changed
                    if(adjacent && adjacent->y_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            adjacent->prev_h=base;
                            offset+=1<<adjacent->size_factor_v;
                            adjacent=adjacent->next_v;
                        }
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
                    offset=partner->x_pos;
                    offset_end=offset+(1<<base->size_factor_h);
                    if(adjacent && adjacent->x_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            adjacent->prev_v=base;
                            offset+=1<<adjacent->size_factor_h;
                            adjacent=adjacent->next_h;
                        }
                    }

                    ///traverse left side
                    adjacent=partner->prev_h;
                    offset=partner->y_pos;
                    offset_end=offset+(1<<base->size_factor_v);
                    if(adjacent && adjacent->y_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            while(adjacent->next_h!=partner)adjacent=adjacent->next_h;
                            adjacent->next_h=base;
                            offset+=1<<adjacent->size_factor_v;
                            adjacent=adjacent->next_v;
                        }
                    }

                    ///traverse right side
                    adjacent=partner->next_h;
                    offset=partner->y_pos;
                    ///o_end hasn't changed
                    if(adjacent && adjacent->y_pos==offset)
                    {
                        while(offset < offset_end)
                        {
                            adjacent->prev_h=base;
                            offset+=1<<adjacent->size_factor_v;
                            adjacent=adjacent->next_v;
                        }
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


    down=ia->available_tiles[base->size_factor_h][base->size_factor_v].count++;
    heap=ia->available_tiles[base->size_factor_h][base->size_factor_v].heap;

    if(down==ia->available_tiles[base->size_factor_h][base->size_factor_v].space)
    {
        heap=ia->available_tiles[base->size_factor_h][base->size_factor_v].heap=realloc(heap,sizeof(cvm_vk_image_atlas_tile*)*(ia->available_tiles[base->size_factor_h][base->size_factor_v].space*=2));
    }

    while(down && heap[(up=(down-1)>>1)]->offset > base->offset)
    {
        heap[up]->heap_index=down;
        heap[down]=heap[up];
        down=up;
    }

    base->heap_index=down;
    base->available=true;
    heap[down]=base;

    ia->available_tiles_bitmasks[base->size_factor_h]|=1<<base->size_factor_v;///partner was last available tile of this size

    if(ia->multithreaded) atomic_store(&ia->acquire_spinlock,0);
}

cvm_vk_image_atlas_tile * cvm_vk_acquire_image_atlas_tile_with_staging(cvm_vk_image_atlas * ia,uint32_t width,uint32_t height,void ** staging)
{
    VkDeviceSize upload_offset;
    uint_fast32_t lock;
    cvm_vk_image_atlas_tile * tile;

    assert(ia->staging_buffer);///IMAGE ATLAS STAGING BUFFER NOT SET
    assert(ia->bytes_per_pixel*width*height <= ia->staging_buffer->total_space);///ATTEMPTED TO ACQUIRE STAGING SPACE FOR IMAGE GREATER THAN STAGING BUFFER TOTAL

    *staging = cvm_vk_staging_buffer_acquire_space(ia->staging_buffer,ia->bytes_per_pixel*width*height,&upload_offset);

    if(!*staging) return NULL;

    tile=cvm_vk_acquire_image_atlas_tile(ia,width,height);

    assert(tile);///NO SPACE LEFT IN IMAGE ATLAS

    if(ia->multithreaded)do lock=atomic_load(&ia->copy_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&ia->copy_spinlock,&lock,1));

    if(ia->pending_copy_count==ia->pending_copy_space)
    {
        ia->pending_copy_actions=realloc(ia->pending_copy_actions,sizeof(VkBufferImageCopy)*(ia->pending_copy_space*=2));
    }

    ia->pending_copy_actions[ia->pending_copy_count++]=(VkBufferImageCopy)
    {
        .bufferOffset=upload_offset,
        .bufferRowLength=width,
        .bufferImageHeight=height,
        .imageSubresource=(VkImageSubresourceLayers)
        {
            .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel=0,
            .baseArrayLayer=0,
            .layerCount=1
        },
        .imageOffset=(VkOffset3D)
        {
            .x=tile->x_pos<<CVM_VK_BASE_TILE_SIZE_FACTOR,
            .y=tile->y_pos<<CVM_VK_BASE_TILE_SIZE_FACTOR,
            .z=0
        },
        .imageExtent=(VkExtent3D)
        {
            .width=width,
            .height=height,
            .depth=1,
        }
    };

    if(ia->multithreaded) atomic_store(&ia->copy_spinlock,0);

    return tile;
}

void * cvm_vk_acquire_staging_for_image_atlas_tile(cvm_vk_image_atlas * ia,cvm_vk_image_atlas_tile * t,uint32_t width,uint32_t height)
{
    VkDeviceSize upload_offset;
    uint_fast32_t lock;
    void * staging;

    assert(ia->staging_buffer);///IMAGE ATLAS STAGING BUFFER NOT SET
    assert(ia->bytes_per_pixel*width*height <= ia->staging_buffer->total_space);///ATTEMPTED TO ACQUIRE STAGING SPACE FOR IMAGE GREATER THAN STAGING BUFFER TOTAL

    staging = cvm_vk_staging_buffer_acquire_space(ia->staging_buffer,ia->bytes_per_pixel*width*height,&upload_offset);

    if(!staging) return NULL;

    if(ia->multithreaded)do lock=atomic_load(&ia->copy_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&ia->copy_spinlock,&lock,1));

    if(ia->pending_copy_count==ia->pending_copy_space)
    {
        ia->pending_copy_actions=realloc(ia->pending_copy_actions,sizeof(VkBufferImageCopy)*(ia->pending_copy_space*=2));
    }

    ia->pending_copy_actions[ia->pending_copy_count++]=(VkBufferImageCopy)
    {
        .bufferOffset=upload_offset,
        .bufferRowLength=width,
        .bufferImageHeight=height,
        .imageSubresource=(VkImageSubresourceLayers)
        {
            .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel=0,
            .baseArrayLayer=0,
            .layerCount=1
        },
        .imageOffset=(VkOffset3D)
        {
            .x=t->x_pos<<CVM_VK_BASE_TILE_SIZE_FACTOR,
            .y=t->y_pos<<CVM_VK_BASE_TILE_SIZE_FACTOR,
            .z=0
        },
        .imageExtent=(VkExtent3D)
        {
            .width=width,
            .height=height,
            .depth=1,
        }
    };

    if(ia->multithreaded) atomic_store(&ia->copy_spinlock,0);

    return staging;
}

void cvm_vk_image_atlas_submit_all_pending_copy_actions(cvm_vk_image_atlas * ia,VkCommandBuffer transfer_cb)
{
    uint_fast32_t lock;

    if(ia->multithreaded)do lock=atomic_load(&ia->copy_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&ia->copy_spinlock,&lock,1));

    #warning might be worth looking into making this support different queues... (probably not possible if copies need to be handled promptly)
    if(ia->pending_copy_count || !ia->initialised)
    {
        VkDependencyInfo copy_acquire_dependencies=
        {
            .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext=NULL,
            .dependencyFlags=0,
            .memoryBarrierCount=0,
            .pMemoryBarriers=NULL,
            .bufferMemoryBarrierCount=0,
            .pBufferMemoryBarriers=NULL,
            .imageMemoryBarrierCount=1,
            .pImageMemoryBarriers=(VkImageMemoryBarrier2[1])
            {
                {
                    .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext=NULL,
                    .srcStageMask=ia->initialised?VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT:VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                    .srcAccessMask=ia->initialised?VK_ACCESS_2_SHADER_READ_BIT:0,
                    .dstStageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .dstAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .oldLayout=ia->initialised?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                    .image=ia->image,
                    .subresourceRange=(VkImageSubresourceRange)
                    {
                        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel=0,
                        .levelCount=1,
                        .baseArrayLayer=0,
                        .layerCount=1
                    }
                }
            }
        };

        vkCmdPipelineBarrier2(transfer_cb,&copy_acquire_dependencies);

        ///actually execute the copies!
        ///unfortunately need another test here in case initialised path is being taken
        if(ia->pending_copy_count)vkCmdCopyBufferToImage(transfer_cb,ia->staging_buffer->buffer,ia->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,ia->pending_copy_count,ia->pending_copy_actions);
        ia->pending_copy_count=0;
        ia->initialised=true;

        VkDependencyInfo copy_release_dependencies=
        {
            .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext=NULL,
            .dependencyFlags=0,
            .memoryBarrierCount=0,
            .pMemoryBarriers=NULL,
            .bufferMemoryBarrierCount=0,
            .pBufferMemoryBarriers=NULL,
            .imageMemoryBarrierCount=1,
            .pImageMemoryBarriers=(VkImageMemoryBarrier2[1])
            {
                {
                    .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext=NULL,
                    .srcStageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .srcAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .dstStageMask=VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    .dstAccessMask=VK_ACCESS_2_SHADER_READ_BIT,
                    .oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                    .image=ia->image,
                    .subresourceRange=(VkImageSubresourceRange)
                    {
                        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel=0,
                        .levelCount=1,
                        .baseArrayLayer=0,
                        .layerCount=1
                    }
                }
            }
        };

        vkCmdPipelineBarrier2(transfer_cb,&copy_release_dependencies);
    }

    if(ia->multithreaded) atomic_store(&ia->copy_spinlock,0);
}

void cvm_vk_image_atlas_submit_all_pending_copy_actions_(cvm_vk_image_atlas * ia,VkCommandBuffer transfer_cb, VkDeviceSize copy_source_offset)
{
    uint_fast32_t lock;
    uint32_t i;

    if(ia->multithreaded)do lock=atomic_load(&ia->copy_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&ia->copy_spinlock,&lock,1));

    #warning might be worth looking into making this support different queues... (probably not possible if copies need to be handled promptly)
    if(ia->pending_copy_count || !ia->initialised)
    {
        VkDependencyInfo copy_acquire_dependencies=
        {
            .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext=NULL,
            .dependencyFlags=0,
            .memoryBarrierCount=0,
            .pMemoryBarriers=NULL,
            .bufferMemoryBarrierCount=0,
            .pBufferMemoryBarriers=NULL,
            .imageMemoryBarrierCount=1,
            .pImageMemoryBarriers=(VkImageMemoryBarrier2[1])
            {
                {
                    .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext=NULL,
                    .srcStageMask=ia->initialised?VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT:VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                    .srcAccessMask=ia->initialised?VK_ACCESS_2_SHADER_READ_BIT:0,
                    .dstStageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .dstAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .oldLayout=ia->initialised?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                    .image=ia->image,
                    .subresourceRange=(VkImageSubresourceRange)
                    {
                        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel=0,
                        .levelCount=1,
                        .baseArrayLayer=0,
                        .layerCount=1
                    }
                }
            }
        };

        vkCmdPipelineBarrier2(transfer_cb,&copy_acquire_dependencies);

        ///actually execute the copies!
        ///unfortunately need another test here in case initialised path is being taken
        if(ia->pending_copy_count)vkCmdCopyBufferToImage(transfer_cb,ia->staging_buffer->buffer,ia->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,ia->pending_copy_count,ia->pending_copy_actions);
        ia->pending_copy_count=0;
        ia->initialised=true;

        VkDependencyInfo copy_release_dependencies=
        {
            .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext=NULL,
            .dependencyFlags=0,
            .memoryBarrierCount=0,
            .pMemoryBarriers=NULL,
            .bufferMemoryBarrierCount=0,
            .pBufferMemoryBarriers=NULL,
            .imageMemoryBarrierCount=1,
            .pImageMemoryBarriers=(VkImageMemoryBarrier2[1])
            {
                {
                    .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext=NULL,
                    .srcStageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .srcAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .dstStageMask=VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    .dstAccessMask=VK_ACCESS_2_SHADER_READ_BIT,
                    .oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                    .image=ia->image,
                    .subresourceRange=(VkImageSubresourceRange)
                    {
                        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel=0,
                        .levelCount=1,
                        .baseArrayLayer=0,
                        .layerCount=1
                    }
                }
            }
        };

        vkCmdPipelineBarrier2(transfer_cb,&copy_release_dependencies);
    }

    if(ia->multithreaded) atomic_store(&ia->copy_spinlock,0);
}




