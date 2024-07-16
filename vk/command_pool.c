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


void cvm_vk_command_pool_initialise(const cvm_vk_device * device, cvm_vk_command_pool * pool, uint32_t device_queue_family_index, uint32_t device_queue_index)
{
    uint32_t i;

    VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=device_queue_family_index,
    };

    CVM_VK_CHECK(vkCreateCommandPool(device->device,&command_pool_create_info,NULL,&pool->pool));

    pool->device_queue_family_index=device_queue_family_index;
    pool->device_queue_index=device_queue_index;
    pool->acquired_buffer_count=0;
    pool->submitted_buffer_count=0;
    pool->total_buffer_count=4;
    pool->buffers=malloc(sizeof(VkCommandBuffer)*4);

    VkCommandBufferAllocateInfo allocate_info=
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext=NULL,
        .commandPool=pool->pool,
        .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount=4,
    };

    CVM_VK_CHECK(vkAllocateCommandBuffers(device->device,&allocate_info,pool->buffers));
}

void cvm_vk_command_pool_terminate(const cvm_vk_device * device, cvm_vk_command_pool * pool)
{
    vkFreeCommandBuffers(device->device,pool->pool,pool->total_buffer_count,pool->buffers);
    free(pool->buffers);

    vkDestroyCommandPool(device->device,pool->pool,NULL);
}

void cvm_vk_command_pool_reset(const cvm_vk_device * device, cvm_vk_command_pool * pool)
{
    vkResetCommandPool(device->device,pool->pool,0);
    assert(pool->acquired_buffer_count==pool->submitted_buffer_count);///not all acquired command buffers were submitted
    pool->acquired_buffer_count=0;
    pool->submitted_buffer_count=0;
}






void cvm_vk_command_pool_acquire_command_buffer(const cvm_vk_device * device, cvm_vk_command_pool * pool, cvm_vk_command_buffer * command_buffer)
{
    if(pool->acquired_buffer_count==pool->total_buffer_count)
    {
        pool->buffers=realloc(pool->buffers,sizeof(VkCommandBuffer)*(pool->total_buffer_count+4));

        VkCommandBufferAllocateInfo allocate_info=
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext=NULL,
            .commandPool=pool->pool,
            .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount=4,
        };

        CVM_VK_CHECK(vkAllocateCommandBuffers(device->device,&allocate_info,pool->buffers+pool->total_buffer_count));
        pool->total_buffer_count+=4;
    }

    command_buffer->parent_pool=pool;
    command_buffer->buffer=pool->buffers[pool->acquired_buffer_count++];
    command_buffer->signal_count=0;
    command_buffer->wait_count=0;
    command_buffer->present_completion_moment=NULL;

    VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext=NULL,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo=NULL
    };

    CVM_VK_CHECK(vkBeginCommandBuffer(command_buffer->buffer, &command_buffer_begin_info));
}

void cvm_vk_command_buffer_submit(const cvm_vk_device * device, cvm_vk_command_pool * pool, cvm_vk_command_buffer * command_buffer, VkPipelineStageFlags2 completion_signal_stages, cvm_vk_timeline_semaphore_moment * completion_moment)
{
    const cvm_vk_device_queue_family * queue_family;
    cvm_vk_device_queue * queue;

    queue_family=device->queue_families+pool->device_queue_family_index;
    assert(pool->device_queue_index<queue_family->queue_count);
    queue=queue_family->queues+pool->device_queue_index;

    CVM_VK_CHECK(vkEndCommandBuffer(command_buffer->buffer));

    assert(command_buffer->signal_count<4);
    command_buffer->signal_info[command_buffer->signal_count++]=cvm_vk_timeline_semaphore_signal_submit_info(&queue->timeline,completion_signal_stages,completion_moment);

    if(command_buffer->present_completion_moment)/// REMOVE THIS
    {
        *command_buffer->present_completion_moment=*completion_moment;
    }

    VkSubmitInfo2 submit_info=
    {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext=NULL,
        .flags=0,
        .waitSemaphoreInfoCount=command_buffer->wait_count,
        .pWaitSemaphoreInfos=command_buffer->wait_info,
        .commandBufferInfoCount=1,
        .pCommandBufferInfos=(VkCommandBufferSubmitInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext=NULL,
                .commandBuffer=command_buffer->buffer,
                .deviceMask=0
            }
        },
        .signalSemaphoreInfoCount=command_buffer->signal_count,
        .pSignalSemaphoreInfos=command_buffer->signal_info,
    };

    CVM_VK_CHECK(vkQueueSubmit2(queue->queue, 1, &submit_info, VK_NULL_HANDLE));

    pool->submitted_buffer_count++;
}


