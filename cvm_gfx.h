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

typedef struct gfx_data gfx_data;

typedef struct gfx_swapchain_image_data
{
    VkImageView image_view;
    VkImage image;
}
gfx_swapchain_image_data;

typedef struct gfx_presentation_instance
{
    VkFramebuffer * framebuffers;
    VkFence completion_fence;
    VkSemaphore acquire_image_semaphore;
    VkSemaphore render_finished_semaphore;
    ///command buffers come from respective pools
    VkCommandBuffer graphics_command_buffer;
    VkCommandBuffer transfer_command_buffer;
}
gfx_presentation_instance;

typedef void(*gfx_function)(gfx_data*);

struct gfx_data
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;///"logical" device

    VkPhysicalDeviceFeatures device_features;


//    VkCommandPool command_pool;
//    uint32_t queue_family;///split into backend and present family queues? probably not, (maybe) very difficult to achieve
//    VkQueue queue;
    /// NO! go full hog - separate transfer, render and present queues/queue_families/command_pools


    gfx_presentation_instance * presentation_instances;
    uint32_t presentation_instance_index;
    uint32_t presentation_instance_count;

//    VkFence previous_fence;
//    VkFence current_fence;

    VkCommandPool graphics_command_pool;
    uint32_t graphics_queue_family;
    VkQueue graphics_queue;
//    VkCommandBuffer previous_graphics_command_buffer;
//    VkCommandBuffer current_graphics_command_buffer;

    VkCommandPool transfer_command_pool;///transfer ops are memory barrier synched before graphics commands so fences for their ops shouldn't be necessary ?? (need to check spec)
    uint32_t transfer_queue_family;
    VkQueue transfer_queue;
//    VkCommandBuffer previous_transfer_command_buffer;
//    VkCommandBuffer current_transfer_command_buffer;

    ///present queue does not use commands (at least not any provided by application)
    uint32_t present_queue_family;
    VkQueue present_queue;

    /// MUST be called in critical section, currently just used for pipelines but has potential for more uses
    uint32_t pipeline_op_count;///per resize ops?
    gfx_function * pipeline_create_ops;
    gfx_function * pipeline_destroy_ops;


    bool separate_present_queue_family;
    bool separate_transfer_queue_family;


    uint32_t swapchain_image_count;
    gfx_swapchain_image_data * swapchain_images;
    ///move to local data as necessary (unless requires cleanup?)
    //VkCommandBuffer * presentation_command_buffers;


    VkPipeline test_pipeline;
    VkBuffer test_buffer;
    VkDeviceMemory  test_buffer_memory;


    VkRenderPass render_pass;

    uint32_t attachment_count;
    VkAttachmentDescription * attachments;
    VkClearValue * clear_values;
    ///next ones must be re-set when resizing window, and before calling initialise_gfx_swapchain
    ///ignore 0 as it's swapchain image location
    ///get format and sample rate for these from [VkAttachmentDescription * attachments] above ??
    VkImage * attachment_images;
    VkImageView * attachment_image_views;



    uint32_t subpass_count;
    VkSubpassDescription * subpasses;



//    VkFence submission_finished_fence;///remove from here (use gfx_primary_render_buffer instead)


    VkSurfaceKHR surface;
    VkPresentModeKHR surface_present_mode;
    VkSurfaceFormatKHR surface_format;

    ///presentation
    VkSwapchainKHR swapchain;
//    VkSemaphore acquire_image_semaphore;
//    VkSemaphore render_finished_semaphore;

    ///below shared/used by lots of / all pipelines
    VkRect2D screen_rectangle;///extent
    VkViewport screen_viewport;
    VkPipelineViewportStateCreateInfo screen_viewport_state;

    uint32_t update_count;
};



///gfx_data can ONLY change in main thread and ONLY be changed when not in use by other threads (inside main sync mutexes or before starting other threads that use it)


gfx_data * create_gfx_data(SDL_Window * window);
void destroy_gfx_data(gfx_data * gfx);


uint32_t add_gfx_colour_attachment(gfx_data * gfx,VkFormat format,VkSampleCountFlagBits samples);
uint32_t add_gfx_depth_stencil_attachment(gfx_data * gfx,VkFormat format,VkSampleCountFlagBits samples);
uint32_t add_gfx_subpass(gfx_data * gfx);
uint32_t add_gfx_pipeline_op(gfx_data * gfx,gfx_function * create,gfx_function * destroy);///actual pipeline variable must be stored elsewhere (usually static variable)
void initialise_gfx_render_pass(gfx_data * gfx);

//void initialise_gfx_swapchain(gfx_data * gfx);///needs secondary (main & overlay) command buffers to be set before being called
//void recreate_gfx_swapchain(gfx_data * gfx);/// re-create all pipelines as well, MUST be called in critical section

void initialise_gfx_display_data(gfx_data * gfx);
void recreate_gfx_display_data(gfx_data * gfx);


void present_gfx_data(gfx_data * gfx);




void create_pipeline(gfx_data * gfx,const char * vert_file,const char * geom_file,const char * frag_file);
void create_vertex_buffer(gfx_data * gfx);

#endif
