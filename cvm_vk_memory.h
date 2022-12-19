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


#ifndef CVM_VK_MEMORY_H
#define CVM_VK_MEMORY_H

typedef struct cvm_vk_staging_buffer_region
{
    uint32_t start;
    uint32_t end;
}
cvm_vk_staging_buffer_region;

typedef struct cvm_vk_staging_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    uint32_t total_space;
    uint32_t alignment_size_factor;

    cvm_vk_staging_buffer_region active_region;///region selected for this frame
    atomic_uint_fast32_t active_offset;///need to test atomic version isn't (significatly) slower than non-atomic for single threaded applications

    cvm_vk_staging_buffer_region * acquired_regions;///one per swapchain image
    cvm_vk_staging_buffer_region * available_regions;///will always be less than swapchain image count of these
    uint32_t available_region_count;

    uint8_t * mapping;
}
cvm_vk_staging_buffer;

void cvm_vk_staging_buffer_create(cvm_vk_staging_buffer * sb,VkBufferUsageFlags usage);
void cvm_vk_staging_buffer_update(cvm_vk_staging_buffer * sb,uint32_t space_per_frame,uint32_t frame_count);
void cvm_vk_staging_buffer_destroy(cvm_vk_staging_buffer * sb);

uint32_t cvm_vk_staging_buffer_get_rounded_allocation_size(cvm_vk_staging_buffer * sb,uint32_t allocation_size);

void cvm_vk_staging_buffer_begin(cvm_vk_staging_buffer * sb);
void cvm_vk_staging_buffer_end(cvm_vk_staging_buffer * sb,uint32_t frame_index);

void * cvm_vk_staging_buffer_acquire_space(cvm_vk_staging_buffer * sb,uint32_t allocation_size,VkDeviceSize * acquired_offset);

void cvm_vk_staging_buffer_release_space(cvm_vk_staging_buffer * sb,uint32_t frame_index);







#define CVM_VK_INVALID_TEMPORARY_ALLOCATION 0xFFFFFFFF

///16 bytes, 4 to a cacheline
typedef struct cvm_vk_temporary_buffer_allocation_data
{
    ///doubt I'll need to support more than 64M allocations, but will fill structure space as efficiently as possible for now, and respect typing
    uint32_t prev;
    uint32_t next;

    uint64_t offset:36;/// actual offset = base_size<<offset_factor
    uint64_t heap_index:22;///4M frees supported should be more than enough
    uint64_t size_factor:5;///32 different sizes is definitely enough,
    uint64_t available:1;///does not have contents, can be retrieved by new allocation or recombined into
    ///get bit for "is_right_buffer" by 1 & (offset >> size_factor) during recombination calculations (256M max buffer size and 256 byte min gives the required size of this)
}
cvm_vk_temporary_buffer_allocation_data;


/// ^ can return offset alongside allocation for efficient use purposes

/// could use more complicated allocator that can recombine sections of arbitrary size with no power of 2 superstructure, may work reasonably well, especially if memory is grouped by expected lifetime
///     ^ this will definitely require a defragger!
///     ^ use expected lifetime to prioritise ordering of sections? will that happen naturally as chunks are allocated/deallocated
///     ^ profile an implementation and only use it if its substantially better than the extant implementation in terms of alloc/dealloc time
///     ^ require all chunks to meet the largest supported types alignment requirements


///cannot use generic heap for this as a bunch of specialised things are done with this heap (randomised deletions, and tracked indices to support that)



typedef struct cvm_vk_available_temporary_allocation_heap
{
    uint32_t * heap;///indices of allocations
    uint32_t space;
    uint32_t count;
}
cvm_vk_available_temporary_allocation_heap;

typedef struct cvm_vk_managed_buffer_barrier_list
{
    VkBufferMemoryBarrier2 * barriers;
    uint32_t space;
    uint32_t count;
}
cvm_vk_managed_buffer_barrier_list;

