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



static inline int cvm_vk_swapchain_create(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain)
{
    uint32_t i,j,fallback_present_queue_family;
    VkBool32 surface_supported;
    VkSwapchainKHR old_swapchain;
    cvm_vk_swapchain_presentable_image * presentable_image;


    assert(swapchain->acquired_image_count==0);
    swapchain->acquired_image_index=CVM_INVALID_U32_INDEX;///no longer have a valid swapchain image (might be better place to put this?)

    swapchain->queue_family_presentable_mask=0;
    for(i=device->queue_family_count;i--;)
    {
        CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device,i,swapchain->surface,&surface_supported));
        if(surface_supported)
        {
            swapchain->fallback_present_queue_family=i;
            swapchain->queue_family_presentable_mask|=1<<i;
        }
    }
    if(swapchain->queue_family_presentable_mask==0) return -1;///cannot present to this surface!


    swapchain->fallback_present_queue_family=device->fallback_present_queue_family_index;
    #warning ABOVE IS FOR TESTING

    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device,swapchain->surface,&swapchain->surface_capabilities));

    if(swapchain->surface_capabilities.supportedUsageFlags&swapchain->usage_flags!=swapchain->usage_flags) return -1;///intened usage not supported
    if((swapchain->surface_capabilities.maxImageCount != 0)&&(swapchain->surface_capabilities.maxImageCount < swapchain->min_image_count)) return -1;///cannot suppirt minimum image count
    if(swapchain->surface_capabilities.supportedCompositeAlpha&VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR==0)return -1;///compositing not supported
    /// would like to support different compositing, need to extend to allow this

    old_swapchain=swapchain->swapchain;

    VkSwapchainCreateInfoKHR swapchain_create_info=(VkSwapchainCreateInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext=NULL,
        .flags=0,
        .surface=swapchain->surface,
        .minImageCount=CVM_MAX(swapchain->surface_capabilities.minImageCount,swapchain->min_image_count),
        .imageFormat=swapchain->surface_format.format,
        .imageColorSpace=swapchain->surface_format.colorSpace,
        .imageExtent=swapchain->surface_capabilities.currentExtent,
        .imageArrayLayers=1,
        .imageUsage=swapchain->usage_flags,
        .imageSharingMode=VK_SHARING_MODE_EXCLUSIVE,///means QFOT is necessary, thus following should not be prescribed
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL,
        .preTransform=swapchain->surface_capabilities.currentTransform,
        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode=swapchain->present_mode,
        .clipped=VK_TRUE,
        .oldSwapchain=old_swapchain,
    };

    swapchain->generation++;
    if(swapchain->generation==CVM_INVALID_U16_INDEX)
    {
        swapchain->generation=0;
    }
    CVM_VK_CHECK(vkCreateSwapchainKHR(device->device,&swapchain_create_info,NULL,&swapchain->swapchain));

    if(old_swapchain!=VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device->device,old_swapchain,NULL);
    }

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(device->device,swapchain->swapchain,&swapchain->image_count,NULL));
    VkImage * swapchain_images=malloc(sizeof(VkImage)*swapchain->image_count);
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(device->device,swapchain->swapchain,&swapchain->image_count,swapchain_images));

    if(swapchain->max_image_count < swapchain->image_count)
    {
        swapchain->image_acquisition_semaphores=realloc(swapchain->image_acquisition_semaphores,(swapchain->image_count+1)*sizeof(VkSemaphore));
        swapchain->presenting_images=realloc(swapchain->presenting_images,swapchain->image_count*sizeof(cvm_vk_swapchain_presentable_image));

        for(i=swapchain->max_image_count;i<swapchain->image_count;i++)
        {
            swapchain->image_acquisition_semaphores[i+1]=cvm_vk_create_binary_semaphore(device);

            presentable_image=swapchain->presenting_images+i;

            presentable_image->parent_swapchain=swapchain;

            presentable_image->present_semaphore=cvm_vk_create_binary_semaphore(device);
            presentable_image->qfot_semaphore=cvm_vk_create_binary_semaphore(device);

            presentable_image->present_acquire_command_buffers=malloc(sizeof(VkCommandBuffer)*device->queue_family_count);
            for(j=0;j<device->queue_family_count;j++)
            {
                presentable_image->present_acquire_command_buffers[j]=VK_NULL_HANDLE;
            }
        }

        swapchain->max_image_count = swapchain->image_count;
    }

    for(i=0;i<swapchain->image_count;i++)
    {
        presentable_image=swapchain->presenting_images+i;

        presentable_image->image=swapchain_images[i];

        presentable_image->unique_image_identifier=swapchain->unique_image_counter++;
        if(swapchain->unique_image_counter==CVM_INVALID_U16_INDEX)
        {
            swapchain->unique_image_counter=0;
        }

        presentable_image->acquire_semaphore=VK_NULL_HANDLE;///set using one of the available image_acquisition_semaphores
        #warning instead of above assert it is NULL and set to VK_NULL_HANDLE upon init and upon being relinquished! (same for below!)
        presentable_image->acquired=false;
        presentable_image->presented=false;
        presentable_image->qfot_required=false;
        presentable_image->present_setup=false;

        ///image view
        VkImageViewCreateInfo image_view_create_info=(VkImageViewCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .image=swapchain_images[i],
            .viewType=VK_IMAGE_VIEW_TYPE_2D,
            .format=swapchain->surface_format.format,
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
        CVM_VK_CHECK(vkCreateImageView(device->device,&image_view_create_info,NULL,&presentable_image->image_view));
    }

    free(swapchain_images);

    cvm_vk_create_swapchain_dependednt_defaults(swapchain->surface_capabilities.currentExtent.width,swapchain->surface_capabilities.currentExtent.height);
    #warning make these defaults part of the swapchain!?

    swapchain->rendering_resources_valid=true;
}

