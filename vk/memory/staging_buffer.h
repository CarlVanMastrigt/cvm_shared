/**
Copyright 2024 Carl van Mastrigt

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


#ifndef CVM_VK_STAGING_BUFFER_H
#define CVM_VK_STAGING_BUFFER_H

/// rename? shared memory?

/// for when its desirable to support a simple staging buffer or a more complex staging manager backing
typedef struct cvm_vk_staging_buffer_allocation
{
    char * mapping;/// has already been offset
    VkDeviceSize acquired_offset;/// offset of the above location
    uint32_t segment_index;
}
cvm_vk_staging_buffer_allocation;


////
typedef struct cvm_vk_staging_buffer_segment
{
    ///fuck, what happens if multiple queues require this moment!? (worry about it later)
    cvm_vk_timeline_semaphore_moment moment_of_last_use;/// when this region is finished being used
    VkDeviceSize offset;
    VkDeviceSize size;/// must be greater than or equal to start (can be greater than buffer_size)
}
cvm_vk_staging_buffer_segment;



///intended for single threaded/single purpose use (direct upload)

///actually do this as a ring buffer
/// small high priority segment set aside, accesses to that segment may circumvent ordering and delays and/or be put on higher priority queue, should this be using gates to control/delay work!?
/// could allow re-ordering and skipping of particular segments?

/// maybe because things are so short lived: just fuck it, allow fragmentation, also stalling, just ensure high priority segments go to one end?
/// ring buffer but limit low priority requests when becoming starved? <-- i REALLY like this actually
typedef struct cvm_vk_staging_buffer_
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    bool mapping_coherent;

    bool threads_waiting_on_semaphore_setup;
    /// separate requests for hhigh prio from requests for low priority?

    char * mapping;/// mapping address
    VkDeviceSize alignment;
    VkDeviceSize buffer_size;
    VkDeviceSize reserved_high_priority_space;

    VkDeviceSize current_offset;
    VkDeviceSize remaining_space;///end of available_space, must be greater than current_offset, may be up to 2*buffer_size-1

    mtx_t access_mutex;
    cnd_t setup_stall_condition;

    cvm_vk_staging_buffer_segment * active_segments;
    uint32_t segment_space;
    uint32_t first_active_segment_index;///must wrap??
    uint32_t active_segment_count;
    /// ^ptrs to thread local variables containing conditions, used to register front of queue (maybe)
    /// ^ this doesnt handle spurrious wakeup...
    /// do we even really want use a condition??
    /// wait on either condition or per entry timeline semaphore

    /// a saturated staging buffer when accessed by many high priority requests may stall forever without appropriate manual scheduling to handle it...

}
cvm_vk_staging_buffer_;

void cvm_vk_staging_buffer_initialise(cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device, VkBufferUsageFlags usage, VkDeviceSize buffer_size, VkDeviceSize reserved_high_priority_space);
void cvm_vk_staging_buffer_terminate(cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device);

cvm_vk_staging_buffer_allocation cvm_vk_staging_buffer_reserve_allocation(cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device, VkDeviceSize requested_space, bool high_priority);/// offset, link to index, void pointer to copy
/// need to be able to stall on this being completed...

void cvm_vk_staging_buffer_flush_allocation(const cvm_vk_staging_buffer_ * staging_buffer, const cvm_vk_device * device, const cvm_vk_staging_buffer_allocation * allocation, VkDeviceSize relative_offset, VkDeviceSize size);

/// allocation index is the index param of struct reurned by `cvm_vk_staging_buffer_reserve_allocation`
void cvm_vk_staging_buffer_complete_allocation(cvm_vk_staging_buffer_ * staging_buffer, uint32_t reserved_allocation_segment_index, cvm_vk_timeline_semaphore_moment moment_of_last_use);

VkDeviceSize cvm_vk_staging_buffer_allocation_align_offset(cvm_vk_staging_buffer_ * staging_buffer, VkDeviceSize offset);

typedef struct cvm_vk_staging_shunt_buffer
{
    char * backing;
    VkDeviceSize alignment;
    VkDeviceSize size;
    VkDeviceSize offset;
    /// add check to make sure nothing added after buffer gets copied?
}
cvm_vk_staging_shunt_buffer;

void cvm_vk_staging_shunt_buffer_initialise(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize alignment);
void cvm_vk_staging_shunt_buffer_terminate(cvm_vk_staging_shunt_buffer * buffer);


void cvm_vk_staging_shunt_buffer_reset(cvm_vk_staging_shunt_buffer * buffer);
/// returns pointer to location which can be written, this pointer is only valid until next use
void * cvm_vk_staging_shunt_buffer_add_bytes(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize byte_count);
VkDeviceSize cvm_vk_staging_shunt_buffer_new_segment(cvm_vk_staging_shunt_buffer * buffer);

void cvm_vk_staging_shunt_buffer_copy(cvm_vk_staging_shunt_buffer * buffer, void * dst);

#endif


