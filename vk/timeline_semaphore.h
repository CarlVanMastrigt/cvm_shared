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


#ifndef CVM_VK_TIMELINE_SEMAPHORE_H
#define CVM_VK_TIMELINE_SEMAPHORE_H

typedef struct cvm_vk_timeline_semaphore
{
    VkSemaphore semaphore;
    uint64_t value;
}
cvm_vk_timeline_semaphore;

typedef struct cvm_vk_timeline_semaphore_moment
{
    VkSemaphore semaphore;
    uint64_t value;
}
cvm_vk_timeline_semaphore_moment;

void cvm_vk_create_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore, cvm_vk_device * device);
void cvm_vk_destroy_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore, cvm_vk_device * device);

inline VkSemaphoreSubmitInfo cvm_vk_create_timeline_semaphore_signal_submit_info(cvm_vk_timeline_semaphore * ts,VkPipelineStageFlags2 stages, cvm_vk_timeline_semaphore_moment * created_moment);
VkSemaphoreSubmitInfo cvm_vk_create_timeline_semaphore_wait_submit_info(cvm_vk_timeline_semaphore * ts,VkPipelineStageFlags2 stages);
void cvm_vk_wait_on_timeline_semaphore(cvm_vk_device * device,const cvm_vk_timeline_semaphore_moment * moment);

#endif

