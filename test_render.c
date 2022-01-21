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

static VkDescriptorPool test_descriptor_pool;

static VkDescriptorSetLayout test_geo_descriptor_set_layout;
static VkDescriptorSet * test_geo_descriptor_sets;

static VkPipelineLayout test_geo_pipeline_layout;///relevant descriptors, share as much as possible
static VkPipeline test_geo_pipeline;
static VkPipelineShaderStageCreateInfo test_geo_vertex_stage;
static VkPipelineShaderStageCreateInfo test_geo_fragment_stage;

static VkDescriptorSetLayout test_post_descriptor_set_layout;
static VkDescriptorSet * test_post_descriptor_sets;

static VkPipelineLayout test_post_pipeline_layout;///relevant descriptors, share as much as possible
static VkPipeline test_post_pipeline;
static VkPipelineShaderStageCreateInfo test_post_fragment_stage_1;
static VkPipelineShaderStageCreateInfo test_post_fragment_stage_2;
static VkPipelineShaderStageCreateInfo test_post_fragment_stage_4;

static VkFramebuffer * test_framebuffers[TEST_FRAMEBUFFER_CYCLES];
static uint32_t test_current_framebuffer_index;

static VkDeviceMemory test_framebuffer_image_memory;

static VkFormat test_depth_format;
static VkImage test_framebuffer_depth_images[TEST_FRAMEBUFFER_CYCLES];
static VkImageView test_framebuffer_depth_stencil_views[TEST_FRAMEBUFFER_CYCLES];///views of single array slice from image
static VkImageView test_framebuffer_depth_only_views[TEST_FRAMEBUFFER_CYCLES];///views of single array slice from image

static VkFormat test_colour_format;
static VkImage test_framebuffer_colour_images[TEST_FRAMEBUFFER_CYCLES];
static VkImageView test_framebuffer_colour_views[TEST_FRAMEBUFFER_CYCLES];///views of single array slice from image

static VkFormat test_normal_format;
static VkImage test_framebuffer_normal_images[TEST_FRAMEBUFFER_CYCLES];
static VkImageView test_framebuffer_normal_views[TEST_FRAMEBUFFER_CYCLES];///views of single array slice from image


static cvm_vk_managed_buffer test_managed_buffer;

static cvm_vk_staging_buffer test_staging_buffer;

static cvm_vk_transient_buffer test_transient_buffer;


static cvm_managed_mesh test_stellated_octahedron_mesh;
static cvm_managed_mesh test_stub_stellated_octahedron_mesh;
static cvm_managed_mesh test_cube_mesh;
//static cvm_managed_mesh test_face_mesh;


typedef struct test_geo_uniform_data
{
    matrix4f proj;
    float colour_offsets[4];///1 extra for implicit padding
    float colour_multipliers[4];///1 extra for implicit padding
}
test_geo_uniform_data;

typedef struct test_post_uniform_data
{
    matrix4f inv_proj;
    float inv_screen_size_x;
    float inv_screen_size_y;
    uint32_t sample_count;
    uint32_t padding;
}
test_post_uniform_data;

static void create_test_descriptor_set_layouts(void)
{
    VkDescriptorSetLayoutCreateInfo geo_layout_create_info=(VkDescriptorSetLayoutCreateInfo)
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

    cvm_vk_create_descriptor_set_layout(&test_geo_descriptor_set_layout,&geo_layout_create_info);

    VkDescriptorSetLayoutCreateInfo post_layout_create_info=(VkDescriptorSetLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .bindingCount=4,
        .pBindings=(VkDescriptorSetLayoutBinding[4])
        {
            {
                .binding=0,
                .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount=1,
                .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=NULL
            },
            {
                .binding=1,
                .descriptorType=VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount=1,
                .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=NULL
            },
            {
                .binding=2,
                .descriptorType=VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount=1,
                .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=NULL
            },
            {
                .binding=3,
                .descriptorType=VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount=1,
                .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=NULL
            }
        }
    };

    cvm_vk_create_descriptor_set_layout(&test_post_descriptor_set_layout,&post_layout_create_info);
}

