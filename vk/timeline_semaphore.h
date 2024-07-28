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
#define cvm_vk_timeline_semaphore_moment_null ((cvm_vk_timeline_semaphore_moment){.semaphore=VK_NULL_HANDLE,.value=0})

void cvm_vk_timeline_semaphore_initialise(const cvm_vk_device * device,cvm_vk_timeline_semaphore * timeline_semaphore);
void cvm_vk_timeline_semaphore_terminate(const cvm_vk_device * device,cvm_vk_timeline_semaphore * timeline_semaphore);

VkSemaphoreSubmitInfo cvm_vk_timeline_semaphore_signal_submit_info(cvm_vk_timeline_semaphore * ts,VkPipelineStageFlags2 stages, cvm_vk_timeline_semaphore_moment * created_moment);
VkSemaphoreSubmitInfo cvm_vk_timeline_semaphore_moment_wait_submit_info(const cvm_vk_timeline_semaphore_moment * moment,VkPipelineStageFlags2 stages);

void cvm_vk_timeline_semaphore_moment_wait(const cvm_vk_device * device,const cvm_vk_timeline_semaphore_moment * moment);
bool cvm_vk_timeline_semaphore_moment_query(const cvm_vk_device * device,const cvm_vk_timeline_semaphore_moment * moment);///returns true if this moment has elapsed

#endif