static inline void cvm_vk_swapchain_destroy(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain)
{
    uint32_t i,j;
    cvm_vk_swapchain_presentable_image * presentable_image;

    cvm_vk_destroy_swapchain_dependednt_defaults();
    #warning make above defaults part of the swapchain


    assert(swapchain->acquired_image_count==0);///MUST WAIT TILL ALL IMAGES ARE RETURNED BEFORE TERMINATING SWAPCHAIN

    for(i=0;i<swapchain->image_count;i++)
    {
        presentable_image=swapchain->presenting_images+i;

        /// swapchain frames
        vkDestroyImageView(device->device,presentable_image->image_view,NULL);

        ///queue ownership transfer stuff
        for(j=0;j<device->queue_family_count;j++)
        {
            if(presentable_image->present_acquire_command_buffers[j]!=VK_NULL_HANDLE)
            {
                vkFreeCommandBuffers(device->device,device->queue_families[swapchain->fallback_present_queue_family].internal_command_pool,1,presentable_image->present_acquire_command_buffers+j);
                #warning requires synchronization! (uses shared command pool, ergo not thread safe)
                #warning should (?) really be using a shared command pool that is just reset

                presentable_image->present_acquire_command_buffers[j]=VK_NULL_HANDLE;
            }
        }
    }
}




