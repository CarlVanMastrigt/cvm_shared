/**
Copyright 2022 Carl van Mastrigt

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


#ifndef CVM_VK_DEFAULTS_H
#define CVM_VK_DEFAULTS_H

///everything here must be called within critical (thread safe) section of code, generally this is how you should handle resource recreation anyway so it should be fine

void cvm_vk_create_defaults(void);
void cvm_vk_destroy_defaults(void);

void cvm_vk_create_swapchain_dependednt_defaults(uint32_t width,uint32_t height);
void cvm_vk_destroy_swapchain_dependednt_defaults(void);


VkRect2D cvm_vk_get_default_render_area(void);
VkExtent3D cvm_vk_get_default_framebuffer_extent(void);

VkPipelineInputAssemblyStateCreateInfo * cvm_vk_get_default_input_assembly_state(void);
VkPipelineViewportStateCreateInfo * cvm_vk_get_default_viewport_state(void);
VkPipelineRasterizationStateCreateInfo * cvm_vk_get_default_raster_state(bool culled);
VkPipelineMultisampleStateCreateInfo * cvm_vk_get_default_multisample_state(void);
VkPipelineDepthStencilStateCreateInfo * cvm_vk_get_default_greater_depth_stencil_state(void);
VkPipelineDepthStencilStateCreateInfo * cvm_vk_get_default_equal_depth_stencil_state(void);
VkPipelineDepthStencilStateCreateInfo * cvm_vk_get_default_null_depth_stencil_state(void);
VkPipelineColorBlendAttachmentState cvm_vk_get_default_null_blend_state(void);
VkPipelineColorBlendAttachmentState cvm_vk_get_default_alpha_blend_state(void);
VkPipelineColorBlendAttachmentState cvm_vk_get_default_additive_blend_state(void);


void cvm_vk_create_default_framebuffer_images(VkImage * images,VkFormat format,uint32_t count,VkSampleCountFlagBits samples,VkImageUsageFlags usage);
void cvm_vk_create_default_framebuffer_image_views(VkImageView * views,VkImage * images,VkFormat format,VkImageAspectFlags aspects,uint32_t count);
void cvm_vk_create_default_framebuffer(VkFramebuffer * framebuffer,VkRenderPass render_pass,VkImageView * attachments,uint32_t attachment_count);

VkPipelineVertexInputStateCreateInfo * cvm_vk_get_mesh_vertex_input_state(uint16_t flags);
VkPipelineVertexInputStateCreateInfo * cvm_vk_get_empty_vertex_input_state(void);

VkSampler cvm_vk_get_fetch_sampler(void);


VkPipelineShaderStageCreateInfo cvm_vk_get_default_fullscreen_vertex_stage(void);
void cvm_vk_render_fullscreen_pass(VkCommandBuffer cb);

VkAttachmentDescription cvm_vk_get_default_colour_attachment(VkFormat format,VkSampleCountFlagBits sample_count,bool clear,bool load,bool store);
VkAttachmentDescription cvm_vk_get_default_depth_stencil_attachment(VkFormat format,VkSampleCountFlagBits sample_count,bool depth_clear,bool depth_load,bool depth_store,bool stencil_clear,bool stencil_load,bool stencil_store);

VkSubpassDependency cvm_vk_get_default_colour_attachment_dependency(uint32_t src_subpass,uint32_t dst_subpass);
VkSubpassDependency cvm_vk_get_default_colour_attachment_dependency_specialised(uint32_t src_subpass,bool src_input_attachment,bool src_blended,uint32_t dst_subpass,bool dst_input_attachment,bool dst_blended);
VkSubpassDependency cvm_vk_get_default_depth_stencil_attachment_dependency(uint32_t srcSubpass,uint32_t dstSubpass);

#endif

