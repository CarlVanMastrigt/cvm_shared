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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif


#ifndef CVM_VK_IMAGE_H
#define CVM_VK_IMAGE_H


#define CVM_VK_BASE_TILE_SIZE_FACTOR 2
#define CVM_VK_BASE_TILE_SIZE (1u<<CVM_VK_BASE_TILE_SIZE_FACTOR)
#define CVM_VK_RESERVED_IMAGE_ATLAS_TILE_COUNT 256


///probably want to use a base tile size of 4 by default? yeah, i think i should use this

typedef struct cvm_vk_image_atlas_tile cvm_vk_image_atlas_tile;

struct cvm_vk_image_atlas_tile/// (dont ref this by pointer, only store index)
{
    ///probably use h in list of unused tiles
    /// both prev and next sets must always be the topmost and leftmost relevant adjacent tile
    cvm_vk_image_atlas_tile * prev_h;///left
    cvm_vk_image_atlas_tile * next_h;///right

    cvm_vk_image_atlas_tile * prev_v;///up (screen space coordinates)
    cvm_vk_image_atlas_tile * next_v;///down (screen space coordinates)

    union
    {
        //replace with: max_x_y:12,x:12,y:12, and sort by max_x_y! (this should give best distribution strategy!)--does limit max texture size, which will need an assert
        uint32_t offset;
        struct
        {
            uint16_t x_pos;
            uint16_t y_pos;
        };
    };
    //uint32_t offset;///top 16 bits are x location, bottom 16 are y location
    ///get bit for "is_right_buffer" by 1 & (offset >> size_factor) during recombination calculations (256M max buffer size and 256 byte min gives the required size of this)
    uint32_t heap_index:22;///4m should be enough...

    ///32788 * base size is definitely enough
    uint32_t size_factor_h:4;///width
    uint32_t size_factor_v:4;///height
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
    mtx_t structure_mutex;///put at top in order to get padding between spinlocks (other being copy_spinlock)
    bool multithreaded;
    ///whether the images initial state has been set...
    bool initialised;

    ///creation/allocation of actual vk resources must be handled externally
    VkImage image;
    VkImageView image_view;

    size_t bytes_per_pixel;

    uint32_t width;
    uint32_t height;

    uint32_t num_tile_sizes_h;
    uint32_t num_tile_sizes_v;

    cvm_vk_image_atlas_tile * first_unused_tile;
    uint32_t unused_tile_count;

    cvm_vk_available_atlas_tile_heap ** available_tiles;

    uint16_t available_tiles_bitmasks[16];///h based

/// following required even on UMA systems (in contrast to equivalent section in managed buffer) in order to handle buffer(raw data)->image transition
    mtx_t copy_action_mutex;
    /// used to orchistrate uploads, should be filled out
    cvm_vk_staging_shunt_buffer * shunt_buffer;

    cvm_vk_buffer_image_copy_stack pending_copy_actions;
}
cvm_vk_image_atlas;
///probably just going to use simple 2d version of PO2 allocator used in memory...


static inline void cvm_vk_image_atlas_get_tile_coordinates(cvm_vk_image_atlas * ia,cvm_vk_image_atlas_tile * tile,uint16_t * x,uint16_t * y)
{
    *x=tile->x_pos<<CVM_VK_BASE_TILE_SIZE_FACTOR;
    *y=tile->y_pos<<CVM_VK_BASE_TILE_SIZE_FACTOR;
}

void cvm_vk_create_image_atlas(cvm_vk_image_atlas * ia,VkImage image,VkImageView image_view,size_t bytes_per_pixel,uint32_t width,uint32_t height,bool multithreaded,cvm_vk_staging_shunt_buffer * shunt_buffer);
void cvm_vk_destroy_image_atlas(cvm_vk_image_atlas * ia);

cvm_vk_image_atlas_tile * cvm_vk_acquire_image_atlas_tile(cvm_vk_image_atlas * ia,uint32_t width,uint32_t height);
void cvm_vk_relinquish_image_atlas_tile(cvm_vk_image_atlas * ia,cvm_vk_image_atlas_tile * t);

cvm_vk_image_atlas_tile * cvm_vk_acquire_image_atlas_tile_with_staging(cvm_vk_image_atlas * ia,uint32_t width,uint32_t height,void ** staging);
void * cvm_vk_acquire_staging_for_image_atlas_tile(cvm_vk_image_atlas * ia,cvm_vk_image_atlas_tile * t,uint32_t width,uint32_t height);

void cvm_vk_image_atlas_submit_all_pending_copy_actions(cvm_vk_image_atlas * ia,VkCommandBuffer transfer_cb, VkBuffer staging_buffer, VkDeviceSize shunt_buffer_base_offset);
/// transfer_cb MUST be submitted to queue from same queue family to where the graphics commands that will use the image atlas will be used, assuming usage paradigm is correct this can even be the graphics queue itself

#endif