typedef struct cvm_vk_managed_buffer
{
    atomic_uint_fast32_t acquire_spinlock;///put at top in order to get padding between spinlocks (other being copy_spinlock)
    bool multithreaded;

    VkBuffer buffer;
    VkDeviceMemory memory;

    ///VkBufferUsageFlags usage;/// for use if buffer ends up needing recreation

    uint64_t total_space;///actually DO need to track total buffer size so that buffer can be cleaned (all permanent and temporary allocations removed in one go)
    uint64_t permanent_offset;/// taken from end of buffer
    uint32_t temporary_offset;/// taken from start of buffer, is in terms of base offset


    cvm_vk_temporary_buffer_allocation_data * temporary_allocation_data;
    uint32_t temporary_allocation_data_space;

    uint32_t first_unused_temporary_allocation;///singly linked list of allocations to assign space to
    uint32_t unused_temporary_allocation_count;

    uint32_t last_used_allocation;///used for setting left/right of new allocations

    ///for various reasons can't/shouldn't use generic heap data structure here
    cvm_vk_available_temporary_allocation_heap available_temporary_allocations[32];
    uint32_t available_temporary_allocation_bitmask;
    uint32_t base_temporary_allocation_size_factor;


    uint8_t * mapping;///can copy data in directly (no need for staging) on UMA systems
    /// also operates as flag as to whether staging is necessary and how to handle release/free operations
    /// can hypothetically make regions available straight away after freeing on non-uma systems (no release queue) but probably not worth it to do so and just use the free sequence


    ///could have an arbitrary number of cvm_vk_managed_buffer_barrier_list, allowing for more than 1 frame of delay
    ///but after the number exceeds the number of swapchain images barriers become unnecessary unless transferring between queue families

    uint32_t copy_src_queue_family;///DELETE
    uint32_t copy_dst_queue_family;///DELETE

    uint16_t copy_update_counter;///DELETE
    uint16_t copy_delay;///DELETE always 2! (for regular staged copies, 0 for inline, 0 for UMA)
    /// acquire barrier only relevant when a dedicated transfer queue family is involved
    /// is used to acquire ownership of buffer regions after they were released on transfer queue family
    /// when this is the case, upon submitting copies, need to swap these 2 structs and reset the (new) transfer barrier list


    ///paradigm of processing deletion list after its known to be complete (via list of allocation to be made available after timeline semaphore "signals completion) allows this whole process to be SO much simpler
    /// ^ no need for work->transfer dependencies!

    cvm_vk_staging_buffer * staging_buffer;///for when mapping is not available :p


    ///rename to dismissed, are elements scheduled for release
    atomic_uint_fast32_t deletion_spinlock;
    u32_list * deleted_temporary_allocation_indices;
    uint16_t deletion_cycle_count;///same as swapchain image count used to initialise this
    uint16_t deletion_cycle_index;



    ///will always be the same regardless of where the contents end up, should be performed in same "frame" the were generated, just on the DMA queue, which will wait on graphics!
    cvm_vk_buffer_copy_list pending_copies;
    cvm_vk_buffer_barrier_list copy_release_barriers;
    /// this is absolutely a build up and flush paradigm
    //uint16_t availability_tracker;
    /// need something to track availability of allocations, u32 may be better (more stable) option?
    /// could incorporate into returned data? allocation_id -> allocation_metadata index,state,availability_token
    /// needs to take into account decision made regarding 1 frame delay or 2 (synchronous DMA even allowing 1 frame delay is so awesome)


    /// OOOOH, transfer queue (unified) should have singular per frame update of shared availability token!


    ///make memory available again when its KNOWN to be unused, either by swapchan image acquisition or by queue semaphore testing
    /// module batch wouldn't be a bad place...

    atomic_uint_fast32_t copy_spinlock;///put at bottom in order to get padding between spinlocks (other being acquire_spinlock)
}
cvm_vk_managed_buffer;

/**
from the spec:

If memory dependencies are correctly expressed between uses of such a resource between two
queues in different families, but no ownership transfer is defined, the contents of that resource are
undefined for any read accesses performed by the second queue family.

Note
If an application does not need the contents of a resource to remain valid when
transferring from one queue family to another, then the ownership transfer
SHOULD be skipped.


fuck. yes.   confirmation
*/

void cvm_vk_managed_buffer_create(cvm_vk_managed_buffer * mb,uint32_t buffer_size,uint32_t min_size_factor,VkBufferUsageFlags usage,bool multithreaded,bool host_visible);
void cvm_vk_managed_buffer_update(cvm_vk_managed_buffer * mb,uint32_t deletion_cycle_count);
///deletion_cycle_count should be initialised with swapchain image count in the simple paradigm, should essentially be used to ensure a frame delay sufficient to prevent re-use of "deleted" resources that are still in flight
void cvm_vk_managed_buffer_destroy(cvm_vk_managed_buffer * mb);

