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

#include "cvm_shared.h"



void cvm_vk_create_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore, cvm_vk_device * device)
{
    VkSemaphoreCreateInfo timeline_semaphore_create_info=(VkSemaphoreCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=(VkSemaphoreTypeCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                .pNext=NULL,
                .semaphoreType=VK_SEMAPHORE_TYPE_TIMELINE,
                .initialValue=0,
            }
        },
        .flags=0
    };

    CVM_VK_CHECK(vkCreateSemaphore(device->device,&timeline_semaphore_create_info,NULL,&timeline_semaphore->semaphore));
    timeline_semaphore->value=0;
}

void cvm_vk_destroy_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore, cvm_vk_device * device)
{
    vkDestroySemaphore(device->device,timeline_semaphore->semaphore,NULL);
}

VkSemaphoreSubmitInfo cvm_vk_create_timeline_semaphore_signal_submit_info(cvm_vk_timeline_semaphore * ts,VkPipelineStageFlags2 stages, cvm_vk_timeline_semaphore_moment * created_moment)
{
    ts->value++;

    if(created_moment)
    {
        created_moment->semaphore=ts->semaphore;
        created_moment->value=ts->value;
    }

    return (VkSemaphoreSubmitInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext=NULL,
        .semaphore=ts->semaphore,
        .value= ts->value,
        .stageMask=stages,
        .deviceIndex=0
    };
}

VkSemaphoreSubmitInfo cvm_vk_create_timeline_semaphore_wait_submit_info(const cvm_vk_timeline_semaphore_moment * moment,VkPipelineStageFlags2 stages)
{
    return (VkSemaphoreSubmitInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext=NULL,
        .semaphore=moment->semaphore,
        .value=moment->value,
        .stageMask=stages,
        .deviceIndex=0
    };
}

void cvm_vk_wait_on_timeline_semaphore(cvm_vk_device * device,const cvm_vk_timeline_semaphore_moment * moment)
{
    VkSemaphoreWaitInfo wait=
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext=NULL,
        .flags=0,
        .semaphoreCount=1,
        .pSemaphores=&moment->semaphore,
        .pValues=&moment->value,
    };
    CVM_VK_CHECK(vkWaitSemaphores(device->device,&wait,CVM_VK_DEFAULT_TIMEOUT));
}
