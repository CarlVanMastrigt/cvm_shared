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

static VkPipelineInputAssemblyStateCreateInfo cvm_vk_default_input_assembly_state;
static VkPipelineViewportStateCreateInfo cvm_vk_default_viewport_state;
static VkPipelineRasterizationStateCreateInfo cvm_vk_default_unculled_raster_state;
static VkPipelineRasterizationStateCreateInfo cvm_vk_default_culled_raster_state;
static VkPipelineMultisampleStateCreateInfo cvm_vk_default_multisample_state;
static VkPipelineDepthStencilStateCreateInfo cvm_vk_default_depth_stencil_state;
static VkPipelineColorBlendAttachmentState cvm_vk_default_no_blend_state;
static VkPipelineColorBlendAttachmentState cvm_vk_default_alpha_blend_state;

static VkImageCreateInfo cvm_vk_default_framebuffer_image_creation_info;
static VkImageViewCreateInfo cvm_vk_default_framebuffer_image_view_creation_info;
static VkFramebufferCreateInfo cvm_vk_default_framebuffer_creation_info;

///implement more variants when mesh supports vertex normas and tex-coords
VkVertexInputBindingDescription cvm_vk_default_pos_vertex_input_binding;
VkVertexInputAttributeDescription cvm_vk_default_pos_vertex_input_attribute;
VkPipelineVertexInputStateCreateInfo cvm_vk_default_pos_vertex_input_state;

VkPipelineVertexInputStateCreateInfo cvm_vk_default_empty_vertex_input_state;



static VkSampler cvm_vk_fetch_sampler;



static VkPipelineShaderStageCreateInfo cvm_vk_default_fullscreen_vertex_stage;



static void cvm_vk_create_default_vertex_bindings(void)
{
    cvm_vk_default_pos_vertex_input_binding= (VkVertexInputBindingDescription)
    {
        .binding=0,
        .stride=sizeof(cvm_mesh_data_pos),
        .inputRate=VK_VERTEX_INPUT_RATE_VERTEX
    };

    cvm_vk_default_pos_vertex_input_attribute=(VkVertexInputAttributeDescription)
    {
        .location=0,
        .binding=0,
        .format=cvm_mesh_get_pos_format(),
        .offset=offsetof(cvm_mesh_data_pos,pos)
    };

    cvm_vk_default_pos_vertex_input_state=(VkPipelineVertexInputStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .vertexBindingDescriptionCount=1,
        .pVertexBindingDescriptions=&cvm_vk_default_pos_vertex_input_binding,
        .vertexAttributeDescriptionCount=1,
        .pVertexAttributeDescriptions=&cvm_vk_default_pos_vertex_input_attribute
    };

    cvm_vk_default_empty_vertex_input_state=(VkPipelineVertexInputStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .vertexBindingDescriptionCount=0,
        .pVertexBindingDescriptions=NULL,
        .vertexAttributeDescriptionCount=0,
        .pVertexAttributeDescriptions=NULL
    };
}

static void cvm_vk_destroy_default_vertex_bindings(void)
{
}

static void cvm_vk_create_default_samplers(void)
{
    ///should perhaps make this one wrap? (for use with bayer dither or similar applications)
    VkSamplerCreateInfo fetch_sampler_creation_info=(VkSamplerCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .magFilter=VK_FILTER_NEAREST,
        .minFilter=VK_FILTER_NEAREST,
        .mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias=0.0f,
        .anisotropyEnable=VK_FALSE,
        .maxAnisotropy=0.0f,
        .compareEnable=VK_FALSE,
        .compareOp=VK_COMPARE_OP_NEVER,
        .minLod=0.0f,
        .maxLod=0.0f,
        .borderColor=VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates=VK_TRUE,
    };

    cvm_vk_create_sampler(&cvm_vk_fetch_sampler,&fetch_sampler_creation_info);
}

static void cvm_vk_destroy_default_samplers(void)
{
    cvm_vk_destroy_sampler(cvm_vk_fetch_sampler);
}

