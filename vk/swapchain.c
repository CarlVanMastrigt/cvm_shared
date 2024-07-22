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

static inline void cvm_vk_swapchain_presentable_image_initialise(cvm_vk_swapchain_presentable_image * presentable_image, const cvm_vk_device * device, VkImage image, uint16_t index, const cvm_vk_swapchain_instance * parent_swapchain_instance)
{
    uint32_t i;

    presentable_image->image = image;
    presentable_image->index = index;
    presentable_image->parent_swapchain_instance = parent_swapchain_instance;

    presentable_image->present_semaphore = cvm_vk_create_binary_semaphore(device);
    presentable_image->qfot_semaphore = cvm_vk_create_binary_semaphore(device);

    presentable_image->acquire_semaphore=VK_NULL_HANDLE;///set using one of the available image_acquisition_semaphores
    presentable_image->acquired=false;
    presentable_image->qfot_required=false;
    presentable_image->present_attempted=false;
    presentable_image->present_successful=false;

    presentable_image->last_use_queue_family = CVM_INVALID_U32_INDEX;
    presentable_image->last_use_moment = cvm_vk_timeline_semaphore_moment_null;

    presentable_image->present_acquire_command_buffers = malloc(sizeof(VkCommandBuffer) * device->queue_family_count);
    for(i=0;i<device->queue_family_count;i++)
    {
        presentable_image->present_acquire_command_buffers[i] = VK_NULL_HANDLE;
    }

    ///image view
    presentable_image->image_view = VK_NULL_HANDLE;
    VkImageViewCreateInfo image_view_create_info=(VkImageViewCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .image=image,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .format=parent_swapchain_instance->surface_format.format,
        .components=(VkComponentMapping)
        {
            .r=VK_COMPONENT_SWIZZLE_IDENTITY,
            .g=VK_COMPONENT_SWIZZLE_IDENTITY,
            .b=VK_COMPONENT_SWIZZLE_IDENTITY,
            .a=VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange=(VkImageSubresourceRange)
        {
            .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel=0,
            .levelCount=1,
            .baseArrayLayer=0,
            .layerCount=1
        }
    };
    CVM_VK_CHECK(vkCreateImageView(device->device, &image_view_create_info, NULL, &presentable_image->image_view));
}

static inline void cvm_vk_swapchain_presentable_image_terminate(cvm_vk_swapchain_presentable_image * presentable_image, const cvm_vk_device * device, const cvm_vk_swapchain_instance * parent_swapchain_instance)
{
    uint32_t i;
    vkDestroyImageView(device->device,presentable_image->image_view,NULL);

    assert(presentable_image->acquire_semaphore == VK_NULL_HANDLE);

    vkDestroySemaphore(device->device, presentable_image->present_semaphore, NULL);
    vkDestroySemaphore(device->device, presentable_image->qfot_semaphore, NULL);

    ///queue ownership transfer stuff
    for(i=0;i<device->queue_family_count;i++)
    {
        if(presentable_image->present_acquire_command_buffers[i]!=VK_NULL_HANDLE)
        {
            /// FUUUUCK, this BS probably means each instance wants its own set of command pools, freeing command pools can cause fragmentation! WTF should I do!?
            vkFreeCommandBuffers(device->device, device->queue_families[parent_swapchain_instance->fallback_present_queue_family].internal_command_pool, 1, presentable_image->present_acquire_command_buffers+i);
            #warning requires synchronization! (uses shared command pool, ergo not thread safe)
        }
    }
    free(presentable_image->present_acquire_command_buffers);
}

static inline int cvm_vk_swapchain_instance_initialise(cvm_vk_swapchain_instance * instance, const cvm_vk_device * device, const cvm_vk_surface_swapchain * swapchain, uint32_t generation, VkSwapchainKHR old_swapchain)
{
    uint32_t i,j,fallback_present_queue_family,format_count,present_mode_count,image_acquisition_semaphore_count;
    VkBool32 surface_supported;
    VkSurfaceFormatKHR * formats;
    VkPresentModeKHR * present_modes;
    VkImage * images;

    instance->generation = generation;
    instance->acquired_image_count = 0;

    /// select format (image format and colourspace)
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device, swapchain->setup_info.surface, &format_count, NULL));
    formats = malloc(sizeof(VkSurfaceFormatKHR)*format_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device, swapchain->setup_info.surface, &format_count, formats));

    instance->surface_format = formats[0];///fallback in case preferred isnt found, required to have at least 1 at this point
    for(i=0;i<format_count;i++)
    {
        if(swapchain->setup_info.preferred_surface_format.colorSpace==formats[i].colorSpace && swapchain->setup_info.preferred_surface_format.format==formats[i].format)
        {
            ///preferred format exists
            instance->surface_format = swapchain->setup_info.preferred_surface_format;
            break;
        }
    }
    free(formats);


    ///select screen present mode
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, swapchain->setup_info.surface, &present_mode_count, NULL));
    present_modes = malloc(sizeof(VkPresentModeKHR)*present_mode_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, swapchain->setup_info.surface, &present_mode_count, present_modes));

    instance->present_mode = VK_PRESENT_MODE_FIFO_KHR;/// this is required to be supported
    for(i=0;i<present_mode_count;i++)
    {
        if(present_modes[i] == swapchain->setup_info.preferred_present_mode)
        {
            instance->present_mode = swapchain->setup_info.preferred_present_mode;
            break;
        }
    }
    free(present_modes);


    /// search for presentable queue families
    instance->queue_family_presentable_mask=0;
    i=device->queue_family_count;
    instance->fallback_present_queue_family = CVM_INVALID_U32_INDEX;
    while(i--)/// search last to first so that fallback is minimum supported
    {
        CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device, i, swapchain->setup_info.surface, &surface_supported));
        if(surface_supported)
        {
            instance->fallback_present_queue_family = i;
            instance->queue_family_presentable_mask |= 1<<i;
        }
    }
    if(instance->queue_family_presentable_mask==0) return -1;///cannot present to this surface!
    assert(instance->fallback_present_queue_family != CVM_INVALID_U32_INDEX);
    instance->fallback_present_queue_family = device->fallback_present_queue_family_index;
    #warning ABOVE IS FOR TESTING


    /// check surface capabilities and create a the new swapchain
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, swapchain->setup_info.surface, &instance->surface_capabilities));

    if(instance->surface_capabilities.supportedUsageFlags & swapchain->setup_info.usage_flags != swapchain->setup_info.usage_flags) return -1;///intened usage not supported
    if((instance->surface_capabilities.maxImageCount != 0) && (instance->surface_capabilities.maxImageCount < swapchain->setup_info.min_image_count)) return -1;///cannot suppirt minimum image count
    if(instance->surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR == 0)return -1;///compositing not supported
    /// would like to support different compositing, need to extend to allow this

    VkSwapchainCreateInfoKHR swapchain_create_info=(VkSwapchainCreateInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext=NULL,
        .flags=0,
        .surface=swapchain->setup_info.surface,
        .minImageCount=CVM_MAX(instance->surface_capabilities.minImageCount, swapchain->setup_info.min_image_count),
        .imageFormat=instance->surface_format.format,
        .imageColorSpace=instance->surface_format.colorSpace,
        .imageExtent=instance->surface_capabilities.currentExtent,
        .imageArrayLayers=1,
        .imageUsage=swapchain->setup_info.usage_flags,
        .imageSharingMode=VK_SHARING_MODE_EXCLUSIVE,///means QFOT is necessary, thus following should not be prescribed
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL,
        .preTransform=instance->surface_capabilities.currentTransform,
        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode=instance->present_mode,
        .clipped=VK_TRUE,
        .oldSwapchain=old_swapchain,
    };

    instance->swapchain = VK_NULL_HANDLE;
    CVM_VK_CHECK(vkCreateSwapchainKHR(device->device,&swapchain_create_info,NULL,&instance->swapchain));


    /// get images and set up image data
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(device->device, instance->swapchain, &instance->image_count, NULL));
    images = malloc(sizeof(VkImage) * instance->image_count);
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(device->device, instance->swapchain, &instance->image_count, images));

    instance->presentable_images = malloc(sizeof(cvm_vk_swapchain_presentable_image) * instance->image_count);
    for(i=0;i<instance->image_count;i++)
    {
        cvm_vk_swapchain_presentable_image_initialise(instance->presentable_images + i, device, images[i], i, instance);
    }

    free(images);


    image_acquisition_semaphore_count = instance->image_count + 1;
    instance->image_acquisition_semaphores = malloc(sizeof(VkSemaphore) * image_acquisition_semaphore_count);/// need 1 more than image count
    for(i=0;i<image_acquisition_semaphore_count;i++)
    {
        instance->image_acquisition_semaphores[i]=cvm_vk_create_binary_semaphore(device);
    }


    cvm_vk_create_swapchain_dependent_defaults(instance->surface_capabilities.currentExtent.width,instance->surface_capabilities.currentExtent.height);
    #warning make these defaults part of the swapchain_instance!?

    return 0;
}

