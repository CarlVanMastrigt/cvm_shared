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



/// consider making sized types that represent offsets VkDeviceSize ?? (but then cannot make assumptions about their size...)


/// could use more complicated allocator that can recombine sections of arbatrary size with no power of 2 superstructure, may work reasonably well, especially if memory is grouped by expected lifetime
///     ^ this will definitely require a defragger!
///     ^ use expected lifetime to prioritise ordering of sections? will that happen naturally as chunks are allocated/deallocated
///     ^ profile an implementation and only use it if its substantially better than the extant implementation in terms of alloc/dealloc time
///     ^ require all chunks to meet the largest supported types alignment requirements







///pointer approach was at best very marginally better at the cost of extra allocation complexity and extra storage space :sob:
/// may want to try optimising both this and old approach

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
    atomic_uint_fast32_t spinlock;

    cvm_vk_dynamic_buffer_allocation * first_unused_allocation;///singly linked list of allocations to assign space to
    uint32_t unused_allocation_count;

    cvm_vk_dynamic_buffer_allocation * last_used_allocation;///used for setting left/right of new allocations

    ///consider making following fixed array with max size of ~24 (whole struct should fit on 2-3 cache lines)
    cvm_vk_available_dynamic_allocation_heap * available_dynamic_allocations;
    ///intelligent application of last pointer to linked list creation isn't applicable, cannot reference/dereference bitfield!
    uint32_t num_dynamic_allocation_sizes;///essentially max value of dynamic_buffer_allocation.size_factor derived from the exponent of max_allocation_size/base_allocation_size
    uint32_t available_dynamic_allocation_bitmask;
    uint32_t base_dynamic_allocation_size_factor;

    void * mapping;///used for device generic buffers on UMA platforms and staging/uniform buffers on all others
}
cvm_vk_managed_buffer;

void cvm_vk_create_managed_buffer(cvm_vk_managed_buffer * buffer,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,VkBufferUsageFlags usage,bool multithreaded,bool host_visible);
void cvm_vk_destroy_managed_buffer(cvm_vk_managed_buffer * mb);

cvm_vk_dynamic_buffer_allocation * cvm_vk_acquire_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint64_t size);
void cvm_vk_relinquish_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation);

uint64_t cvm_vk_acquire_static_buffer_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment);

///worth making these inline?
void * cvm_vk_get_dynamic_buffer_allocation_mapping(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation);
void * cvm_vk_get_static_buffer_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset);

void cvm_vk_bind_dymanic_allocation_vertex(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation,uint32_t binding);
void cvm_vk_bind_dymanic_allocation_index(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation,VkIndexType type);

void cvm_vk_flush_buffer_allocation_mapping(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation);
//void cvm_vk_get_flush_buffer_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset);
///static allocations have no means to know their size so would have to collectively flush ALL static allocations, which will need profiling

void cvm_vk_flush_managed_buffer(cvm_vk_managed_buffer * mb);






///probably want to use pointers for all of these

///can reasonably easily combine the node and the allocation list

/// allocate the exact required size (rounded to multiple of largest targeted alignment) and recombine sections as necessary, trying to best fit any new allocations (does require defragger quite badly)

