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


#ifndef CVM_VK_MEMORY_H
#define CVM_VK_MEMORY_H

#define CVM_VK_RESERVED_DYNAMIC_BUFFER_ALLOCATION_COUNT 256


typedef struct cvm_vk_staging_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    uint32_t total_space;///round to PO2
    uint32_t max_offset;///fence, should be updated before multithreading becomes applicable this frame
    uint32_t alignment_size_factor;
    atomic_uint_fast32_t space_remaining;
    uint32_t initial_space_remaining;
    ///need to test atomic version isn't (significatly) slower than non-atomic
    uint32_t * acquisitions;

    void * mapping;
}
cvm_vk_staging_buffer;

void cvm_vk_staging_buffer_create(cvm_vk_staging_buffer * sb,VkBufferUsageFlags usage);
void cvm_vk_staging_buffer_update(cvm_vk_staging_buffer * sb,uint32_t space_per_frame, uint32_t frame_count);
void cvm_vk_staging_buffer_destroy(cvm_vk_staging_buffer * sb);

uint32_t cvm_vk_staging_buffer_get_rounded_allocation_size(cvm_vk_staging_buffer * sb,uint32_t allocation_size);

void cvm_vk_staging_buffer_begin(cvm_vk_staging_buffer * sb);
void cvm_vk_staging_buffer_end(cvm_vk_staging_buffer * sb,uint32_t frame_index);

void * cvm_vk_staging_buffer_get_allocation(cvm_vk_staging_buffer * sb,uint32_t allocation_size,VkDeviceSize * acquired_offset);

void cvm_vk_staging_buffer_relinquish_space(cvm_vk_staging_buffer * sb,uint32_t frame_index);





/// could use more complicated allocator that can recombine sections of arbatrary size with no power of 2 superstructure, may work reasonably well, especially if memory is grouped by expected lifetime
///     ^ this will definitely require a defragger!
///     ^ use expected lifetime to prioritise ordering of sections? will that happen naturally as chunks are allocated/deallocated
///     ^ profile an implementation and only use it if its substantially better than the extant implementation in terms of alloc/dealloc time
///     ^ require all chunks to meet the largest supported types alignment requirements


typedef struct cvm_vk_dynamic_buffer_allocation cvm_vk_dynamic_buffer_allocation;

struct cvm_vk_dynamic_buffer_allocation/// (dont ref this by pointer, only store index)
{
    cvm_vk_dynamic_buffer_allocation * prev;
    cvm_vk_dynamic_buffer_allocation * next;/// both next in horizontal structure of all buffers and next in linked list of unused allocations

    uint32_t offset;/// actual offset = base_size<<offset_factor base allocation size 10 allows 16G buffer (which i quite like)
    ///get bit for "is_right_buffer" by 1 & (offset >> size_factor) during recombination calculations (256M max buffer size and 256 byte min gives the required size of this)
    uint32_t heap_index:25;///32m should be enough...
    uint32_t size_factor:5;
    uint32_t available:1;///does not have contents, can be retrieved by new allocation or recombined into
    uint32_t is_base:1;///need to free this pointer at end
};

///need to ensure heap ALWAYS has enough space for this test,create heap with max number of allocations! (2^16)
typedef struct cvm_vk_available_dynamic_allocation_heap
{
    cvm_vk_dynamic_buffer_allocation ** heap;///potentially worth testing the effect of storing in a struct with the pointer to the allocation (avoids double indirection in about half of cases)
    uint32_t space;
    uint32_t count;
}
cvm_vk_available_dynamic_allocation_heap;

typedef struct cvm_vk_managed_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    ///VkBufferUsageFlags usage;/// for use if buffer ends up needing recreation

    uint64_t total_space;///actually DO need to track total buffer size so that buffer can be cleaned (all static and dynamic allocations removed in one go)
    uint64_t static_offset;/// taken from end of buffer
    uint32_t dynamic_offset;/// taken from start of buffer, is a multiple of base offset, actual offset = dynamic_offset<<base_dynamic_allocation_size_factor
    ///recursively free dynamic allocations (if they are the last allocation) when making available
    ///give error if dynamic_offset would become greater than static_offset
    bool multithreaded;
    atomic_uint_fast32_t acquire_spinlock;

    cvm_vk_dynamic_buffer_allocation * first_unused_allocation;///singly linked list of allocations to assign space to
    uint32_t unused_allocation_count;

    cvm_vk_dynamic_buffer_allocation * last_used_allocation;///used for setting left/right of new allocations

    ///consider making following fixed array with max size of ~24 (whole struct should fit on 2-3 cache lines)
    cvm_vk_available_dynamic_allocation_heap * available_dynamic_allocations;
    ///intelligent application of last pointer to linked list creation isn't applicable, cannot reference/dereference bitfield!
    uint32_t num_dynamic_allocation_sizes;///essentially max value of dynamic_buffer_allocation.size_factor derived from the exponent of max_allocation_size/base_allocation_size
    uint32_t available_dynamic_allocation_bitmask;
    uint32_t base_dynamic_allocation_size_factor;

    void * mapping;///used for device generic buffers on UMA platforms and staging/uniform buffers on all others (assuming you would event want this for those prposes...) also operates as flag as to whether staging is necessary

    atomic_uint_fast32_t copy_spinlock;/// could really do with padding between this and other spinlock...
    cvm_vk_staging_buffer * staging_buffer;///for when mapping is not available
    VkBufferCopy * pending_copy_actions;
    uint32_t pending_copy_space;
    uint32_t pending_copy_count;
}
cvm_vk_managed_buffer;

