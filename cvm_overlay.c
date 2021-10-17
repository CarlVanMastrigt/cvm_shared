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

#include "cvm_shared.h"






static cvm_vk_module_data overlay_module_data;

static VkRenderPass overlay_render_pass;

///if uniform paradigm is based on max per frame being respected then the descrptor sets can be pre baked with offsets per swapchain image
/// separate uniform buffer for fixed data (colour, screen size &c.) ?
static VkDescriptorSetLayout overlay_descriptor_set_layout;///rename this set to uniform
static VkDescriptorPool overlay_descriptor_pool;///make separate pool for swapchain dependent and swapchain independent resources
static VkDescriptorSet * overlay_descriptor_sets;

static VkDescriptorPool overlay_consistent_descriptor_pool;
static VkDescriptorSetLayout overlay_consistent_descriptor_set_layout;///rename this set to image
static VkDescriptorSet overlay_consistent_descriptor_set;

static VkPipelineLayout overlay_pipeline_layout;///relevant descriptors, share as much as possible
static VkPipeline overlay_pipeline;
static VkPipelineShaderStageCreateInfo overlay_vertex_stage;
static VkPipelineShaderStageCreateInfo overlay_fragment_stage;

static VkFramebuffer * overlay_framebuffers;

static VkDeviceMemory overlay_image_memory;

static VkImage overlay_transparency_image;
static VkImageView overlay_transparency_image_view;///views of single array slice from image
static cvm_vk_image_atlas overlay_transparency_image_atlas;

static VkImage overlay_colour_image;
static VkImageView overlay_colour_image_view;///views of single array slice from image
static cvm_vk_image_atlas overlay_colour_image_atlas;

/// actually we want a way to handle uploads from here, and as dynamics are the same as instance data its probably worth having a dedicated buffer for them and uniforms AND upload/transfers
static cvm_vk_ring_buffer overlay_uniform_buffer;
static uint32_t * overlay_uniform_buffer_acquisitions;

static cvm_vk_ring_buffer overlay_staging_buffer;
static uint32_t * overlay_staging_buffer_acquisitions;

static VkDependencyInfoKHR overlay_uninitialised_to_transfer_dependencies;
static VkImageMemoryBarrier2KHR overlay_uninitialised_to_transfer_image_barriers[2];

static VkDependencyInfoKHR overlay_transfer_to_graphics_dependencies;
static VkImageMemoryBarrier2KHR overlay_transfer_to_graphics_image_barriers[2];

static VkDependencyInfoKHR overlay_graphics_to_transfer_dependencies;
static VkImageMemoryBarrier2KHR overlay_graphics_to_transfer_image_barriers[2];

//static bool overlay_graphics_transfer_ownership_changes;



///temporary test stuff
static cvm_overlay_font overlay_test_font;

static void create_overlay_descriptor_set_layouts(void)
{
    VkSampler fetch_sampler=cvm_vk_get_fetch_sampler();

    VkDescriptorSetLayoutCreateInfo layout_create_info;

    layout_create_info=(VkDescriptorSetLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .bindingCount=1,
        .pBindings=(VkDescriptorSetLayoutBinding[1])
        {
            {
                .binding=0,
                .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
                .descriptorCount=1,
                .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=NULL
            }
        }
    };

    cvm_vk_create_descriptor_set_layout(&overlay_descriptor_set_layout,&layout_create_info);



    ///consistent sets
    layout_create_info=(VkDescriptorSetLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .bindingCount=1,
        .pBindings=(VkDescriptorSetLayoutBinding[1])
        {
            {
                .binding=0,
                .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
                .descriptorCount=1,
                .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=&fetch_sampler /// also test w/ null & setting samplers directly
            }
        }
    };

    cvm_vk_create_descriptor_set_layout(&overlay_consistent_descriptor_set_layout,&layout_create_info);
}

static void create_overlay_consistent_descriptors(void)
{
    VkDescriptorPoolCreateInfo pool_create_info=(VkDescriptorPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,///by not specifying individual free must reset whole pool (which is fine)
        .maxSets=1,
        .poolSizeCount=1,
        .pPoolSizes=(VkDescriptorPoolSize[1])
        {
            {
                .type=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount=1
            }
        }
    };

    cvm_vk_create_descriptor_pool(&overlay_consistent_descriptor_pool,&pool_create_info);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info=(VkDescriptorSetAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=overlay_consistent_descriptor_pool,
        .descriptorSetCount=1,
        .pSetLayouts=&overlay_consistent_descriptor_set_layout
    };

    cvm_vk_allocate_descriptor_sets(&overlay_consistent_descriptor_set,&descriptor_set_allocate_info);

    VkWriteDescriptorSet writes[1]=
    {
        (VkWriteDescriptorSet)
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=overlay_consistent_descriptor_set,
            .dstBinding=0,
            .dstArrayElement=0,
            .descriptorCount=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo=(VkDescriptorImageInfo[1])
            {
                {
                    .sampler=VK_NULL_HANDLE,///using immutable sampler (for now)
                    .imageView=overlay_transparency_image_view,
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
            },
            .pBufferInfo=NULL,
            .pTexelBufferView=NULL
        }
    };

    cvm_vk_write_descriptor_sets(writes,1);
}

static void create_overlay_descriptor_sets(uint32_t swapchain_image_count)
{
    uint32_t i;
    VkDescriptorSetLayout set_layouts[swapchain_image_count];
    for(i=0;i<swapchain_image_count;i++)set_layouts[i]=overlay_descriptor_set_layout;

    ///separate pool for image descriptor sets? (so that they dont need to be reallocated/recreated upon swapchain changes)

    ///pool size dependent upon swapchain image count so must go here
    VkDescriptorPoolCreateInfo pool_create_info=(VkDescriptorPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,///by not specifying individual free must reset whole pool (which is fine)
        .maxSets=swapchain_image_count,
        .poolSizeCount=1,
        .pPoolSizes=(VkDescriptorPoolSize[1])
        {
            {
                .type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
                .descriptorCount=swapchain_image_count
            }
        }
    };

    cvm_vk_create_descriptor_pool(&overlay_descriptor_pool,&pool_create_info);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info=(VkDescriptorSetAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=overlay_descriptor_pool,
        .descriptorSetCount=swapchain_image_count,
        .pSetLayouts=set_layouts
    };

    overlay_descriptor_sets=malloc(swapchain_image_count*sizeof(VkDescriptorSet));

    cvm_vk_allocate_descriptor_sets(overlay_descriptor_sets,&descriptor_set_allocate_info);
}

static void update_overlay_uniforms(uint32_t swapchain_image,VkDeviceSize offset,uint32_t colour_count)
{
    ///investigate whether its necessary to update this every frame if its basically unchanging data and war hazards are a problem. may want to avoid just to stop validation from picking it up.
    VkWriteDescriptorSet writes[1]=
    {
        (VkWriteDescriptorSet)
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=overlay_descriptor_sets[swapchain_image],
            .dstBinding=0,
            .dstArrayElement=0,
            .descriptorCount=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here... then use either RGBA8 unorm colour or possibly RGBA16
            .pImageInfo=NULL,
            .pBufferInfo=(VkDescriptorBufferInfo[1])
            {
                {
                    .buffer=overlay_uniform_buffer.buffer,
                    .offset=offset,
                    .range=colour_count*4*sizeof(float)
                }
            },
            .pTexelBufferView=NULL
        }
    };

    cvm_vk_write_descriptor_sets(writes,1);
}

static void create_overlay_pipeline_layouts(void)
{
    VkPipelineLayoutCreateInfo pipeline_create_info=(VkPipelineLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=2,
        .pSetLayouts=(VkDescriptorSetLayout[2])
        {
            overlay_descriptor_set_layout,
            overlay_consistent_descriptor_set_layout
        },
        .pushConstantRangeCount=1,
        .pPushConstantRanges=(VkPushConstantRange[1])
        {
            {
                .stageFlags=VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset=0,
                .size=sizeof(float)*4
            }
        }
    };

    cvm_vk_create_pipeline_layout(&overlay_pipeline_layout,&pipeline_create_info);
}

