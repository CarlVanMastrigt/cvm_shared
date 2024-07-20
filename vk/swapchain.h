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
    VkSurfaceKHR surface;

    uint32_t min_image_count;///0=use system minimum
    VkImageUsageFlagBits usage_flags;
//    VkCompositeAlphaFlagsKHR compositing_mode;

    VkSurfaceFormatKHR preferred_surface_format;
    VkPresentModeKHR preferred_present_mode;
}
cvm_vk_swapchain_setup;

typedef struct cvm_vk_surface_swapchain cvm_vk_surface_swapchain;

typedef struct cvm_vk_swapchain_presentable_image
{
    VkImage image;///theese are provided by the WSI
    VkImageView image_view;

    /// "unique" identifier used to differentiate images after swapchain recreation
    uint16_t unique_image_identifier;

    VkSemaphore acquire_semaphore;///held temporarily by this struct, not owner, not created or destroyed as part of it

    cvm_vk_surface_swapchain * parent_swapchain;

    VkSemaphore present_semaphore;///needed by VkPresentInfoKHR, which doesn not accept timeline semaphores
    VkSemaphore qfot_semaphore;///required in cases where a QFOT is required, i.e. present_semaphore was signalled but alter work is required to actually present the image
    bool qfot_required;/// whether the above was used
    bool present_setup;/// present semaphore has been set

    bool acquired;
    bool presented;

    uint32_t last_use_queue_family;

    /// the command buffers to recieve the QFOT should be created as necessary
    VkCommandBuffer * present_acquire_command_buffers;

    cvm_vk_timeline_semaphore_moment last_use_moment;///changes throughout the frame, not strictly necessary for most applications, can probably be removed

    #warning make this the "last used moment" for this image, use it to track work across multiple "libraries" (e.g. game and overlay)
//    cvm_vk_timeline_semaphore_moment present_moment;///this is actually the last u
}
cvm_vk_swapchain_presentable_image;

/// all the data associated with a window and rendering to a surface(usually a window)
typedef struct cvm_vk_surface_swapchain
{
    VkSurfaceKHR surface;/// effectively the window

    VkFence metering_fence;///wait till previous fence is acquired before acquiring another
    bool metering_fence_active;

    uint32_t min_image_count;
    uint32_t max_image_count;///max count experienced over the lifetime of this swapchain
    VkImageUsageFlagBits usage_flags;

    uint64_t queue_family_presentable_mask;
    ///if we can't present on the current queue family we'll present on the fallback (lowest indexed queue family that supports present)
    uint32_t fallback_present_queue_family;

    VkSurfaceCapabilitiesKHR surface_capabilities;


    VkSwapchainKHR swapchain;
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

    uint16_t unique_image_counter;
    uint16_t generation;

    ///following used to determine number of swapchain images to allocate
    bool rendering_resources_valid;/// starts false, used to determine if rebuilding of resources is required due to swapchain invalidation (e.g. because window was resized)
}
cvm_vk_surface_swapchain;


int cvm_vk_swapchain_initialse(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, const cvm_vk_swapchain_setup * setup);
void cvm_vk_swapchain_terminate(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain);

int cvm_vk_swapchain_regenerate(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain);

#warning remove cleanup index at some point
const cvm_vk_swapchain_presentable_image * cvm_vk_swapchain_acquire_presentable_image(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, bool rendering_resources_invalid, uint32_t * cleanup_index);
bool cvm_vk_check_for_remaining_frames(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, uint32_t * cleanup_index);

void cvm_vk_swapchain_present_image(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, cvm_vk_swapchain_presentable_image * presentable_image);


#endif