static inline void cvm_vk_swapchain_instance_terminate(cvm_vk_swapchain_instance * instance, const cvm_vk_device * device)
{
    uint32_t i,image_acquisition_semaphore_count;

    /// free up instances ??

    #warning FUCK FUCK FUCK how do we actually track completion
    #warning need to ensure this instance has catually completed

    assert(instance->acquired_image_count == 0);///MUST WAIT TILL ALL IMAGES ARE RETURNED BEFORE TERMINATING SWAPCHAIN

    cvm_vk_destroy_swapchain_dependent_defaults();
    #warning make above defaults part of the swapchain_instance


    image_acquisition_semaphore_count = instance->image_count + 1;
    for(i=0;i<image_acquisition_semaphore_count;i++)
    {
        vkDestroySemaphore(device->device, instance->image_acquisition_semaphores[i], NULL);
    }
    free(instance->image_acquisition_semaphores);

    for(i=0;i<instance->image_count;i++)
    {
        cvm_vk_swapchain_presentable_image_terminate(instance->presentable_images+i, device, instance);
    }
    free(instance->presentable_images);

    vkDestroySwapchainKHR(device->device, instance->swapchain, NULL);
}



int cvm_vk_swapchain_initialse(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, const cvm_vk_swapchain_setup * setup)
{
    swapchain->setup_info = *setup;

    swapchain->metering_fence=cvm_vk_create_fence(device,false);
    swapchain->metering_fence_active=false;
    swapchain->rendering_resources_valid = false;

    if(setup->preferred_surface_format.format == VK_FORMAT_UNDEFINED)
    {
        swapchain->setup_info.preferred_surface_format=(VkSurfaceFormatKHR){.format=VK_FORMAT_B8G8R8A8_SRGB,.colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    cvm_vk_swapchain_instance_queue_initialise(&swapchain->swapchain_queue);
}

void cvm_vk_swapchain_terminate(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain)
{
    cvm_vk_swapchain_instance * instance;
    cvm_vk_swapchain_presentable_image * presentable_image;
    uint32_t i;

    vkDeviceWaitIdle(device->device);
    /// above required b/c presently there is no way to know that the semaphore used by present has actually been used by WSI

    while((instance = cvm_vk_swapchain_instance_queue_dequeue_ptr(&swapchain->swapchain_queue)))
    {
        for(i=0;i<instance->image_count;i++)
        {
            presentable_image = instance->presentable_images+i;

            if(presentable_image->acquired)
            {
                assert(presentable_image->last_use_moment.semaphore != VK_NULL_HANDLE);/// probs wrong!
                assert(presentable_image->acquire_semaphore != VK_NULL_HANDLE);

                cvm_vk_timeline_semaphore_moment_wait(device, &presentable_image->last_use_moment);

                assert(instance->acquired_image_count > 0);
                instance->image_acquisition_semaphores[--instance->acquired_image_count] = presentable_image->acquire_semaphore;
                presentable_image->last_use_moment=cvm_vk_timeline_semaphore_moment_null;
                presentable_image->acquire_semaphore = VK_NULL_HANDLE;
                presentable_image->acquired=false;
            }
            else
            {
                assert(presentable_image->last_use_moment.semaphore == VK_NULL_HANDLE);
                assert(presentable_image->acquire_semaphore == VK_NULL_HANDLE);
            }
        }
        assert(instance->acquired_image_count == 0);
        cvm_vk_swapchain_instance_terminate(instance, device);
    }
    cvm_vk_swapchain_instance_queue_terminate(&swapchain->swapchain_queue);

    if(swapchain->metering_fence_active)
    {
        cvm_vk_wait_on_fence_and_reset(device,swapchain->metering_fence);
        swapchain->metering_fence_active=false;
    }
    cvm_vk_destroy_fence(device,swapchain->metering_fence);
}












/// MUST be called AFTER present for the frame
/// need robust input parameter(s?) to control prevention of frame acquisition (can probably assume recreation needed if game_running is still true) also allows other reasons to recreate, e.g. settings change
///      ^ this paradigm might even avoid swapchain recreation when not changing things that affect it! (e.g. 1 modules MSAA settings)
/// need a better name for this
/// rely on this func to detect swapchain resize? couldn't hurt to double check based on screen resize
/// returns newly finished frames image index so that data waiting on it can be cleaned up for threads in upcoming critical section

const cvm_vk_swapchain_presentable_image * cvm_vk_swapchain_acquire_presentable_image(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, uint32_t * cleanup_index)
{
    cvm_vk_swapchain_presentable_image * presentable_image;
    cvm_vk_swapchain_instance * instance, * prev_instance;
    uint32_t instance_index, i, image_index;
    VkSemaphore acquire_semaphore;
    VkResult acquire_result;

    *cleanup_index=CVM_INVALID_U32_INDEX;

    #warning can/will instead ALWAYS return a valid image, if failure occurrs then rebuild swapchain immediately, no extra function call


    #warning needs massive cleanup

    do
    {
        /// check old swapchain to see if they can be destroyed
        #warning make this a function!
        while(cvm_vk_swapchain_instance_queue_get_count(&swapchain->swapchain_queue) > 2)
        {
            instance = cvm_vk_swapchain_instance_queue_get_front_ptr(&swapchain->swapchain_queue);

            for(i=0;i<instance->image_count;i++)
            {
                presentable_image = instance->presentable_images+i;

                if(presentable_image->acquired)
                {
                    assert(presentable_image->last_use_moment.semaphore != VK_NULL_HANDLE);/// probs wrong!
                    assert(presentable_image->acquire_semaphore != VK_NULL_HANDLE);

                    if(cvm_vk_timeline_semaphore_moment_query(device, &presentable_image->last_use_moment))
                    {
                        assert(instance->acquired_image_count > 0);
                        instance->image_acquisition_semaphores[--instance->acquired_image_count] = presentable_image->acquire_semaphore;
                        presentable_image->last_use_moment=cvm_vk_timeline_semaphore_moment_null;
                        presentable_image->acquire_semaphore = VK_NULL_HANDLE;
                    }
                }
                else
                {
                    assert(presentable_image->last_use_moment.semaphore == VK_NULL_HANDLE);
                    assert(presentable_image->acquire_semaphore == VK_NULL_HANDLE);
                }
            }

            if(instance->acquired_image_count == 0)
            {
                cvm_vk_swapchain_instance_terminate(instance, device);
                cvm_vk_swapchain_instance_queue_dequeue(&swapchain->swapchain_queue, NULL);
            }
            else
            {
                break;
            }
        }



        if( ! swapchain->rendering_resources_valid)
        {
            prev_instance = cvm_vk_swapchain_instance_queue_get_back_ptr(&swapchain->swapchain_queue);

            instance_index = cvm_vk_swapchain_instance_queue_new_index(&swapchain->swapchain_queue);
            instance = cvm_vk_swapchain_instance_queue_get_ptr(&swapchain->swapchain_queue, instance_index);

            cvm_vk_swapchain_instance_initialise(instance, device, swapchain, instance_index, prev_instance ? prev_instance->swapchain : VK_NULL_HANDLE);
            swapchain->rendering_resources_valid = true;/// ??? is this fine?
        }
        else
        {
            instance = cvm_vk_swapchain_instance_queue_get_back_ptr(&swapchain->swapchain_queue);
        }



        acquire_semaphore = instance->image_acquisition_semaphores[instance->acquired_image_count++];

        if(swapchain->metering_fence_active)
        {
            /// this should be the source stalls now
            cvm_vk_wait_on_fence_and_reset(device, swapchain->metering_fence);
            swapchain->metering_fence_active=false;
        }

        presentable_image = NULL;
        acquire_result = vkAcquireNextImageKHR(device->device, instance->swapchain, CVM_VK_DEFAULT_TIMEOUT, acquire_semaphore, swapchain->metering_fence, &image_index);



        if(acquire_result>=VK_SUCCESS)
        {
            swapchain->metering_fence_active=true;
            if(acquire_result==VK_SUBOPTIMAL_KHR)
            {
                fprintf(stderr,"acquired swapchain image suboptimal\n");
                swapchain->rendering_resources_valid=false;
            }
            else if(acquire_result!=VK_SUCCESS)
            {
                fprintf(stderr,"ACQUIRE NEXT IMAGE SUCCEEDED WITH RESULT : %d\n",acquire_result);
                /// not sure what to do in these cases
            }

            assert(image_index < instance->image_count);
            presentable_image = instance->presentable_images + image_index;

            if(presentable_image->acquired)
            {
                /// if we're getting this image back it shouldnt still be in use!
                assert(presentable_image->last_use_moment.semaphore != VK_NULL_HANDLE);
//                assert(cvm_vk_timeline_semaphore_moment_query(device, &presentable_image->last_use_moment));
#warning might not have been present sucessfully??? (fast path to recreation while image still in flight)
                assert(presentable_image->acquire_semaphore != VK_NULL_HANDLE);
                instance->image_acquisition_semaphores[--instance->acquired_image_count] = presentable_image->acquire_semaphore;
            }
            else
            {
                assert(presentable_image->last_use_moment.semaphore == VK_NULL_HANDLE);
                assert(presentable_image->acquire_semaphore == VK_NULL_HANDLE);
            }

            presentable_image->last_use_moment = cvm_vk_timeline_semaphore_moment_null;
            presentable_image->acquire_semaphore=acquire_semaphore;
            presentable_image->acquired=true;
            presentable_image->qfot_required=false;
            presentable_image->present_attempted=false;
            presentable_image->present_successful=false;
            presentable_image->last_use_queue_family=CVM_INVALID_U32_INDEX;
        }
        else
        {
            if(acquire_result==VK_ERROR_OUT_OF_DATE_KHR) fprintf(stderr,"acquired swapchain image out of date\n");
            else fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",acquire_result);

            instance->image_acquisition_semaphores[--instance->acquired_image_count] = acquire_semaphore;
            assert(acquire_result==VK_ERROR_OUT_OF_DATE_KHR);
            swapchain->rendering_resources_valid = false;
        }
    }
    while(presentable_image == NULL);

    return presentable_image;
}

static VkCommandBuffer cvm_vk_swapchain_create_image_qfot_command_buffer(const cvm_vk_device * device, VkImage image, uint32_t dst_queue_family, uint32_t src_queue_family)
{
    VkCommandBuffer command_buffer;
    #warning mutex lock on device internal command pool (device no longer const?)
    VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext=NULL,
        .commandPool=device->queue_families[dst_queue_family].internal_command_pool,
        .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount=1
    };

    CVM_VK_CHECK(vkAllocateCommandBuffers(device->device,&command_buffer_allocate_info,&command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext=NULL,
        .flags=0,///not one time use
        .pInheritanceInfo=NULL
    };

    VkDependencyInfo present_acquire_dependencies=
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
                .srcStageMask=0,/// from examles: no srcStage/AccessMask or dstStage/AccessMask is needed, waiting for a semaphore does that automatically.
                .srcAccessMask=0,
                .dstStageMask=VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,/// ??? what stage even is present? (stage and access can probably be 0, just being overly safe here)
                .dstAccessMask=VK_ACCESS_2_MEMORY_READ_BIT,
                .oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex=src_queue_family,
                .dstQueueFamilyIndex=dst_queue_family,
                .image=image,
                .subresourceRange=(VkImageSubresourceRange)
                {
                    .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel=0,
                    .levelCount=1,
                    .baseArrayLayer=0,
                    .layerCount=1
                }
            }
        }
    };


    CVM_VK_CHECK(vkBeginCommandBuffer(command_buffer,&command_buffer_begin_info));

    vkCmdPipelineBarrier2(command_buffer,&present_acquire_dependencies);

    CVM_VK_CHECK(vkEndCommandBuffer(command_buffer));
}

