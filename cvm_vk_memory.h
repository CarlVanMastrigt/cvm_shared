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
    /// is possible to make all contents here bitfields (20->12 bytes), no noticeable drop in performance was observed when doing so, need to set CVM_VK_NULL_ALLOCATION_INDEX appropriately if doing so
    /// does limit to 2M max allocations, but this should be more than enough
    cvm_vk_dynamic_buffer_allocation * left;
    cvm_vk_dynamic_buffer_allocation * right;

    union
    {
        cvm_vk_dynamic_buffer_allocation * next;
        uint32_t heap_index;
    }link;

    uint32_t offset;/// actual offset = base_size<<offset_factor base allocation size 10 allows 16G buffer (which i quite like)
    ///get bit for "is_right_buffer" by 1 & (offset >> size_factor) during recombination calculations (256M max buffer size and 256 byte min gives the required size of this)
    uint32_t size_factor:5;
    uint32_t available:1;///does not have contents, can be retrieved by new allocation or recombined into
    uint32_t is_base:1;///need to free this pointer at end
};

///need to ensure heap ALWAYS has enough space for this test,create heap with max number of allocations! (2^16)
typedef struct cvm_vk_availablility_heap
{
    cvm_vk_dynamic_buffer_allocation ** heap;///potentially worth testing the effect of storing in a struct with the pointer to the allocation (avoids double indirection in about half of cases)
    uint32_t space;
    uint32_t count;
}
cvm_vk_availablility_heap;

typedef struct cvm_vk_managed_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    uint64_t total_space;///actually DO need to track total buffer size so that buffer can be cleaned (all static and dynamic allocations removed in one go)
    uint64_t static_offset;/// taken from end of buffer
    uint32_t dynamic_offset;/// taken from start of buffer, is a multiple of base offset, actual offset = dynamic_offset<<base_dynamic_allocation_size_factor
    ///recursively free dynamic allocations (if they are the last allocation) when making available
    ///give error if dynamic_offset would become greater than static_offset
    bool multithreaded;
    atomic_uint_fast32_t spinlock;

    uint32_t reserved_allocation_count;/// max allocations expected to be made per frame (amount to expand by when reallocing?)
    cvm_vk_dynamic_buffer_allocation * first_unused_allocation;///singly linked list of allocations to assign space to
    uint32_t unused_allocation_count;

    cvm_vk_dynamic_buffer_allocation * rightmost_allocation;///used for setting left/right of new allocations

    ///consider making following fixed array with max size of ~24 (whole struct should fit on 2-3 cache lines)
    cvm_vk_availablility_heap * available_dynamic_allocations;
    ///intelligent application of last pointer to linked list creation isn't applicable, cannot reference/dereference bitfield!
    uint32_t num_dynamic_allocation_sizes;///essentially max value of dynamic_buffer_allocation.size_factor derived from the exponent of max_allocation_size/base_allocation_size
    uint32_t available_dynamic_allocation_bitmask;
    ///defragger would also need to keep track of active allocations
    //potentially worth it to sort by index (especially if defragging) then "relinquish" last dynamic_buffer_allocation and decrease count
    // ^ can be done recursively by checking if left dynamic_buffer_allocation has been relinquished, then removing it from its linked list
    //     ^ because defragging of these allocations requires copying contents of one dynamic_buffer_allocation to another this wont work very well at all!
    uint32_t base_dynamic_allocation_size_factor;

    void * mapping;///for use on UMA platforms

}
cvm_vk_managed_buffer;

///wait, number of dynamic_buffer_allocation can be realloced in critical section, right...?
/// also having a threshold of required available dynamic_buffer_allocation is reasonable as staging buffer limits new contents anyway

/// may want to register all modules memory together (before creating allocation/buffers) so that they can share a memory allocation

/// may be possible to have multiple allocations/buffers for enabling expanding memory, would be a good use of leftover bits in cvm_vk_dynamic_buffer_index (and is also possible in cvm_vk_static_buffer_offset)
///     ^ can have bitmask of available allocation sizes in managed binary tree across all allocations
///     ^ though tbh this whole idea is definitely overkill, and still needs a way to manage failure case of system running out of memory... (do as advanced version?) (current imp. will likely be quicker)


///instead could allocate from power of 2 sections, storing neighbouring blocks when difference from power of 2 warrants it, and using start-end alignment to allow replacing paired block of mem
/// would still allocate from power of 2 sections and free them in similar way...

void cvm_vk_create_managed_buffer(cvm_vk_managed_buffer * buffer,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,uint32_t reserved_allocation_count,VkBufferUsageFlags usage,bool multithreaded);
void cvm_vk_destroy_managed_buffer(cvm_vk_managed_buffer * mb);

void cvm_vk_update_managed_buffer_reservations(cvm_vk_managed_buffer * mb);///creates more allocations as necessary

cvm_vk_dynamic_buffer_allocation * cvm_vk_acquire_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint64_t size);
void cvm_vk_relinquish_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation);

uint64_t cvm_vk_acquire_static_buffer_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment);







///probably want to use pointers for all of these

///can reasonably easily combine the node and the allocation list

typedef struct cvm_vk_dynamic_buffer_allocation_3 cvm_vk_dynamic_buffer_allocation_3;

/// *just* fits in one cache line
struct cvm_vk_dynamic_buffer_allocation_3/// (dont ref this by pointer, only store index)
{
    ///horizontal structure
    cvm_vk_dynamic_buffer_allocation_3 * left;
    cvm_vk_dynamic_buffer_allocation_3 * right;

    ///
    cvm_vk_dynamic_buffer_allocation_3 * l_child;
    cvm_vk_dynamic_buffer_allocation_3 * r_child;
    cvm_vk_dynamic_buffer_allocation_3 * parent;///also used in singly linked list

    uint64_t offset;
    uint32_t size;
    int8_t bias;
    uint8_t free;///if copying contents of 2 allocations as part of defragger then DO NOT copy this variable
};

///no distinction between static and dynamic allocations
typedef struct cvm_vk_managed_buffer_3
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    void * mapping;///for use on UMA platforms

    uint64_t total_space;
    uint64_t used_space;
    uint64_t base_alignment;

    cvm_vk_dynamic_buffer_allocation_3 * first_unused_allocation;///singly linked list of allocations to assign space to, will only ever require a single new allocation at a time
    cvm_vk_dynamic_buffer_allocation_3 * rightmost_allocation;///used for setting left/right of new allocations

    uint32_t reserved_allocation_count;/// max allocations expected to be made per frame (amount to expand by when reallocing?)
    uint32_t unused_allocation_count;
}
cvm_vk_managed_buffer_3;

void cvm_vk_create_managed_buffer_3(cvm_vk_managed_buffer_3 * buffer,uint64_t buffer_size,uint32_t reserved_allocation_count,VkBufferUsageFlags usage);///determine alignment from usage flags and round size up to multiple
void cvm_vk_destroy_managed_buffer_3(cvm_vk_managed_buffer_3 * mb);

uint32_t cvm_vk_acquire_dynamic_buffer_allocation_3(cvm_vk_managed_buffer_3 * mb,uint64_t size);
void cvm_vk_relinquish_dynamic_buffer_allocation_3(cvm_vk_managed_buffer_3 * mb,uint32_t index);

















#endif