static void create_overlay_render_pass(VkFormat swapchain_format,VkSampleCountFlagBits sample_count)
{
    VkRenderPassCreateInfo create_info=(VkRenderPassCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .attachmentCount=1,
        .pAttachments=(VkAttachmentDescription[1])
        {
            {
                .flags=0,
                .format=swapchain_format,
                .samples=VK_SAMPLE_COUNT_1_BIT,///sample_count not relevant for actual render target (swapchain image)
                .loadOp=VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,/// is first render pass (ergo the clear above) thus the VK_IMAGE_LAYOUT_UNDEFINED rather than VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                .finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR///is last render pass thus the VK_IMAGE_LAYOUT_PRESENT_SRC_KHR rather than VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
        },
        .subpassCount=1,
        .pSubpasses=(VkSubpassDescription[1])
        {
            {
                .flags=0,
                .pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount=0,
                .pInputAttachments=NULL,
                .colorAttachmentCount=1,
                .pColorAttachments=(VkAttachmentReference[1])
                {
                    {
                        .attachment=0,
                        .layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    }
                },
                .pResolveAttachments=NULL,
                .pDepthStencilAttachment=NULL,
                .preserveAttachmentCount=0,
                .pPreserveAttachments=NULL
            }
        },
        .dependencyCount=2,
        .pDependencies=(VkSubpassDependency[2])
        {
            {
                .srcSubpass=VK_SUBPASS_EXTERNAL,
                .dstSubpass=0,
                ///not sure on specific dependencies related to swapchain images being read/written by presentation engine
                .srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            },
            {
                .srcSubpass=0,
                .dstSubpass=VK_SUBPASS_EXTERNAL,
                ///not sure on specific dependencies related to swapchain images being read/written by presentation engine
                .srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            }
        }
    };

    cvm_vk_create_render_pass(&overlay_render_pass,&create_info);
}

static void create_overlay_pipelines(VkRect2D screen_rectangle)
{
    VkGraphicsPipelineCreateInfo create_info=(VkGraphicsPipelineCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .stageCount=2,
        .pStages=(VkPipelineShaderStageCreateInfo[2])
        {
            overlay_vertex_stage,
            overlay_fragment_stage
        },///could be null then provided by each actual pipeline type (for sets of pipelines only variant based on shader)
        .pVertexInputState=(VkPipelineVertexInputStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .vertexBindingDescriptionCount=0,
                .pVertexBindingDescriptions=NULL,
//                .vertexBindingDescriptionCount=1,
//                .pVertexBindingDescriptions= (VkVertexInputBindingDescription[1])
//                {
//                    {
//                        .binding=0,
//                        .stride=sizeof(overlay_render_data),
//                        .inputRate=VK_VERTEX_INPUT_RATE_VERTEX
//                    }
//                },
                .vertexAttributeDescriptionCount=0,
                .pVertexAttributeDescriptions=NULL
//                .vertexAttributeDescriptionCount=2,
//                .pVertexAttributeDescriptions=(VkVertexInputAttributeDescription[2])
//                {
//                    {
//                        .location=0,
//                        .binding=0,
//                        .format=VK_FORMAT_R32G32B32_SFLOAT,
//                        .offset=offsetof(overlay_render_data,pos)
//                    },
//                    {
//                        .location=1,
//                        .binding=0,
//                        .format=VK_FORMAT_R8G8B8A8_UNORM,
//                        .offset=offsetof(overlay_render_data,col)
//                    }
//                }
            }
        },
        .pInputAssemblyState=(VkPipelineInputAssemblyStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                .primitiveRestartEnable=VK_FALSE
            }
        },
        .pTessellationState=NULL,///not needed (yet)
        .pViewportState=(VkPipelineViewportStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .viewportCount=1,
                .pViewports=(VkViewport[1])
                {
                    {
                        .x=(float)screen_rectangle.offset.x,
                        .y=(float)screen_rectangle.offset.y,
                        .width=(float)screen_rectangle.extent.width,
                        .height=(float)screen_rectangle.extent.height,
                        .minDepth=0.0,
                        .maxDepth=1.0
                    }
                },
                .scissorCount=1,
                .pScissors= &screen_rectangle
            }
        },
        .pRasterizationState=(VkPipelineRasterizationStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .depthClampEnable=VK_FALSE,
                .rasterizerDiscardEnable=VK_FALSE,
                .polygonMode=VK_POLYGON_MODE_FILL,
                .cullMode=VK_CULL_MODE_NONE,
                .frontFace=VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable=VK_FALSE,
                .depthBiasConstantFactor=0.0,
                .depthBiasClamp=0.0,
                .depthBiasSlopeFactor=0.0,
                .lineWidth=1.0
            }
        },
        .pMultisampleState=(VkPipelineMultisampleStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .rasterizationSamples=VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable=VK_FALSE,
                .minSampleShading=1.0,///this can be changed, only requires rebuild of pipelines
                .pSampleMask=NULL,
                .alphaToCoverageEnable=VK_FALSE,
                .alphaToOneEnable=VK_FALSE
            }
        },
        .pDepthStencilState=NULL,
        .pColorBlendState=(VkPipelineColorBlendStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .logicOpEnable=VK_FALSE,
                .logicOp=VK_LOGIC_OP_COPY,
                .attachmentCount=1,///must equal colorAttachmentCount in subpass
                .pAttachments= (VkPipelineColorBlendAttachmentState[])
                {
                    {
                        .blendEnable=VK_TRUE,
                        .srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA,//VK_BLEND_FACTOR_ONE
                        .dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,//VK_BLEND_FACTOR_ZERO
                        .colorBlendOp=VK_BLEND_OP_ADD,
                        .srcAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,// VK_BLEND_FACTOR_ONE// both ZERO as alpha is ignored?
                        .dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,//VK_BLEND_FACTOR_ONE
                        .alphaBlendOp=VK_BLEND_OP_ADD,
                        .colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
                    }
                },
                .blendConstants={0.0,0.0,0.0,0.0}
            }
        },
        .pDynamicState=NULL,
        .layout=overlay_pipeline_layout,
        .renderPass=overlay_render_pass,
        .subpass=0,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };

    ///can pass above into multiple functions as parameter
    cvm_vk_create_graphics_pipeline(&overlay_pipeline,&create_info);
}

static void create_overlay_framebuffers(VkRect2D screen_rectangle,uint32_t swapchain_image_count)
{
    uint32_t i;
    VkImageView attachments[1];

    VkFramebufferCreateInfo create_info=(VkFramebufferCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .renderPass=overlay_render_pass,
        .attachmentCount=1,
        .pAttachments=attachments,
        .width=screen_rectangle.extent.width,
        .height=screen_rectangle.extent.height,
        .layers=1
    };

    overlay_framebuffers=malloc(swapchain_image_count*sizeof(VkFramebuffer));

    for(i=0;i<swapchain_image_count;i++)
    {
        attachments[0]=cvm_vk_get_swapchain_image_view(i);

        cvm_vk_create_framebuffer(overlay_framebuffers+i,&create_info);
    }
}

static void create_overlay_images(uint32_t t_w,uint32_t t_h,uint32_t c_w,uint32_t c_h)
{
    VkImageCreateInfo image_creation_info=(VkImageCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .imageType=VK_IMAGE_TYPE_2D,
        .format=VK_FORMAT_UNDEFINED,///set later
        .extent=(VkExtent3D)
        {
            .width=0,///set later
            .height=0,///set later
            .depth=1
        },
        .mipLevels=1,
        .arrayLayers=1,
        .samples=1,
        .tiling=VK_IMAGE_TILING_OPTIMAL,
        .usage=0,///set later, probably as VK_IMAGE_USAGE_SAMPLED_BIT, and VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT if potentially rendering to overlay (necessary in some circumstances)
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL,
        .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED///transition to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL and VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL as necessary (need to keep track of layout?)
    };


    image_creation_info.format=VK_FORMAT_R8_UNORM;
    image_creation_info.usage=VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_creation_info.extent.width=t_w;
    image_creation_info.extent.height=t_h;
    cvm_vk_create_image(&overlay_transparency_image,&image_creation_info);

    image_creation_info.format=VK_FORMAT_R8G8B8A8_UNORM;
    image_creation_info.usage=VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;///conditionally VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ???
    image_creation_info.extent.width=c_w;
    image_creation_info.extent.height=c_h;
    cvm_vk_create_image(&overlay_colour_image,&image_creation_info);

    VkImage images[2]={overlay_transparency_image,overlay_colour_image};
    cvm_vk_create_and_bind_memory_for_images(&overlay_image_memory,images,2);



    VkImageViewCreateInfo view_creation_info=(VkImageViewCreateInfo)
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
            .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel=0,
            .levelCount=1,
            .baseArrayLayer=0,
            .layerCount=1
        }
    };

    view_creation_info.format=VK_FORMAT_R8_UNORM;
    view_creation_info.image=overlay_transparency_image;
    cvm_vk_create_image_view(&overlay_transparency_image_view,&view_creation_info);

    view_creation_info.format=VK_FORMAT_R8G8B8A8_UNORM;
    view_creation_info.image=overlay_colour_image;
    cvm_vk_create_image_view(&overlay_colour_image_view,&view_creation_info);


    cvm_vk_create_image_atlas(&overlay_transparency_image_atlas,overlay_transparency_image_view,t_w,t_h,false);
    cvm_vk_create_image_atlas(&overlay_colour_image_atlas,overlay_colour_image_view,c_w,c_h,false);
}

