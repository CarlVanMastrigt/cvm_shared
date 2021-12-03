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
static cvm_vk_transient_buffer overlay_transient_buffer;
//static uint32_t * overlay_uniform_buffer_acquisitions;

static cvm_vk_staging_buffer overlay_staging_buffer;
//static uint32_t * overlay_staging_buffer_acquisitions;

static VkCommandBuffer overlay_upload_command_buffer;

static uint32_t max_overlay_elements=4096;

static VkDependencyInfoKHR overlay_uninitialised_to_transfer_dependencies;
static VkImageMemoryBarrier2KHR overlay_uninitialised_to_transfer_image_barriers[2];

static VkDependencyInfoKHR overlay_transfer_to_graphics_dependencies;
static VkImageMemoryBarrier2KHR overlay_transfer_to_graphics_image_barriers[2];

static VkDependencyInfoKHR overlay_graphics_to_transfer_dependencies;
static VkImageMemoryBarrier2KHR overlay_graphics_to_transfer_image_barriers[2];

//static bool overlay_graphics_transfer_ownership_changes;


void test_timing(bool start,char * name)
{
    static struct timespec tso,tsn;
    uint64_t dt;

    clock_gettime(CLOCK_REALTIME,&tsn);

    dt=(tsn.tv_sec-tso.tv_sec)*1000000000 + tsn.tv_nsec-tso.tv_nsec;

    if(start) tso=tsn;

    if(name)printf("%s = %lu\n",name,dt);
}



///temporary test stuff
//static cvm_overlay_font overlay_test_font;

static void create_overlay_descriptor_set_layouts(void)
{
    VkSampler fetch_sampler=cvm_vk_get_fetch_sampler();

    VkDescriptorSetLayoutCreateInfo layout_create_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .bindingCount=0,
        .pBindings=NULL,
    };



    layout_create_info.bindingCount=1;
    layout_create_info.pBindings=(VkDescriptorSetLayoutBinding[1])
    {
        {
            .binding=0,
            .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
            .descriptorCount=1,
            .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers=NULL
        }
    };

    cvm_vk_create_descriptor_set_layout(&overlay_descriptor_set_layout,&layout_create_info);


    ///consistent sets
    layout_create_info.bindingCount=2;
    layout_create_info.pBindings=(VkDescriptorSetLayoutBinding[2])
    {
        {
            .binding=0,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
            .descriptorCount=1,
            .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers=&fetch_sampler /// also test w/ null & setting samplers directly
        },
        {
            .binding=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
            .descriptorCount=1,
            .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers=&fetch_sampler /// also test w/ null & setting samplers directly
        }
    };

    cvm_vk_create_descriptor_set_layout(&overlay_consistent_descriptor_set_layout,&layout_create_info);
}

static void create_overlay_consistent_descriptors(void)
{
    VkDescriptorPoolCreateInfo pool_create_info=
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
                .descriptorCount=2
            }
        }
    };

    cvm_vk_create_descriptor_pool(&overlay_consistent_descriptor_pool,&pool_create_info);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=overlay_consistent_descriptor_pool,
        .descriptorSetCount=1,
        .pSetLayouts=&overlay_consistent_descriptor_set_layout
    };

    cvm_vk_allocate_descriptor_sets(&overlay_consistent_descriptor_set,&descriptor_set_allocate_info);

    VkWriteDescriptorSet writes[2]=
    {
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
        },
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=overlay_consistent_descriptor_set,
            .dstBinding=1,
            .dstArrayElement=0,
            .descriptorCount=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo=(VkDescriptorImageInfo[1])
            {
                {
                    .sampler=VK_NULL_HANDLE,///using immutable sampler (for now)
                    .imageView=overlay_colour_image_view,
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
            },
            .pBufferInfo=NULL,
            .pTexelBufferView=NULL
        }
    };

    cvm_vk_write_descriptor_sets(writes,2);
}