void cvm_vk_create_defaults(void)
{
    cvm_vk_create_default_samplers();
    cvm_vk_create_default_vertex_bindings();

    cvm_vk_default_input_assembly_state=(VkPipelineInputAssemblyStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable=VK_FALSE
    };

    cvm_vk_default_unculled_raster_state=(VkPipelineRasterizationStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .depthClampEnable=VK_FALSE,
        .rasterizerDiscardEnable=VK_FALSE,
        .polygonMode=VK_POLYGON_MODE_FILL,
        .cullMode=VK_CULL_MODE_NONE,
        .frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable=VK_FALSE,
        .depthBiasConstantFactor=0.0,
        .depthBiasClamp=0.0,
        .depthBiasSlopeFactor=0.0,
        .lineWidth=1.0
    };

    cvm_vk_default_culled_raster_state=(VkPipelineRasterizationStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .depthClampEnable=VK_FALSE,
        .rasterizerDiscardEnable=VK_FALSE,
        .polygonMode=VK_POLYGON_MODE_FILL,
        .cullMode=VK_CULL_MODE_BACK_BIT,
        .frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable=VK_FALSE,
        .depthBiasConstantFactor=0.0,
        .depthBiasClamp=0.0,
        .depthBiasSlopeFactor=0.0,
        .lineWidth=1.0
    };

    cvm_vk_default_multisample_state=(VkPipelineMultisampleStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .rasterizationSamples=VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable=VK_FALSE,
        .minSampleShading=1.0,
        .pSampleMask=NULL,
        .alphaToCoverageEnable=VK_FALSE,
        .alphaToOneEnable=VK_FALSE
    };

    cvm_vk_default_depth_stencil_state=(VkPipelineDepthStencilStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .depthTestEnable=VK_TRUE,
        .depthWriteEnable=VK_TRUE,
        .depthCompareOp=VK_COMPARE_OP_GREATER,
        .depthBoundsTestEnable=VK_FALSE,
        .stencilTestEnable=VK_FALSE,
        .front=(VkStencilOpState)
        {
            .failOp=VK_STENCIL_OP_KEEP,
            .passOp=VK_STENCIL_OP_KEEP,
            .depthFailOp=VK_STENCIL_OP_KEEP,
            .compareOp=VK_COMPARE_OP_ALWAYS,
            .compareMask=0x00,
            .writeMask=0x00,
            .reference=0x00
        },
        .back=(VkStencilOpState)
        {
            .failOp=VK_STENCIL_OP_KEEP,
            .passOp=VK_STENCIL_OP_KEEP,
            .depthFailOp=VK_STENCIL_OP_KEEP,
            .compareOp=VK_COMPARE_OP_ALWAYS,
            .compareMask=0x00,
            .writeMask=0x00,
            .reference=0x00
        },
        .minDepthBounds=0.0,
        .maxDepthBounds=1.0,
    };


    cvm_vk_default_no_blend_state=(VkPipelineColorBlendAttachmentState)
    {
        .blendEnable=VK_FALSE,
        .srcColorBlendFactor=VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor=VK_BLEND_FACTOR_ZERO,
        .colorBlendOp=VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp=VK_BLEND_OP_ADD,
        .colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
    };

    cvm_vk_default_alpha_blend_state=(VkPipelineColorBlendAttachmentState)
    {
        .blendEnable=VK_TRUE,
        .srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp=VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp=VK_BLEND_OP_ADD,
        .colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
    };

    cvm_vk_create_shader_stage_info(&cvm_vk_default_fullscreen_vertex_stage,"cvm_shared/shaders/fullscreen_vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
}

void cvm_vk_destroy_defaults(void)
{
    cvm_vk_destroy_default_samplers();
    cvm_vk_destroy_default_vertex_bindings();

    cvm_vk_destroy_shader_stage_info(&cvm_vk_default_fullscreen_vertex_stage);
}

void cvm_vk_create_swapchain_dependednt_defaults(uint32_t width,uint32_t height)
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

void cvm_vk_destroy_swapchain_dependednt_defaults(void)
{
}

VkRect2D cvm_vk_get_default_render_area(void)
{
    return cvm_vk_default_screen_rectangle;
}

VkPipelineInputAssemblyStateCreateInfo * cvm_vk_get_default_input_assembly_state(void)
{
    return &cvm_vk_default_input_assembly_state;
}

VkPipelineViewportStateCreateInfo * cvm_vk_get_default_viewport_state(void)
{
    return &cvm_vk_default_viewport_state;
}

VkPipelineRasterizationStateCreateInfo * cvm_vk_get_default_raster_state(bool culled)
{
    return culled ? &cvm_vk_default_culled_raster_state : &cvm_vk_default_unculled_raster_state;
}

VkPipelineMultisampleStateCreateInfo * cvm_vk_get_default_multisample_state(void)
{
    return &cvm_vk_default_multisample_state;
}

VkPipelineDepthStencilStateCreateInfo * cvm_vk_get_default_depth_stencil_state(void)
{
    return &cvm_vk_default_depth_stencil_state;
}

VkPipelineColorBlendAttachmentState cvm_vk_get_default_no_blend_state(void)
{
    return cvm_vk_default_no_blend_state;
}

VkPipelineColorBlendAttachmentState cvm_vk_get_default_alpha_blend_state(void)
{
    return cvm_vk_default_alpha_blend_state;
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


VkPipelineVertexInputStateCreateInfo * cvm_vk_get_mesh_vertex_input_state(uint16_t flags)
{
    if(flags & (CVM_MESH_VERTEX_NORMALS|CVM_MESH_TEXTURE_COORDS))
    {
        fprintf(stderr,"MESH NORMALS AND TEX-COORDS VERTEX STATE NYI\n");
        exit(-1);
    }

    return &cvm_vk_default_pos_vertex_input_state;
}

VkPipelineVertexInputStateCreateInfo * cvm_vk_get_empty_vertex_input_state(void)
{
    return &cvm_vk_default_empty_vertex_input_state;
}



VkSampler cvm_vk_get_fetch_sampler(void)
{
    return cvm_vk_fetch_sampler;
}



VkPipelineShaderStageCreateInfo cvm_vk_get_default_fullscreen_vertex_stage(void)
{
    return cvm_vk_default_fullscreen_vertex_stage;
}
