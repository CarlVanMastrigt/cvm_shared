/**
Copyright 2020 Carl van Mastrigt

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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif


#ifndef CVM_GFX_H
#define CVM_GFX_H

#ifndef CVM_VK_CHECK
#define CVM_VK_CHECK(f)                                                         \
{                                                                               \
    VkResult r=f;                                                               \
    if(r!=VK_SUCCESS)                                                           \
    {                                                                           \
        fprintf(stderr,"VULKAN FUNCTION FAILED : %d :%s\n",r,#f);               \
        exit(-1);                                                               \
    }                                                                           \
}

#endif



typedef struct gfx_presentation_instance
{
    VkImageView image_view;
    VkFramebuffer framebuffer;
    VkCommandBuffer command_buffer;///remove from here (use gfx_primary_render_buffer instead)
}
gfx_presentation_instance;

typedef struct gfx_primary_render_buffer
{
    VkCommandBuffer command_buffer;///uses main command pool

    VkFence fence;
}
gfx_primary_render_buffer;

//typedef struct gfx_secondary_render_buffer
//{
//    VkCommandPool command_pool;
//    VkCommandBuffer * command_buffers;///all secondary command buffers executed inside gfx_primary_render_buffer's command buffer
//    uint32_t buffer_count;
//    uint32_t update_count;///make sure command buffers with outdated pipeline aren't executed
//
//    ///needs instance/texture buffers, may need to instead have function to call, data buffer and update_count
//
//    ///maintain queue for these separately, as more than 2 may be needed if filled in separate thread
//}
//gfx_secondary_render_buffer;

typedef struct gfx_data
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;///"logical" device

    VkPhysicalDeviceFeatures device_features;


    VkCommandPool command_pool;
    uint32_t queue_family;///split into backend and present family queues? probably not, (maybe) very difficult to achieve
    VkQueue queue;
    /// NO! go full hog - separate transfer, render and present queues/queue_families/command_pools

    VkCommandPool graphics_command_pool;
    uint32_t graphics_queue_family;
    VkQueue graphics_queue;

    VkCommandPool transfer_command_pool;///for primary command buffer of transfer operations (provided by
    uint32_t transfer_queue_family;
    VkQueue transfer_queue;

    ///present queue does not use commands (at least not any provided by application)
    uint32_t present_queue_family;
    VkQueue present_queue;



    bool separate_present_queue_family;
    bool separate_transfer_queue_family;


    uint32_t presentation_instance_count;
    gfx_presentation_instance * presentation_instances;
    ///move to local data as necessary (unless requires cleanup?)
    //VkCommandBuffer * presentation_command_buffers;


    VkPipeline test_pipeline;


    VkFence submission_finished_fence;///remove from here (use gfx_primary_render_buffer instead)


    VkSurfaceKHR surface;
    VkPresentModeKHR surface_present_mode;
    VkSurfaceFormatKHR surface_format;

    ///presentation
    VkSwapchainKHR swapchain;
    VkSemaphore acquire_image_semaphore;
    VkSemaphore render_finished_semaphore;
    VkRenderPass render_pass;


    VkRect2D screen_rectangle;

    uint32_t update_count;
}
gfx_data;



///gfx_data can ONLY change in main thread and ONLY be changed when not in use by other threads (inside main sync mutexes or before starting other threads that use it)


gfx_data * create_gfx_data(SDL_Window * window);
void destroy_gfx_data(gfx_data * gfx);

void initialise_gfx_swapchain(gfx_data * gfx);///needs secondary (main & overlay) command buffers to be set before being called
void recreate_gfx_swapchain(gfx_data * gfx);

bool present_gfx_data(gfx_data * gfx);

void create_pipeline(gfx_data * gfx,const char * vert_file,const char * geom_file,const char * frag_file);

#endif