int cvm_vk_swapchain_initialse(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, const cvm_vk_swapchain_setup * setup)
{
    uint32_t i,format_count,present_mode_count;
    SDL_bool created_surface;
    VkSurfaceFormatKHR * formats=NULL;
    VkPresentModeKHR * present_modes=NULL;
    VkSurfaceFormatKHR preferred_surface_format;

    swapchain->surface=setup->surface;
    swapchain->swapchain=VK_NULL_HANDLE;

    swapchain->min_image_count=setup->min_image_count;
    swapchain->max_image_count=0;
    swapchain->usage_flags=setup->usage_flags;

    swapchain->metering_fence=cvm_vk_create_fence(device,false);
    swapchain->metering_fence_active=false;

    swapchain->image_acquisition_semaphores=malloc(sizeof(VkSemaphore));

    swapchain->image_acquisition_semaphores[0]=cvm_vk_create_binary_semaphore(device);

    swapchain->presenting_images=NULL;

    swapchain->acquired_image_count=0;

    swapchain->generation=0;
    swapchain->unique_image_counter=0;

    assert(device->queue_family_count<=64);/// cannot hold bitmask

    /// select screen image format
    if(setup->preferred_surface_format.format) preferred_surface_format=setup->preferred_surface_format;
    else preferred_surface_format=(VkSurfaceFormatKHR){.format=VK_FORMAT_B8G8R8A8_SRGB,.colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device,swapchain->surface,&format_count,NULL));
    formats=malloc(sizeof(VkSurfaceFormatKHR)*format_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device,swapchain->surface,&format_count,formats));

    swapchain->surface_format=formats[0];///fallback in case preferred isnt found, required to have at least 1 at this point
    for(i=0;i<format_count;i++)
    {
        if(preferred_surface_format.colorSpace==formats[i].colorSpace && preferred_surface_format.format==formats[i].format)
        {
            ///preferred format exists
            swapchain->surface_format=preferred_surface_format;
            break;
        }
    }
    free(formats);


    ///select screen present mode
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device,swapchain->surface,&present_mode_count,NULL));
    present_modes=malloc(sizeof(VkPresentModeKHR)*present_mode_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device,swapchain->surface,&present_mode_count,present_modes));

    swapchain->present_mode=VK_PRESENT_MODE_FIFO_KHR;/// this is required to be supported
    for(i=0;i<present_mode_count;i++)
    {
        if(present_modes[i]==setup->preferred_present_mode)
        {
            swapchain->present_mode=setup->preferred_present_mode;
            break;
        }
    }
    free(present_modes);

    return cvm_vk_swapchain_create(device,swapchain);
}

void cvm_vk_swapchain_terminate(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain)
{
    uint32_t i;
    cvm_vk_swapchain_presentable_image * presentable_image;

    cvm_vk_swapchain_destroy(device, swapchain);


    vkDeviceWaitIdle(device->device);
    /// above required b/c presently there is no way to know that the semaphore used by present has actually been used by WSI

    if(swapchain->metering_fence_active)
    {
        cvm_vk_wait_on_fence_and_reset(device,swapchain->metering_fence);
        swapchain->metering_fence_active=false;
    }
    cvm_vk_destroy_fence(device,swapchain->metering_fence);

    for(i=0;i<swapchain->max_image_count+1;i++)
    {
        cvm_vk_destroy_binary_semaphore(device,swapchain->image_acquisition_semaphores[i]);
    }

    for(i=0;i<swapchain->max_image_count;i++)
    {
        presentable_image=swapchain->presenting_images+i;

        cvm_vk_destroy_binary_semaphore(device,presentable_image->present_semaphore);
        cvm_vk_destroy_binary_semaphore(device,presentable_image->qfot_semaphore);
        free(swapchain->presenting_images[i].present_acquire_command_buffers);
    }

    vkDestroySwapchainKHR(device->device,swapchain->swapchain,NULL);
}


