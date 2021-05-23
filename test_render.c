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

/// should make all magig numbers const static integers (e.g. num attachments)

static cvm_vk_module_data test_module_data;

///actual resultant structures defined/created by above
static VkRenderPass test_render_pass;
static VkPipelineLayout test_pipeline_layout;///relevant descriptors, share as much as possible
static VkPipeline test_pipeline;
static VkPipelineShaderStageCreateInfo test_stages[2];

static VkFramebuffer * test_framebuffers;
static VkImageView test_framebuffer_attachments[1];///first reserved for swapchain, rest are views of locally created images





static void create_test_pipeline_layouts(void)
{
    VkPipelineLayoutCreateInfo create_info=(VkPipelineLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=0,
        .pSetLayouts=NULL,
        .pushConstantRangeCount=0,
        .pPushConstantRanges=NULL
    };

    cvm_vk_create_pipeline_layout(&test_pipeline_layout,&create_info);
}

static void create_test_render_pass(VkFormat swapchain_format,VkSampleCountFlagBits sample_count)
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
                .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,/// is first render pass (ergo the clear above) thus the VK_IMAGE_LAYOUT_UNDEFINED rather than VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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
                .srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,///must be the same as wait stage in submit (the one associated w/ the relevant semaphore)
                .dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask=0,
                .dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            },
            {
                .srcSubpass=0,
                .dstSubpass=VK_SUBPASS_EXTERNAL,
                .srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,///dont know which stage in present uses image data so use all
                .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask=VK_ACCESS_MEMORY_READ_BIT,
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
        .pStages=test_stages,///could be null then provided by each actual pipeline type (for sets of pipelines only variant based on shader)
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
                        .stride=sizeof(test_render_data),
                        .inputRate=VK_VERTEX_INPUT_RATE_VERTEX
                    }
                },
                .vertexAttributeDescriptionCount=2,
                .pVertexAttributeDescriptions=(VkVertexInputAttributeDescription[2])
                {
                    {
                        .location=0,
                        .binding=0,
                        .format=VK_FORMAT_R32G32B32_SFLOAT,
                        .offset=offsetof(test_render_data,pos)
                    },
                    {
                        .location=1,
                        .binding=0,
                        .format=VK_FORMAT_R32G32B32_SFLOAT,
                        .offset=offsetof(test_render_data,c)
                    }
                }
            }
        },
        .pInputAssemblyState=(VkPipelineInputAssemblyStateCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext=NULL,
                .flags=0,
                .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,///VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
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

static void create_test_framebuffers(VkRect2D screen_rectangle)
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    uint32_t i;

    VkFramebufferCreateInfo create_info=(VkFramebufferCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .renderPass=test_render_pass,
        .attachmentCount=1,
        .pAttachments=test_framebuffer_attachments,
        .width=screen_rectangle.extent.width,
        .height=screen_rectangle.extent.height,
        .layers=1
    };

    test_framebuffers=malloc(swapchain_image_count*sizeof(VkFramebuffer));

    for(i=0;i<swapchain_image_count;i++)
    {
        test_framebuffer_attachments[0]=cvm_vk_get_swapchain_image_view(i);

        cvm_vk_create_framebuffer(test_framebuffers+i,&create_info);
    }
}











///put initialisation of transitory/mutable data inside init, then only alter as part of update

void initialise_test_render_data_ext()
{
    create_test_render_pass(cvm_vk_get_screen_format(),VK_SAMPLE_COUNT_1_BIT);

    create_test_pipeline_layouts();

    cvm_vk_create_shader_stage_info(test_stages+0,"shaders/test_vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(test_stages+1,"shaders/test_frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    cvm_vk_create_module_data(&test_module_data,false);
}

void terminate_test_render_data_ext()
{
    cvm_vk_destroy_render_pass(test_render_pass);

    cvm_vk_destroy_shader_stage_info(test_stages+0);
    cvm_vk_destroy_shader_stage_info(test_stages+1);

    cvm_vk_destroy_pipeline_layout(test_pipeline_layout);

    cvm_vk_destroy_module_data(&test_module_data,false);
}


void initialise_test_swapchain_dependencies_ext(void)
{
    VkRect2D screen_rectangle=cvm_vk_get_screen_rectangle();

    create_test_pipelines(screen_rectangle,VK_SAMPLE_COUNT_1_BIT,1.0);

    create_test_framebuffers(screen_rectangle);

    cvm_vk_resize_module_graphics_data(&test_module_data,0);
}

void terminate_test_swapchain_dependencies_ext()
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    uint32_t i;

    cvm_vk_destroy_pipeline(test_pipeline);

    for(i=0;i<swapchain_image_count;i++)
    {
        cvm_vk_destroy_framebuffer(test_framebuffers[i]);
    }

    free(test_framebuffers);
}

VkPipeline get_test_pipeline(void)
{
    return test_pipeline;
}

VkFramebuffer get_test_framebuffer(uint32_t swapchain_image_index)
{
    return test_framebuffers[swapchain_image_index];
}


cvm_vk_module_graphics_block * test_render_frame(void)
{
    VkCommandBuffer command_buffer;
    uint32_t swapchain_image_index;

//    command_buffer = cvm_vk_begin_module_frame_transfer(&test_module_data,false);
//    if(command_buffer!=VK_NULL_HANDLE)
//    {
//        ///do transfers
//    }

    command_buffer = cvm_vk_begin_module_graphics_block(&test_module_data,0,&swapchain_image_index);
    if(command_buffer!=VK_NULL_HANDLE)
    {
        ///do graphics
        VkClearValue clear_value;///other clear colours should probably be provided by other chunks of application
        clear_value.color=(VkClearColorValue){{0.95f,0.5f,0.75f,1.0f}};

        VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext=NULL,
            .renderPass=test_render_pass,
            .framebuffer=test_framebuffers[swapchain_image_index],
            .renderArea=cvm_vk_get_screen_rectangle(),
            .clearValueCount=1,
            .pClearValues= &clear_value
        };

        vkCmdBeginRenderPass(command_buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

        vkCmdBindPipeline(command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,test_pipeline);

        VkDeviceSize offset=0;
        vkCmdBindVertexBuffers(command_buffer,0,1,get_test_buffer(),&offset);

        vkCmdDraw(command_buffer,4,1,0,0);

        vkCmdEndRenderPass(command_buffer);///================
    }

    return cvm_vk_end_module_graphics_block(&test_module_data);
}





