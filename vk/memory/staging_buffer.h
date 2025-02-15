/**
Copyright 2024 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef solipsix_H
#include "solipsix.h"
#endif


#ifndef CVM_VK_STAGING_BUFFER_H
#define CVM_VK_STAGING_BUFFER_H


/// for when its desirable to support a simple staging buffer or a more complex staging manager backing
struct cvm_vk_staging_buffer_allocation
{
    struct cvm_vk_staging_buffer_* parent;
    VkDeviceSize acquired_offset;/// offset of the above location
    char * mapping;/// has already been offset
    uint32_t segment_index;
    bool flushed;
};

struct cvm_vk_staging_buffer_segment
{
    ///fuck, what happens if multiple queues require this moment!? (worry about it later)
    cvm_vk_timeline_semaphore_moment moment_of_last_use;/// when this region is finished being used
    VkDeviceSize offset;
    VkDeviceSize size;/// must be greater than or equal to start (can be greater than buffer_size)
};

SOL_QUEUE(struct cvm_vk_staging_buffer_segment, cvm_vk_staging_buffer_segment, 16)

struct cvm_vk_staging_buffer_
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    bool mapping_coherent;

    bool threads_waiting_on_semaphore_setup;

    bool terminating;///debug
    /// separate requests for high prio from requests for low priority?

    char * mapping;/// mapping address
    VkDeviceSize alignment;
    VkDeviceSize buffer_size;
    VkDeviceSize reserved_high_priority_space;

    VkDeviceSize current_offset;
    VkDeviceSize remaining_space;///end of available_space, must be greater than current_offset, may be up to 2*buffer_size-1

    mtx_t access_mutex;
    cnd_t setup_stall_condition;

    cvm_vk_staging_buffer_segment_queue segment_queue;
};

VkResult cvm_vk_staging_buffer_initialise(struct cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device, VkBufferUsageFlags usage, VkDeviceSize buffer_size, VkDeviceSize reserved_high_priority_space);
void cvm_vk_staging_buffer_terminate(struct cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device);

struct cvm_vk_staging_buffer_allocation cvm_vk_staging_buffer_allocation_acquire(struct cvm_vk_staging_buffer_ * staging_buffer, const cvm_vk_device * device, VkDeviceSize requested_space, bool high_priority);/// offset, link to index, void pointer to copy
/// need to be able to stall on this being completed...

void cvm_vk_staging_buffer_allocation_flush_range(const struct cvm_vk_staging_buffer_ * staging_buffer, const struct cvm_vk_device * device, struct cvm_vk_staging_buffer_allocation* allocation, VkDeviceSize relative_offset, VkDeviceSize size);

/// allocation index is the index param of struct reurned by `cvm_vk_staging_buffer_reserve_allocation`
void cvm_vk_staging_buffer_allocation_release(struct cvm_vk_staging_buffer_allocation* allocation, cvm_vk_timeline_semaphore_moment moment_of_last_use);

VkDeviceSize cvm_vk_staging_buffer_allocation_align_offset(const struct cvm_vk_staging_buffer_ * staging_buffer, VkDeviceSize offset);

#endif


