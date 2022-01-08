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

#include "cvm_shared.h"

static VkRect2D cvm_vk_default_screen_rectangle;
static VkViewport cvm_vk_default_viewport;
static VkPipelineViewportStateCreateInfo cvm_vk_default_viewport_state;

static VkImageCreateInfo cvm_vk_default_framebuffer_image_creation_info;
static VkImageViewCreateInfo cvm_vk_default_framebuffer_image_view_creation_info;
static VkFramebufferCreateInfo cvm_vk_default_framebuffer_creation_info;

void cvm_vk_initialise_defaults(void)
{
}

void cvm_vk_initialise_swapchain_dependednt_defaults(uint32_t width,uint32_t height)
{
    cvm_vk_default_screen_rectangle=(VkRect2D)
    {
        .offset=(VkOffset2D){.x=0,.y=0},
        .extent=(VkExtent2D){.width=width,.height=height}
    };

    cvm_vk_default_viewport=(VkViewport)
    {
        .x=0.0,
        .y=0.0,
        .width=(float)width,
        .height=(float)height,
        .minDepth=0.0,
        .maxDepth=1.0
    };

    cvm_vk_default_viewport_state=(VkPipelineViewportStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .viewportCount=1,
        .pViewports=&cvm_vk_default_viewport,
        .scissorCount=1,
        .pScissors= &cvm_vk_default_screen_rectangle
    };


    cvm_vk_default_framebuffer_image_creation_info=(VkImageCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .imageType=VK_IMAGE_TYPE_2D,
        .format=VK_FORMAT_UNDEFINED,///set later
        .extent=(VkExtent3D)
        {
            .width=width,
            .height=height,
            .depth=1
        },
        .mipLevels=1,
        .arrayLayers=0,///set later
        .samples=0,///set later
        .tiling=VK_IMAGE_TILING_OPTIMAL,
        .usage=0,///set later
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL,
        .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED
    };

    cvm_vk_default_framebuffer_image_view_creation_info=(VkImageViewCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .image=VK_NULL_HANDLE,///set later
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .format=VK_FORMAT_UNDEFINED,///set later
        .components=(VkComponentMapping)
        {
            .r=VK_COMPONENT_SWIZZLE_IDENTITY,
            .g=VK_COMPONENT_SWIZZLE_IDENTITY,
            .b=VK_COMPONENT_SWIZZLE_IDENTITY,
            .a=VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange=(VkImageSubresourceRange)
        {
            .aspectMask=0,///set later
            .baseMipLevel=0,
            .levelCount=1,
            .baseArrayLayer=0,///set later
            .layerCount=1
        }
    };

    cvm_vk_default_framebuffer_creation_info=(VkFramebufferCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .renderPass=VK_NULL_HANDLE,///set later
        .attachmentCount=0,///set later
        .pAttachments=NULL,///set later
        .width=width,
        .height=height,
        .layers=1
    };
}


VkPipelineViewportStateCreateInfo * cvm_vk_get_default_viewport_state(void)
{
    return &cvm_vk_default_viewport_state;
}

void cvm_vk_create_default_framebuffer_image(VkImage * image,VkFormat format,uint32_t layers,VkSampleCountFlagBits samples,VkImageUsageFlags usage)
{
    cvm_vk_default_framebuffer_image_creation_info.format=format;
    cvm_vk_default_framebuffer_image_creation_info.arrayLayers=layers;
    cvm_vk_default_framebuffer_image_creation_info.samples=samples;
    cvm_vk_default_framebuffer_image_creation_info.usage=usage;

    cvm_vk_create_image(image,&cvm_vk_default_framebuffer_image_creation_info);
}

void cvm_vk_create_default_framebuffer_image_views(VkImageView * views,VkImage image,VkFormat format,VkImageAspectFlags aspects,uint32_t layers)
{
    cvm_vk_default_framebuffer_image_view_creation_info.image=image;
    cvm_vk_default_framebuffer_image_view_creation_info.format=format;
    cvm_vk_default_framebuffer_image_view_creation_info.subresourceRange.aspectMask=aspects;

    while(layers--)
    {
        cvm_vk_default_framebuffer_image_view_creation_info.subresourceRange.baseArrayLayer=layers;
        cvm_vk_create_image_view(views+layers,&cvm_vk_default_framebuffer_image_view_creation_info);
    }
}

void cvm_vk_create_default_framebuffer(VkFramebuffer * framebuffer,VkRenderPass render_pass,VkImageView * attachments,uint32_t attachment_count)
{
    cvm_vk_default_framebuffer_creation_info.renderPass=render_pass;
    cvm_vk_default_framebuffer_creation_info.attachmentCount=attachment_count;
    cvm_vk_default_framebuffer_creation_info.pAttachments=attachments;

    cvm_vk_create_framebuffer(framebuffer,&cvm_vk_default_framebuffer_creation_info);

}