int cvm_vk_swapchain_regenerate(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain)
{
    #warning wait wait wait, old swapchain images can be presented, don't need to wait and cleanup
    #warning this implies a better design is possible! cleanup on the fly and make presentable images a object derived from the current swapchain state!
    #warning how the fuck can i make this work with fixed size framebuffers!? (should i not bother in this case?)
    /**
    Note
Multiple retired swapchains can be associated with the same VkSurfaceKHR through
multiple uses of oldSwapchain that outnumber calls to vkDestroySwapchainKHR.

After oldSwapchain is retired, the application can pass to vkQueuePresentKHR any
images it had already acquired from oldSwapchain. E.g., an application may present
an image from the old swapchain before an image from the new swapchain is
ready to be presented. As usual, vkQueuePresentKHR may fail if oldSwapchain has
entered a state that causes VK_ERROR_OUT_OF_DATE_KHR to be returned.

The application can continue to use a shared presentable image obtained from
oldSwapchain until a presentable image is acquired from the new swapchain, as
long as it has not entered a state that causes it to return VK_ERROR_OUT_OF_DATE_KHR.
    */

    /// could recreate resources on the fly (as needed)


    /// is somewhat hard to guarantee that the recycling of present semaphores here between different swapchains is valid, can the new swapchain be presenting while the prior swapchain still has frames enqueued?
    cvm_vk_swapchain_destroy(device, swapchain);
    return cvm_vk_swapchain_create(device, swapchain);
}





















/// MUST be called AFTER present for the frame
/// need robust input parameter(s?) to control prevention of frame acquisition (can probably assume recreation needed if game_running is still true) also allows other reasons to recreate, e.g. settings change
///      ^ this paradigm might even avoid swapchain recreation when not changing things that affect it! (e.g. 1 modules MSAA settings)
/// need a better name for this
/// rely on this func to detect swapchain resize? couldn't hurt to double check based on screen resize
/// returns newly finished frames image index so that data waiting on it can be cleaned up for threads in upcoming critical section

const cvm_vk_swapchain_presentable_image * cvm_vk_swapchain_acquire_presentable_image(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, bool rendering_resources_invalid, uint32_t * cleanup_index)
{
    cvm_vk_swapchain_presentable_image * presentable_image;
    VkSemaphore acquire_semaphore;
    VkResult acquire_result;

    if(rendering_resources_invalid) swapchain->rendering_resources_valid=false;

    presentable_image=NULL;
    *cleanup_index=CVM_INVALID_U32_INDEX;

    if(swapchain->rendering_resources_valid)
    {
        acquire_semaphore=swapchain->image_acquisition_semaphores[swapchain->acquired_image_count++];

        if(swapchain->metering_fence_active)
        {
            /// this should be the source stalls now
            cvm_vk_wait_on_fence_and_reset(device,swapchain->metering_fence);
            swapchain->metering_fence_active=false;
        }

        VkResult acquire_result=vkAcquireNextImageKHR(device->device,swapchain->swapchain,CVM_VK_DEFAULT_TIMEOUT,acquire_semaphore,swapchain->metering_fence,&swapchain->acquired_image_index);

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

            presentable_image = swapchain->presenting_images + swapchain->acquired_image_index;
            /// cleanup prior use of this frame index

            if(presentable_image->acquired)///if it was acquired, cleanup is in order
            {
                *cleanup_index=swapchain->acquired_image_index;
                #warning when does cleanup need to happen!?

                #warning do all the above need to be registered with the presentable_image/renderaable_frame?? (including present semaphore)

                swapchain->image_acquisition_semaphores[--swapchain->acquired_image_count]=presentable_image->acquire_semaphore;

                cvm_vk_timeline_semaphore_moment_wait(device,&presentable_image->last_use_moment);
            }

            if(presentable_image->presented)
            {
                assert(presentable_image->acquired);///SOMEHOW PREPARING/CLEANING UP FRAME THAT WAS PRESENTED BUT NOT ACQUIRED
            }

            #warning are we incrementing timeline semaphores that dont actually get run correctly!

            presentable_image->acquired=true;
            presentable_image->presented=false;
            presentable_image->acquire_semaphore=acquire_semaphore;
            presentable_image->qfot_required=false;
            presentable_image->present_setup=false;
            presentable_image->last_use_queue_family=CVM_INVALID_U32_INDEX;
        }
        else
        {
            if(acquire_result==VK_ERROR_OUT_OF_DATE_KHR) fprintf(stderr,"acquired swapchain image out of date\n");
            else fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",acquire_result);
            swapchain->acquired_image_index=CVM_INVALID_U32_INDEX;
            swapchain->rendering_resources_valid=false;
            swapchain->image_acquisition_semaphores[--swapchain->acquired_image_count]=acquire_semaphore;
        }
    }
    else
    {
        swapchain->acquired_image_index=CVM_INVALID_U32_INDEX;
    }

    return presentable_image;
}

