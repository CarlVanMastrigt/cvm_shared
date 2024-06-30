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



static inline int cvm_vk_swapchain_create(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device)
{
    uint32_t i,j,fallback_present_queue_family;
    VkBool32 surface_supported;
    VkSwapchainKHR old_swapchain;
    cvm_vk_swapchain_presentable_image * presentable_image;



    swapchain->acquired_image_index=CVM_INVALID_U32_INDEX;///no longer have a valid swapchain image (might be better place to put this?)

    swapchain->queue_family_support_mask=0;
    for(i=device->queue_family_count;i--;)
    {
        #warning do this on init ans instead assert it is the same!
        CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device,i,swapchain->surface,&surface_supported));
        if(surface_supported)
        {
            swapchain->fallback_present_queue_family=i;
            swapchain->queue_family_support_mask|=1<<i;
        }
    }
    if(swapchain->queue_family_support_mask==0) return -1;///cannot present to this surface!


    swapchain->fallback_present_queue_family=device->fallback_present_queue_family_index;
    #warning ABOVE IS FOR TESTING

    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device,swapchain->surface,&swapchain->surface_capabilities));
    #warning above obviously changes upon resize, but we can just ensure the selected image count is maintained! (falls within capabilities)

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

    CVM_VK_CHECK(vkCreateSwapchainKHR(device->device,&swapchain_create_info,NULL,&swapchain->swapchain));

    if(old_swapchain!=VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device->device,old_swapchain,NULL);
    }

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(device->device,swapchain->swapchain,&swapchain->image_count,NULL));
    VkImage * swapchain_images=malloc(sizeof(VkImage)*swapchain->image_count);
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(device->device,swapchain->swapchain,&swapchain->image_count,swapchain_images));

    VkSemaphoreCreateInfo binary_semaphore_create_info=(VkSemaphoreCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=NULL,
        .flags=0
    };
    #warning make binary semaphores using a function instead of the above BS

    if(swapchain->max_image_count < swapchain->image_count)
    {
        swapchain->image_acquisition_semaphores=realloc(swapchain->image_acquisition_semaphores,(swapchain->image_count+1)*sizeof(VkSemaphore));
        swapchain->presenting_images=realloc(swapchain->presenting_images,swapchain->image_count*sizeof(cvm_vk_swapchain_presentable_image));

        for(i=swapchain->max_image_count;i<swapchain->image_count;i++)
        {
            CVM_VK_CHECK(vkCreateSemaphore(device->device,&binary_semaphore_create_info,NULL,swapchain->image_acquisition_semaphores+i+1));


            presentable_image=swapchain->presenting_images+i;

            CVM_VK_CHECK(vkCreateSemaphore(device->device,&binary_semaphore_create_info,NULL,&presentable_image->present_semaphore));

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
            ///.image_view,//set later
        presentable_image->acquire_semaphore=VK_NULL_HANDLE;///set using one of the available image_acquisition_semaphores
        #warning instead of above assert it is NULL and set to VK_NULL_HANDLE upon init and upon being relinquished! (same for below!)
        presentable_image->successfully_acquired=false;
        presentable_image->successfully_submitted=false;
            ///.present_semaphore,//set later
            ///.present_acquire_command_buffers,///set later

            #warning being reliant upon graphics is a red flag, this needs to be generalised! make them per QFOT? per queue?? something else entirely?
            #warning also: why TF is transfer here ? it should be added as a dependency on the ops that use it!
            #warning instead of specific timeline semaphores, have "frame" relevant list of timeline semaphore moments to wait on!

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

static inline void cvm_vk_swapchain_destroy(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device)
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
                vkFreeCommandBuffers(device->device,device->internal_command_pools[swapchain->fallback_present_queue_family],1,presentable_image->present_acquire_command_buffers+j);

                presentable_image->present_acquire_command_buffers[j]=VK_NULL_HANDLE;
            }
        }
    }
}




int cvm_vk_swapchain_initialse(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device, const cvm_vk_swapchain_setup * setup)
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

    cvm_vk_create_timeline_semaphore(&swapchain->present_timeline, device);

    swapchain->image_acquisition_semaphores=malloc(sizeof(VkSemaphore));

    VkSemaphoreCreateInfo binary_semaphore_create_info=(VkSemaphoreCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=NULL,
        .flags=0
    };
    #warning make binary semaphores using a function instead of the above BS
    CVM_VK_CHECK(vkCreateSemaphore(device->device,&binary_semaphore_create_info,NULL,swapchain->image_acquisition_semaphores));

    swapchain->presenting_images=NULL;

    assert(device->queue_family_count<=64);/// cannot hold bitmask

    #warning what amongst these needs to be called AGAIN after the surface gets recreated??

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

    return cvm_vk_swapchain_create(swapchain,device);
}