static void create_overlay_barriers()
{
    //overlay_graphics_transfer_ownership_changes = cvm_vk_get_transfer_queue_family()==cvm_vk_get_graphics_queue_family();
    ///put back queue ownership transfers as soon as its possible to test and profile, also needs backing structure to handle it

    VkImageMemoryBarrier2KHR image_barrier=(VkImageMemoryBarrier2KHR)///initialise all shared values;
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
        .pNext=NULL,
        .subresourceRange=(VkImageSubresourceRange)
        {
            .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel=0,
            .levelCount=1,
            .baseArrayLayer=0,
            .layerCount=1
        }
    };

    ///uninitialised to transfer
    image_barrier.srcStageMask=0;
    image_barrier.srcAccessMask=0;
    image_barrier.dstStageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    image_barrier.dstAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
    image_barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
    image_barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;

    image_barrier.image=overlay_transparency_image;
    overlay_uninitialised_to_transfer_image_barriers[0]=image_barrier;

    image_barrier.image=overlay_colour_image;
    overlay_uninitialised_to_transfer_image_barriers[1]=image_barrier;

    overlay_uninitialised_to_transfer_dependencies=(VkDependencyInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        .pNext=NULL,
        .dependencyFlags=0,
        .memoryBarrierCount=0,
        .pMemoryBarriers=NULL,
        .bufferMemoryBarrierCount=0,
        .pBufferMemoryBarriers=NULL,
        .imageMemoryBarrierCount=2,
        .pImageMemoryBarriers=overlay_uninitialised_to_transfer_image_barriers
    };

    ///transfer to graphics
    image_barrier.srcStageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    image_barrier.srcAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
    image_barrier.dstStageMask=VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
    image_barrier.dstAccessMask=VK_ACCESS_2_SHADER_READ_BIT_KHR;
    image_barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,//cvm_vk_get_transfer_queue_family();
    image_barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,//cvm_vk_get_graphics_queue_family();

    image_barrier.image=overlay_transparency_image;
    overlay_transfer_to_graphics_image_barriers[0]=image_barrier;

    image_barrier.image=overlay_colour_image;
    overlay_transfer_to_graphics_image_barriers[1]=image_barrier;

    overlay_transfer_to_graphics_dependencies=(VkDependencyInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        .pNext=NULL,
        .dependencyFlags=0,
        .memoryBarrierCount=0,
        .pMemoryBarriers=NULL,
        .bufferMemoryBarrierCount=0,
        .pBufferMemoryBarriers=NULL,
        .imageMemoryBarrierCount=2,
        .pImageMemoryBarriers=overlay_transfer_to_graphics_image_barriers
    };

    ///graphics to transfer
    image_barrier.srcStageMask=VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
    image_barrier.srcAccessMask=VK_ACCESS_2_SHADER_READ_BIT_KHR;
    image_barrier.dstStageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    image_barrier.dstAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
    image_barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,//cvm_vk_get_graphics_queue_family();
    image_barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,//cvm_vk_get_transfer_queue_family();

    image_barrier.image=overlay_transparency_image;
    overlay_graphics_to_transfer_image_barriers[0]=image_barrier;

    image_barrier.image=overlay_colour_image;
    overlay_graphics_to_transfer_image_barriers[1]=image_barrier;

    overlay_graphics_to_transfer_dependencies=(VkDependencyInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        .pNext=NULL,
        .dependencyFlags=0,
        .memoryBarrierCount=0,
        .pMemoryBarriers=NULL,
        .bufferMemoryBarrierCount=0,
        .pBufferMemoryBarriers=NULL,
        .imageMemoryBarrierCount=2,
        .pImageMemoryBarriers=overlay_graphics_to_transfer_image_barriers
    };
}

void initialise_overlay_render_data(void)
{
    create_overlay_render_pass(cvm_vk_get_screen_format(),VK_SAMPLE_COUNT_1_BIT);

    create_overlay_descriptor_set_layouts();

    create_overlay_pipeline_layouts();

    cvm_vk_create_shader_stage_info(&overlay_vertex_stage,"cvm_shared/shaders/overlay_vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(&overlay_fragment_stage,"cvm_shared/shaders/overlay_frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    cvm_vk_create_module_data(&overlay_module_data,false);

    cvm_vk_create_ring_buffer(&overlay_uniform_buffer,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,0);
    cvm_vk_create_ring_buffer(&overlay_staging_buffer,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,0);

    create_overlay_images(1024,1024,1024,1024);
    create_overlay_barriers();///must go after creation of all resources these barriers act upon

    create_overlay_consistent_descriptors();

    cvm_overlay_create_font(&overlay_test_font,"cvm_shared/resources/cvm_font_1.ttf",32);
}

void terminate_overlay_render_data(void)
{
    cvm_overlay_destroy_font(&overlay_test_font);

    cvm_vk_destroy_image_atlas(&overlay_transparency_image_atlas);
    cvm_vk_destroy_image_atlas(&overlay_colour_image_atlas);

    cvm_vk_destroy_image_view(overlay_transparency_image_view);
    cvm_vk_destroy_image_view(overlay_colour_image_view);

    cvm_vk_destroy_image(overlay_transparency_image);
    cvm_vk_destroy_image(overlay_colour_image);

    cvm_vk_free_memory(overlay_image_memory);

    cvm_vk_destroy_render_pass(overlay_render_pass);

    cvm_vk_destroy_shader_stage_info(&overlay_vertex_stage);
    cvm_vk_destroy_shader_stage_info(&overlay_fragment_stage);

    cvm_vk_destroy_pipeline_layout(overlay_pipeline_layout);

    cvm_vk_destroy_descriptor_pool(overlay_consistent_descriptor_pool);

    cvm_vk_destroy_descriptor_set_layout(overlay_descriptor_set_layout);
    cvm_vk_destroy_descriptor_set_layout(overlay_consistent_descriptor_set_layout);

    cvm_vk_destroy_module_data(&overlay_module_data,false);

    cvm_vk_destroy_ring_buffer(&overlay_uniform_buffer);
    cvm_vk_destroy_ring_buffer(&overlay_staging_buffer);
}


void initialise_overlay_swapchain_dependencies(void)
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    VkRect2D screen_rectangle=cvm_vk_get_screen_rectangle();

    create_overlay_pipelines(screen_rectangle);

    create_overlay_framebuffers(screen_rectangle,swapchain_image_count);

    create_overlay_descriptor_sets(swapchain_image_count);

    cvm_vk_resize_module_graphics_data(&overlay_module_data,0);

    ///set these properly at some point
    uint32_t uniform_size=1024;
    uint32_t staging_size=4096;

    cvm_vk_update_ring_buffer(&overlay_uniform_buffer,swapchain_image_count*uniform_size);
    cvm_vk_update_ring_buffer(&overlay_staging_buffer,swapchain_image_count*staging_size);

    overlay_uniform_buffer_acquisitions=calloc(swapchain_image_count,sizeof(uint32_t));
    overlay_staging_buffer_acquisitions=calloc(swapchain_image_count,sizeof(uint32_t));
}

void terminate_overlay_swapchain_dependencies(void)
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    uint32_t i;

    cvm_vk_destroy_pipeline(overlay_pipeline);

    for(i=0;i<swapchain_image_count;i++)
    {
        cvm_vk_destroy_framebuffer(overlay_framebuffers[i]);

        cvm_vk_relinquish_ring_buffer_space(&overlay_uniform_buffer,overlay_uniform_buffer_acquisitions+i);
        cvm_vk_relinquish_ring_buffer_space(&overlay_staging_buffer,overlay_staging_buffer_acquisitions+i);
    }

    cvm_vk_destroy_descriptor_pool(overlay_descriptor_pool);

    free(overlay_descriptor_sets);
    free(overlay_framebuffers);
    free(overlay_uniform_buffer_acquisitions);
    free(overlay_staging_buffer_acquisitions);
}
/*
static uint32_t calculate_code_point(uint8_t * text)
{
    uint32_t id;

    if(*text & 0x80)
    {
        id=0;
        while(*text & (0x80 >> incr))
        {
            ///error check, dont put in release version
            if((text[incr]&0xC0) != 0x80)
            {
                fprintf(stderr,"ATTEMPTING TO RENDER AS INVALID UTF-8 STRING (TOP BIT MISMATCH)\n");
                exit(-1);
            }
            id<<=6;
            printf("=====%x\n",text[incr]&0x3F);
            id|=text[incr]&0x3F;
            incr++;
        }
        id|=(*text&(1<<(7-incr))-1)   <<(6u*incr-6u);
        ///error check, dont put in release version
        if(incr==1)
        {
            fprintf(stderr,"ATTEMPTING TO RENDER AS INVALID UTF-8 STRING (INVALID LENGTH SPECIFIED)\n");
            exit(-1);
        }
    }
    else
    {
        id=*text;
    }

    return id;
}
*/


