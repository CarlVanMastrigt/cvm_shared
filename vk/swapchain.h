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


#ifndef CVM_VK_SWAPCHAIN_H
#define CVM_VK_SWAPCHAIN_H


typedef struct cvm_vk_swapchain_setup
{
    const cvm_vk_device * device;

    VkSurfaceKHR surface;

    uint32_t min_image_count;///0=use system minimum
    VkImageUsageFlagBits usage_flags;
//    VkCompositeAlphaFlagsKHR compositing_mode;

    VkSurfaceFormatKHR preferred_surface_format;
    VkPresentModeKHR preferred_present_mode;
}
cvm_vk_swapchain_setup;

typedef struct cvm_vk_swapchain_presentable_image
{
    VkImage image;///theese are provided by the WSI
    VkImageView image_view;

    VkSemaphore acquire_semaphore;///held temporarily by this struct, not owner, not created or destroyed as part of it

    //bool in_flight;///error checking
    uint32_t successfully_acquired:1;
    uint32_t successfully_submitted:1;
    #warning instead is really sucessfully presented!

    VkSemaphore present_semaphore;///needed by VkPresentInfoKHR, which doesn not accept timeline semaphores



    /// the command buffers to recieve the QFOT should be created as necessary
    VkCommandBuffer * present_acquire_command_buffers;

    cvm_vk_timeline_semaphore_moment last_use_moment;///changes throughout the frame

    #warning make this the "last used moment" for this image, use it to track work across multiple "libraries" (e.g. game and overlay)
//    cvm_vk_timeline_semaphore_moment present_moment;///this is actually the last u
}
cvm_vk_swapchain_presentable_image;

/// all the data associated with a window and rendering to a surface(usually a window)
typedef struct cvm_vk_surface_swapchain
{
    const cvm_vk_device * device;/// device that this swapchain belongs to

    VkSurfaceKHR surface;/// effectively the window

    VkFence metering_fence;///wait till previous fence is acquired before acquiring another
    bool metering_fence_active;

    uint32_t min_image_count;
    uint32_t max_image_count;///max count experienced over the lifetime of this swapchain
    VkImageUsageFlagBits usage_flags;

    uint64_t queue_family_support_mask;
    ///if we can't present on the current queue family we'll present on the fallback (lowest indexed queue family that supports present)
    uint32_t fallback_present_queue_family;

    VkSurfaceCapabilitiesKHR surface_capabilities;


    VkSwapchainKHR swapchain;//VK_NULL_HANDLE
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;

    ///may want to rename cvm_vk_frames, cvm_vk_presentation_instance probably isnt that bad tbh...
    ///realloc these only if number of swapchain image count changes (it wont really)
    VkSemaphore * image_acquisition_semaphores;///number of these should match swapchain image count
    cvm_vk_swapchain_presentable_image * presenting_images;
    uint32_t image_count;/// this is also the number of swapchain images
    uint32_t acquired_image_index;///CVM_INVALID_U32_INDEX
    #warning above can be removed!

    ///both frames in flight and frames acquired by rendereer
    uint32_t acquired_image_count;/// init as 0

    ///following used to determine number of swapchain images to allocate
    bool rendering_resources_valid;/// starts false, used to determine if rebuilding of resources is required due to swapchain invalidation (e.g. because window was resized)
}
cvm_vk_surface_swapchain;


int cvm_vk_swapchain_initialse(cvm_vk_surface_swapchain * swapchain, const cvm_vk_swapchain_setup * setup);
void cvm_vk_swapchain_terminate(cvm_vk_surface_swapchain * swapchain);

int cvm_vk_swapchain_regenerate(cvm_vk_surface_swapchain * swapchain);


const cvm_vk_swapchain_presentable_image * cvm_vk_swapchain_acquire_presentable_image(cvm_vk_surface_swapchain * swapchain, bool rendering_resources_invalid, uint32_t * cleanup_index);
bool cvm_vk_check_for_remaining_frames(cvm_vk_surface_swapchain * swapchain, uint32_t * cleanup_index);

#endif

