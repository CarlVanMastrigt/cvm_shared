/**
Copyright 2020,2021 Carl van Mastrigt

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

#define TEST_FRAMEBUFFER_CYCLES 2

/// should make all magic numbers const static integers (e.g. num attachments)

///switch to dynamic descriptors or use fixed in place uniform buffer (with appropriate size and no need to alter offset at any point)

static cvm_vk_module_data test_module_data;

///actual resultant structures defined/created by above
static VkRenderPass test_render_pass;

static VkDescriptorSetLayout test_descriptor_set_layout;
static VkDescriptorPool test_descriptor_pool;
static VkDescriptorSet * test_descriptor_sets;

///figure out if pool needs to be reallocated based on whether the pool size changed?

static VkPipelineLayout test_pipeline_layout;///relevant descriptors, share as much as possible
static VkPipeline test_pipeline;
static VkPipelineShaderStageCreateInfo test_vertex_stage;
static VkPipelineShaderStageCreateInfo test_fragment_stage;

static VkFramebuffer * test_framebuffers[TEST_FRAMEBUFFER_CYCLES];
static uint32_t test_current_framebuffer_index;

static VkDeviceMemory test_framebuffer_image_memory;

static VkImage test_framebuffer_depth;
static VkImageView test_framebuffer_depth_views[TEST_FRAMEBUFFER_CYCLES];///views of single array slice from image

static VkImage test_framebuffer_colour;
static VkImageView test_framebuffer_colour_views[TEST_FRAMEBUFFER_CYCLES];///views of single array slice from image


static cvm_vk_managed_buffer test_managed_buffer;

static cvm_vk_staging_buffer test_staging_buffer;

static cvm_vk_transient_buffer test_transient_buffer;


static cvm_mesh test_stellated_octahedron_mesh;
static cvm_mesh test_stub_stellated_octahedron_mesh;
static cvm_mesh test_cube_mesh;

static void create_test_descriptor_set_layouts(void)
{
    VkDescriptorSetLayoutCreateInfo layout_create_info=(VkDescriptorSetLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .bindingCount=1,
        .pBindings=(VkDescriptorSetLayoutBinding[1])
        {
            {
                .binding=0,
                .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount=1,
                .stageFlags=VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=NULL
            }
        }
    };

    cvm_vk_create_descriptor_set_layout(&test_descriptor_set_layout,&layout_create_info);
}

static void create_test_descriptor_sets(uint32_t swapchain_image_count)
{
    uint32_t i;
    VkDescriptorSetLayout set_layouts[swapchain_image_count];
    for(i=0;i<swapchain_image_count;i++)set_layouts[i]=test_descriptor_set_layout;

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
                .type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount=swapchain_image_count
            }
        }
    };

    cvm_vk_create_descriptor_pool(&test_descriptor_pool,&pool_create_info);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info=(VkDescriptorSetAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=test_descriptor_pool,
        .descriptorSetCount=swapchain_image_count,
        .pSetLayouts=set_layouts
    };

    test_descriptor_sets=malloc(swapchain_image_count*sizeof(VkDescriptorSet));

    cvm_vk_allocate_descriptor_sets(test_descriptor_sets,&descriptor_set_allocate_info);
}

static void update_and_bind_test_uniforms(uint32_t swapchain_image,VkDeviceSize offset)
{
    VkWriteDescriptorSet write=(VkWriteDescriptorSet)
    {
        .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext=NULL,
        .dstSet=test_descriptor_sets[swapchain_image],
        .dstBinding=0,
        .dstArrayElement=0,
        .descriptorCount=1,
        .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo=NULL,
        .pBufferInfo=(VkDescriptorBufferInfo[1])
        {
            {
                .buffer=test_transient_buffer.buffer,
                .offset=offset,
                .range=sizeof(matrix4f)+sizeof(float)*8
            }
        },
        .pTexelBufferView=NULL
    };

    cvm_vk_write_descriptor_sets(&write,1);
}

static void create_test_pipeline_layouts(void)
{
    VkPipelineLayoutCreateInfo pipeline_create_info=(VkPipelineLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=1,
        .pSetLayouts=&test_descriptor_set_layout,
        .pushConstantRangeCount=1,
        .pPushConstantRanges=(VkPushConstantRange[1])
        {
            {
                .stageFlags=VK_SHADER_STAGE_VERTEX_BIT,
                .offset=0,
                .size=sizeof(float)*12
            }
        }
    };

    cvm_vk_create_pipeline_layout(&test_pipeline_layout,&pipeline_create_info);
}

static void create_test_render_pass(VkFormat swapchain_format,VkSampleCountFlagBits sample_count)
{
    VkRenderPassCreateInfo create_info=(VkRenderPassCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .attachmentCount=2,
        .pAttachments=(VkAttachmentDescription[2])
        {
            {
                .flags=0,
                .format=swapchain_format,
                .samples=VK_SAMPLE_COUNT_1_BIT,///sample_count not relevant for actual render target (swapchain image)
                .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,/// is first render pass (ergo the clear above) thus the VK_IMAGE_LAYOUT_UNDEFINED rather than VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                .finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL///VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL so that next pass can also write to swapchain image without transitioning layout
            },
            {
                .flags=0,
                .format=VK_FORMAT_D32_SFLOAT_S8_UINT,
                .samples=sample_count,
                .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
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
                .pDepthStencilAttachment=(VkAttachmentReference[1])
                {
                    {
                        .attachment=1,
                        .layout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    }
                },
                .preserveAttachmentCount=0,
                .pPreserveAttachments=NULL
            }
        },
        .dependencyCount=4,
        .pDependencies=(VkSubpassDependency[4])
        {
            ///these define dependencies for both depth and swapchain/colour, can probably separate them to be just scope associated w/ colour/depth &c.
            /// if future attachments aren't used until/after specific subpasses, then will need external dependencies for them as well
            /// these dependencies don't necessarily mean multiple framebuffer images isnt a valid approach, as they are *sub* resource specific "barriers"
            ///     ^ so image views from same array will be treated independently?
            ///     ^ but reusing the exact same image view for every submission should also be handled i believe
            /// wait... is scope (synchronisation) general or is it constrained to resources wo which it is applied??? - think so, yes
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
                .srcSubpass=VK_SUBPASS_EXTERNAL,
                .dstSubpass=0,
                .srcStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
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
            },
            {
                .srcSubpass=0,
                .dstSubpass=VK_SUBPASS_EXTERNAL,
                .srcStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            }
        }
    };

    cvm_vk_create_render_pass(&test_render_pass,&create_info);
}

static void create_test_pipelines(VkRect2D screen_rectangle,VkSampleCountFlagBits sample_count,float min_sample_shading)
{
    VkGraphicsPipelineCreateInfo create_info=(VkGraphicsPipelineCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .stageCount=2,
        .pStages=(VkPipelineShaderStageCreateInfo[2])
        {
            test_vertex_stage,
            test_fragment_stage
        },///could be null then provided by each actual pipeline type (for sets of pipelines only variant based on shader)
        .pVertexInputState=(VkPipelineVertexInputStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .vertexBindingDescriptionCount=1,
                .pVertexBindingDescriptions= (VkVertexInputBindingDescription[1])
                {
                    {
                        .binding=0,
                        .stride=sizeof(float)*3,
                        .inputRate=VK_VERTEX_INPUT_RATE_VERTEX
                    }
                },
                .vertexAttributeDescriptionCount=1,
                .pVertexAttributeDescriptions=(VkVertexInputAttributeDescription[1])
                {
                    {
                        .location=0,
                        .binding=0,
                        .format=VK_FORMAT_R32G32B32_SFLOAT,
                        .offset=0//offsetof(test_render_data,pos)
                    },
//                    {
//                        .location=1,
//                        .binding=0,
//                        .format=VK_FORMAT_R8G8B8A8_UNORM,
//                        .offset=offsetof(test_render_data,col)
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
                .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,/// VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
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
                .cullMode=VK_CULL_MODE_BACK_BIT,
                .frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
                .rasterizationSamples=sample_count,///this can be changed, requires rebuild all render passes, framebuffers &c.
                .sampleShadingEnable=VK_FALSE,
                .minSampleShading=min_sample_shading,///this can be changed, only requires rebuild of pipelines
                .pSampleMask=NULL,
                .alphaToCoverageEnable=VK_FALSE,
                .alphaToOneEnable=VK_FALSE
            }
        },
        .pDepthStencilState=(VkPipelineDepthStencilStateCreateInfo[1])
        {
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
                    .compareOp=VK_COMPARE_OP_NEVER,
                    .compareMask=0,
                    .writeMask=0,
                    .reference=0
                },
                .back=(VkStencilOpState)
                {
                    .failOp=VK_STENCIL_OP_KEEP,
                    .passOp=VK_STENCIL_OP_KEEP,
                    .depthFailOp=VK_STENCIL_OP_KEEP,
                    .compareOp=VK_COMPARE_OP_NEVER,
                    .compareMask=0,
                    .writeMask=0,
                    .reference=0
                },
                .minDepthBounds=0.0,
                .maxDepthBounds=1.0,
            }
        },
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
                        .blendEnable=VK_FALSE,
                        .srcColorBlendFactor=VK_BLEND_FACTOR_ONE,//VK_BLEND_FACTOR_SRC_ALPHA,
                        .dstColorBlendFactor=VK_BLEND_FACTOR_ZERO,//VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
                        .colorBlendOp=VK_BLEND_OP_ADD,
                        .srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE,//VK_BLEND_FACTOR_ZERO // both ZERO as alpha is ignored?
                        .dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,//VK_BLEND_FACTOR_ONE
                        .alphaBlendOp=VK_BLEND_OP_ADD,
                        .colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
                    }
                },
                .blendConstants={0.0,0.0,0.0,0.0}
            }
        },
        .pDynamicState=NULL,
        .layout=test_pipeline_layout,
        .renderPass=test_render_pass,
        .subpass=0,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };

    ///can pass above into multiple functions as parameter
    cvm_vk_create_graphics_pipeline(&test_pipeline,&create_info);
}

static void create_test_framebuffers(VkRect2D screen_rectangle,uint32_t swapchain_image_count)
{
    uint32_t i,j;
    VkImageView attachments[2];

    test_current_framebuffer_index=0;

    VkFramebufferCreateInfo create_info=(VkFramebufferCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .renderPass=test_render_pass,
        .attachmentCount=2,
        .pAttachments=attachments,
        .width=screen_rectangle.extent.width,
        .height=screen_rectangle.extent.height,
        .layers=1
    };

    for(i=0;i<TEST_FRAMEBUFFER_CYCLES;i++)
    {
        test_framebuffers[i]=malloc(swapchain_image_count*sizeof(VkFramebuffer));

        for(j=0;j<swapchain_image_count;j++)
        {
            attachments[0]=cvm_vk_get_swapchain_image_view(j);
            attachments[1]=test_framebuffer_depth_views[i];

            cvm_vk_create_framebuffer(test_framebuffers[i]+j,&create_info);
        }
    }
}







/// these need to be recreated whenever swapchain image changes size
/// unless we allocate at max and alter with image views... investigate potential of this, use max size, let user set max_size from startup options?
static void create_test_framebuffer_images(VkRect2D screen_rect,VkSampleCountFlagBits sample_count)
{
    uint32_t i;

    VkImageCreateInfo image_creation_info=(VkImageCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .imageType=VK_IMAGE_TYPE_2D,
        .format=VK_FORMAT_UNDEFINED,///set later
        .extent=(VkExtent3D)
        {
            .width=screen_rect.extent.width,
            .height=screen_rect.extent.height,
            .depth=1
        },
        .mipLevels=1,
        .arrayLayers=TEST_FRAMEBUFFER_CYCLES,///one for each level
        .samples=sample_count,
        .tiling=VK_IMAGE_TILING_OPTIMAL,
        .usage=0,///set later
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL,
        .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED
    };

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
            .aspectMask=0,///set later
            .baseMipLevel=0,
            .levelCount=1,
            .baseArrayLayer=0,///set later
            .layerCount=1
        }
    };



    image_creation_info.format=VK_FORMAT_D32_SFLOAT_S8_UINT;
    image_creation_info.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;///VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    cvm_vk_create_image(&test_framebuffer_depth,&image_creation_info);

    image_creation_info.format=VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    image_creation_info.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;///VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    cvm_vk_create_image(&test_framebuffer_colour,&image_creation_info);

    VkImage images[2]={test_framebuffer_depth,test_framebuffer_colour};
    cvm_vk_create_and_bind_memory_for_images(&test_framebuffer_image_memory,images,2);


    for(i=0;i<TEST_FRAMEBUFFER_CYCLES;i++)
    {
        view_creation_info.format=VK_FORMAT_D32_SFLOAT_S8_UINT;
        view_creation_info.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;
        view_creation_info.image=test_framebuffer_depth;
        view_creation_info.subresourceRange.baseArrayLayer=i;
        cvm_vk_create_image_view(test_framebuffer_depth_views+i,&view_creation_info);

        view_creation_info.format=VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        view_creation_info.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        view_creation_info.image=test_framebuffer_colour;
        view_creation_info.subresourceRange.baseArrayLayer=i;
        cvm_vk_create_image_view(test_framebuffer_colour_views+i,&view_creation_info);
    }


    ///  | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT

    ///VK_FORMAT_D32_SFLOAT_S8_UINT,
    ///VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT

    /// for colour and normal attachments:
    ///VK_FORMAT_A2R10G10B10_UNORM_PACK32
    ///VK_FORMAT_A2R10G10B10_SNORM_PACK32
    /// higher quality variants
    ///VK_FORMAT_R16G16B16_UNORM
    ///VK_FORMAT_R16G16B16_SNORM
    /// lighting &c.
    /// VK_FORMAT_R16_UNORM

    ///VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT


}

void initialise_test_render_data()
{
    create_test_render_pass(cvm_vk_get_screen_format(),VK_SAMPLE_COUNT_1_BIT);

    create_test_descriptor_set_layouts();

    create_test_pipeline_layouts();

    cvm_vk_create_shader_stage_info(&test_vertex_stage,"cvm_shared/shaders/test_vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(&test_fragment_stage,"cvm_shared/shaders/test_frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    cvm_vk_create_module_data(&test_module_data,false,0,1,1);

    cvm_vk_managed_buffer_create(&test_managed_buffer,65536,10,16,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,false,false);
    ///VK_BUFFER_USAGE_TRANSFER_DST_BIT only necessary if not UMA system, but is it even possible to set this before finding that out??
    /// may need a pass where it ONLY looks for device only memory with VK_BUFFER_USAGE_TRANSFER_DST_BIT on by default
    ///then, if that doesn't exist, removes the VK_BUFFER_USAGE_TRANSFER_DST_BIT flag and accepts host accessible memory
    ///     ^ this will absolutely fuck with testing though, will need to add proper compile flag for simulating non-uma

    cvm_vk_transient_buffer_create(&test_transient_buffer,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    cvm_vk_staging_buffer_create(&test_staging_buffer,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    //create_test_vertex_buffer();
    test_stellated_octahedron_mesh.started=false;
    test_stub_stellated_octahedron_mesh.started=false;
    test_cube_mesh.started=false;

}

void terminate_test_render_data()
{
    cvm_vk_destroy_render_pass(test_render_pass);

    cvm_vk_destroy_shader_stage_info(&test_vertex_stage);
    cvm_vk_destroy_shader_stage_info(&test_fragment_stage);

    cvm_vk_destroy_pipeline_layout(test_pipeline_layout);

    cvm_vk_destroy_descriptor_set_layout(test_descriptor_set_layout);

    cvm_vk_destroy_module_data(&test_module_data,false);

    cvm_mesh_relinquish(&test_stellated_octahedron_mesh,&test_managed_buffer);
    cvm_mesh_relinquish(&test_stub_stellated_octahedron_mesh,&test_managed_buffer);
    cvm_mesh_relinquish(&test_cube_mesh,&test_managed_buffer);

    cvm_vk_managed_buffer_destroy(&test_managed_buffer);

    cvm_vk_transient_buffer_destroy(&test_transient_buffer);

    cvm_vk_staging_buffer_destroy(&test_staging_buffer);
}


void initialise_test_swapchain_dependencies(void)
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    VkRect2D screen_rectangle=cvm_vk_get_screen_rectangle();

    create_test_pipelines(screen_rectangle,VK_SAMPLE_COUNT_1_BIT,1.0);

    create_test_framebuffer_images(screen_rectangle,VK_SAMPLE_COUNT_1_BIT);

    create_test_framebuffers(screen_rectangle,swapchain_image_count);

    create_test_descriptor_sets(swapchain_image_count);

    cvm_vk_resize_module_graphics_data(&test_module_data,0);

    uint32_t uniform_size=0;
    uniform_size+=cvm_vk_transient_buffer_get_rounded_allocation_size(&test_transient_buffer,sizeof(matrix4f)+sizeof(float)*8);
    cvm_vk_transient_buffer_update(&test_transient_buffer,uniform_size,swapchain_image_count);

    uint32_t staging_size=65536;
    cvm_vk_staging_buffer_update(&test_staging_buffer,staging_size,swapchain_image_count);

    test_managed_buffer.staging_buffer=&test_staging_buffer;
}

void terminate_test_swapchain_dependencies()
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    uint32_t i,j;

    cvm_vk_destroy_pipeline(test_pipeline);

    for(i=0;i<swapchain_image_count;i++) for(j=0;j<TEST_FRAMEBUFFER_CYCLES;j++) cvm_vk_destroy_framebuffer(test_framebuffers[j][i]);

    for(i=0;i<TEST_FRAMEBUFFER_CYCLES;i++)
    {
        cvm_vk_destroy_image_view(test_framebuffer_depth_views[i]);
        cvm_vk_destroy_image_view(test_framebuffer_colour_views[i]);
    }

    cvm_vk_destroy_image(test_framebuffer_depth);
    cvm_vk_destroy_image(test_framebuffer_colour);

    cvm_vk_free_memory(test_framebuffer_image_memory);

    cvm_vk_destroy_descriptor_pool(test_descriptor_pool);

    free(test_descriptor_sets);
    for(i=0;i<TEST_FRAMEBUFFER_CYCLES;i++)free(test_framebuffers[i]);
}

cvm_vk_module_batch * test_render_frame(cvm_camera * c)
{
    cvm_vk_module_batch * batch;
    uint32_t swapchain_image_index;

    static rotor3f rots[4];
    static uint64_t rand=0xDEADBEEFC00FC00F;
    static uint32_t iter=0;

    cvm_transform_stack ts;
    cvm_transform_stack_reset(&ts);

    if(iter==0)for(iter=0;iter<4;iter++)rots[iter]=cvm_rng_rotation_3d(&rand);

    ///perhaps this should return previous image index as well, such that that value can be used in cleanup (e.g. relinquishing ring buffer space)
    batch = cvm_vk_begin_module_batch(&test_module_data,0,&swapchain_image_index);

    /// look into benefit of cycling an array of framebuffer & backing images (TEST_FRAMEBUFFER_CYCLES)

    if(swapchain_image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        cvm_vk_transient_buffer_begin(&test_transient_buffer,swapchain_image_index);
        cvm_vk_staging_buffer_begin(&test_staging_buffer);

        ///move above to end ??

        ///technically only necessary if using staging!

        if(!test_stellated_octahedron_mesh.ready)
        {
            if(!cvm_mesh_load_file(&test_stellated_octahedron_mesh,"cvm_shared/resources/stellated_octahedron.mesh",CVM_MESH_ADGACENCY|CVM_MESH_PER_FACE_MATERIAL,true,&test_managed_buffer))
            {
                puts("load stellated octahedron mesh failed!!!");
            }
        }

        if(!test_stub_stellated_octahedron_mesh.ready)
        {
            if(!cvm_mesh_load_file(&test_stub_stellated_octahedron_mesh,"cvm_shared/resources/stub_stellated_octahedron.mesh",CVM_MESH_ADGACENCY|CVM_MESH_PER_FACE_MATERIAL,true,&test_managed_buffer))
            {
                puts("load stub stellated octahedron mesh failed!!!");
            }
        }

        if(!test_cube_mesh.ready)
        {
            if(!cvm_mesh_load_file(&test_cube_mesh,"cvm_shared/resources/cube.mesh",CVM_MESH_ADGACENCY|CVM_MESH_PER_FACE_MATERIAL,true,&test_managed_buffer))
            {
                puts("load cube mesh failed!!!");
            }
        }

        cvm_vk_staging_buffer_end(&test_staging_buffer,swapchain_image_index);

        #warning need to figure out how to incorporate synchronization (semaphore) into dependency, just implicitly handled by module?
        ///     ^ specifically in the case of a queue ownership transfer scheduled by above
        cvm_vk_managed_buffer_submit_all_pending_copy_actions(&test_managed_buffer,batch->graphics_pcb);///must go AFTER used staging buffer gets flushed, needs external synchronisation when being used in MT environment

        ///end of transfer


        ///start of graphics


        /// is probably worthwhile having separate command buffers for transfer and graphics regardless of whether queues are actually different, that way all copies can happen before render
        /// but render and copy/create can be called from same function (i.e. next to each other)
        /// ofc. if building up structures, i.e. mapped multiindirect draws this paradigm isnt really necessary, but should be supported regardless


        VkDeviceSize uniforms_offset;
        struct
        {
            matrix4f projection;
            float offsets[4];//1 extra for padding
            float multipliers[4];//1 extra for padding
        }
        *uniforms;

        uniforms = cvm_vk_transient_buffer_get_allocation(&test_transient_buffer,sizeof(matrix4f)+sizeof(float)*8,&uniforms_offset);
        if(uniforms==NULL)puts("FAILED");

        uniforms->projection=*get_view_matrix_pointer(c);

        uniforms->multipliers[0]=1;//cos(SDL_GetTicks()*0.005);
        uniforms->multipliers[1]=1;//cos(SDL_GetTicks()*0.007);
        uniforms->multipliers[2]=1;//cos(SDL_GetTicks()*0.011);

        update_and_bind_test_uniforms(swapchain_image_index,uniforms_offset);


        float transformation[12];

        rotor3f r;
        int index;
        float lerp;


        lerp=(((float)(iter&127))+0.5)/127.0;
        index=(iter>>7)&3;
        if((iter&127)==0)rots[(index+2)&3]=cvm_rng_rotation_3d(&rand);
        iter++;

//        r=r3f_spherical_interp(rots[index],rots[(index+1)&3],lerp);
//        r=r3f_lerp(rots[index],rots[(index+1)&3],lerp);
        r=r3f_bezier_interp(rots[(index+3)&3],rots[index],rots[(index+1)&3],rots[(index+2)&3],lerp);

        cvm_transform_stack_push(&ts);
            cvm_transform_stack_rotate(&ts,r);
            cvm_transform_stack_push(&ts);
                //cvm_transform_stack_offset_position(&ts,((vec3f){1,1,1}));
                //cvm_transform_stack_rotate_around_vector(&ts,((vec3f){SQRT_THIRD,SQRT_THIRD,SQRT_THIRD}),((float)(iter&31))*TAU/32);
                cvm_transform_stack_get(&ts,transformation);
            cvm_transform_stack_pop(&ts);
        cvm_transform_stack_pop(&ts);

        vkCmdPushConstants(batch->graphics_pcb,test_pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(float)*12,transformation);

        vkCmdBindDescriptorSets(batch->graphics_pcb,VK_PIPELINE_BIND_POINT_GRAPHICS,test_pipeline_layout,0,1,test_descriptor_sets+swapchain_image_index,0,NULL);

        ///do graphics
        VkClearValue clear_values[2];///other clear colours should probably be provided by other chunks of application
        clear_values[0].color=(VkClearColorValue){{0.95f,0.5f,0.75f,1.0f}};
        clear_values[1].depthStencil=(VkClearDepthStencilValue){.depth=0.0,.stencil=0};

        cvm_vk_managed_buffer_bind_as_index(batch->graphics_pcb,&test_managed_buffer,VK_INDEX_TYPE_UINT16);
        cvm_vk_managed_buffer_bind_as_vertex(batch->graphics_pcb,&test_managed_buffer,0);

        VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext=NULL,
            .renderPass=test_render_pass,
            .framebuffer=test_framebuffers[test_current_framebuffer_index][swapchain_image_index],
            .renderArea=cvm_vk_get_screen_rectangle(),
            .clearValueCount=2,
            .pClearValues=clear_values
        };

        vkCmdBeginRenderPass(batch->graphics_pcb,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

        vkCmdBindPipeline(batch->graphics_pcb,VK_PIPELINE_BIND_POINT_GRAPHICS,test_pipeline);

        cvm_mesh_render(&test_stub_stellated_octahedron_mesh,batch->graphics_pcb,1,0);

        vkCmdEndRenderPass(batch->graphics_pcb);///================

        cvm_vk_transient_buffer_end(&test_transient_buffer);

        test_current_framebuffer_index++;
        test_current_framebuffer_index*= test_current_framebuffer_index<TEST_FRAMEBUFFER_CYCLES;
    }

    return cvm_vk_end_module_batch(&test_module_data);
}

void test_frame_cleanup(uint32_t swapchain_image_index)
{
    if(swapchain_image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        cvm_vk_staging_buffer_relinquish_space(&test_staging_buffer,swapchain_image_index);
    }
}