static void overlay_render_text(VkCommandBuffer upload_buffer,cvm_overlay_font * font,uint8_t * text,int x,int y)
{
    uint32_t id,index,incr,s,e,w;
    int s_x=x;
    uint8_t tmp;///extracted value to allow valid string of just this utf8 character (i.e. "current" text variable) to quickly be used by replacing following char w/ null terminator
    SDL_Surface * text_surface;
    VkDeviceSize upload_offset;
    uint8_t * staging;
    cvm_vk_image_atlas_tile * tile;

    while(*text)
    {
        incr=1;
        if(*text & 0x80)
        {
            id=0;
            while(*text & (0x80 >> incr))
            {
                ///error check, dont put in release version
                if((text[incr]&0xC0) != 0x80)
                {
                    fprintf(stderr,"ATTEMPTING TO RENDER AS INVALID UTF-8 STRING (TOP BIT MISMATCH)\n");
                    exit(-1);
                }
                id<<=8;
                id|=text[incr];
                incr++;
            }
            id|=*text<<(8*incr-8);
            ///error check, dont put in release version
            if(incr==1)
            {
                fprintf(stderr,"ATTEMPTING TO RENDER AS INVALID UTF-8 STRING (INVALID LENGTH SPECIFIED)\n");
                exit(-1);
            }
        }
        else id=*text;

        ///special handling for space and other non-glyph characters here???

        s=0;
        e=font->glyph_count;
        if(e)while(font->glyphs[(index=(s+e)>>1)].id!=id)
        {
            if(index==s)
            {
                ///does not exist, insert at index | index+1
                index+=(font->glyphs[index].id<id);

                if(font->glyph_count==font->glyph_space)font->glyphs=realloc(font->glyphs,sizeof(cvm_overlay_glyph)*(font->glyph_space*=2));

                memmove(font->glyphs+index+1,font->glyphs+index,(font->glyph_count-index)*sizeof(cvm_overlay_glyph));

                font->glyphs[index].tile=NULL;
                font->glyphs[index].id=id;

                font->glyph_count++;

                break;
            }
            else if(font->glyphs[index].id<id) s=index;
            else e=index;
        }
        else
        {
            index=0;

            font->glyphs[0].tile=NULL;
            font->glyphs[0].id=id;
            font->glyph_count++;
        }

        printf("%u\n",index);

        tile=font->glyphs[index].tile;
        if(tile==NULL)
        {
            tmp=text[incr];
            text[incr]='\0';
            if((text_surface=TTF_RenderUTF8_Shaded(font->font,(const char*)text,(SDL_Color){255,255,255,255},(SDL_Color){0,0,0,0})))
            {
                w=((text_surface->w-1)&~3)+4;
                staging = cvm_vk_get_ring_buffer_allocation(&overlay_staging_buffer,sizeof(uint8_t)*w*text_surface->h,&upload_offset);

                if(staging)
                {
                    tile=font->glyphs[index].tile=cvm_vk_acquire_image_atlas_tile(&overlay_transparency_image_atlas,w,text_surface->h);
                    if(tile)
                    {
                        memcpy(staging,text_surface->pixels,sizeof(uint8_t)*w*text_surface->h);

                        VkBufferImageCopy cpy=(VkBufferImageCopy)
                        {
                            .bufferOffset=upload_offset,
                            .bufferRowLength=w,
                            .bufferImageHeight=text_surface->h,
                            .imageSubresource=(VkImageSubresourceLayers)
                            {
                                .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                                .mipLevel=0,
                                .baseArrayLayer=0,
                                .layerCount=1
                            },
                            .imageOffset=(VkOffset3D)
                            {
                                .x=tile->x_pos<<CVM_VK_BASE_TILE_SIZE_FACTOR,
                                .y=tile->y_pos<<CVM_VK_BASE_TILE_SIZE_FACTOR,
                                .z=0
                            },
                            .imageExtent=(VkExtent3D)
                            {
                                .width=w,
                                .height=text_surface->h,
                                .depth=1,
                            }
                        };

                        vkCmdCopyBufferToImage(upload_buffer,overlay_staging_buffer.buffer,overlay_transparency_image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&cpy);
                    }
                }

                SDL_FreeSurface(text_surface);
            }
            text[incr]=tmp;
        }
//
//        if(tile)///need to check again as can return null once again;
//        {
//            ///
//        }

        text+=incr;
    }
}

cvm_vk_module_work_block * overlay_render_frame(int screen_w,int screen_h)
{
    cvm_vk_module_work_block * work_block;
    uint32_t swapchain_image_index;

    static bool first=true;



    ///perhaps this should return previous image index as well, such that that value can be used in cleanup (e.g. relinquishing ring buffer space)
    work_block = cvm_vk_begin_module_work_block(&overlay_module_data,0,&swapchain_image_index);



    /// definitely want to clean up targets &c.
    /// "resultant" (swapchain) attachment goes at end
    /// actually loop framebuffers, look into benefit
    /// define state that varies between attachments in base, referenceable struct that can be shared across functions and invocations (inc. clear values?)
    /// dont bother storing framebuffer images/data in enumed array, store individually

    /// could have a single image atlas and use different aspects of part of it (r,g,b,a via image views) for different alpha/transparent image_atlases
    /// this would be fairly inefficient to retrieve though..., probably better not to.


    if(swapchain_image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        cvm_vk_begin_ring_buffer(&overlay_staging_buffer);///build barriers into begin/end paradigm maybe???
        #warning need to use the appropriate queue for all following transfer ops
        ///     ^ possibly detect when they're different and use gfx directly to avoid double submission?
        ///         ^ have cvm_vk_begin_module_work_block return same command buffer?
        ///             ^could have meta level function that inserts both barriers to appropriate queues simultaneously as necessary??? (VK_NULL_HANDLE when unit-transfer?)
        ///   module handles semaphore when necessary, perhaps it can also handle some aspect(s) of barriers?
        if(first)
        {
            CVM_TMP_vkCmdPipelineBarrier2KHR(work_block->graphics_work,&overlay_uninitialised_to_transfer_dependencies);
            first=false;


            VkDeviceSize upload_offset;

            //TTF_Font * this_font = TTF_OpenFont("cvm_shared/resources/cvm_font_1.ttf",16);
//            TTF_Font * this_font = TTF_OpenFont("cvm_shared/resources/FreeMono.ttf",24);
//            TTF_Font * this_font = TTF_OpenFont("cvm_shared/resources/HanaMinA.ttf",32);


            SDL_Surface *text_surface;
//            char str[]={0xC3,0xBE,0x00,0x00,0x00};
            //char str[]={0xFF66,0x00,0x00,0x00};
            SDL_Color fg={255,255,255,255};
            SDL_Color bg={0,0,0,0};
            //if((text_surface=TTF_RenderUNICODE_Shaded(this_font,str,fg,bg)))//
            char * str=strdup("Hello\tWorld! ばか");
            if((text_surface=TTF_RenderUTF8_Shaded(overlay_test_font.font,str,fg,bg)))//
            {

                overlay_render_text(work_block->graphics_work,&overlay_test_font,(uint8_t*)str,0,0);

                uint8_t * staging = cvm_vk_get_ring_buffer_allocation(&overlay_staging_buffer,sizeof(uint8_t)*text_surface->w*text_surface->h,&upload_offset);

                //SDL_LockSurface(text_surface);
                int i,j,w;
                w=((text_surface->w-1)&~3)+4;
                //w=text_surface->w;
                for(i=0;i<text_surface->h;i++)
                {
                    for(j=0;j<text_surface->w;j++)
                    {
                        //TTF_GetFontKerningSizeGlyphs();
                        staging[i*text_surface->w+j]=((uint8_t*)text_surface->pixels)[i*w+j];
                    }
                }
//                uint8_t * ss="ばか";
//                for(i=0;ss[i];i++)printf("%x\n",(ss[i]));
                //memcpy(staging,text_surface->pixels,sizeof(uint8_t)*text_surface->w*text_surface->h);
                //SDL_UnlockSurface(text_surface);
                //memset(staging,255,sizeof(uint8_t)*text_surface->w*text_surface->h);

                VkBufferImageCopy cpy=(VkBufferImageCopy)
                {
                    .bufferOffset=upload_offset,
                    .bufferRowLength=text_surface->w,
                    .bufferImageHeight=text_surface->h,
                    .imageSubresource=(VkImageSubresourceLayers)
                    {
                        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel=0,
                        .baseArrayLayer=0,
                        .layerCount=1
                    },
                    .imageOffset=(VkOffset3D)
                    {
                        .x=350,
                        .y=200,
                        .z=0
                    },
                    .imageExtent=(VkExtent3D)
                    {
                        .width=text_surface->w,
                        .height=text_surface->h,
                        .depth=1,
                    }
                };
                vkCmdCopyBufferToImage(work_block->graphics_work,overlay_staging_buffer.buffer,overlay_transparency_image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&cpy);

                SDL_FreeSurface(text_surface);
            }
            free(str);
        }
        else
        {
            CVM_TMP_vkCmdPipelineBarrier2KHR(work_block->graphics_work,&overlay_graphics_to_transfer_dependencies);
        }

        CVM_TMP_vkCmdPipelineBarrier2KHR(work_block->graphics_work,&overlay_transfer_to_graphics_dependencies);



        cvm_vk_begin_ring_buffer(&overlay_uniform_buffer);

        VkDeviceSize uniforms_offset;
        float * uniforms = cvm_vk_get_ring_buffer_allocation(&overlay_uniform_buffer,sizeof(float)*8,&uniforms_offset);
        //printf("C %u\n",atomic_load(&overlay_uniform_buffer.space_remaining));
        if(uniforms==NULL)puts("FAILED");

        uniforms[0]=0.7+0.3*cos(SDL_GetTicks()*0.005);
        uniforms[1]=0.7+0.3*cos(SDL_GetTicks()*0.007);
        uniforms[2]=0.7+0.3*cos(SDL_GetTicks()*0.011);
        uniforms[3]=1.0;
        uniforms[4]=0;
        uniforms[5]=0;
        uniforms[6]=0;
        uniforms[7]=1.0;

        update_overlay_uniforms(swapchain_image_index,uniforms_offset,2);///should really build buffer acquisition into update uniforms function

        float screen_dimensions[4]={1.0/((float)screen_w),1.0/((float)screen_h),(float)screen_w,(float)screen_h};

        vkCmdPushConstants(work_block->graphics_work,overlay_pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,4*sizeof(float),screen_dimensions);

        vkCmdBindDescriptorSets(work_block->graphics_work,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline_layout,0,1,overlay_descriptor_sets+swapchain_image_index,0,NULL);

        vkCmdBindDescriptorSets(work_block->graphics_work,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline_layout,1,1,&overlay_consistent_descriptor_set,0,NULL);

        VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext=NULL,
            .renderPass=overlay_render_pass,
            .framebuffer=overlay_framebuffers[swapchain_image_index],
            .renderArea=cvm_vk_get_screen_rectangle(),
            .clearValueCount=0,
            .pClearValues=NULL
        };

        vkCmdBeginRenderPass(work_block->graphics_work,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

        vkCmdBindPipeline(work_block->graphics_work,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline);

        vkCmdDraw(work_block->graphics_work,4,1,0,0);

        vkCmdEndRenderPass(work_block->graphics_work);///================


        overlay_uniform_buffer_acquisitions[swapchain_image_index]=cvm_vk_end_ring_buffer(&overlay_uniform_buffer);
        overlay_staging_buffer_acquisitions[swapchain_image_index]=cvm_vk_end_ring_buffer(&overlay_staging_buffer);
    }

    return cvm_vk_end_module_work_block(&overlay_module_data);
}