void cvm_vk_swapchain_terminate(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device)
{
    uint32_t i;
    cvm_vk_swapchain_presentable_image * presentable_image;

    cvm_vk_swapchain_destroy(swapchain,device);


    cvm_vk_destroy_timeline_semaphore(&swapchain->present_timeline, device);

    vkDeviceWaitIdle(device->device);
    /// above required b/c presently there is no way to know that the semaphore used by present has actually been used by WSI

    for(i=0;i<swapchain->max_image_count+1;i++)
    {
        vkDestroySemaphore(device->device,swapchain->image_acquisition_semaphores[i],NULL);
    }

    for(i=0;i<swapchain->max_image_count;i++)
    {
        presentable_image=swapchain->presenting_images+i;

        vkDestroySemaphore(device->device,presentable_image->present_semaphore,NULL);
        free(swapchain->presenting_images[i].present_acquire_command_buffers);
    }

    vkDestroySwapchainKHR(device->device,swapchain->swapchain,NULL);
}


int cvm_vk_swapchain_regenerate(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device)
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
    cvm_vk_swapchain_destroy(swapchain,device);
    return cvm_vk_swapchain_create(swapchain,device);
}





















/// MUST be called AFTER present for the frame
/// need robust input parameter(s?) to control prevention of frame acquisition (can probably assume recreation needed if game_running is still true) also allows other reasons to recreate, e.g. settings change
///      ^ this paradigm might even avoid swapchain recreation when not changing things that affect it! (e.g. 1 modules MSAA settings)
/// need a better name for this
/// rely on this func to detect swapchain resize? couldn't hurt to double check based on screen resize
/// returns newly finished frames image index so that data waiting on it can be cleaned up for threads in upcoming critical section

#warning the following returning don't NECESSARILY mean that we're free to cleanup frames, the fence or the semaphore passed int acquire must be "satisfied" before interacting with any of the frames underlying data!

const cvm_vk_swapchain_presentable_image * cvm_vk_swapchain_acquire_presentable_image(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device, bool rendering_resources_invalid, uint32_t * cleanup_index)
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

        VkResult acquire_result=vkAcquireNextImageKHR(device->device,swapchain->swapchain,1000000000,acquire_semaphore,VK_NULL_HANDLE,&swapchain->acquired_image_index);

        if(acquire_result>=VK_SUCCESS)
        {
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

            if(presentable_image->successfully_acquired)///if it was acquired, cleanup is in order
            {
                cvm_vk_wait_on_timeline_semaphore(presentable_image->transfer_wait,device,1000000000);
                cvm_vk_wait_on_timeline_semaphore(presentable_image->graphics_wait,device,1000000000);

                #warning remove above (?) still need to wait on last thing that used the image, even if it didnt get presented sucessfully
                #warning need to review when present happens vs submit, may be cause of earlier issues!?

                *cleanup_index=swapchain->acquired_image_index;
                #warning when does cleanup need to happen!?

                #warning do all the above need to be registered with the presentable_image/renderaable_frame?? (including present semaphore)

                swapchain->image_acquisition_semaphores[--swapchain->acquired_image_count]=presentable_image->acquire_semaphore;

                cvm_vk_wait_on_timeline_semaphore(presentable_image->present_moment,device,1000000000);
            }

            if(presentable_image->successfully_submitted)
            {
                assert(presentable_image->successfully_acquired);///SOMEHOW PREPARING/CLEANING UP FRAME THAT WAS SUBMITTED BUT NOT ACQUIRED
            }

            #warning are we incrementing timeline semaphores that dont actually get run correctly!

            presentable_image->successfully_acquired=true;
            presentable_image->successfully_submitted=false;
            presentable_image->acquire_semaphore=acquire_semaphore;

            presentable_image->graphics_wait.value=cvm_vk_graphics_timeline_get()->value;
            presentable_image->graphics_wait.semaphore=cvm_vk_graphics_timeline_get()->semaphore;
            presentable_image->transfer_wait.value=cvm_vk_transfer_timeline_get()->value;
            presentable_image->transfer_wait.semaphore=cvm_vk_transfer_timeline_get()->semaphore;
            #warning remove above
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

bool cvm_vk_check_for_remaining_frames(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device, uint32_t * cleanup_index)
{
    uint32_t i;
    #warning swapchain should retain/know its device!, swapchains need to be created from a device
    cvm_vk_swapchain_presentable_image * presentable_image;

//    vkDeviceWaitIdle(cvm_vk_.device);
    #warning remove above, BUT for now it seems behaviour is fucked, "batches" refer to semaphores that are in flight, despite definitely being completed (a fence WAITS ON COMPETION and its still "referenced")

    ///this will leave some images acquired... (and some command buffers in the pending state)

    if(swapchain->acquired_image_count)
    {
        for(i=0;i<swapchain->image_count;i++)
        {
            presentable_image = swapchain->presenting_images + i;
            if(presentable_image->successfully_acquired)
            {
                cvm_vk_wait_on_timeline_semaphore(presentable_image->transfer_wait,device,1000000000);
                cvm_vk_wait_on_timeline_semaphore(presentable_image->graphics_wait,device,1000000000);
                cvm_vk_wait_on_timeline_semaphore(presentable_image->present_moment,device,1000000000);

                presentable_image->successfully_acquired=false;
                presentable_image->successfully_submitted=false;
                swapchain->image_acquisition_semaphores[--swapchain->acquired_image_count]=presentable_image->acquire_semaphore;
                *cleanup_index=i;
                return true;
            }
            else assert(!presentable_image->successfully_submitted);///SOMEHOW CLEANING UP FRAME THAT WAS SUBMITTED BUT NOT ACQUIRED
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