///as with all uses of this paradigm, if all swapchain images aren't actually cycled through, the deletion/free list may get stuck in limbo, may want to test potentially completed in-flight-frame semaphores occasionally?
///     ^ perhaps only when frame delay is greater than swapchain image count?
void cvm_vk_managed_buffer_process_deletion_list(cvm_vk_managed_buffer * mb,uint32_t deletion_cycle_index);

bool cvm_vk_managed_buffer_acquire_temporary_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint32_t * allocation_index,uint64_t * allocation_offset);
void cvm_vk_managed_buffer_dismiss_temporary_allocation(cvm_vk_managed_buffer * mb,uint32_t allocation_index);///schedule for deletion with simple cycling of frames (or other appropriate schedule)
void cvm_vk_managed_buffer_release_temporary_allocation(cvm_vk_managed_buffer * mb,uint32_t allocation_index);///delete immediately, must be correctly synchronised



uint64_t cvm_vk_managed_buffer_acquire_permanent_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment);///cannot be released, exists until entire buffer is cleared or deleted


void cvm_vk_managed_buffer_submit_all_pending_copy_actions(cvm_vk_managed_buffer * mb,VkCommandBuffer transfer_cb);///includes necessary barriers
/// for the love of god only call this once per buffer per frame, transfer and graphics cb necessary for case of QFOT (process acquisitions for previous frames releases)

void cvm_vk_managed_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,uint32_t binding);
void cvm_vk_managed_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,VkIndexType type);

///low priority versions
void * cvm_vk_managed_buffer_acquire_temporary_allocation_with_mapping(cvm_vk_managed_buffer * mb,uint64_t size,
            queue_transfer_synchronization_data * transfer_data,VkPipelineStageFlags2 use_stage_mask,VkAccessFlags2 use_access_mask,
            uint32_t * allocation_index,uint64_t * allocation_offset,uint16_t * availability_token);///perhaps move some/all of this to a unified structure

void * cvm_vk_managed_buffer_acquire_permanent_allocation_with_mapping(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment,
            queue_transfer_synchronization_data * transfer_data,VkPipelineStageFlags2 use_stage_mask,VkAccessFlags2 use_access_mask,
            uint64_t * allocation_offset,uint16_t * availability_token);

/// need high priority and low priority variants of above? probably wouldn't hurt...







///rename as transient upload buffer as transient workspace (GPU only) buffer would also be a useful paradigm to have access to
typedef struct cvm_vk_transient_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    uint32_t total_space;
    uint32_t space_per_frame;
    uint32_t max_offset;///fence, should be updated before multithreading becomes applicable this frame
    uint32_t alignment_size_factor;
    atomic_uint_fast32_t current_offset;
    ///need to test atomic version isn't (significatly) slower than non-atomic

    uint8_t * mapping;
}
cvm_vk_transient_buffer;

void cvm_vk_transient_buffer_create(cvm_vk_transient_buffer * tb,VkBufferUsageFlags usage);
void cvm_vk_transient_buffer_update(cvm_vk_transient_buffer * tb,uint32_t space_per_frame,uint32_t frame_count);
void cvm_vk_transient_buffer_destroy(cvm_vk_transient_buffer * tb);

uint32_t cvm_vk_transient_buffer_get_rounded_allocation_size(cvm_vk_transient_buffer * tb,uint32_t allocation_size,uint32_t alignment);
///having fixed locations wont work if non PO2 alignments are required, though this could be fixed by binding at a set offset

void cvm_vk_transient_buffer_begin(cvm_vk_transient_buffer * tb,uint32_t frame_index);
void cvm_vk_transient_buffer_end(cvm_vk_transient_buffer * tb);

///alignment 0 indicates to use base buffer alignment, likely because it will be bound with this offset rather than at 0 and indexed into
void * cvm_vk_transient_buffer_get_allocation(cvm_vk_transient_buffer * tb,uint32_t allocation_size,uint32_t alignment,VkDeviceSize * acquired_offset);

void cvm_vk_transient_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_transient_buffer * tb,uint32_t binding,VkDeviceSize offset);
void cvm_vk_transient_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_transient_buffer * tb,VkIndexType type,VkDeviceSize offset);




#endif

