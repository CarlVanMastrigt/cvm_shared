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

#define CVM_VK_RESERVED_DYNAMIC_BUFFER_ALLOCATION_COUNT 256


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

void * cvm_vk_staging_buffer_get_allocation(cvm_vk_staging_buffer * sb,uint32_t allocation_size,VkDeviceSize * acquired_offset);

void cvm_vk_staging_buffer_relinquish_space(cvm_vk_staging_buffer * sb,uint32_t frame_index);







/// could use more complicated allocator that can recombine sections of arbitrary size with no power of 2 superstructure, may work reasonably well, especially if memory is grouped by expected lifetime
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
    uint32_t size_factor:5;///32 different sizes is definitely enough, probably overkill but 16 is perhaps a little too few
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

    uint64_t total_space;///actually DO need to track total buffer size so that buffer can be cleaned (all static and dynamic allocations removed in one go)
    uint64_t static_offset;/// taken from end of buffer
    uint32_t dynamic_offset;/// taken from start of buffer, is a multiple of base offset, actual offset = dynamic_offset<<base_dynamic_allocation_size_factor
    ///recursively free dynamic allocations (if they are the last allocation) when making available
    ///give error if dynamic_offset would become greater than static_offset


    cvm_vk_dynamic_buffer_allocation * first_unused_allocation;///singly linked list of allocations to assign space to
    uint32_t unused_allocation_count;

    cvm_vk_dynamic_buffer_allocation * last_used_allocation;///used for setting left/right of new allocations

    ///consider making following fixed array with max size of ~24 (whole struct should fit on 2-3 cache lines)
    cvm_vk_available_dynamic_allocation_heap * available_dynamic_allocations;
    ///intelligent application of last pointer to linked list creation isn't applicable, cannot reference/dereference bitfield!
    uint32_t num_dynamic_allocation_sizes;///essentially max value of dynamic_buffer_allocation.size_factor derived from the exponent of max_allocation_size/base_allocation_size
    uint32_t available_dynamic_allocation_bitmask;
    uint32_t base_dynamic_allocation_size_factor;


    uint8_t * mapping;///can copy data in directly (no need for staging) on UMA systems
    /// also operates as flag as to whether staging is necessary and how to handle release/free operations
    /// can hypothetically make regions available straight away after freeing on non-uma systems (no release queue) but probably not worth it to do so and just use the free sequence

/// relevant data for copying, if UMA none of what follows will be used

    cvm_vk_staging_buffer * staging_buffer;///for when mapping is not available

    VkBufferCopy * pending_copy_actions;///DELETE
    uint32_t pending_copy_space;///DELETE
    uint32_t pending_copy_count;///DELETE

    cvm_vk_managed_buffer_barrier_list copy_release_barriers;
    cvm_vk_managed_buffer_barrier_list copy_acquire_barriers;
    ///could have an arbitrary number of cvm_vk_managed_buffer_barrier_list, allowing for more than 1 frame of delay
    ///but after the number exceeds the number of swapchain images barriers become unnecessary unless transferring between queue families

    uint32_t copy_src_queue_family;///DELETE
    uint32_t copy_dst_queue_family;///DELETE

    uint16_t copy_update_counter;///DELETE
    uint16_t copy_delay;///DELETE
    /// acquire barrier only relevant when a dedicated transfer queue family is involved
    /// is used to acquire ownership of buffer regions after they were released on transfer queue family
    /// when this is the case, upon submitting copies, need to swap these 2 structs and reset the (new) transfer barrier list

    ///fuuuuck copy actions need to happen in correct command buffer which isn't always just the graphics CB!
    /// schedule preemptive local copy actions

    uint16_t maximum_copy_delay_test;///0-max_copy_delay
    uint64_t low_priority_update_counter;///transfer queue semaphore value which resources uploaded to this buffer this frame must wait upon to finish before being used
    /// WAIT, as above value(s) can be shared across multiple buffers/sources it should come from the module!
    /// semaphore vaule representing VALID values can be set up at time of module acquisition

    atomic_uint_fast32_t copy_spinlock;///put at bottom in order to get padding between spinlocks (other being acquire_spinlock)
}
cvm_vk_managed_buffer;

///though perhaps "invalid" when not transferring ownership of a freed buffer region to the transfer queue to have new writes done (no QFOT on those regions, and doing so would require knowledge of ownership prior to blocks being coalesced) this *should* be fine if we're relying on command buffer having completed (timeline semaphore)
/// ^ no, if all caches were flushed upon semaphore then no barriers would be needed to begin with, need some kind of synchronisation primitive between queues upon deletion, as part of deletion paradigm move resources back to the graphics queue (if necessary) then treat all coalesced resources as belonging to the graphics queue (need some way to acquire on compute should that be first use case though)
///because all barriers must be matched there may end up being some hanging barriers that i'll need to prevent/deal with

///     ^ though... we know via semaphores that the region has finished being read from... that SHOULD be sufficient as well to handle new writes to recycled gpu memory (but not caches...)

void cvm_vk_managed_buffer_create(cvm_vk_managed_buffer * buffer,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,VkBufferUsageFlags usage,bool multithreaded,bool host_visible);
void cvm_vk_managed_buffer_destroy(cvm_vk_managed_buffer * mb);

cvm_vk_dynamic_buffer_allocation * cvm_vk_managed_buffer_acquire_dynamic_allocation(cvm_vk_managed_buffer * mb,uint64_t size);
void cvm_vk_managed_buffer_relinquish_dynamic_allocation(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation);
static inline uint64_t cvm_vk_managed_buffer_get_dynamic_allocation_offset(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation)
{
    return allocation->offset<<mb->base_dynamic_allocation_size_factor;
}

uint64_t cvm_vk_managed_buffer_acquire_static_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment);///cannot be relinquished, exists until

/// from spec regarding QFOT: A release operation is used to release exclusive ownership of a range of a buffer or image subresource range.
/// so availibility being moved for only

/// need high priority and low priority variants of these
/// need to test QFOT management on buffer regions (not whole buffer) - look at spec...
void * cvm_vk_managed_buffer_get_dynamic_allocation_mapping(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask,uint16_t * availability_token);///if size==0 returns full allocation
void * cvm_vk_managed_buffer_get_static_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask,uint16_t * availability_token);

void cvm_vk_managed_buffer_submit_all_acquire_barriers(cvm_vk_managed_buffer * mb,VkCommandBuffer graphics_cb);
void cvm_vk_managed_buffer_submit_all_pending_copy_actions(cvm_vk_managed_buffer * mb,VkCommandBuffer transfer_cb);///includes necessary barriers
/// for the love of god only call this once per buffer per frame, transfer and graphics cb necessary for case of QFOT (process acquisitions for previous frames releases)

void cvm_vk_managed_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,uint32_t binding);
void cvm_vk_managed_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,VkIndexType type);







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