void overlay_frame_cleanup(uint32_t swapchain_image_index)
{
    if(swapchain_image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        cvm_vk_relinquish_ring_buffer_space(&overlay_uniform_buffer,overlay_uniform_buffer_acquisitions+swapchain_image_index);
        cvm_vk_relinquish_ring_buffer_space(&overlay_staging_buffer,overlay_staging_buffer_acquisitions+swapchain_image_index);
    }
}


void cvm_overlay_create_font(cvm_overlay_font * font,char * filename,int pixel_size)
{
    font->font = TTF_OpenFont(filename,pixel_size);

    if(!font->font)
    {
        fprintf(stderr,"COULD NOT LOAD FONT FILE %s\n",filename);
        exit(-1);
    }

    font->glyphs=malloc(4*sizeof(cvm_overlay_glyph));
    font->glyph_space=4;
    font->glyph_count=0;
}

void cvm_overlay_destroy_font(cvm_overlay_font * font)
{
    uint32_t i;
    TTF_CloseFont(font->font);

    for(i=0;i<font->glyph_count;i++)
    {
        cvm_vk_relinquish_image_atlas_tile(&overlay_transparency_image_atlas,font->glyphs[i].tile);
    }

    free(font->glyphs);
}

/*
int test(void)
{
    SDL_Color color={0,0,0};
    SDL_Surface *text_surface;
    if(!(text_surface=TTF_RenderUTF8_Solid(font,"Hello World!",color))) {
        //handle error here, perhaps print TTF_GetError at least
    } else {
        SDL_BlitSurface(text_surface,NULL,screen,NULL);
        //perhaps we can reuse it, but I assume not for simplicity.
        SDL_FreeSurface(text_surface);
}
}
*/

/*
void overlay_test(void)
{
    struct timespec ts1,ts2;
    uint64_t t_total;
    char str[10];
    int t,w;
    str[2]=' ';
    str[3]='i';
    str[4]='j';
    str[5]=' ';
    str[6]='0';
    str[7]='1';
    str[8]='0';
    str[9]='\0';

    TTF_Font * this_font = TTF_OpenFont("cvm_shared/resources/CVM_font_1.ttf",16);
    t=0;
    clock_gettime(CLOCK_REALTIME,&ts1);
//    for(str[0]='!';str[0]<='~';str[0]++)
//    {
//        for(str[1]='!';str[1]<='~';str[1]++)
//        {
//            TTF_SizeUTF8(this_font,str,&w,NULL);
//            t+=w;
//        }
//    }
TTF_SizeUTF8(this_font,"ab",&t,NULL);
puts("a\tb");

    clock_gettime(CLOCK_REALTIME,&ts2);

    t_total=(ts2.tv_sec-ts1.tv_sec)*1000000000 + ts2.tv_nsec-ts1.tv_nsec;

    printf("overlay_test time: %lu %u\n",t_total/1000,t);

    TTF_CloseFont(this_font);
}

*/






















static GLuint overlay_buffer;
static size_t overlay_buffer_size;

static GLuint overlay_shader;
static GLuint overlay_vbo;
static GLuint overlay_vao;


void initialise_overlay_data(overlay_data * od)
{
    od->data=malloc(sizeof(overlay_render_data));

    od->count=0;
    od->space=1;
}

void delete_overlay_data(overlay_data * od)
{
    free(od->data);
}

void ensure_overlay_data_space(overlay_data * od)
{
    if(od->count==od->space)
    {
        od->data=realloc(od->data,sizeof(overlay_render_data)*(od->space*=2));
    }
}

void load_font_to_overlay(gl_functions * glf,overlay_theme * theme,char * ttf_file,int size)
{
    int i,j,k,w;
    SDL_Surface *surf;

    cvm_font * font= &theme->font;

    TTF_Font * this_font = TTF_OpenFont(ttf_file,size);

    font->line_spacing=TTF_FontAscent(this_font)-TTF_FontDescent(this_font)+4;
    font->font_size=size;



    TTF_GlyphMetrics(this_font,' ',NULL,NULL,NULL,NULL,&font->space_width);


    font->max_glyph_width=font->space_width;



    font->glyphs[0].offset=0;

    int minx,maxx;
    for(i=0;i<94;i++)
    {
        TTF_GlyphMetrics(this_font,'!'+i,&minx,&maxx,NULL,NULL,&font->glyphs[i].advance );

        font->glyphs[i].width=maxx-minx;
        font->glyphs[i].bearingX=minx;

        if(i)font->glyphs[i].offset=font->glyphs[i-1].offset+font->glyphs[i-1].width;

        if(font->max_glyph_width<font->glyphs[i].advance)
        {
            font->max_glyph_width=font->glyphs[i].advance;
            //printf("max_char: %c\n",'!'+i);
        }
    }


    int total_width=((font->glyphs[93].offset+font->glyphs[93].width)/4 + 1)*4;

    uint8_t * pixels=calloc((font->line_spacing)*total_width,sizeof(char));

    char c[3];
    c[1]=0;
    SDL_Color col;
    col.a=0xFF;


    for(i=0;i<94;i++)
    {
        c[0]='!'+i;

        surf = TTF_RenderText_Blended(this_font, c , col);
        minx=(font->glyphs[i].bearingX>0)?font->glyphs[i].bearingX:0;
        w=surf->w;


        for(j=0;j<surf->h;j++)
        {
            for(k=0;k<font->glyphs[i].width;k++)
            {
                pixels[j*total_width+k+font->glyphs[i].offset]=((uint8_t*)surf->pixels)[(j*w+k+minx)*4+3];
            }
        }

        SDL_FreeSurface(surf);
    }


    c[2]=0;

    for(i=0;i<94;i++)
    {
        c[0]=i+'!';
        for(j=0;j<94;j++)
        {
            c[1]=j+'!';

            int xx,yy;

            TTF_SizeText(this_font,c,&xx,&yy);


            minx=(font->glyphs[i].bearingX<0)?-font->glyphs[i].bearingX:0;

            //font->glyphs[i].kerning[j]=TTF_GetFontKerningSize(font->font, i+'!', j+'!');
            font->glyphs[i].kerning[j]=xx-minx-font->glyphs[i].advance-font->glyphs[j].advance;

//            if(font->glyphs[i].kerning[j]!=0)
//            {
//                printf("%d %c %c\n",font->glyphs[i].kerning[j],i+'!', j+'!');
//            }
        }
    }


    int miny,maxy;
    miny=-1;
    maxy=0;
    bool line_active;
    for(i=0;i<font->line_spacing;i++)
    {
        line_active=false;
        for(j=0;j<total_width;j++)
        {
            if(pixels[i*total_width+j])
            {
                line_active=true;
            }
        }

        if(line_active)
        {
            if(miny==-1)miny=i;
            maxy=i;
        }
    }

//    for(i=0;i<total_width;i++)
//    {
//        pixels[miny*total_width+i]=255;
//        pixels[maxy*total_width+i]=255;
//    }


    font->font_height=maxy-miny+1;
    font->line_spacing-=4;




    glf->glBindTexture(GL_TEXTURE_2D,font->text_image);
    glf->glTexImage2D(GL_TEXTURE_2D,0,GL_RED,total_width,font->font_height,0,GL_RED,GL_UNSIGNED_BYTE,pixels+miny*total_width);
    glf->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glf->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glf->glBindTexture(GL_TEXTURE_2D,0);

    free(pixels);

    TTF_CloseFont(this_font);
}