void cvm_vk_command_buffer_wait_on_timeline_moment(cvm_vk_command_buffer * command_buffer, const cvm_vk_timeline_semaphore_moment * moment, VkPipelineStageFlags2 wait_stages)
{
    assert(command_buffer->wait_count<11);

    command_buffer->wait_info[command_buffer->wait_count++]=cvm_vk_timeline_semaphore_moment_wait_submit_info(moment,wait_stages);
}


void cvm_vk_command_buffer_wait_on_presentable_image_acquisition(cvm_vk_command_buffer * command_buffer, cvm_vk_swapchain_presentable_image * presentable_image)
{
    assert(command_buffer->wait_count<11);

    /// need to wait on image becoming available before writing to it, writing can only happen 2 ways, cover both
    command_buffer->wait_info[command_buffer->wait_count++]=cvm_vk_binary_semaphore_submit_info(presentable_image->acquire_semaphore,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
}

void cvm_vk_command_buffer_signal_presenting_image_complete(cvm_vk_command_buffer * command_buffer, cvm_vk_swapchain_presentable_image * presentable_image)
{
    #warning remove parent references and just pass the parents in here
    assert(command_buffer->signal_count < 3);
    assert(!presentable_image->present_setup);
    presentable_image->present_setup=true;
    presentable_image->last_use_queue_family=command_buffer->parent_pool->device_queue_family_index;

    bool can_present = presentable_image->parent_swapchain->queue_family_presentable_mask | (1<<command_buffer->parent_pool->device_queue_family_index);
    if(can_present)
    {
        /// signal after any stage that could modify the image contents
        command_buffer->signal_info[command_buffer->signal_count++]=cvm_vk_binary_semaphore_submit_info(presentable_image->present_semaphore,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        command_buffer->present_completion_moment=&presentable_image->last_use_moment;///REMOVE also this feels unsafe as all hell
    }
    else
    {
        presentable_image->qfot_required=true;

        command_buffer->signal_info[command_buffer->signal_count++]=cvm_vk_binary_semaphore_submit_info(presentable_image->qfot_semaphore,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        VkDependencyInfo present_relinquish_dependencies=
        {
            .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext=NULL,
            .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT,
            .memoryBarrierCount=0,
            .pMemoryBarriers=NULL,
            .bufferMemoryBarrierCount=0,
            .pBufferMemoryBarriers=NULL,
            .imageMemoryBarrierCount=1,
            .pImageMemoryBarriers=(VkImageMemoryBarrier2[1])
            {
                {
                    .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext=NULL,
                    .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .srcAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
                    /// ignored by QFOT??
                    .dstStageMask=0,///no relevant stage representing present... (afaik), maybe VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT ??
                    .dstAccessMask=0,///should be 0 by spec
                    .oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,///colour attachment optimal? modify  renderpasses as necessary to accommodate this (must match present acquire)
                    .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .srcQueueFamilyIndex=command_buffer->parent_pool->device_queue_family_index,
                    .dstQueueFamilyIndex=presentable_image->parent_swapchain->fallback_present_queue_family,
                    .image=presentable_image->image,
                    .subresourceRange=(VkImageSubresourceRange)
                    {
                        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel=0,
                        .levelCount=1,
                        .baseArrayLayer=0,
                        .layerCount=1
                    }
                }
            },
        };

        vkCmdPipelineBarrier2(command_buffer->buffer,&present_relinquish_dependencies);
    }
}









