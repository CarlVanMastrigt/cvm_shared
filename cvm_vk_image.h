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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif


#ifndef CVM_VK_IMAGE_H
#define CVM_VK_IMAGE_H


#define CVM_VK_BASE_TILE_SIZE_FACTOR 2
#define CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT 256

#define CVM_VK_GET_IMAGE_ATLAS_POS_X(O) (O>>16)
#define CVM_VK_GET_IMAGE_ATLAS_POS_Y(O) (O&0x0000FFFF)
#define CVM_VK_SET_IMAGE_ATLAS_POS(X,Y) (X<<16|Y&0x0000FFFF)

///probably want to use a base tile size of 4 by default? yeah, i think i should use this

/// need to search 2d grid for best available sized tile, have overarching input to prefer horizontal or vertical entries

/// 2d grid of available tiles prefers those in same column/row

/// error handling, if none available then have base error texture or render red box in overlay, will have to be implementation dependent

///the most difficult part is definitely recombination, have structure of linked list, h_prev,h_next,v_prev,v_next for recombining

///selection/search: probably want one that reqires fewest splits of resources... even though this may limit recombination substantially...

///prev and next management is particularly challenging, needs to reference the tile it can be recombined with, but that tile can get split up, so adjacent references need to be very careful.
/// actually, because spliting/recombination can alter which tile is actually considered adjacent, and one can be th prev/next of many it may actually be impossible,
/// or close to it to do without traversing linked lists, which is definitely undesirable, may be possible to alleviate this by limiting v/h ratio to 2,
/// but maybe the limits on what can be recombined/split (at size matched po2 bounds) means this isnt actually a problem and everything will just work fine

/// could track which size it was split from at which time? any approach does will to fragmentation though...

/// by setting adjacency data for tiles not from original split to NULL we can signal that this isnt the direction to recombine in, which might not even be necessary.
/// just using naive inherited prev/next along with sizes it should be ensured that tiles are recombined in order they were split and with it ensure everything works okay (despite some "unnecessary" fragmentation)

///need to track how many tiles reference a tile from each given diection

typedef struct cvm_vk_image_atlas_tile cvm_vk_image_atlas_tile;

struct cvm_vk_image_atlas_tile/// (dont ref this by pointer, only store index)
{
    ///probably use h in list of unused tiles
    /// both prev and next sets must always be the topmost and leftmost relevant adjacent tile
    cvm_vk_image_atlas_tile * prev_h;
    cvm_vk_image_atlas_tile * next_h;

    cvm_vk_image_atlas_tile * prev_v;
    cvm_vk_image_atlas_tile * next_v;

//    union
//    {
//        uint32_t offset;
//        struct
//        {
//            uint32_t x:16;
//            uint32_t y:16;
//        }pos;
//    }location;
    uint32_t offset;///top 16 bits are x location, bottom 16 are y location
    ///get bit for "is_right_buffer" by 1 & (offset >> size_factor) during recombination calculations (256M max buffer size and 256 byte min gives the required size of this)
    uint32_t heap_index:22;///4m should be enough...
    uint32_t size_factor_h:4;///32788 * base size is definitely enough
    uint32_t size_factor_v:4;
    ///above leads to 256 max sized heaps, which is reasonable
    uint32_t available:1;///does not have contents, can be retrieved by new allocation or recombined into
    uint32_t is_base:1;///need to free this pointer at end
};

///need to ensure heap ALWAYS has enough space for this test,create heap with max number of allocations! (2^16)
typedef struct cvm_vk_available_atlas_tile_heap
{
    cvm_vk_image_atlas_tile ** heap;///potentially worth testing the effect of storing in a struct with the pointer to the allocation (avoids double indirection in about half of cases)
    uint32_t space;
    uint32_t count;
}
cvm_vk_available_atlas_tile_heap;

/// usage needed?

typedef struct cvm_vk_image_atlas
{
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;

    uint32_t width;
    uint32_t height;

    uint32_t num_tile_sizes_h;
    uint32_t num_tile_sizes_v;

    bool multithreaded;
    atomic_uint_fast32_t spinlock;

    cvm_vk_image_atlas_tile * first_unused_tile;
    uint32_t unused_tile_count;

    cvm_vk_available_atlas_tile_heap ** available_tiles;

    uint16_t available_tiles_bitmasks[16];///h based
}
cvm_vk_image_atlas;
///probably just going to use simple 2d version of PO2 allocator used in memory...


void cvm_vk_create_image_atlas(cvm_vk_image_atlas * ia,uint32_t width,uint32_t height,bool multithreaded);
void cvm_vk_destroy_image_atlas(cvm_vk_image_atlas * ia);

cvm_vk_image_atlas_tile * cvm_vk_acquire_image_atlas_tile(cvm_vk_image_atlas * ia,uint32_t width,uint32_t height);

#endif