static bool find_sprite_offset_by_name(overlay_theme * theme,char * name,uint32_t * offset)
{
    if((theme->sprite_count==0)||(name==NULL))
    {
        *offset=0;
        return false;
    }
    ///0=none
    uint32_t start,end,mid;
    int cmp;

    start=0;
    end=theme->sprite_count-1;

    cmp=strcmp(name,theme->sprite_data[start].name);
    if(cmp==0)
    {
        *offset=start;
        return true;
    }
    else if(cmp<0)
    {
        *offset=start;
        return false;
    }

    cmp=strcmp(name,theme->sprite_data[end].name);
    if(cmp==0)
    {
        *offset=end;
        return true;
    }
    else if(cmp>0)
    {
        *offset=end+1;
        return false;
    }

    while((end-start)>1)
    {
        mid=(end+start)/2;

        cmp=strcmp(name,theme->sprite_data[mid].name);
        if(cmp==0)
        {
            *offset=mid;
            return true;
        }
        else if(cmp>0) start=mid;
        else end=mid;
    }

    *offset=end;
    return false;
}

static bool get_sized_sprite_location(sprite_heirarchy_level * sprite_heirarchy,uint32_t tier,int * x,int * y)
{
    if(tier>=NUM_OVERLAY_SPRITE_LEVELS)return false;


    if(sprite_heirarchy[tier].remaining)
    {
        sprite_heirarchy[tier].remaining--;
        *x=sprite_heirarchy[tier].x_positions[sprite_heirarchy[tier].remaining];
        *y=sprite_heirarchy[tier].y_positions[sprite_heirarchy[tier].remaining];
    }
    else
    {
        if(!get_sized_sprite_location(sprite_heirarchy,tier+1,x,y))return false;

        sprite_heirarchy[tier].remaining=3;

        sprite_heirarchy[tier].x_positions[0]= *x + (BASE_OVERLAY_SPRITE_SIZE<<tier);
        sprite_heirarchy[tier].y_positions[0]= *y + (BASE_OVERLAY_SPRITE_SIZE<<tier);

        sprite_heirarchy[tier].x_positions[1]= *x;
        sprite_heirarchy[tier].y_positions[1]= *y + (BASE_OVERLAY_SPRITE_SIZE<<tier);

        sprite_heirarchy[tier].x_positions[2]= *x + (BASE_OVERLAY_SPRITE_SIZE<<tier);
        sprite_heirarchy[tier].y_positions[2]= *y;
    }

    return true;
}

overlay_sprite_data create_overlay_sprite(overlay_theme * theme,char * name,int max_w,int max_h,overlay_sprite_type type)
{
    overlay_sprite_data osd;

    uint32_t offset;
    int box_size;
    bool room;

    box_size=BASE_OVERLAY_SPRITE_SIZE;
    osd.name=NULL;
    osd.size_factor=0;
    osd.x_pos=osd.y_pos=0;
    osd.type=type;


    if(find_sprite_offset_by_name(theme,name,&offset))
    {
        puts("ERROR: OVERLAY SPRITE NAME ALREADY TAKEN");

        return osd;
    }
    if(name==NULL)
    {
        puts("ERROR: OVERLAY SPRITE NAME ALREADY TAKEN (NOT PERMITTED)");

        return osd;
    }

    while((max_w>box_size)||(max_h>box_size))
    {
        box_size*=2;
        osd.size_factor++;
    }

    room=false;

    if(type==OVERLAY_SHADED_SPRITE) room=get_sized_sprite_location(theme->shaded_sprite_levels,osd.size_factor,&osd.x_pos,&osd.y_pos);
    else if(type==OVERLAY_COLOURED_SPRITE) room=get_sized_sprite_location(theme->coloured_sprite_levels,osd.size_factor,&osd.x_pos,&osd.y_pos);
    else puts("SPRITE TYPE INCORRECT");

    if(room)
    {
        osd.name=strdup(name);

        if(theme->sprite_count==theme->sprite_space)
        {
            theme->sprite_data=realloc(theme->sprite_data,sizeof(overlay_sprite_data)*(theme->sprite_space*=2));
        }

        memmove(theme->sprite_data+offset+1,theme->sprite_data+offset,sizeof(overlay_sprite_data)*(theme->sprite_count-offset));
        theme->sprite_count++;

        theme->sprite_data[offset]=osd;
    }
    else
    {
        puts("ERROR: INSUFFICIENT REMAINING SPACE FOR OVERLAY SPRITE");
    }

    return osd;
}

overlay_sprite_data get_overlay_sprite_data(overlay_theme * theme,char * name)
{
    overlay_sprite_data osd;
    uint32_t offset;
    if(find_sprite_offset_by_name(theme,name,&offset))
    {
        return theme->sprite_data[offset];
    }
    else
    {

        osd.x_pos=osd.y_pos=0;
        osd.size_factor=0;
        osd.type=0;
        osd.name=NULL;

        return osd;
    }
}

///if elem not found return 0,0 which should be first thing added (error icon)


overlay_theme create_overlay_theme(gl_functions * glf,uint32_t shaded_texture_size,uint32_t coloured_texture_size)
{
    int i;
    overlay_theme ot;


    glf->glGenTextures(1,&ot.font.text_image);

    ot.sprite_data=malloc(sizeof(overlay_sprite_data));
    ot.sprite_count=0;///1 reserved
    ot.sprite_space=1;

    ot.shaded_texture_updated=true;
    ot.shaded_texture_size=512;
    ot.shaded_texture_tier=5;
    ot.shaded_texture_data=calloc(ot.shaded_texture_size*ot.shaded_texture_size,sizeof(uint8_t));

    ot.coloured_texture_updated=true;
    ot.coloured_texture_size=512;
    ot.coloured_texture_tier=5;
    ot.coloured_texture_data=calloc(4*ot.coloured_texture_size*ot.coloured_texture_size,sizeof(uint8_t));///RGBA

    glf->glGenTextures(1,&ot.shaded_texture);
    glf->glBindTexture(GL_TEXTURE_2D,ot.shaded_texture);
    glf->glTexImage2D(GL_TEXTURE_2D,0,GL_R8,ot.shaded_texture_size,ot.shaded_texture_size,0,GL_RED,GL_UNSIGNED_BYTE,NULL);
    glf->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glf->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glf->glBindTexture(GL_TEXTURE_2D,0);

    glf->glGenTextures(1,&ot.coloured_texture);
    glf->glBindTexture(GL_TEXTURE_2D,ot.coloured_texture);
    glf->glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,ot.coloured_texture_size,ot.coloured_texture_size,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glf->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glf->glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glf->glBindTexture(GL_TEXTURE_2D,0);

    for(i=0;i<NUM_OVERLAY_SPRITE_LEVELS;i++)
    {
        ot.shaded_sprite_levels[i].remaining=0;
        ot.coloured_sprite_levels[i].remaining=0;
    }

    ot.shaded_sprite_levels[ot.shaded_texture_tier].remaining=1;
    ot.shaded_sprite_levels[ot.shaded_texture_tier].x_positions[0]=0;
    ot.shaded_sprite_levels[ot.shaded_texture_tier].y_positions[0]=0;

    ot.coloured_sprite_levels[ot.coloured_texture_tier].remaining=1;
    ot.coloured_sprite_levels[ot.coloured_texture_tier].x_positions[0]=0;
    ot.coloured_sprite_levels[ot.coloured_texture_tier].y_positions[0]=0;

    return ot;
}

