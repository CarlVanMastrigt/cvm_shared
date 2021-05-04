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


///actual resultant structures defined/created by above
static VkRenderPass test_render_pass;
static VkPipelineLayout test_pipeline_layout;///relevant descriptors, share as much as possible
static VkPipeline test_pipeline;
static VkPipelineShaderStageCreateInfo test_stages[2];

static VkFramebuffer * test_framebuffers;
static uint32_t test_framebuffer_count;///hopefully won't need to store locally
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
                .samples=VK_SAMPLE_COUNT_1_BIT,//sample_count,
                .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,/// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL , VK_IMAGE_LAYOUT_UNDEFINED ?
                .finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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

static void create_test_framebuffers(VkRect2D screen_rectangle,uint32_t swapchain_image_count,VkImageView * swapchain_image_views)
{
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

    for(test_framebuffer_count=0;test_framebuffer_count<swapchain_image_count;test_framebuffer_count++)
    {
        test_framebuffer_attachments[0]=swapchain_image_views[test_framebuffer_count];

        cvm_vk_create_framebuffer(test_framebuffers+test_framebuffer_count,&create_info);
    }
}











///put initialisation of transitory/mutable data inside init, then only alter as part of update

void initialise_test_render_data_ext()
{
    create_test_render_pass(cvm_vk_get_screen_format(),VK_SAMPLE_COUNT_1_BIT);

    create_test_pipeline_layouts();

    cvm_vk_create_shader_stage_info(test_stages+0,"shaders/test_vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(test_stages+1,"shaders/test_frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);
}

void terminate_test_render_data_ext()
{
    cvm_vk_destroy_render_pass(test_render_pass);

    cvm_vk_destroy_shader_stage_info(test_stages+0);
    cvm_vk_destroy_shader_stage_info(test_stages+1);

    cvm_vk_destroy_pipeline_layout(test_pipeline_layout);
}


void initialise_test_swapchain_dependencies_ext(VkRect2D screen_rectangle,uint32_t swapchain_image_count,VkImageView * swapchain_image_views)
{
    uint32_t i;

    create_test_pipelines(screen_rectangle,VK_SAMPLE_COUNT_1_BIT,1.0);

    create_test_framebuffers(screen_rectangle,swapchain_image_count,swapchain_image_views);
}

void terminate_test_swapchain_dependencies_ext()
{
    uint32_t i;

    cvm_vk_destroy_pipeline(test_pipeline);

    for(i=0;i<test_framebuffer_count;i++)
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


void update_test_render_resources(bool screen_resized)
{
    //
}

void test_render_ext(VkCommandBuffer command_buffer,uint32_t swapchain_image_index,VkRect2D screen_rectangle, const VkBuffer * vertex_buffer)
{
    VkClearValue clear_value;///other clear colours should probably be provided by other chunks of application
    clear_value.color=(VkClearColorValue){{0.95f,0.5f,0.75f,1.0f}};

    VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext=NULL,
        .renderPass=test_render_pass,
        .framebuffer=test_framebuffers[swapchain_image_index],
        .renderArea=screen_rectangle,
        .clearValueCount=1,
        .pClearValues= &clear_value
    };

    vkCmdBeginRenderPass(command_buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

    vkCmdBindPipeline(command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,test_pipeline);

    VkDeviceSize offset=0;
    vkCmdBindVertexBuffers(command_buffer,0,1,vertex_buffer,&offset);

    vkCmdDraw(command_buffer,4,1,0,0);

    vkCmdEndRenderPass(command_buffer);///================
}

/// need a good way to get VkDevice here, could actually use external initialisation functions, need to work out naming scheme though,
///     ^ possible naming pairs: initialise/terminate create/destroy start/stop/end reference/derefernce link/unlink establish/abolish erect/dismantle
/// also need to work out how to handle out of date data
/// need a good way to manage screen size, wrt render-passes & sub-passes vs pipelines, maybe only use base struct (VkRect2D? or even self defined width and height struct)
///     ^ rectangle/width/height would allow sub-modules/passes to define their own depth ranges... (probably a good idea!)
/// allowing sub-modules to define their own MSAA settings fits well with above, though both then need a robust way to handle being out of date and recreated
///     ^ maybe as part of render step? or even as separate update step?
///     ^ slight modification to update might allow destruction and recreation of pipelines and passes to happen completely seperately from cvm_vk (it isnt at the moment)
///         ^ need to ensure lingering data from other parts of engine in undestroyed parts wont cause problems (e.g. renderpass referencing deleted swapchain image)
///     ^ generally this separation would be a good idea, would allow some passes to have non-screen size targets and so not require rebuild/recreation on window resize, same goes for overlay when msaa changes
/// the need for render pass objects (which include multisample state) is a pain in the ass, means need to feed render pass info into
/// have different recreation flags, such that module need only recreate renderpass when it actually changes (e.g. MSAA changes)
/// also of note is that viewport changes relate to modules differently to msaa changes, so different pathways on per-module basis really are needed

/// if only 2 cases that can trigger rebuild are resizing swapchain and altering MSAA, it may be okay to trigger same resource rebuild as there is a lot of overlap (framebuffer rebuild for both)
/// and although render passes dont need to be rebuilt after resize, pipelines do which is likely more expensive, also msaa change requires everything to be rebuilt
/// add dirt flag for these things to render passes which get checked? could also just have separate function for when either changes? (neither is great tbh)

/// when screen size changes, rebuild: framebuffers, pipelines & swapchain
/// when sample count changes, rebuild: framebuffers, pipelines & render passes (render passes requires pipeline rebuild anyway)
/// ^ in both cases want a way to know if rebuild is necessary for this submodule and handle resources inteligently
///






