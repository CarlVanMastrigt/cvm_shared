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

#define CVM_VK_MAX_EXTERNAL 256
///256 better?

#define CVM_VK_PRESENTATION_INSTANCE_COUNT 2
///need 1 more than this for each extra layer of cycling (e.g. thread-sync-separation, that is written in thread then passed to main thread before being rendered with)

#define CVM_VK_MAX_SWAPCHAIN_IMAGES 16

typedef struct gfx_data gfx_data;///move all unshared data to static variables in .c file ?

typedef struct gfx_swapchain_image_data
{
    VkImageView image_view;
    VkImage image;
}
gfx_swapchain_image_data;

typedef struct gfx_presentation_instance
{
    VkFramebuffer * framebuffers;///dont need framebuffers
    VkFence completion_fence;
    VkSemaphore acquire_semaphore;
    VkSemaphore graphics_semaphore;
    VkSemaphore present_semaphore;
    ///command buffers come from respective pools
    VkCommandBuffer graphics_command_buffer;
    VkCommandBuffer present_command_buffer;

    ///need to cycle images used in framebuffers
    VkImage * attachment_images;
    VkImageView * attachment_image_views;
}
gfx_presentation_instance;

typedef void(*cvm_vk_external_initialise)(VkDevice,VkPhysicalDevice,VkRect2D,VkViewport,uint32_t,VkImageView*);
typedef void(*cvm_vk_external_terminate)(VkDevice);
typedef void(*cvm_vk_external_render)(VkCommandBuffer,uint32_t,VkRect2D);///primary command buffer into which to render,  (framebuffer/swapchain_image) index to use for rendering, render_area
///first uint is queue_family, second is swapchain image count





/// gfx_data may not even be necessary

struct gfx_data
{


    ///move mose of these to file local? only keep items necessary for other threads to see in this struct (device &c.)
    gfx_presentation_instance * presentation_instances;
    uint32_t presentation_instance_index;
    uint32_t presentation_instance_count;

    VkCommandPool graphics_command_pool;
    uint32_t graphics_queue_family;
    VkQueue graphics_queue;

    VkCommandPool transfer_command_pool;///transfer ops are memory barrier synched before graphics commands so fences for their ops shouldn't be necessary ?? (need to check spec)
    VkCommandBuffer setup_transfer_command_buffer;///only used in setup phase, before transfers become necessary
    uint32_t transfer_queue_family;
    VkQueue transfer_queue;

    // [NO LONGER THE CASE, PRESENT NEEDS TRANSITION COMMAND] present queue does not use commands (at least not any provided by application)
    VkCommandPool present_command_pool;///transfer ops are memory barrier synched before graphics commands so fences for their ops shouldn't be necessary ?? (need to check spec)
    //VkCommandBuffer present_transition_command_buffer;
    uint32_t present_queue_family;
    VkQueue present_queue;

    /// MUST be called in critical section, currently just used for pipelines but has potential for more uses
//    uint32_t swapchain_dependent_function_count;///per resize ops?
//    cvm_vk_function * swapchain_dependent_create_function;
//    cvm_vk_function * swapchain_dependent_destroy_function;


    uint32_t swapchain_image_count;
    gfx_swapchain_image_data * swapchain_images;


//    VkPipeline test_pipeline;
//    VkBuffer test_buffer;
//    VkDeviceMemory  test_buffer_memory;


//    VkRenderPass render_pass;

//    VkSurfaceKHR surface;
//    VkPresentModeKHR surface_present_mode;
//    VkSurfaceFormatKHR surface_format;

    ///presentation
    VkSwapchainKHR swapchain;

    ///below shared/used by lots of / all pipelines
    VkRect2D screen_rectangle;///extent
    VkViewport screen_viewport;
    VkPipelineViewportStateCreateInfo screen_viewport_state;
};



///gfx_data can ONLY change in main thread and ONLY be changed when not in use by other threads (inside main sync mutexes or before starting other threads that use it)


gfx_data * create_gfx_data(SDL_Window * window);
void destroy_gfx_data(gfx_data * gfx);


//uint32_t add_cvm_vk_swapchain_dependent_functions(gfx_data * gfx,cvm_vk_function * create,cvm_vk_function * destroy);

//void initialise_gfx_render_pass(gfx_data * gfx);

void initialise_cvm_vk_swapchain_and_dependents(void);
void reinitialise_cvm_vk_swapchain_and_dependents(void);
void terminate_cvm_vk_swapchain_and_dependents(void);

//void initialise_gfx_swapchain(gfx_data * gfx);///needs secondary (main & overlay) command buffers to be set before being called
//void recreate_gfx_swapchain(gfx_data * gfx);/// re-create all pipelines as well, MUST be called in critical section

void initialise_gfx_display_data(gfx_data * gfx);
void recreate_gfx_display_data(gfx_data * gfx);


void present_gfx_data(gfx_data * gfx);





///test functions

//void create_pipeline(const char * vert_file,const char * geom_file,const char * frag_file);
//void create_vertex_buffer(gfx_data * gfx);


void initialise_test_render_data(void);
void terminate_test_render_data(void);

#endif