void update_theme_textures_on_videocard(gl_functions * glf,overlay_theme * theme)
{
    /// possibly allow writing of textures directly to avoid need for this step (possibly via opengl texture as well) e.g. for map widget in games, (wouldn't allow changes of texture size though)

    if(theme->shaded_texture_updated)
    {
        glf->glBindTexture(GL_TEXTURE_2D,theme->shaded_texture);
        glf->glTexSubImage2D(GL_TEXTURE_2D,0,0,0,theme->shaded_texture_size,theme->shaded_texture_size,GL_RED,GL_UNSIGNED_BYTE,theme->shaded_texture_data);
        glf->glBindTexture(GL_TEXTURE_2D,0);

        theme->shaded_texture_updated=false;
    }
    if(theme->coloured_texture_updated)
    {
        glf->glBindTexture(GL_TEXTURE_2D,theme->coloured_texture);
        glf->glTexSubImage2D(GL_TEXTURE_2D,0,0,0,theme->coloured_texture_size,theme->coloured_texture_size,GL_RGBA,GL_UNSIGNED_BYTE,theme->coloured_texture_data);
        glf->glBindTexture(GL_TEXTURE_2D,0);

        theme->coloured_texture_updated=false;
    }
}

void delete_overlay_theme(overlay_theme * theme)
{
    int i;
    free(theme->shaded_texture_data);
    free(theme->coloured_texture_data);

    for(i=0;i<theme->sprite_count;i++)
    {
        free(theme->sprite_data[i].name);
    }
    free(theme->sprite_data);
}



void initialise_overlay(gl_functions * glf)
{
    char defines[1024];
    sprintf(defines,"#define NUM_OVERLAY_COLOURS %d \n\n",NUM_OVERLAY_COLOURS);


    overlay_shader=initialise_shader_program(glf,defines,"cvm_shared/shaders/overlay_vert.glsl",NULL,"cvm_shared/shaders/overlay_frag.glsl" );


    float v[8]={0.0f,0.0f,0.0f,1.0f,1.0f,1.0f,1.0f,0.0f};


    glf->glGenVertexArrays(1,&overlay_vao);
    glf->glBindVertexArray(overlay_vao);

    glf->glGenBuffers(1,&overlay_vbo);
    glf->glBindBuffer(GL_ARRAY_BUFFER,overlay_vbo);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,v,GL_STATIC_DRAW);

    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);


    glf->glGenBuffers(1,&overlay_buffer);
    glf->glBindBuffer(GL_ARRAY_BUFFER,overlay_buffer);
    overlay_buffer_size=0;

    set_instanced_attribute_i(glf,1,4,sizeof(overlay_render_data),(const GLvoid*)offsetof(overlay_render_data,data1),GL_SHORT);
    set_instanced_attribute_i(glf,2,4,sizeof(overlay_render_data),(const GLvoid*)offsetof(overlay_render_data,data2),GL_SHORT);
    set_instanced_attribute_i(glf,3,4,sizeof(overlay_render_data),(const GLvoid*)offsetof(overlay_render_data,data3),GL_SHORT);

    glf->glBindVertexArray(0);
}

void render_overlay(gl_functions * glf,overlay_data * od,overlay_theme * theme,int screen_w,int screen_h,vec4f * overlay_colours)
{
    glf->glBindFramebuffer(GL_FRAMEBUFFER,0);
    glf->glDrawBuffer(GL_BACK);

    glf->glViewport(0,0,screen_w,screen_h);

    glf->glEnable(GL_BLEND);
    glf->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(od->count)
    {
        glf->glBindBuffer(GL_ARRAY_BUFFER,overlay_buffer);
        if(sizeof(overlay_render_data)*od->count > overlay_buffer_size)
        {
            overlay_buffer_size=sizeof(overlay_render_data)*od->count;
            glf->glBufferData(GL_ARRAY_BUFFER,overlay_buffer_size,od->data,GL_STREAM_DRAW);
        }
        else
        {
            glf->glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(overlay_render_data)*od->count,od->data);
        }
        glf->glBindBuffer(GL_ARRAY_BUFFER,0);


        glf->glUseProgram(overlay_shader);

        glf->glUniform2f(glf->glGetUniformLocation(overlay_shader,"inv_window_size"),1.0/((float)screen_w),1.0/((float)screen_h));
        glf->glUniform1i(glf->glGetUniformLocation(overlay_shader,"window_height"),screen_h);

        ///make overlay colours use GLfloat instead

        overlay_colours[OVERLAY_MAIN_HIGHLIGHTED_COLOUR]=vec4f_blend(overlay_colours[OVERLAY_MAIN_COLOUR],overlay_colours[OVERLAY_HIGHLIGHTING_COLOUR]);///apply highlighting colour
        glf->glUniform4fv(glf->glGetUniformLocation(overlay_shader,"colours"),NUM_OVERLAY_COLOURS-1,(GLfloat*)(overlay_colours+1));


        glf->glActiveTexture(GL_TEXTURE0);
        glf->glBindTexture(GL_TEXTURE_2D,theme->font.text_image);

        glf->glActiveTexture(GL_TEXTURE1);
        glf->glBindTexture(GL_TEXTURE_2D,theme->shaded_texture);

        glf->glActiveTexture(GL_TEXTURE2);
        glf->glBindTexture(GL_TEXTURE_2D,theme->coloured_texture);

        glf->glBindVertexArray(overlay_vao);
        glf->glDrawArraysInstanced(GL_TRIANGLE_FAN,0 ,4 ,od->count);
        glf->glBindVertexArray(0);
    }

    glf->glDisable(GL_BLEND);

    od->count=0;
}






void render_rectangle(overlay_data * od,rectangle r,rectangle bound,overlay_colour colour_index)
{
    if((get_rectangle_overlap(&r,bound))&&(r.w>0)&&(r.h>0)&&(colour_index))
    {
        ensure_overlay_data_space(od);

        od->data[od->count++]=(overlay_render_data){.data1={r.x,r.y,r.w,r.h},.data2={OVERLAY_ELEMENT_RECTANGLE,colour_index,0,0},.data3={0,0,0,0}};
    }
}

void render_border(overlay_data * od,rectangle r,rectangle bound,rectangle discard,overlay_colour colour_index)
{
    if((get_rectangle_overlap(&r,bound))&&(colour_index))
    {
        ensure_overlay_data_space(od);

        od->data[od->count++]=(overlay_render_data){.data1={r.x,r.y,r.w,r.h},.data2={OVERLAY_ELEMENT_BORDER,colour_index,0,0},.data3={discard.x,discard.y,discard.x+discard.w,discard.y+discard.h}};
    }
}

void render_character(overlay_data * od,rectangle r,rectangle bound,int char_offset,int font_index,overlay_colour colour_index)
{
    int x=r.x;
    int y=r.y;

    if((get_rectangle_overlap(&r,bound))&&(colour_index))
    {
        ensure_overlay_data_space(od);

        od->data[od->count++]=(overlay_render_data){.data1={r.x,r.y,r.w,r.h},.data2={OVERLAY_ELEMENT_CHARACTER,colour_index,char_offset,font_index},.data3={x,y,0,0}};
    }
}



void render_overlay_shade(overlay_data * od,rectangle r,rectangle bound,int source_x,int source_y,overlay_colour colour)
{
    int x=r.x;
    int y=r.y;

	if((get_rectangle_overlap(&r,bound))&&(colour))
    {
        ensure_overlay_data_space(od);

        od->data[od->count++]=(overlay_render_data){.data1={r.x,r.y,r.w,r.h},.data2={OVERLAY_ELEMENT_SHADED,colour,source_x,source_y},.data3={x,y,0,0}};
    }
}

void render_shaded_sprite(overlay_data * od,rectangle r,rectangle bound,int source_x,int source_y,overlay_colour colour)
{
    int x=r.x;
    int y=r.y;

	if((get_rectangle_overlap(&r,bound))&&(colour))
    {
        ensure_overlay_data_space(od);

        od->data[od->count++]=(overlay_render_data){.data1={r.x,r.y,r.w,r.h},.data2={OVERLAY_ELEMENT_SHADED,colour,source_x,source_y},.data3={x,y,0,0}};
    }
}

void render_coloured_sprite(overlay_data * od,rectangle r,rectangle bound,int source_x,int source_y)
{
    puts("RENDER COLOURED OVERLAY ELEMENT NOT YET IMPLEMENTED");
}

void upload_overlay_render_datum(overlay_data * od,overlay_render_data * ord)
{
    ensure_overlay_data_space(od);

    od->data[od->count++]=(*ord);
}

bool check_click_on_shaded_sprite(overlay_theme * theme,rectangle r,int sprite_x,int sprite_y)
{
    if((r.x<=0)&&(r.y<=0)&&((r.x+r.w)>0)&&((r.y+r.h)>0))
    {
		if(theme->shaded_texture_data[sprite_x-r.x+theme->shaded_texture_size*(sprite_y-r.y)])
        {
            return true;
        }
    }

    return false;
}

