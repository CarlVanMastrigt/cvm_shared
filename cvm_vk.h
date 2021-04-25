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


#ifndef CVM_VK_H
#define CVM_VK_H

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

#define CVM_VK_PRESENTATION_INSTANCE_COUNT 3
///need 1 more than this for each extra layer of cycling (e.g. thread-sync-separation, that is written in thread then passed to main thread before being rendered with)




/**
features to be cognisant of:

fragmentStoresAndAtomics



(maybe)

vertexPipelineStoresAndAtomics
*/

//typedef enum
//{
//    CVM_VK_CHANGED_SCREEN_SIZE  = 0x01,
//    CVM_VK_CHANGED_MSAA         = 0x02,
//    CVM_VK_CHANGED_ALL          = 0xFF
//}
//cvm_vk_change_flags;

///won't be supporting submodules having msaa output

typedef struct cvm_vk_external_module
{
    /// initialise and terminate are per-swapchain instantiation
    void    (*initialise)   (VkDevice,VkPhysicalDevice,VkRect2D,uint32_t,VkImageView*);
    /// per module update, each module internally keeps links to track data that requires rebuilding of resources (then this func executes those rebuilds)
    void    (*render    )   (VkCommandBuffer,uint32_t,VkRect2D);///primary command buffer into which to render,  (framebuffer/swapchain_image) index to use for rendering, render_area
    void    (*terminate )   (VkDevice);
}
cvm_vk_external_module;

uint32_t cvm_vk_add_external_module(cvm_vk_external_module module);

///gfx_data can ONLY change in main thread and ONLY be changed when not in use by other threads (inside main sync mutexes or before starting other threads that use it)

void cvm_vk_initialise(SDL_Window * window,uint32_t frame_count);/// frame count is number of frames worth of rendering data needed (i.e. max level of thread-sync-separation across all rendering sources plus two)
void cvm_vk_terminate(void);///also terminates swapchain dependant data at same time

void cvm_vk_initialise_swapchain(void);
void cvm_vk_reinitialise_swapchain(void);

void cvm_vk_present(void);

void cvm_vk_wait(void);



VkFormat cvm_vk_get_screen_format(void);///can remove?


void cvm_vk_create_render_pass(VkRenderPass * render_pass,VkRenderPassCreateInfo * info);
void cvm_vk_destroy_render_pass(VkRenderPass render_pass);

void cvm_vk_create_framebuffer(VkFramebuffer * framebuffer,VkFramebufferCreateInfo * info);
void cvm_vk_destroy_framebuffer(VkFramebuffer framebuffer);

void cvm_vk_create_pipeline_layout(VkPipelineLayout * pipeline_layout,VkPipelineLayoutCreateInfo * info);
void cvm_vk_destroy_pipeline_layout(VkPipelineLayout pipeline_layout);

void cvm_vk_create_graphics_pipeline(VkPipeline * pipeline,VkGraphicsPipelineCreateInfo * info);
void cvm_vk_destroy_pipeline(VkPipeline pipeline);

void cvm_vk_create_shader_stage_info(VkPipelineShaderStageCreateInfo * stage_info,const char * filename,VkShaderStageFlagBits stage);
void cvm_vk_destroy_shader_stage_info(VkPipelineShaderStageCreateInfo * stage_info);



///test stuff
void initialise_test_render_data(void);
void terminate_test_render_data(void);


#endif