void cvm_vk_managed_buffer_create(cvm_vk_managed_buffer * buffer,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,VkBufferUsageFlags usage,bool multithreaded,bool host_visible);
void cvm_vk_managed_buffer_destroy(cvm_vk_managed_buffer * mb);

cvm_vk_dynamic_buffer_allocation * cvm_vk_managed_buffer_acquire_dynamic_allocation(cvm_vk_managed_buffer * mb,uint64_t size);
void cvm_vk_managed_buffer_relinquish_dynamic_allocation(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation);
static inline uint64_t cvm_vk_managed_buffer_get_dynamic_allocation_offset(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation)
{
    return allocation->offset<<mb->base_dynamic_allocation_size_factor;
}

uint64_t cvm_vk_managed_buffer_acquire_static_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment);///cannot be relinquished, exists until

void * cvm_vk_managed_buffer_get_dynamic_allocation_mapping(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation,uint64_t size);///if size==0 returns full allocation
void * cvm_vk_managed_buffer_get_static_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size);

void cvm_vk_managed_buffer_submit_all_pending_copy_actions(cvm_vk_managed_buffer * mb,VkCommandBuffer transfer_cb);

void cvm_vk_managed_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,uint32_t binding);
void cvm_vk_managed_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,VkIndexType type);

///on UMA don't flush individual regions, instead every frame flush enture buffer (may want to profile?)
//void cvm_vk_flush_managed_buffer(cvm_vk_managed_buffer * mb);






/**
ring buffer solves issues for uniform buffers and most of upload problems but isnt really extensible in the case of instance data

instance data is somewhat problematic anyway, as it potentially varies a lot frame to frame, removing potential of allocating space only as necessary
    and relying on enough being available this frame from memory being released this frame requires constant allocation with hard limits on particular instance data
    this isnt the end of the world but it is wasteful both in terms of memory and flushing
    a better approach might be to upfront allocate a LOT of space, using the same uniform space insurance BUT allowing space allocated for instance to expand as contract dependent on use
        (even if this means frames where not everything gets drawn...) expanding available allocations by some count as necessary
            perhaps increasing to ensure some max expected delta factor is satisfied while increasing in multiples of some some base allocation size, even though this is slightly wasteful it isn't terribly wasteful

    this paradigm should also include the capacity of submodules to pause rendering while their allocated buffers expand.
    BUT as ring buffer is completely transient there could be a system to seamlessly replace ring buffers with new, larger ones where possible (falling back to failed allocations when that happens)
        ^ a system like this should expand by some reasonable factor, perhaps multiples of 2?
        ^ this does sound more like user side code: probably good to provide example of this in use and/or default management tools to accomplish this (even though will likely end up implementation specific...)

    uniform-  base constant allocation
    instance- base unit, frame reserve space, expansion reserve space or factor (perhaps same as max expected allocation or half of it, used in sizing buffer)
    upload-   expansion reserve space or factor (perhaps same as max expected allocation or half of it, used in sizing buffer)

    perhaps expansion reserve ensures that at least that much (in multiples) will always be available when expanding/resizing buffer

    immediate cleanup of space taken by finished frame ensures no space hangs around

    by having error handling (unable to allocate space) and sufficient reserves having just 1 buffer to switch to when deciding to resize should be sufficient
*/




typedef struct cvm_vk_transient_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    uint32_t total_space;
    uint32_t space_per_frame;///round to PO2
    uint32_t max_offset;///fence, should be updated before multithreading becomes applicable this frame
    uint32_t alignment_size_factor;
    atomic_uint_fast32_t space_remaining;
    ///need to test atomic version isn't (significatly) slower than non-atomic

    void * mapping;
}
cvm_vk_transient_buffer;

void cvm_vk_transient_buffer_create(cvm_vk_transient_buffer * tb,VkBufferUsageFlags usage);
void cvm_vk_transient_buffer_update(cvm_vk_transient_buffer * tb,uint32_t space_per_frame, uint32_t frame_count);
void cvm_vk_transient_buffer_destroy(cvm_vk_transient_buffer * tb);

