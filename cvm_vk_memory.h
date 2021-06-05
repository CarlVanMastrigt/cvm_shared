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


//#define CVM_VK_NULL_ALLOCATION_INDEX 0x1FFFFF
#define CVM_VK_NULL_ALLOCATION_INDEX 0xFFFFFFFF
/// ^ max representable index for any dynamic buffer allocation (1 than max number of allocations)

/// buffer offset is actually MORE important than reference index capacity (each reference identifies a unique offset!)

/// 2M max references (21 bits) and 16M max memory locations (24 bits)

/// despite them not really being necessary, limit offsets of both static and dynamic allocations to the

/// this design doesnt technically allow multithreaded alterations to different parts, but if accessed values go unaltered then it actually will... (this definitely jank, but seems to be defined behaviour)

/// probably worth testing performance if this used base size primitives (uint32_t) and was a little larger (24 bytes vs 16)... (yeah its almost certianly woth it)
//typedef struct cvm_vk_dynamic_buffer_allocation/// (dont ref this by pointer, only store index)
//{
//    uint64_t valid:1;///memory has been copied to this location
//
//    ///in following; invalid/null == 0x001FFFFF
//    uint64_t left:21;
//    uint64_t right:21;
//    uint64_t prev:21;
//    uint64_t next:21;///this is also used to store replacement index, possible because when representing memory, this dynamic allocation isn't in any linked list
//    ///                 ^ actually, no, will need to be put in list of buffers that need relinquishing, but these can be singly linked list using prev
//
//    ///due to packing this into a u64 can easily reference more than 4 gig of memory!
//    uint64_t offset:24;/// actual offset = base_size<<offset_factor base allocation size 10 allows 16G buffer (which i quite like)
//    ///get bit for "is_right_buffer" by 1 & (offset >> size_factor) during recombination calculations (256M max buffer size and 256 byte min gives the required size of this)
//    uint64_t size_factor:5;
//
//
//    uint64_t staged:1;/// data has been moved here by transfer queue, so this can be utilised
//    uint64_t replaced:1;///set this when replacement exists in vram; is this even necessary?
//    /// FUCK, if swapping/replacing contents of cvm_vk_dynamic_buffer_allocation as part of defraggimg then above is invalid, entire struct changes as part of defrag
//    /// for now; plan to do defragging in objects parent thread, then depending on which is more useful, use spinlocks (possibly per 256 allocations, intersplicing the locks between allocations) to sync
//    ///         ^ when needing multiple locks always lock lower index first!
//    ///         ^ by placing spinlocks at the end of a cluster, the final index (CVM_VK_NULL_ALLOCATION_INDEX) isn't "wasted"
//    /// or just spit work of defragging across threads (i do like above though tbh)
//    ///remember, this requires reallocation of buffers inside critical section
//    uint64_t relinquished:1;///is available for recombination (for easy checking of left-right adjacent allocatios as part of recombination)
//}
//cvm_vk_dynamic_buffer_allocation;

//typedef enum
//{
//    CVM_VK_DYNAMIC_BUFFER_AVAILABLE=0,///does not represent memory yet
//    CVM_VK_DYNAMIC_BUFFER_ALLOCATED=1,/// represents memory but does not have valid contents yet
//    CVM_VK_DYNAMIC_BUFFER_VALID=2,///has valid contents
//    ///may need more as time goes on
//}
//cvm_vk_dynamic_buffer_allocation_state;

typedef struct cvm_vk_dynamic_buffer_allocation/// (dont ref this by pointer, only store index)
{
    uint32_t left;
    uint32_t right;
    uint32_t prev;
    uint32_t next;///this is also used to store replacement index, possible because when representing memory, this dynamic allocation isn't in any linked list
    ///                 ^ actually, no, will need to be put in list of buffers that need relinquishing, but these can be singly linked list using prev

    uint32_t offset;/// actual offset = base_size<<offset_factor base allocation size 10 allows 16G buffer (which i quite like)
    ///get bit for "is_right_buffer" by 1 & (offset >> size_factor) during recombination calculations (256M max buffer size and 256 byte min gives the required size of this)
    uint32_t size_factor:5;
    uint32_t available:1;///does not have contents, can be retrieved by new allocation or recombined into

    ///could probably replace these "booleans" with a single state variable that can have all 4(?) possible states
//    uint32_t valid:1;///memory has been copied to this location
//    uint32_t staged:1;/// data has been moved here by transfer queue, so this can be utilised
//    uint32_t replaced:1;///set this when replacement exists in vram; is this even necessary?
//    /// FUCK, if swapping/replacing contents of cvm_vk_dynamic_buffer_allocation as part of defraggimg then above is invalid, entire struct changes as part of defrag
//    /// for now; plan to do defragging in objects parent thread, then depending on which is more useful, use spinlocks (possibly per 256 allocations, intersplicing the locks between allocations) to sync
//    ///         ^ when needing multiple locks always lock lower index first!
//    ///         ^ by placing spinlocks at the end of a cluster, the final index (CVM_VK_NULL_ALLOCATION_INDEX) isn't "wasted"
//    /// or just spit work of defragging across threads (i do like above though tbh)
//    ///remember, this requires reallocation of buffers inside critical section
//    uint32_t relinquished:1;///is available for recombination (for easy checking of left-right adjacent allocatios as part of recombination)
}
cvm_vk_dynamic_buffer_allocation;


//typedef uint32_t cvm_vk_dynamic_buffer_index;
//typedef uint64_t cvm_vk_static_buffer_offset;///raw offset (?) in managed buffer (multiple of base mag?)

typedef struct cvm_vk_managed_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    ///uint64_t total_space;
    uint64_t static_offset;/// taken from end of buffer
    uint32_t dynamic_offset;/// taken from start of buffer, is a multiple of base offset, actual offset = dynamic_offset<<base_dynamic_allocation_size_factor
    ///recursively free dynamic allocations (if they are the last allocation) when making available
    ///give error if dynamic_offset would become greater than static_offset

    cvm_vk_dynamic_buffer_allocation * dynamic_allocations;
    uint32_t dynamic_allocation_space;
    uint32_t reserved_allocation_count;/// max allocations expected to be made per frame (amount to expand by when reallocing?)
    uint32_t first_unused_allocation;///singly linked list of allocations to assign space to
    uint32_t unused_allocation_count;

    uint32_t rightmost_allocation;///used for setting left/right of new allocations

    ///consider making following fixed array with max size of ~24 (whole struct should fit on 2-3 cache lines)
    uint32_t * available_dynamic_allocations_start;///array with index reflecting each possible size, is index of first in doubly linked list of those sizes
    uint32_t * available_dynamic_allocations_end;
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


void cvm_vk_create_managed_buffer(cvm_vk_managed_buffer * buffer,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,uint32_t reserved_allocation_count,VkBufferUsageFlags usage);


#endif

