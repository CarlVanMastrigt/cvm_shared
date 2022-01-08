/**
Copyright 2020,2021,2022 Carl van Mastrigt

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

void cvm_vk_initialise_defaults(void);

void cvm_vk_initialise_swapchain_dependednt_defaults(uint32_t width,uint32_t height);



VkPipelineViewportStateCreateInfo * cvm_vk_get_default_viewport_state(void);
void cvm_vk_create_default_framebuffer_image(VkImage * image,VkFormat format,uint32_t layers,VkSampleCountFlagBits samples,VkImageUsageFlags usage);
void cvm_vk_create_default_framebuffer_image_views(VkImageView * views,VkImage image,VkFormat format,VkImageAspectFlags aspects,uint32_t layers);
void cvm_vk_create_default_framebuffer(VkFramebuffer * framebuffer,VkRenderPass render_pass,VkImageView * attachments,uint32_t attachment_count);

#endif