//typedef struct cvm_vk_dynamic_buffer_allocation_3 cvm_vk_dynamic_buffer_allocation_3;
//
///// *just* fits in one cache line
//struct cvm_vk_dynamic_buffer_allocation_3/// (dont ref this by pointer, only store index)
//{
//    ///horizontal structure
//    cvm_vk_dynamic_buffer_allocation_3 * left;
//    cvm_vk_dynamic_buffer_allocation_3 * right;
//
//    ///
//    cvm_vk_dynamic_buffer_allocation_3 * l_child;
//    cvm_vk_dynamic_buffer_allocation_3 * r_child;
//    cvm_vk_dynamic_buffer_allocation_3 * parent;///also used in singly linked list
//
//    uint64_t offset;
//    uint32_t size;
//    int8_t bias;
//    uint8_t free;///if copying contents of 2 allocations as part of defragger then DO NOT copy this variable
//};
//
/////no distinction between static and dynamic allocations
//typedef struct cvm_vk_managed_buffer_3
//{
//    VkBuffer buffer;
//    VkDeviceMemory memory;
//
//    void * mapping;///for use on UMA platforms
//
//    uint64_t total_space;
//    uint64_t used_space;
//    uint64_t base_alignment;
//
//    cvm_vk_dynamic_buffer_allocation_3 * first_unused_allocation;///singly linked list of allocations to assign space to, will only ever require a single new allocation at a time
//    cvm_vk_dynamic_buffer_allocation_3 * rightmost_allocation;///used for setting left/right of new allocations
//
//    uint32_t reserved_allocation_count;/// max allocations expected to be made per frame (amount to expand by when reallocing?)
//    uint32_t unused_allocation_count;
//}
//cvm_vk_managed_buffer_3;
//
//void cvm_vk_create_managed_buffer_3(cvm_vk_managed_buffer_3 * buffer,uint64_t buffer_size,uint32_t reserved_allocation_count,VkBufferUsageFlags usage);///determine alignment from usage flags and round size up to multiple
//void cvm_vk_destroy_managed_buffer_3(cvm_vk_managed_buffer_3 * mb);
//
//uint32_t cvm_vk_acquire_dynamic_buffer_allocation_3(cvm_vk_managed_buffer_3 * mb,uint64_t size);
//void cvm_vk_relinquish_dynamic_buffer_allocation_3(cvm_vk_managed_buffer_3 * mb,uint32_t index);






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
typedef struct cvm_vk_ring_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    uint32_t total_space;///round to PO2
    uint32_t max_offset;///fence, should be updated before multithreading becomes applicable this frame
    //uint32_t required_alignment;///could/should do this per type with offsetting being done to compensate, rounding size allocated to multiple of that base (can assume PO2, or make PO2)
    //bool multithreaded;//implicitly supports multithreading b/c its easy :D
    uint32_t alignment_size_factor;
    atomic_uint_fast32_t space_remaining;
    uint32_t initial_space_remaining;
    ///need to test atomic version isn't (significatly) slower than non-atomic

    ///instead have desired per frame upload space and per frame uniform space? no, that can be handled user side as desired

    void * mapping;///ring buffer should REQUIRE mapability, use as intermediary when main buffer isn't mapable, should test mapping to determine necessary size based on this (user defined behaviour)
}
cvm_vk_ring_buffer;

void cvm_vk_create_ring_buffer(cvm_vk_ring_buffer * rb,VkBufferUsageFlags usage,uint32_t buffer_size);
void cvm_vk_update_ring_buffer(cvm_vk_ring_buffer * rb,uint32_t buffer_size);
void cvm_vk_destroy_ring_buffer(cvm_vk_ring_buffer * rb);

uint32_t cvm_vk_ring_buffer_get_rounded_allocation_size(cvm_vk_ring_buffer * rb,uint32_t allocation_size);

/// as size of ring buffer is (or should be) a product of swapchain image count its probably best to also have an upload buffer shared by all submodules used at initialisation time

void cvm_vk_begin_ring_buffer(cvm_vk_ring_buffer * rb);
uint32_t cvm_vk_end_ring_buffer(cvm_vk_ring_buffer * rb);

void * cvm_vk_get_ring_buffer_allocation(cvm_vk_ring_buffer * rb,uint32_t allocation_size,VkDeviceSize * acquired_offset);///alignment_factor should be power of 2

void cvm_vk_relinquish_ring_buffer_space(cvm_vk_ring_buffer * rb,uint32_t * relinquished_space);

/// function to copy from ring buffer to both managed buffer and texture/image here??


///ring buffer paradigm works well for upload/staging buffer but not as well for uploading rendering data (instance and uniform) may want separate design with fixed maximum sizes?



#endif

