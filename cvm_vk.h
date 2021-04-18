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


typedef struct cvm_vk_external_module
{
    /// initialise and terminate are per-swapchain instantiation
    void    (*initialise)   (VkDevice,VkPhysicalDevice,VkRect2D,VkViewport,uint32_t,VkImageView*);
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

void cvm_vk_create_render_pass(VkRenderPassCreateInfo * render_pass_creation_info,VkRenderPass * render_pass);

VkFormat cvm_vk_get_screen_format(void);

typedef struct cvm_vk_pipeline_state
{
    /// stages
    VkShaderModule * stage_modules;
    VkPipelineShaderStageCreateInfo * stage_module_infos;
    uint32_t stage_count;


    /// vertex inputs
    VkPipelineVertexInputStateCreateInfo vert_input_info;

    VkVertexInputBindingDescription * input_bindings;
    uint32_t input_binding_count;

    VkVertexInputAttributeDescription * input_attributes;
    uint32_t input_attribute_count;


    /// blending (attachment related)
    VkPipelineColorBlendStateCreateInfo blend_info;
    VkPipelineColorBlendAttachmentState * blend_attachment_infos;
    uint32_t blend_attachment_count;///basically should match colorAttachmentCount in subpass of pipeline


    /// misc static
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    VkPipelineRasterizationStateCreateInfo rasterization_info;


    /// misc mutable
    VkPipelineViewportStateCreateInfo viewport_state_info;
    VkPipelineMultisampleStateCreateInfo multisample_info;/// could possible be changed my altering game settings



    /// general input data, descriptor sets and push constants (like uniforms)
    ///     these can probably be tied to/inherited from the subpass in most cases
    VkPipelineLayout layout;

    VkDescriptorSetLayout * descrptor_sets;
    uint32_t descrptor_set_count;

    VkPushConstantRange * push_constants;
    uint32_t push_constant_count;
}
cvm_vk_pipeline_state;

cvm_vk_pipeline_state cvm_vk_create_pipeline_state();


void cvm_vk_alter_pipeline_states_target(VkRenderPass render_pass,VkRect2D screen_rectangle,VkViewport screen_viewport);
cvm_vk_pipeline_state cvm_vk_destroy_pipeline_state();



///test stuff
void initialise_test_render_data(void);
void terminate_test_render_data(void);


#endif