static void create_overlay_descriptor_sets(uint32_t swapchain_image_count)
{
    uint32_t i;
    VkDescriptorSetLayout set_layouts[swapchain_image_count];
    for(i=0;i<swapchain_image_count;i++)set_layouts[i]=overlay_descriptor_set_layout;

    ///separate pool for image descriptor sets? (so that they dont need to be reallocated/recreated upon swapchain changes)

    ///pool size dependent upon swapchain image count so must go here
    VkDescriptorPoolCreateInfo pool_create_info=
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

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info=
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

static void update_overlay_uniforms(uint32_t swapchain_image,VkDeviceSize offset)
{
    ///investigate whether its necessary to update this every frame if its basically unchanging data and war hazards are a problem. may want to avoid just to stop validation from picking it up.
    VkWriteDescriptorSet writes[1]=
    {
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
                    .buffer=overlay_transient_buffer.buffer,
                    .offset=offset,
                    .range=OVERLAY_NUM_COLOURS_*4*sizeof(float)
                }
            },
            .pTexelBufferView=NULL
        }
    };

    cvm_vk_write_descriptor_sets(writes,1);
}

static void create_overlay_pipeline_layouts(void)
{
    VkPipelineLayoutCreateInfo pipeline_create_info=
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
    VkRenderPassCreateInfo create_info=
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
    VkGraphicsPipelineCreateInfo create_info=
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
                .vertexBindingDescriptionCount=1,
                .pVertexBindingDescriptions= (VkVertexInputBindingDescription[1])
                {
                    {
                        .binding=0,
                        .stride=sizeof(cvm_overlay_render_data),
                        .inputRate=VK_VERTEX_INPUT_RATE_INSTANCE
                    }
                },
                .vertexAttributeDescriptionCount=0,
                .pVertexAttributeDescriptions=NULL,
                .vertexAttributeDescriptionCount=2,//3
                .pVertexAttributeDescriptions=(VkVertexInputAttributeDescription[3])
                {
                    {
                        .location=0,
                        .binding=0,
                        .format=VK_FORMAT_R16G16B16A16_UINT,
                        .offset=offsetof(cvm_overlay_render_data,data0)
                    },
                    {
                        .location=1,
                        .binding=0,
                        .format=VK_FORMAT_R16G16B16A16_UINT,
                        .offset=offsetof(cvm_overlay_render_data,data1)
                    },
//                    {
//                        .location=2,
//                        .binding=0,
//                        .format=VK_FORMAT_R16G16B16A16_UINT,
//                        .offset=offsetof(cvm_overlay_render_data,data2)
//                    }
                }
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
                        .srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA,
                        .dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                        .colorBlendOp=VK_BLEND_OP_ADD,
                        .srcAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,
                        .dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,
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

    VkFramebufferCreateInfo create_info=
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
    VkImageCreateInfo image_creation_info=
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



    VkImageViewCreateInfo view_creation_info=
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

    cvm_vk_create_transient_buffer(&overlay_transient_buffer,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cvm_vk_create_staging_buffer(&overlay_staging_buffer,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    create_overlay_images(1024,1024,1024,1024);
    create_overlay_barriers();///must go after creation of all resources these barriers act upon

    create_overlay_consistent_descriptors();

    cvm_overlay_open_freetype();

    overlay_upload_command_buffer=VK_NULL_HANDLE;

//    cvm_overlay_create_font(&overlay_test_font,"cvm_shared/resources/cvm_font_1.ttf",32);
    //cvm_overlay_create_font(&overlay_test_font,"cvm_shared/resources/cvm_font_1.ttf",24);
//    cvm_overlay_create_font(&overlay_test_font,"cvm_shared/resources/OpenSans-Regular.ttf",24);
//    cvm_overlay_create_font(&overlay_test_font,"cvm_shared/resources/HanaMinA.ttf",24);
//    cvm_overlay_create_font(&overlay_test_font,"cvm_shared/resources/FreeMono.ttf",24);
//    cvm_overlay_create_font(&overlay_test_font,"cvm_shared/resources/Symbola_hint.ttf",32);
}

void terminate_overlay_render_data(void)
{
    //cvm_overlay_destroy_font(&overlay_test_font);

    cvm_overlay_close_freetype();

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

    cvm_vk_destroy_transient_buffer(&overlay_transient_buffer);
    cvm_vk_destroy_staging_buffer(&overlay_staging_buffer);
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
    uint32_t uniform_size=0;
    uniform_size+=cvm_vk_transient_buffer_get_rounded_allocation_size(&overlay_transient_buffer,sizeof(float)*4*OVERLAY_NUM_COLOURS_);
    uniform_size+=cvm_vk_transient_buffer_get_rounded_allocation_size(&overlay_transient_buffer,max_overlay_elements*sizeof(cvm_overlay_render_data));
    uint32_t staging_size=65536;

    cvm_vk_update_transient_buffer(&overlay_transient_buffer,uniform_size,swapchain_image_count);
    cvm_vk_update_staging_buffer(&overlay_staging_buffer,staging_size,swapchain_image_count);
}

void terminate_overlay_swapchain_dependencies(void)
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    uint32_t i;

    cvm_vk_destroy_pipeline(overlay_pipeline);

    for(i=0;i<swapchain_image_count;i++)
    {
        cvm_vk_destroy_framebuffer(overlay_framebuffers[i]);
    }

    cvm_vk_destroy_descriptor_pool(overlay_descriptor_pool);

    free(overlay_descriptor_sets);
    free(overlay_framebuffers);
}

float overlay_colours[OVERLAY_NUM_COLOURS_*4]=
{
    1.0,0.1,0.1,1.0,///OVERLAY_NO_COLOUR (error)
    0.24,0.24,0.6,0.9,///OVERLAY_BACKGROUND_COLOUR
    0.12,0.12,0.36,0.85,///OVERLAY_MAIN_COLOUR
    0.12,0.12,0.48,0.85,///OVERLAY_MAIN_ALTERNATE_COLOUR
    0.4,0.6,0.9,0.3,///OVERLAY_MAIN_ALTERNATE_COLOUR
    0.4,0.6,0.9,0.8,///OVERLAY_TEXT_COLOUR_0
};

#warning texel buffers probably are NOT the best solution as they would potentially require creating a buffer view every frame (if offset changes) or a least a buffer view per swapchain image
//uint8_t overlay_colours[OVERLAY_NUM_COLOURS_COLOURS*4]=
//{
//    0x40,0x40,0xA0,0xD0,///OVERLAY_BACKGROUND_COLOUR_
//    0x20,0x20,0x50,0xC0,///OVERLAY_MAIN_COLOUR_
//    0x50,0x50,0xA0,0xB0,///OVERLAY_TEXT_COLOUR_0_
//};

cvm_vk_module_work_block * overlay_render_frame(int screen_w,int screen_h,widget * menu_widget)
{
    cvm_vk_module_work_block * work_block;
    uint32_t swapchain_image_index;

    static bool first=true;

    cvm_overlay_element_render_buffer element_render_buffer;
    VkDeviceSize uniform_offset,vertex_offset;

    ///perhaps this should return previous image index as well, such that that value can be used in cleanup (e.g. relinquishing ring buffer space)
    work_block = cvm_vk_begin_module_work_block(&overlay_module_data,0,&swapchain_image_index);



    /// definitely want to clean up targets &c.
    /// "resultant" (swapchain) attachment goes at end
    /// actually loop framebuffers, look into benefit
    /// define state that varies between attachments in base, referenceable struct that can be shared across functions and invocations (inc. clear values?)
    /// dont bother storing framebuffer images/data in enumed array, store individually

    /// could have a single image atlas and use different aspects of part of it (r,g,b,a via image views) for different alpha/transparent image_atlases
    /// this would be fairly inefficient to retrieve though..., probably better not to.
    static char * str;

    if(swapchain_image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        overlay_upload_command_buffer=work_block->graphics_work;

        cvm_vk_begin_staging_buffer(&overlay_staging_buffer);///build barriers into begin/end paradigm maybe???

        cvm_vk_begin_transient_buffer(&overlay_transient_buffer,swapchain_image_index);
        #warning need to use the appropriate queue for all following transfer ops
        ///     ^ possibly detect when they're different and use gfx directly to avoid double submission?
        ///         ^ have cvm_vk_begin_module_work_block return same command buffer?
        ///             ^could have meta level function that inserts both barriers to appropriate queues simultaneously as necessary??? (VK_NULL_HANDLE when unit-transfer?)
        ///   module handles semaphore when necessary, perhaps it can also handle some aspect(s) of barriers?
        if(first)
        {
            CVM_TMP_vkCmdPipelineBarrier2KHR(work_block->graphics_work,&overlay_uninitialised_to_transfer_dependencies);
            first=false;

//            str=strdup("Hello World! Far リサフランク420 - 現代のコンピュー");
//            str=strdup("Hello World!         Far a リサフランク420 - 現代のコンピュー  Hello World!\nFar ncdjvfng gvbgf bmgnf dvn vgbf  fjdvbfng vngfb,  ffnf\n bmngk,fgfh vfdbfg fd df gtv dfmbd f vfdhfv g dvb fbnb\n\n  f  dfgg dfmv vdv  xmcnbx fd mjdbvm v\
d cf fn\n vsrs  sdfmmgt dgf thd gmdrg1234567890-=qwertyuiop[]\\asdfgh\njkl;'zxcvbnm,./ZXCVBNM<>?ASDFGHJKL:QWERTYUIOP{}|!#$%^&*()_+\n ⬆ ❌ 🔄 📁 📷 📄 🎵 ➕ ➖ 👻 🖋 🔓 🔗 🖼 ✂ 👁");
            str="8Hello World!\tFar リサフランク420 - 現代のコンピュー  Hello World!Far ncdjvfng gvbgf bmgnf dvn vgbf  fjdvbfng vngfb,  ffnf bmngk,fgfh vfdbfg fd df gtv dfmbd f vfdhfv g dvb fbnb  f  dfgg dfmv vdv\
  xmcnbx fd mjdbvm vd cf fn vsrs  sdfmmgt dgf thd gmdrg1234567890-=qwertyuiop[]\\asdfgh\njkl;'zxcvbnm,./ZXCVBNM<>?ASDFGHJKL:QWERTYUIOP{}|!#$%^&*(_+ ↑←↓→🗙 ❌ 🔄 💾 📁 📷 📄 🎵 ➕ ➖ 👻 🖋 🔓 🔗 🖼 ✂ 👁 ⚙";
//                       str=strdup("Far");
//            str=strdup("Helloasa sas");
        }
        else
        {
            CVM_TMP_vkCmdPipelineBarrier2KHR(work_block->graphics_work,&overlay_graphics_to_transfer_dependencies);
        }

        element_render_buffer.buffer=cvm_vk_get_transient_buffer_allocation(&overlay_transient_buffer,max_overlay_elements*sizeof(cvm_overlay_render_data),&vertex_offset);
        element_render_buffer.space=max_overlay_elements;
        element_render_buffer.count=0;

        //str="The attempt to impose upon man, a creature of growth and capable of sweetness, to ooze juicily at the last round the bearded lips of God, to attempt to impose, I say, laws and conditions appropriate to a mechanical creation, against this I raise my sword-pen.";
        //str="This planet has - or rather had - a problem, which was this: most of the people living on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movement of small green pieces of paper, which was odd because on the whole it wasn't the small green pieces of paper that were unhappy.";

        //test_timing(true,NULL);
        //rectangle_ r={.x1=30+(int)(25.0*cos(SDL_GetTicks()*0.002)),.y1=20+(int)(15.0*cos(SDL_GetTicks()*0.0044)),.x2=300+(int)(50.0*cos(SDL_GetTicks()*0.0033)),.y2=160+(int)(40.0*cos(SDL_GetTicks()*0.007))};
        rectangle_ r={.x1=-1000,.y1=-1000,.x2=3000,.y2=3000};
        render_widget_overlay(&element_render_buffer,menu_widget);
        //overlay_render_text_complex(&element_render_buffer,&overlay_test_font,(uint8_t*)str,0,0,&r,650);
        //cubic_test(&element_render_buffer,0,0,&r);
        //test_timing(false,"render text");

        CVM_TMP_vkCmdPipelineBarrier2KHR(work_block->graphics_work,&overlay_transfer_to_graphics_dependencies);







        float * colours = cvm_vk_get_transient_buffer_allocation(&overlay_transient_buffer,sizeof(float)*4*OVERLAY_NUM_COLOURS_,&uniform_offset);

        memcpy(colours,overlay_colours,sizeof(float)*4*OVERLAY_NUM_COLOURS_);

        update_overlay_uniforms(swapchain_image_index,uniform_offset);///should really build buffer acquisition into update uniforms function




        float screen_dimensions[4]={2.0/((float)screen_w),2.0/((float)screen_h),(float)screen_w,(float)screen_h};

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

        vkCmdBindVertexBuffers(work_block->graphics_work,0,1,&overlay_transient_buffer.buffer,&vertex_offset);

        vkCmdDraw(work_block->graphics_work,4,element_render_buffer.count,0,0);

        vkCmdEndRenderPass(work_block->graphics_work);///================

        ///huh, could store this in buffer maybe as it occurrs once per frame...
        cvm_vk_end_transient_buffer(&overlay_transient_buffer);
        cvm_vk_end_staging_buffer(&overlay_staging_buffer,swapchain_image_index);

        overlay_upload_command_buffer=VK_NULL_HANDLE;
    }

    return cvm_vk_end_module_work_block(&overlay_module_data);
}

void overlay_frame_cleanup(uint32_t swapchain_image_index)
{
    if(swapchain_image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        cvm_vk_relinquish_staging_buffer_space(&overlay_staging_buffer,swapchain_image_index);
    }
}



cvm_vk_image_atlas_tile * overlay_create_transparent_image_tile_with_staging(void ** staging,uint32_t w, uint32_t h)
{
    VkDeviceSize upload_offset;
    cvm_vk_image_atlas_tile * tile;

    tile=NULL;
    *staging = cvm_vk_get_staging_buffer_allocation(&overlay_staging_buffer,sizeof(uint8_t)*w*h,&upload_offset);

    if(*staging)
    {
        tile=cvm_vk_acquire_image_atlas_tile(&overlay_transparency_image_atlas,w,h);

        if(tile)
        {
            VkBufferImageCopy copy_info=
            {
                .bufferOffset=upload_offset,
                .bufferRowLength=w,
                .bufferImageHeight=h,
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
                    .height=h,
                    .depth=1,
                }
            };

            vkCmdCopyBufferToImage(overlay_upload_command_buffer,overlay_staging_buffer.buffer,overlay_transparency_image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy_info);
        }
    }

    return tile;
}

void overlay_destroy_transparent_image_tile(cvm_vk_image_atlas_tile * tile)
{
    cvm_vk_relinquish_image_atlas_tile(&overlay_transparency_image_atlas,tile);
}

cvm_vk_image_atlas_tile * overlay_create_colour_image_tile_with_staging(void ** staging,uint32_t w, uint32_t h)
{
    //
}

void overlay_destroy_colour_image_tile(cvm_vk_image_atlas_tile * tile)
{
    //
}

//bool check_click_on_interactable_element(cvm_overlay_interactable_element * element,int x,int y)
//{
//    uint32_t i;
//    if((x>=0)&&(y>=0)&&(x<element->w)&&(y<element->h))
//    {
//        uint32_t i=y*element->w+x;
//		if(element->selection_grid[i>>4] & (1<<(i&0x0F)))
//        {
//            return true;
//        }
//    }
//
//    return false;
//}






























