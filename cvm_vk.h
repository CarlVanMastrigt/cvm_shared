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

void cvm_vk_create_render_pass(VkRenderPassCreateInfo * render_pass_creation_info,VkRenderPass * render_pass);///?

VkPipelineShaderStageCreateInfo cvm_vk_initialise_shader_stage_creation_info(const char * filename,VkShaderStageFlagBits stage);
void cvm_vk_terminate_shader_stage_creation_info(VkPipelineShaderStageCreateInfo * stage_creation_info);

VkFormat cvm_vk_get_screen_format(void);
///VkPipelineViewportStateCreateInfo * cvm_vk_get_screen_viewport_state(void);


/// break this down into portions that can be initialised and shared around seperately
/// have a way to copy in or set manually, also (where appropriate) automatic/semi-automatic initialization
/// checks that each part are initialised

/// this is just the pipeline state duplicated! instead initialise a static VkGraphicsPipelineCreateInfo where needed, with shared pointers of creation data where appropriate
/// then just build as necessary (viewport & similar data can actually be set AFTER having passed in pointer to the structure)
/// local/static init function where VkGraphicsPipelineCreateInfo is initially set should also create shader modules and manage them
/// can read directly from the VkGraphicsPipelineCreateInfo to figure out which VkShaderModule's need destroying at program termination
/// all VkGraphicsPipelineCreateInfo should be set (or at least have appropriate pointer provided) at setup of their containing module
/// VkPipelineLayout should be managed (created and destroyed) as appropriate (either created and destroyed locally or left to parent layer, i.e. the place that provides it)
typedef struct cvm_vk_pipeline_state
{
    /// stages
    ///these can be read from/known by VkGraphicsPipelineCreateInfo, no need to store copies!
    VkShaderModule * stage_modules;
    VkPipelineShaderStageCreateInfo * stage_module_infos;
    uint32_t stage_count;


    ///beyond modules above, a lot of the following content can/should be shared between different pipelines and only known here by reference, probably never even being changed
    /// i.e. above is locally owned, below is externally owned.
    /// externally owned things shouldnt have their contents known here!
    /// probably best to treat externally owned items as being mutable and potentially requiring pipeline re-init (even though this is invalid for a lot of items that match parts of shader)

    /// vertex inputs
    VkPipelineVertexInputStateCreateInfo * vert_input_info;

    //VkVertexInputBindingDescription * input_bindings;
    //uint32_t input_binding_count;

    //VkVertexInputAttributeDescription * input_attributes;
    //uint32_t input_attribute_count;


    /// blending (attachment related)
    /// make this part of, or related to subpass
    VkPipelineColorBlendStateCreateInfo * blend_info;
    //VkPipelineColorBlendAttachmentState * blend_attachment_infos;
    //uint32_t blend_attachment_count;///basically should match colorAttachmentCount in subpass of pipeline


    /// these are probably mutable, but i cant see that ever realistically being needed
    VkPipelineInputAssemblyStateCreateInfo * input_assembly_info;///some of these can be shared
    VkPipelineRasterizationStateCreateInfo * rasterization_info;


    /// general input data, descriptor sets and push constants (like uniforms)
    /// can be shared between similar pipelines that have/use same bindings, should be provided externally and only referenced here
    /// layout itself wont change, and is used to specify how data is actually loaded into descriptor sets which themselves provide data to their respective binding points in the shader
            /// ^ this happens more or less completely seperately from the pipeline itself
            /// ^ can bind contents of layout with vkCmdBindDescriptorSets prior to submitting any work or even binding pipelines (i prefer this approach, rather than binding after pipeline)
    VkPipelineLayout * layout;

    //VkDescriptorSetLayout * descrptor_set_layouts;///take these from external setup, use in conjunction with recyclable descriptor sets from pools
    //uint32_t descrptor_set_count;

    //VkPushConstantRange * push_constants;
    //uint32_t push_constant_count;


    /// these are actually mutable without completely breaking!
    VkPipelineViewportStateCreateInfo * viewport_state_info;
    VkPipelineMultisampleStateCreateInfo * multisample_state_info;/// could possible be changed my altering game settings
}
cvm_vk_pipeline_state;

//cvm_vk_pipeline_state cvm_vk_create_pipeline_state();
//
//
//void cvm_vk_alter_pipeline_states_target(VkRenderPass render_pass,VkRect2D screen_rectangle,VkViewport screen_viewport);
//cvm_vk_pipeline_state cvm_vk_destroy_pipeline_state();



///test stuff
void initialise_test_render_data(void);
void terminate_test_render_data(void);


#endif
