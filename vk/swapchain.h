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


enum cvm_vk_presentable_image_state
{
    cvm_vk_presentable_image_state_ready = 0,/// basically uninitialised
    cvm_vk_presentable_image_state_acquired = 1,
    cvm_vk_presentable_image_state_started = 2,/// have waited on acquire semaphore basically
    cvm_vk_presentable_image_state_tranferred = 3, /// for QFOT
    cvm_vk_presentable_image_state_complete = 4,
    cvm_vk_presentable_image_state_presented = 5,
};

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

typedef struct cvm_vk_swapchain_instance cvm_vk_swapchain_instance;

typedef struct cvm_vk_swapchain_presentable_image
{
    VkImage image;///theese are provided by the WSI, need access to this for synchronization purposes
    VkImageView image_view;

    /// "unique" identifier used to differentiate images after swapchain recreation
    uint16_t index;

    VkSemaphore acquire_semaphore;///held temporarily by this struct, not owner, not created or destroyed as part of it

    cvm_vk_swapchain_instance * parent_swapchain_instance;/// use with care

    VkSemaphore present_semaphore;///needed by VkPresentInfoKHR, which doesn not accept timeline semaphores
    VkSemaphore qfot_semaphore;///required in cases where a QFOT is required, i.e. present_semaphore was signalled but alter work is required to actually present the image

    enum cvm_vk_presentable_image_state state;
    VkImageLayout layout;

    uint32_t last_use_queue_family;

    /// the command buffers to recieve the QFOT should be created as necessary
    VkCommandBuffer * present_acquire_command_buffers;

    cvm_vk_timeline_semaphore_moment last_use_moment;/// used for ensuring a swapchain has had all its images returned before being destroyed
}
cvm_vk_swapchain_presentable_image;

struct cvm_vk_swapchain_instance
{
    /// its assumed these can change as the surface changes, probably not the case but whatever
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;

    VkSwapchainKHR swapchain;

    uint32_t generation;/// essentially just index in the queue, used to identify this instance as novel

    uint32_t fallback_present_queue_family;///if we can't present on the current queue family we'll present on the fallback (lowest indexed queue family that supports present)
    uint64_t queue_family_presentable_mask;

    VkSurfaceCapabilitiesKHR surface_capabilities;

    VkSemaphore * image_acquisition_semaphores;///number of these should match swapchain image count plus 1
    cvm_vk_swapchain_presentable_image * presentable_images;
    uint32_t image_count;/// this is also the number of swapchain images
    uint32_t acquired_image_count;/// init as 0, used for tracking use of image_acquisition_semaphores

    bool out_of_date;/// can be used

//    struct /// pregenerated defaults for use in creating other state
//    {
//        VkRect2D scissor;
//        VkViewport viewport;
//        VkPipelineViewportStateCreateInfo pipeline_viewport_state;
////        VkExtent3D ??
//    }
//    defaults;
};

CVM_QUEUE(cvm_vk_swapchain_instance,cvm_vk_swapchain_instance,4)

/// all the data associated with a window and rendering to a surface(usually a window)
typedef struct cvm_vk_surface_swapchain
{
    cvm_vk_swapchain_setup setup_info;

    VkFence metering_fence;///wait till previous fence is acquired before acquiring another
    bool metering_fence_active;

    cvm_vk_swapchain_instance_queue swapchain_queue;/// need to preserve out of date/invalid swapchains while they're still in use
}
cvm_vk_surface_swapchain;


int cvm_vk_swapchain_initialse(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain, const cvm_vk_swapchain_setup * setup);
void cvm_vk_swapchain_terminate(const cvm_vk_device * device, cvm_vk_surface_swapchain * swapchain);

const cvm_vk_swapchain_presentable_image * cvm_vk_surface_swapchain_acquire_presentable_image(cvm_vk_surface_swapchain * swapchain, const cvm_vk_device * device);

void cvm_vk_surface_swapchain_present_image(const cvm_vk_surface_swapchain * swapchain, const cvm_vk_device * device, cvm_vk_swapchain_presentable_image * presentable_image);


#endif