bool check_click_on_coloured_sprite(overlay_theme * theme,rectangle r,int sprite_x,int sprite_y)
{
    if((r.x<=0)&&(r.y<=0)&&((r.x+r.w)>0)&&((r.y+r.h)>0))
    {
		if(theme->coloured_texture_data[3+4*(sprite_x-r.x+theme->coloured_texture_size*(sprite_y-r.y))])
        {
            return true;
        }
    }

    return false;
}




#warning is bool return of below ever properly used ???
bool process_regular_text_character(cvm_font * cf,char prev,char current,int * offset)
{
    if((current>='!')&&(current<='~'))
    {
        if(*offset)
        {
            if((prev>='!')&&(prev<='~'))
            {
                *offset+=cf->glyphs[prev-'!'].kerning[current-'!'];
            }
        }
        else
        {
            *offset-=cf->glyphs[current-'!'].bearingX;
        }

        *offset+=cf->glyphs[current-'!'].advance;

        return true;
    }
    return false;
}

int get_new_text_offset(cvm_font * cf,char prev,char current,int current_offset)
{
    if(current=='\t') return ((current_offset/(cf->space_width*4))+1)*cf->space_width*4;
    if(current==' ') return current_offset+cf->space_width;

    #warning wtf is below, when would it ever be used
    if(current=='\n') return current_offset+cf->space_width;

    process_regular_text_character(cf,prev,current,&current_offset);

    return current_offset;
}

int process_simple_text_character(cvm_font * cf,char prev,char current)///doesn't allow spaces or tabs, returns extra with necessary for character
{
    int extra_width=0;
    if((current>='!')&&(current<='~'))
    {
        if(prev)
        {
            if((prev>='!')&&(prev<='~'))
            {
                extra_width+=cf->glyphs[prev-'!'].kerning[current-'!'];
            }
        }
        else
        {
            extra_width-=cf->glyphs[current-'!'].bearingX;
        }

        extra_width+=cf->glyphs[current-'!'].advance;
    }
    if(current==' ')extra_width=cf->space_width;

    return extra_width;
}

int calculate_text_length(overlay_theme * theme,char * text,int font_index)
{
    int offset=0;
    char prev=0;
    int i;

    if(text)
    {
        for(i=0;text[i];i++)
        {
            offset=get_new_text_offset(&theme->font,prev,text[i],offset);
            prev=text[i];
        }
    }

    return offset;
}


char * shorten_text_to_fit_width_start_ellipses(overlay_theme * theme,int width,char * text,int font_index,char * buffer,int buffer_size,int * x_offset)
{
    cvm_font * f= &theme->font;
    int i,bi,d,l,offset=0;

    l=strlen(text);

    if(l==0)return text;
    offset=0;

    buffer[buffer_size-1]='\0';

    if(x_offset)*x_offset=0;

    for(i=(l-1);i>=0;i--)
    {
        offset+=process_simple_text_character(f,i ? text[i-1] : 0,text[i]);

        bi=buffer_size-1+i-l;

        buffer[bi]=text[i];

        if((offset>width)||(bi==3))///check i== test
        {
            for(;(i<l);i++)
            {
                offset-=process_simple_text_character(f,i ? text[i-1] : 0,text[i]);

                d=process_simple_text_character(f,0,'.');
                d+=2*process_simple_text_character(f,'.','.');
                d+=process_simple_text_character(f,'.',text[i]);

                if((offset+d)<width)
                {
                    bi=buffer_size-1+i-l;

                    buffer[bi-1]=buffer[bi-2]=buffer[bi-3]='.';

                    if(x_offset)*x_offset=width-d-offset;

                    return buffer+bi-3;
                }
            }

            ///there wasn't enight room for even 1 text character and ellipses
            buffer[0]=buffer[1]=buffer[2]='.';
            buffer[3]='\0';
            return buffer;
        }
    }

    return text;
}

char * shorten_text_to_fit_width_end_ellipses(overlay_theme * theme,int width,char * text,int font_index,char * buffer,int buffer_size)//,int * x_offset
{
    cvm_font * f= &theme->font;
    int i,d,offset=0;
    char prev=0;

    //if(x_offset)*x_offset=0;

    for(i=0;text[i];i++)
    {
        offset+=process_simple_text_character(f,prev,text[i]);
        prev=text[i];

        if((offset>width)||(i==buffer_size-4))
        {
            for(;i>0;i--)
            {
                offset-=process_simple_text_character(f,text[i-1],text[i]);

                d=process_simple_text_character(f,text[i-1],'.');
                d+=2*process_simple_text_character(f,'.','.');

                if((offset+d)<width)
                {
                    buffer[i]=buffer[i+1]=buffer[i+2]='.';
                    buffer[i+3]='\0';

                    //if(x_offset)*x_offset=width-d-offset;

                    return buffer;
                }
            }

            ///there wasn't enight room for even 1 text character and ellipses
            buffer[0]=buffer[1]=buffer[2]='.';
            buffer[3]='\0';
            return buffer;
        }

        buffer[i]=text[i];
    }

    return text;
}

void render_overlay_text(overlay_data * od,overlay_theme * theme,char * text,int x_off,int y_off,rectangle bounds,int font_index,int colour)
{
    int offset=0;
    char prev=0;
    int i;//font.

    rectangle r;

    if(text==NULL)return;

    for(i=0;text[i];i++)
    {
        offset=get_new_text_offset(&theme->font,prev,text[i],offset);
        prev=text[i];

        if( (text[i]>='!') && (text[i]<='~') )
        {
            r=(rectangle)
            {
                .x=x_off+offset+theme->font.glyphs[text[i]-'!'].bearingX-theme->font.glyphs[text[i]-'!'].advance,
                .y=y_off,
                .w=theme->font.glyphs[text[i]-'!'].width,
                .h=theme->font.font_height
            };

            render_character(od,r,bounds,theme->font.glyphs[text[i]-'!'].offset,font_index,OVERLAY_TEXT_COLOUR_0+colour);
        }
    }
}


void set_overlay_sprite_image_data(overlay_theme * theme,overlay_sprite_data osd,uint8_t * source_data,int source_w,int w,int h,int x,int y)
{
    uint8_t * destination_data;
    size_t copy_count=w;
    size_t src_inc=source_w;
    size_t dst_inc;
    int i;

    if(osd.type==OVERLAY_SHADED_SPRITE)
    {
        theme->shaded_texture_updated=true;

        dst_inc=theme->shaded_texture_size;
        destination_data=theme->shaded_texture_data+osd.x_pos+osd.y_pos*theme->shaded_texture_size;
        source_data+=x+y*source_w;
    }
    else if(osd.type==OVERLAY_COLOURED_SPRITE)
    {
        theme->coloured_texture_updated=true;

        copy_count*=4;
        src_inc*=4;
        dst_inc=4*theme->coloured_texture_size;
        destination_data=theme->coloured_texture_data+4*(osd.x_pos+osd.y_pos*theme->coloured_texture_size);
        source_data+=4*(x+y*source_w);
    }

    for(i=0;i<h;i++)
    {
        memcpy(destination_data,source_data,copy_count);
        destination_data+=dst_inc;
        source_data+=src_inc;
    }
}



void set_overlay_sprite_image_data_from_sdl_surface(overlay_theme * theme,overlay_sprite_data osd,SDL_Surface * surface,int w,int h,int x,int y)
{
    int i,j;
    uint8_t dummy;
    uint8_t * dst;


    if(osd.type==OVERLAY_SHADED_SPRITE)
    {
        theme->shaded_texture_updated=true;

        for(i=0;i<w;i++)
        {
            for(j=0;j<h;j++)
            {
                dst=theme->shaded_texture_data+osd.x_pos+i+theme->shaded_texture_size*(osd.y_pos+j);
                SDL_GetRGBA(*((uint32_t*)(((uint8_t*) surface->pixels)+(y+j)*surface->pitch+(x+i)*surface->format->BytesPerPixel)),surface->format,&dummy,&dummy,&dummy,dst);
            }
        }
    }
    else if(osd.type==OVERLAY_COLOURED_SPRITE)
    {
        theme->coloured_texture_updated=true;

        for(i=0;i<w;i++)
        {
            for(j=0;j<h;j++)
            {
                dst=theme->coloured_texture_data+4*(osd.x_pos+i+theme->coloured_texture_size*(osd.y_pos+j));
                SDL_GetRGBA(*((uint32_t*)(((uint8_t*) surface->pixels)+(y+j)*surface->pitch+(x+i)*surface->format->BytesPerPixel)),surface->format,dst+0,dst+1,dst+2,dst+3);
            }
        }
    }
}

overlay_sprite_data create_overlay_sprite_from_sdl_surface(overlay_theme * theme,char * name,SDL_Surface * surface,int w,int h,int x,int y,overlay_sprite_type type)
{
    overlay_sprite_data osd=create_overlay_sprite(theme,name,w,h,type);
    set_overlay_sprite_image_data_from_sdl_surface(theme,osd,surface,w,h,x,y);
    return osd;
}




