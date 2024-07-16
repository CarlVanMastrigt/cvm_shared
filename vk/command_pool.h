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

#ifndef CVM_VK_H
#include "cvm_vk.h"
#endif


#ifndef CVM_VK_COMMAND_POOL_H
#define CVM_VK_COMMAND_POOL_H

typedef struct cvm_vk_command_pool
{
    uint32_t device_queue_family_index;
    uint32_t device_queue_index;

    VkCommandPool pool;

    VkCommandBuffer * buffers;/// this almost certainly wants to be a list/stack and init as necessary
    uint32_t acquired_buffer_count;
    uint32_t submitted_buffer_count;
    uint32_t total_buffer_count;

    /// when the last of the work done by the command pool has been completed (may want this to become a stack/list)
    /// may need multiple buffers if this "entry" has more than one command buffer
//    cvm_vk_timeline_semaphore_moment completion_moment;///if null handle is not presently in flight

    /// cleanup information? (buffer sections to relinquish &c. framebuffer images that have expired? -- though that probably wants special handling)
}
cvm_vk_command_pool;

typedef struct cvm_vk_command_buffer
{
    cvm_vk_command_pool * parent_pool;
    VkCommandBuffer buffer;

    uint32_t signal_count;
    uint32_t wait_count;

    cvm_vk_timeline_semaphore_moment * present_completion_moment;///REMOVE (when last use moment can be removed)

    VkSemaphoreSubmitInfo signal_info[4];/// left open in case we add a way to signal arbitrary semphores
    VkSemaphoreSubmitInfo wait_info[11];
    ///above relatively arbitrarily chosen for struct size reasons
}
cvm_vk_command_buffer;

void cvm_vk_command_pool_initialise(const cvm_vk_device * device, cvm_vk_command_pool * pool, uint32_t device_queue_family_index, uint32_t device_queue_index);
void cvm_vk_command_pool_terminate(const cvm_vk_device * device, cvm_vk_command_pool * pool);
void cvm_vk_command_pool_reset(const cvm_vk_device * device, cvm_vk_command_pool * pool);

void cvm_vk_command_pool_acquire_command_buffer(const cvm_vk_device * device, cvm_vk_command_pool * pool, cvm_vk_command_buffer * command_buffer);
void cvm_vk_command_buffer_submit(const cvm_vk_device * device, cvm_vk_command_pool * pool, cvm_vk_command_buffer * command_buffer, VkPipelineStageFlags2 completion_signal_stages, cvm_vk_timeline_semaphore_moment * completion_moment);

void cvm_vk_command_buffer_wait_on_timeline_moment(cvm_vk_command_buffer * command_buffer, const cvm_vk_timeline_semaphore_moment * moment, VkPipelineStageFlags2 wait_stages);

/// these should be called exactly(?) once per presentable image, is really a combination of the 2 paradigms, so could go here or in swapchain
void cvm_vk_command_buffer_wait_on_presentable_image_acquisition(cvm_vk_command_buffer * command_buffer, cvm_vk_swapchain_presentable_image * presentable_image);/// can potentially be called multiple times, must be called before presentable_image is written to
void cvm_vk_command_buffer_signal_presenting_image_complete(cvm_vk_command_buffer * command_buffer, cvm_vk_swapchain_presentable_image * presentable_image);///all modification of presentable_image has completed, must be called after last modification of image data


#endif