bool cvm_vk_check_for_remaining_frames(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, uint32_t * cleanup_index)
{
    #warning a better design might be to do cleanup via a callback, not necessarily associated with any single index but instead just refrencing a particular frame
    /// ^ unfortunately this will likely require semi-expensive synchronization!
    uint32_t i;
    #warning swapchain should retain/know its device!, after all swapchains need to be created from/for a device!
    cvm_vk_swapchain_presentable_image * presentable_image;

    ///this will leave some images acquired... (and some command buffers in the pending state)

    if(swapchain->acquired_image_count)
    {
        for(i=0;i<swapchain->image_count;i++)
        {
            presentable_image = swapchain->presenting_images + i;
            if(presentable_image->acquired)
            {
                cvm_vk_timeline_semaphore_moment_wait(device,&presentable_image->last_use_moment);

                presentable_image->acquired=false;
                presentable_image->presented=false;
                swapchain->image_acquisition_semaphores[--swapchain->acquired_image_count]=presentable_image->acquire_semaphore;
                *cleanup_index=i;
                return true;
            }
            else assert(!presentable_image->presented);///SOMEHOW CLEANING UP FRAME THAT WAS PRESENTED BUT NOT ACQUIRED
        }
        assert(false);/// if there were acquired frames remaining they should have been found
        *cleanup_index=CVM_INVALID_U32_INDEX;
        return false;
    }
    else
    {
        *cleanup_index=CVM_INVALID_U32_INDEX;
        return false;
    }
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
    VkSemaphoreSubmitInfo wait_semaphores[1];
    VkSemaphoreSubmitInfo signal_semaphores[2];
    cvm_vk_device_queue_family * present_queue_family;
    cvm_vk_device_queue * present_queue;
    uint32_t image_index = presentable_image - swapchain->presenting_images;

    assert(presentable_image->last_use_queue_family != CVM_INVALID_U32_INDEX);

    if(presentable_image->qfot_required)
    {
        if(presentable_image->present_acquire_command_buffers[presentable_image->last_use_queue_family]==VK_NULL_HANDLE)
        {
            #warning could allocate the command buffers upfront and only reset upon swapchain recreation...
            presentable_image->present_acquire_command_buffers[presentable_image->last_use_queue_family] =
                cvm_vk_swapchain_create_image_qfot_command_buffer(device, presentable_image->image, swapchain->fallback_present_queue_family, presentable_image->last_use_queue_family);
        }


        present_queue_family = device->queue_families+swapchain->fallback_present_queue_family;
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
        assert(swapchain->queue_family_presentable_mask | (1 << presentable_image->last_use_queue_family));///must be presentable on last used queue family
        present_queue_family = device->queue_families+presentable_image->last_use_queue_family;
        present_queue = present_queue_family->queues+0;/// use queue 0
    }



    VkPresentInfoKHR present_info=(VkPresentInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext=NULL,
        .waitSemaphoreCount=1,
        .pWaitSemaphores=&presentable_image->present_semaphore,
        .swapchainCount=1,
        .pSwapchains=&swapchain->swapchain,
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
            swapchain->rendering_resources_valid=false;///requires resource rebuild
        }
        else if(r!=VK_SUCCESS)fprintf(stderr,"PRESENTATION SUCCEEDED WITH RESULT : %d\n",r);
        presentable_image->presented=true;
    }
    else
    {
        if(r==VK_ERROR_OUT_OF_DATE_KHR)puts("couldn't present (out of date)");///common error
        else fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);
    }
}

