void cvm_vk_swapchain_present_image(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, cvm_vk_swapchain_presentable_image * presentable_image)
{
    #warning needs massive cleanup
    VkSemaphoreSubmitInfo wait_semaphores[1];
    VkSemaphoreSubmitInfo signal_semaphores[2];
    cvm_vk_device_queue_family * present_queue_family;
    cvm_vk_device_queue * present_queue;
    uint32_t image_index = presentable_image->index;
    const cvm_vk_swapchain_instance * parent_swapchain_instance;

    parent_swapchain_instance = presentable_image->parent_swapchain_instance;

    assert(presentable_image->last_use_queue_family != CVM_INVALID_U32_INDEX);

    if(presentable_image->qfot_required)
    {
        if(presentable_image->present_acquire_command_buffers[presentable_image->last_use_queue_family]==VK_NULL_HANDLE)
        {
            #warning could allocate the command buffers upfront and only reset upon swapchain recreation...
            presentable_image->present_acquire_command_buffers[presentable_image->last_use_queue_family] =
                cvm_vk_swapchain_create_image_qfot_command_buffer(device, presentable_image->image, parent_swapchain_instance->fallback_present_queue_family, presentable_image->last_use_queue_family);
        }


        present_queue_family = device->queue_families + parent_swapchain_instance->fallback_present_queue_family;
        present_queue = present_queue_family->queues+0;/// use queue 0

        ///fixed count and layout of wait and signal semaphores here
        wait_semaphores[0]=cvm_vk_binary_semaphore_submit_info(presentable_image->qfot_semaphore,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

        /// presentable_image->present_semaphore triggered either here or above when CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE, this path being taken when present queue != graphics queue
        signal_semaphores[0]=cvm_vk_binary_semaphore_submit_info(presentable_image->present_semaphore,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        signal_semaphores[1]=cvm_vk_timeline_semaphore_signal_submit_info(&present_queue->timeline, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, &presentable_image->last_use_moment);///REMOVE THIS

        VkSubmitInfo2 submit_info=(VkSubmitInfo2)
        {
            .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext=NULL,
            .flags=0,
            .waitSemaphoreInfoCount=1,///fixed number, set above
            .pWaitSemaphoreInfos=wait_semaphores,
            .commandBufferInfoCount=1,
            .pCommandBufferInfos=(VkCommandBufferSubmitInfo[1])
            {
                {
                    .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                    .pNext=NULL,
                    .commandBuffer=presentable_image->present_acquire_command_buffers[presentable_image->last_use_queue_family],
                    .deviceMask=0
                }
            },
            .signalSemaphoreInfoCount=2,///fixed number, set above
            .pSignalSemaphoreInfos=signal_semaphores
        };

        CVM_VK_CHECK(vkQueueSubmit2(present_queue->queue,1,&submit_info,VK_NULL_HANDLE));
    }
    else
    {
        assert(parent_swapchain_instance->queue_family_presentable_mask | (1 << presentable_image->last_use_queue_family));///must be presentable on last used queue family
        present_queue_family = device->queue_families + presentable_image->last_use_queue_family;
        present_queue = present_queue_family->queues + 0;/// use queue 0
    }



    VkPresentInfoKHR present_info=(VkPresentInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext=NULL,
        .waitSemaphoreCount=1,
        .pWaitSemaphores=&presentable_image->present_semaphore,
        .swapchainCount=1,
        .pSwapchains=&parent_swapchain_instance->swapchain,
        .pImageIndices=&image_index,
        .pResults=NULL
    };
    #warning present should be synchronised (check this is true)

    #warning separate present out from modules/work, it should operate on the presentable_image


    VkResult r=vkQueuePresentKHR(present_queue->queue,&present_info);

    if(r>=VK_SUCCESS)
    {
        if(r==VK_SUBOPTIMAL_KHR)
        {
            puts("presented swapchain suboptimal");///common error
            swapchain->rendering_resources_valid=false;/// suggests swapchain recreation
            #warning move validity check to most recent instance?
        }
        else if(r!=VK_SUCCESS)fprintf(stderr,"PRESENTATION SUCCEEDED WITH RESULT : %d\n",r);
        presentable_image->present_successful=true;
    }
    else
    {
        if(r==VK_ERROR_OUT_OF_DATE_KHR)puts("couldn't present (out of date)");///common error
        else fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);
        swapchain->rendering_resources_valid=false;/// requires swapchain recreation
    }
}


