static void create_test_descriptor_sets(uint32_t swapchain_image_count)
{
    uint32_t i;
    VkDescriptorSetLayout set_layouts[swapchain_image_count];

    ///pool size dependent upon swapchain image count so must go here
    VkDescriptorPoolCreateInfo pool_create_info=(VkDescriptorPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,///by not specifying individual free must reset whole pool (which is fine)
        .maxSets=swapchain_image_count*3,///1 for geo, 1 for post
        .poolSizeCount=2,
        .pPoolSizes=(VkDescriptorPoolSize[2])
        {
            {
                .type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount=swapchain_image_count*2///1 for geo, 1 for post
            },
            {
                .type=VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount=swapchain_image_count*3///3 for post
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

    for(i=0;i<swapchain_image_count;i++)set_layouts[i]=test_geo_descriptor_set_layout;
    test_geo_descriptor_sets=malloc(swapchain_image_count*sizeof(VkDescriptorSet));
    cvm_vk_allocate_descriptor_sets(test_geo_descriptor_sets,&descriptor_set_allocate_info);

    for(i=0;i<swapchain_image_count;i++)set_layouts[i]=test_post_descriptor_set_layout;
    test_post_descriptor_sets=malloc(swapchain_image_count*sizeof(VkDescriptorSet));
    cvm_vk_allocate_descriptor_sets(test_post_descriptor_sets,&descriptor_set_allocate_info);
}

static void update_and_write_test_geo_descriptors(uint32_t swapchain_image,VkDeviceSize uniform_offset)
{
    VkWriteDescriptorSet write=(VkWriteDescriptorSet)
    {
        .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext=NULL,
        .dstSet=test_geo_descriptor_sets[swapchain_image],
        .dstBinding=0,
        .dstArrayElement=0,
        .descriptorCount=1,
        .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo=NULL,
        .pBufferInfo=(VkDescriptorBufferInfo[1])
        {
            {
                .buffer=test_transient_buffer.buffer,
                .offset=uniform_offset,
                .range=sizeof(test_geo_uniform_data)
            }
        },
        .pTexelBufferView=NULL
    };

    cvm_vk_write_descriptor_sets(&write,1);
}

static void update_and_write_test_post_descriptors(uint32_t swapchain_image,VkDeviceSize uniform_offset)
{
    VkWriteDescriptorSet writes[2]=
    {
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=test_post_descriptor_sets[swapchain_image],
            .dstBinding=0,
            .dstArrayElement=0,
            .descriptorCount=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo=NULL,
            .pBufferInfo=(VkDescriptorBufferInfo[1])
            {
                {
                    .buffer=test_transient_buffer.buffer,
                    .offset=uniform_offset,
                    .range=sizeof(test_post_uniform_data)
                }
            },
            .pTexelBufferView=NULL
        },
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=test_post_descriptor_sets[swapchain_image],
            .dstBinding=1,
            .dstArrayElement=0,
            .descriptorCount=3,
            .descriptorType=VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .pImageInfo=(VkDescriptorImageInfo[3])
            {
                {
                    .sampler=VK_NULL_HANDLE,
                    .imageView=test_framebuffer_depth_only_views[test_current_framebuffer_index],
                    .imageLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL///HMMMM, actually using depth only
                },
                {
                    .sampler=VK_NULL_HANDLE,
                    .imageView=test_framebuffer_colour_views[test_current_framebuffer_index],
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                {
                    .sampler=VK_NULL_HANDLE,
                    .imageView=test_framebuffer_normal_views[test_current_framebuffer_index],
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
            },
            .pBufferInfo=NULL,
            .pTexelBufferView=NULL
        }
    };

    cvm_vk_write_descriptor_sets(writes,2);
}

static void create_test_pipeline_layouts(void)
{
    VkPipelineLayoutCreateInfo geo_pipeline_create_info=(VkPipelineLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=1,
        .pSetLayouts=&test_geo_descriptor_set_layout,
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

    cvm_vk_create_pipeline_layout(&test_geo_pipeline_layout,&geo_pipeline_create_info);

    VkPipelineLayoutCreateInfo post_pipeline_create_info=(VkPipelineLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=1,
        .pSetLayouts=&test_post_descriptor_set_layout,
        .pushConstantRangeCount=0,
        .pPushConstantRanges=NULL
    };

    cvm_vk_create_pipeline_layout(&test_post_pipeline_layout,&post_pipeline_create_info);
}

static void create_test_render_pass(VkSampleCountFlagBits sample_count)
{
    if(cvm_vk_format_check_optimal_feature_support(VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))test_depth_format=VK_FORMAT_D32_SFLOAT_S8_UINT;
    else test_depth_format=VK_FORMAT_D24_UNORM_S8_UINT;///one of these 2 is required format

    test_colour_format=VK_FORMAT_A2B10G10R10_UNORM_PACK32;///required format

    test_normal_format=VK_FORMAT_A2B10G10R10_UNORM_PACK32;///required format, nome of the snorm formats are required, so probably just have to scale [0,1] to [-1,1]

    VkRenderPassCreateInfo create_info=(VkRenderPassCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .attachmentCount=4,
        .pAttachments=(VkAttachmentDescription[4])
        {
            {
                .flags=0,
                .format=cvm_vk_get_screen_format(),
                .samples=VK_SAMPLE_COUNT_1_BIT,///sample_count not relevant for actual render target (swapchain image)
                .loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,/// is first render pass (ergo the clear above) thus the VK_IMAGE_LAYOUT_UNDEFINED rather than VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                .finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL///VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL so that next pass can also write to swapchain image without transitioning layout
            },
            {
                .flags=0,
                .format=test_depth_format,
                .samples=sample_count,
                .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            },
            {
                .flags=0,
                .format=test_colour_format,
                .samples=sample_count,
                .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
            {
                .flags=0,
                .format=test_normal_format,
                .samples=sample_count,
                .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
        },
        .subpassCount=2,
        .pSubpasses=(VkSubpassDescription[2])
        {
            {
                .flags=0,
                .pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount=0,
                .pInputAttachments=NULL,
                .colorAttachmentCount=2,
                .pColorAttachments=(VkAttachmentReference[2])
                {
                    {
                        .attachment=2,
                        .layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    },
                    {
                        .attachment=3,
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
                .pPreserveAttachments=NULL///this will be important in future!
            },
            {
                .flags=0,
                .pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount=3,
                .pInputAttachments=(VkAttachmentReference[3])
                {
                    {
                        .attachment=1,
                        .layout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,///HMMMM, actually using depth only
                    },
                    {
                        .attachment=2,
                        .layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    },
                    {
                        .attachment=3,
                        .layout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    }
                },
                .colorAttachmentCount=1,
                .pColorAttachments=(VkAttachmentReference[1])
                {
                    {
                        .attachment=0,
                        .layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    },
                },
                .pResolveAttachments=NULL,
                .pDepthStencilAttachment=NULL,
                .preserveAttachmentCount=0,
                .pPreserveAttachments=NULL///this will be important in future!
            }
        },
        .dependencyCount=6,
        .pDependencies=(VkSubpassDependency[6])
        {
            #warning needs review now that there are multiple internal stages
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
                .dstSubpass=1,
                .srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            },
            {
                .srcSubpass=0,
                .dstSubpass=1,
                .srcStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            },
            {
                .srcSubpass=1,
                .dstSubpass=VK_SUBPASS_EXTERNAL,
                ///not sure on specific dependencies related to swapchain images being read/written by presentation engine
                .srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            },
            {
                .srcSubpass=1,
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

static void create_test_pipelines(VkSampleCountFlagBits sample_count,float min_sample_shading)
{
    VkGraphicsPipelineCreateInfo geo_create_info=(VkGraphicsPipelineCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .stageCount=2,
        .pStages=(VkPipelineShaderStageCreateInfo[2])
        {
            test_geo_vertex_stage,
            test_geo_fragment_stage
        },///could be null then provided by each actual pipeline type (for sets of pipelines only variant based on shader)
        .pVertexInputState=cvm_vk_get_mesh_vertex_input_state(0),///position only
        .pInputAssemblyState=cvm_vk_get_default_input_assembly_state(),
        .pTessellationState=NULL,///not needed (yet)
        .pViewportState=cvm_vk_get_default_viewport_state(),
        .pRasterizationState=cvm_vk_get_default_raster_state(true),
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
        .pDepthStencilState=cvm_vk_get_default_depth_stencil_state(),
        .pColorBlendState=(VkPipelineColorBlendStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .logicOpEnable=VK_FALSE,
                .logicOp=VK_LOGIC_OP_COPY,
                .attachmentCount=2,///must equal colorAttachmentCount in subpass
                .pAttachments= (VkPipelineColorBlendAttachmentState[2])
                {
                    cvm_vk_get_default_no_blend_state(),
                    cvm_vk_get_default_no_blend_state()
                },
                .blendConstants={0.0,0.0,0.0,0.0}
            }
        },
        .pDynamicState=NULL,
        .layout=test_geo_pipeline_layout,
        .renderPass=test_render_pass,
        .subpass=0,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };

    ///can pass above into multiple functions as parameter
    cvm_vk_create_graphics_pipeline(&test_geo_pipeline,&geo_create_info);

    VkPipelineShaderStageCreateInfo post_frag;
    if(sample_count==VK_SAMPLE_COUNT_1_BIT)post_frag=test_post_fragment_stage_1;
    else if(sample_count==VK_SAMPLE_COUNT_2_BIT)post_frag=test_post_fragment_stage_2;
    else if(sample_count==VK_SAMPLE_COUNT_4_BIT)post_frag=test_post_fragment_stage_4;
    else
    {
        fprintf(stderr,"SELECTED TEST MSAA MODE NOT SUPPORTED\n");
        exit(-1);
    }


    VkGraphicsPipelineCreateInfo post_create_info=(VkGraphicsPipelineCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .stageCount=2,
        .pStages=(VkPipelineShaderStageCreateInfo[2])
        {
            cvm_vk_get_default_fullscreen_vertex_stage(),
            post_frag
        },///could be null then provided by each actual pipeline type (for sets of pipelines only variant based on shader)
        .pVertexInputState=cvm_vk_get_empty_vertex_input_state(),
        .pInputAssemblyState=cvm_vk_get_default_input_assembly_state(),
        .pTessellationState=NULL,///not needed (yet)
        .pViewportState=cvm_vk_get_default_viewport_state(),
        .pRasterizationState=cvm_vk_get_default_raster_state(false),
        .pMultisampleState=cvm_vk_get_default_multisample_state(),
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
                .pAttachments= (VkPipelineColorBlendAttachmentState[1])
                {
                    cvm_vk_get_default_no_blend_state()
                },
                .blendConstants={0.0,0.0,0.0,0.0}
            }
        },
        .pDynamicState=NULL,
        .layout=test_post_pipeline_layout,
        .renderPass=test_render_pass,
        .subpass=1,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };

    ///can pass above into multiple functions as parameter
    cvm_vk_create_graphics_pipeline(&test_post_pipeline,&post_create_info);
}

static void create_test_framebuffers(uint32_t swapchain_image_count)
{
    uint32_t i,j;
    VkImageView attachments[4];

    test_current_framebuffer_index=0;

    for(i=0;i<TEST_FRAMEBUFFER_CYCLES;i++)
    {
        test_framebuffers[i]=malloc(swapchain_image_count*sizeof(VkFramebuffer));

        for(j=0;j<swapchain_image_count;j++)
        {
            attachments[0]=cvm_vk_get_swapchain_image_view(j);
            attachments[1]=test_framebuffer_depth_stencil_views[i];
            attachments[2]=test_framebuffer_colour_views[i];
            attachments[3]=test_framebuffer_normal_views[i];

            cvm_vk_create_default_framebuffer(test_framebuffers[i]+j,test_render_pass,attachments,4);
        }
    }
}



/// these need to be recreated whenever swapchain image changes size
/// unless we allocate at max and alter with image views... investigate potential of this, use max size, let user set max_size from startup options?
static void create_test_framebuffer_images(VkSampleCountFlagBits sample_count)
{
    cvm_vk_create_default_framebuffer_images(test_framebuffer_depth_images,test_depth_format,TEST_FRAMEBUFFER_CYCLES,sample_count,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
    cvm_vk_create_default_framebuffer_images(test_framebuffer_colour_images,test_colour_format,TEST_FRAMEBUFFER_CYCLES,sample_count,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
    cvm_vk_create_default_framebuffer_images(test_framebuffer_normal_images,test_normal_format,TEST_FRAMEBUFFER_CYCLES,sample_count,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

    VkImage images[6]={
        test_framebuffer_depth_images[0],test_framebuffer_depth_images[1],
        test_framebuffer_colour_images[0],test_framebuffer_colour_images[1],
        test_framebuffer_normal_images[0],test_framebuffer_normal_images[1]};
    cvm_vk_create_and_bind_memory_for_images(&test_framebuffer_image_memory,images,3);

    cvm_vk_create_default_framebuffer_image_views(test_framebuffer_depth_stencil_views,test_framebuffer_depth_images,test_depth_format,VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,TEST_FRAMEBUFFER_CYCLES);
    cvm_vk_create_default_framebuffer_image_views(test_framebuffer_depth_only_views,test_framebuffer_depth_images,test_depth_format,VK_IMAGE_ASPECT_DEPTH_BIT,TEST_FRAMEBUFFER_CYCLES);
    cvm_vk_create_default_framebuffer_image_views(test_framebuffer_colour_views,test_framebuffer_colour_images,test_colour_format,VK_IMAGE_ASPECT_COLOR_BIT,TEST_FRAMEBUFFER_CYCLES);
    cvm_vk_create_default_framebuffer_image_views(test_framebuffer_normal_views,test_framebuffer_normal_images,test_normal_format,VK_IMAGE_ASPECT_COLOR_BIT,TEST_FRAMEBUFFER_CYCLES);
}

void initialise_test_render_data()
{
    create_test_descriptor_set_layouts();

    create_test_pipeline_layouts();

    cvm_vk_create_shader_stage_info(&test_geo_vertex_stage,"cvm_shared/shaders/test_geo_vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(&test_geo_fragment_stage,"cvm_shared/shaders/test_geo_frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    cvm_vk_create_shader_stage_info(&test_post_fragment_stage_1,"cvm_shared/shaders/test_post_frag_1.spv",VK_SHADER_STAGE_FRAGMENT_BIT);
    cvm_vk_create_shader_stage_info(&test_post_fragment_stage_2,"cvm_shared/shaders/test_post_frag_2.spv",VK_SHADER_STAGE_FRAGMENT_BIT);
    cvm_vk_create_shader_stage_info(&test_post_fragment_stage_4,"cvm_shared/shaders/test_post_frag_4.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    cvm_vk_create_module_data(&test_module_data,0,2);

    cvm_vk_managed_buffer_create(&test_managed_buffer,65536,10,16,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,false,false);
    ///VK_BUFFER_USAGE_TRANSFER_DST_BIT only necessary if not UMA system, but is it even possible to set this before finding that out??
    /// may need a pass where it ONLY looks for device only memory with VK_BUFFER_USAGE_TRANSFER_DST_BIT on by default
    ///then, if that doesn't exist, removes the VK_BUFFER_USAGE_TRANSFER_DST_BIT flag and accepts host accessible memory
    ///     ^ this will absolutely fuck with testing though, will need to add proper compile flag for simulating non-uma

    cvm_vk_transient_buffer_create(&test_transient_buffer,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    cvm_vk_staging_buffer_create(&test_staging_buffer,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    //create_test_vertex_buffer();
    cvm_managed_mesh_create(&test_stellated_octahedron_mesh,&test_managed_buffer,"cvm_shared/resources/stellated_octahedron.mesh",CVM_MESH_ADGACENCY|CVM_MESH_PER_FACE_MATERIAL,true);
    cvm_managed_mesh_create(&test_stub_stellated_octahedron_mesh,&test_managed_buffer,"cvm_shared/resources/stub_stellated_octahedron.mesh",CVM_MESH_ADGACENCY|CVM_MESH_PER_FACE_MATERIAL,true);
    cvm_managed_mesh_create(&test_cube_mesh,&test_managed_buffer,"cvm_shared/resources/cube.mesh",CVM_MESH_ADGACENCY|CVM_MESH_PER_FACE_MATERIAL,true);
//    cvm_managed_mesh_create(&test_face_mesh,&test_managed_buffer,"cvm_shared/resources/face.mesh",CVM_MESH_ADGACENCY|CVM_MESH_PER_FACE_MATERIAL,true);
}

void terminate_test_render_data()
{
    cvm_vk_destroy_shader_stage_info(&test_geo_vertex_stage);
    cvm_vk_destroy_shader_stage_info(&test_geo_fragment_stage);

    cvm_vk_destroy_shader_stage_info(&test_post_fragment_stage_1);
    cvm_vk_destroy_shader_stage_info(&test_post_fragment_stage_2);
    cvm_vk_destroy_shader_stage_info(&test_post_fragment_stage_4);

    cvm_vk_destroy_pipeline_layout(test_geo_pipeline_layout);
    cvm_vk_destroy_pipeline_layout(test_post_pipeline_layout);

    cvm_vk_destroy_descriptor_set_layout(test_geo_descriptor_set_layout);
    cvm_vk_destroy_descriptor_set_layout(test_post_descriptor_set_layout);

    cvm_vk_destroy_module_data(&test_module_data);

    cvm_managed_mesh_destroy(&test_stellated_octahedron_mesh);
    cvm_managed_mesh_destroy(&test_stub_stellated_octahedron_mesh);
    cvm_managed_mesh_destroy(&test_cube_mesh);
//    cvm_managed_mesh_destroy(&test_face_mesh);

    cvm_vk_managed_buffer_destroy(&test_managed_buffer);

    cvm_vk_transient_buffer_destroy(&test_transient_buffer);

    cvm_vk_staging_buffer_destroy(&test_staging_buffer);
}


void initialise_test_swapchain_dependencies(VkSampleCountFlagBits sample_count)
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();

    create_test_render_pass(sample_count);

    create_test_pipelines(sample_count,0.0);

    create_test_framebuffer_images(sample_count);

    create_test_framebuffers(swapchain_image_count);

    create_test_descriptor_sets(swapchain_image_count);

    cvm_vk_resize_module_graphics_data(&test_module_data,0);

    uint32_t uniform_size=0;
    uniform_size+=cvm_vk_transient_buffer_get_rounded_allocation_size(&test_transient_buffer,sizeof(test_geo_uniform_data));
    uniform_size+=cvm_vk_transient_buffer_get_rounded_allocation_size(&test_transient_buffer,sizeof(test_post_uniform_data));
    cvm_vk_transient_buffer_update(&test_transient_buffer,uniform_size,swapchain_image_count);

    uint32_t staging_size=65536;
    cvm_vk_staging_buffer_update(&test_staging_buffer,staging_size,swapchain_image_count);

    test_managed_buffer.staging_buffer=&test_staging_buffer;
}

void terminate_test_swapchain_dependencies()
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    uint32_t i,j;

    cvm_vk_destroy_pipeline(test_geo_pipeline);
    cvm_vk_destroy_pipeline(test_post_pipeline);

    for(i=0;i<swapchain_image_count;i++) for(j=0;j<TEST_FRAMEBUFFER_CYCLES;j++) cvm_vk_destroy_framebuffer(test_framebuffers[j][i]);

    for(i=0;i<TEST_FRAMEBUFFER_CYCLES;i++)
    {
        cvm_vk_destroy_image_view(test_framebuffer_depth_stencil_views[i]);
        cvm_vk_destroy_image_view(test_framebuffer_depth_only_views[i]);
        cvm_vk_destroy_image_view(test_framebuffer_colour_views[i]);
        cvm_vk_destroy_image_view(test_framebuffer_normal_views[i]);

        cvm_vk_destroy_image(test_framebuffer_depth_images[i]);
        cvm_vk_destroy_image(test_framebuffer_colour_images[i]);
        cvm_vk_destroy_image(test_framebuffer_normal_images[i]);
    }

    cvm_vk_free_memory(test_framebuffer_image_memory);

    cvm_vk_destroy_descriptor_pool(test_descriptor_pool);

    free(test_geo_descriptor_sets);
    free(test_post_descriptor_sets);

    for(i=0;i<TEST_FRAMEBUFFER_CYCLES;i++)free(test_framebuffers[i]);

    cvm_vk_destroy_render_pass(test_render_pass);
}

cvm_vk_module_batch * test_render_frame(cvm_camera * c)
{
    cvm_vk_module_batch * batch;
    uint32_t swapchain_image_index;

    VkCommandBuffer scb_geo,scb_post;///used for testing, essentially the secondary command buffer currently in use

    static rotor3f rots[4];
    static uint64_t rand=813;
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






        VkDeviceSize uniforms_offset;


        test_geo_uniform_data * geo_uniforms;

        geo_uniforms = cvm_vk_transient_buffer_get_allocation(&test_transient_buffer,sizeof(test_geo_uniform_data),&uniforms_offset);
        if(geo_uniforms==NULL)puts("FAILED");

        geo_uniforms->proj=*get_view_matrix_pointer(c);///try to avoid per item uniforms, when absolutely necessary (cant be put in push constants) use a linear allocator to distribute pre-allocated ones from here

        geo_uniforms->colour_offsets[0]=0.5;//cos(SDL_GetTicks()*0.005);
        geo_uniforms->colour_offsets[1]=0.5;//cos(SDL_GetTicks()*0.007);
        geo_uniforms->colour_offsets[2]=0.5;//cos(SDL_GetTicks()*0.011);


        geo_uniforms->colour_multipliers[0]=0.5;//cos(SDL_GetTicks()*0.005);
        geo_uniforms->colour_multipliers[1]=0.5;//cos(SDL_GetTicks()*0.007);
        geo_uniforms->colour_multipliers[2]=0.5;//cos(SDL_GetTicks()*0.011);


        update_and_write_test_geo_descriptors(swapchain_image_index,uniforms_offset);



        test_post_uniform_data * post_uniforms;

        post_uniforms = cvm_vk_transient_buffer_get_allocation(&test_transient_buffer,sizeof(test_post_uniform_data),&uniforms_offset);
        if(geo_uniforms==NULL)puts("FAILED");

        post_uniforms->sample_count=2;

        update_and_write_test_post_descriptors(swapchain_image_index,uniforms_offset);




        float transformation[12];

        rotor3f r;
        int index;
        float lerp;


        lerp=(((float)(iter&127))+0.5)/128.0;
        index=(iter>>7)&3;
        if((iter&127)==0)rots[(index+2)&3]=cvm_rng_rotation_3d(&rand);
        iter++;

//        r=r3f_spherical_interp(rots[index],rots[(index+1)&3],lerp);
//        r=r3f_lerp(rots[index],rots[(index+1)&3],lerp);
        r=r3f_bezier_interp(rots[(index+3)&3],rots[index],rots[(index+1)&3],rots[(index+2)&3],lerp);

        cvm_transform_stack_push(&ts);
            cvm_transform_stack_rotate(&ts,r);
//            cvm_transform_stack_offset_position(&ts,(vec3f){.x=0,.y=-0.75,.z=0});


            //cvm_transform_stack_push(&ts);
                //cvm_transform_stack_offset_position(&ts,((vec3f){1,1,1}));
                //cvm_transform_stack_rotate_around_vector(&ts,((vec3f){SQRT_THIRD,SQRT_THIRD,SQRT_THIRD}),((float)(iter&31))*TAU/32);
                cvm_transform_stack_get(&ts,transformation);
//            cvm_transform_stack_pop(&ts);
        cvm_transform_stack_pop(&ts);




        ///definitely not the cleanest function ever
        scb_geo=cvm_vk_module_batch_start_secondary_command_buffer(batch,CVM_VK_MAIN_SUB_BATCH_INDEX,0,
                test_framebuffers[test_current_framebuffer_index][swapchain_image_index],test_render_pass,0);

        cvm_vk_managed_buffer_bind_as_index(scb_geo,&test_managed_buffer,VK_INDEX_TYPE_UINT16);
        cvm_vk_managed_buffer_bind_as_vertex(scb_geo,&test_managed_buffer,0);

        vkCmdBindDescriptorSets(scb_geo,VK_PIPELINE_BIND_POINT_GRAPHICS,test_geo_pipeline_layout,0,1,test_geo_descriptor_sets+swapchain_image_index,0,NULL);

        vkCmdBindPipeline(scb_geo,VK_PIPELINE_BIND_POINT_GRAPHICS,test_geo_pipeline);

        vkCmdPushConstants(scb_geo,test_geo_pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(float)*12,transformation);

        cvm_managed_mesh_render(&test_stub_stellated_octahedron_mesh,scb_geo,1,0);
        //cvm_managed_mesh_render(&test_cube_mesh,scb_geo,1,0);
//        cvm_managed_mesh_render(&test_stellated_octahedron_mesh,scb_geo,1,0);
//        cvm_managed_mesh_render(&test_face_mesh,scb_geo,1,0);

        vkEndCommandBuffer(scb_geo);




        scb_post=cvm_vk_module_batch_start_secondary_command_buffer(batch,CVM_VK_MAIN_SUB_BATCH_INDEX,1,
                test_framebuffers[test_current_framebuffer_index][swapchain_image_index],test_render_pass,1);

        vkCmdBindDescriptorSets(scb_post,VK_PIPELINE_BIND_POINT_GRAPHICS,test_post_pipeline_layout,0,1,test_post_descriptor_sets+swapchain_image_index,0,NULL);

        vkCmdBindPipeline(scb_post,VK_PIPELINE_BIND_POINT_GRAPHICS,test_post_pipeline);

        vkCmdDraw(scb_post,3,1,0,0);

        vkEndCommandBuffer(scb_post);




        cvm_vk_staging_buffer_end(&test_staging_buffer,swapchain_image_index);
        cvm_vk_transient_buffer_end(&test_transient_buffer);

        #warning need to figure out how to incorporate synchronization (semaphore) into dependency, just implicitly handled by module?
        ///     ^ specifically in the case of a queue ownership transfer scheduled by above
        cvm_vk_managed_buffer_submit_all_pending_copy_actions(&test_managed_buffer,batch->graphics_pcb,batch->graphics_pcb);



        /// make this static, and initialise it at the same time as render pass
        VkClearValue clear_values[4];///other clear colours should probably be provided by other chunks of application
        clear_values[0].color=(VkClearColorValue){{0.95f,0.5f,0.75f,1.0f}};
        clear_values[1].depthStencil=(VkClearDepthStencilValue){.depth=0.0,.stencil=0};
        clear_values[2].color=(VkClearColorValue){{1.0f,0.5f,0.75f,0.0f}};
//        clear_values[3].color=(VkClearColorValue){{0.0f,0.0f,1.0f,0.0f}};
        clear_values[3].color=(VkClearColorValue){{SQRT_THIRD,SQRT_THIRD,SQRT_THIRD,0.0f}};

        VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext=NULL,
            .renderPass=test_render_pass,
            .framebuffer=test_framebuffers[test_current_framebuffer_index][swapchain_image_index],
            .renderArea=cvm_vk_get_default_render_area(),
            .clearValueCount=4,
            .pClearValues=clear_values
        };

        vkCmdBeginRenderPass(batch->graphics_pcb,&render_pass_begin_info,VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);///================

        #warning for post processing should be able to pre-record seconadry command buffers with multiple submits, without VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, but with VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT

        vkCmdExecuteCommands(batch->graphics_pcb,1,&scb_geo);

        vkCmdNextSubpass(batch->graphics_pcb,VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        ///can also inline post passes or even use multi-submit pre-recorded secondary command buffers

        vkCmdExecuteCommands(batch->graphics_pcb,1,&scb_post);

        vkCmdEndRenderPass(batch->graphics_pcb);///================



        test_current_framebuffer_index++;
        test_current_framebuffer_index*= test_current_framebuffer_index<TEST_FRAMEBUFFER_CYCLES;///resets to 0
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