uint32_t cvm_vk_transient_buffer_get_rounded_allocation_size(cvm_vk_transient_buffer * tb,uint32_t allocation_size);

void cvm_vk_transient_buffer_begin(cvm_vk_transient_buffer * tb,uint32_t frame_index);
void cvm_vk_transient_buffer_end(cvm_vk_transient_buffer * tb);

void * cvm_vk_transient_buffer_get_allocation(cvm_vk_transient_buffer * tb,uint32_t allocation_size,VkDeviceSize * acquired_offset);















/**
related to section 12.7. "Resource Sharing Mode" of vulkan spec.

not sure if single buffer can be used across queue families (transfer,compute,graphics) like cvm_vk_upload_buffer potentially wants to be

Ranges of buffers and image subresources of image objects created using VK_SHARING_MODE_EXCLUSIVE
must only be accessed by queues in the queue family that has ownership of the resource.
Upon creation, such resources are not owned by any queue family; ownership is implicitly acquired upon first use within a queue.
Once a resource using VK_SHARING_MODE_EXCLUSIVE is owned by some queue family,
the application must perform a queue family ownership transfer to make the memory contents of a range or image subresource accessible to a different queue


this isnt very clear on whether ownersip is acquired per buffer or buffer-range, is a resource a buffer or a buffer range? ownership transfers happen per range,
but if resource!=range then it appears ownership is first acquired for the whole buffer at once

similarly

A queue family can take ownership of an image subresource or buffer range of a resource created with
VK_SHARING_MODE_EXCLUSIVE, without an ownership transfer, in the same way as for a resource that was just created;
however, taking ownership in this way has the effect that the contents of the image subresource or buffer range are undefined.


this doesn't specify what happens to data written from mapped regions, ownership is presumably acquired AFTER mapped writes (first use == implicit acquisition -> undefined contents)
however it isnt specific upon when mapped wries become visible (presumably this is detailed somewhere else in the spec)

either way it doesnt allow variable sized allocations so i will be using buffer per type paradigm
*/

/*typedef struct cvm_vk_upload_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    uint32_t alignment_size_factor;

    void * mapping;

    uint32_t frame_count;///basically just swapchain image count, useful for detecting change here and knowing when to recreate
    uint32_t current_frame;///unnecessary also but makes code a tad cleaner (dont need to pass it in for every transient acquisition or end frame)

    /// uniform/instance (transient) stuff
    uint32_t transient_space_per_frame;///used in allocation step, if (0) dont perform setup/ops
    atomic_uint_fast32_t transient_space_remaining;///this frame
    uint32_t * transient_offsets;///all go after staging space b/c storing offsets anyway is required

    ///staging stuff
    uint32_t staging_space_per_frame;///used in allocation step,assert if any attempted staging acquisition is larger than this, if (0) dont perform setup/ops
    uint32_t staging_space;///not really necessesary b/ have space per frame and frame count but is a convenience
    uint32_t max_staging_offset;///fence, should be updated before multithreading becomes applicable this frame
    atomic_uint_fast32_t staging_space_remaining;
    uint32_t initial_staging_space_remaining;
    uint32_t * staging_buffer_acquisitions;
}
cvm_vk_upload_buffer;

void cvm_vk_create_upload_buffer(cvm_vk_upload_buffer * ub,VkBufferUsageFlags usage);
void cvm_vk_update_upload_buffer(cvm_vk_upload_buffer * ub,uint32_t staging_space_per_frame,uint32_t transient_space_per_frame,uint32_t frame_count);
void cvm_vk_destroy_upload_buffer(cvm_vk_upload_buffer * ub);
///may want to set requisite space in separate functions...
/// if adding space to requisite would be good to have set rounding ops to use...

uint32_t cvm_vk_upload_buffer_get_rounded_allocation_size(cvm_vk_upload_buffer * ub,uint32_t allocation_size);///absolutely needed for uniform usage

void cvm_vk_begin_upload_buffer_frame(cvm_vk_upload_buffer * ub,uint32_t frame_index);
void cvm_vk_end_upload_buffer_frame(cvm_vk_upload_buffer * ub);

void cvm_vk_relinquish_upload_buffer_space(cvm_vk_upload_buffer * ub,uint32_t frame_index);

///need to be careful to avoid using a null return in either of these
void * cvm_vk_get_upload_buffer_staging_allocation(cvm_vk_upload_buffer * ub,uint32_t allocation_size,VkDeviceSize * acquired_offset);
void * cvm_vk_get_upload_buffer_transient_allocation(cvm_vk_upload_buffer * ub,uint32_t allocation_size,VkDeviceSize * acquired_offset);*/



#endif

