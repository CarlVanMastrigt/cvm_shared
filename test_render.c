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

static VkGraphicsPipelineCreateInfo test_pipeline_creation_info;///where everything gets stored, contents should be set in initialise

static VkPipelineShaderStageCreateInfo test_stages[2];
static VkPipelineLayout test_pipeline_layout;///relevant descriptors, share as much as possible
static VkPipeline test_pipeline;



static VkPipelineVertexInputStateCreateInfo test_vert_input_info=(VkPipelineVertexInputStateCreateInfo)
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
};

static VkPipelineInputAssemblyStateCreateInfo test_input_assembly_info=(VkPipelineInputAssemblyStateCreateInfo)
{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext=NULL,
    .flags=0,
    .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,///VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    .primitiveRestartEnable=VK_FALSE
};

static VkPipelineRasterizationStateCreateInfo test_rasterization_state_info=(VkPipelineRasterizationStateCreateInfo)
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

static VkPipelineLayoutCreateInfo test_layout_creation_info=(VkPipelineLayoutCreateInfo)
{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext=NULL,
    .flags=0,
    .setLayoutCount=0,
    .pSetLayouts=NULL,
    .pushConstantRangeCount=0,
    .pPushConstantRanges=NULL
};

/// multisample should be provided by base/parent part of module so as to be adaptable on a per-module basis
/// need a way to keep track of whether draw command that pipelines created before updating MSAA settings have become invalid
/// use same approach to track viewport resizing, report error if current "out of date" is ever hit after doing so
static VkPipelineMultisampleStateCreateInfo test_multisample_state_info=(VkPipelineMultisampleStateCreateInfo)
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

static VkPipelineColorBlendStateCreateInfo test_blend_state_info=(VkPipelineColorBlendStateCreateInfo)
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
};




void initialise_test_render_data_ext(VkDevice device,VkRenderPass render_pass,VkPipelineViewportStateCreateInfo * viewport_state_info)//,VkPipelineMultisampleStateCreateInfo * multisample_state_info
{
    CVM_VK_CHECK(vkCreatePipelineLayout(device,&test_layout_creation_info,NULL,&test_pipeline_layout));

    test_stages[0]=cvm_vk_initialise_shader_stage_creation_info("shaders/test_vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    test_stages[1]=cvm_vk_initialise_shader_stage_creation_info("shaders/test_frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    test_pipeline_creation_info=(VkGraphicsPipelineCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .stageCount=2,
        .pStages=test_stages,
        .pVertexInputState=&test_vert_input_info,
        .pInputAssemblyState=&test_input_assembly_info,
        .pTessellationState=NULL,
        .pViewportState= viewport_state_info,
        .pRasterizationState=&test_rasterization_state_info,
        .pMultisampleState=&test_multisample_state_info,
        .pDepthStencilState=NULL,
        .pColorBlendState=&test_blend_state_info,
        .pDynamicState=NULL,
        .layout=test_pipeline_layout,
        .renderPass=render_pass,
        .subpass=0,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };
}

void terminate_test_render_data_ext(VkDevice device)
{
    cvm_vk_terminate_shader_stage_creation_info(test_stages+0);
    cvm_vk_terminate_shader_stage_creation_info(test_stages+1);

    vkDestroyPipelineLayout(device,test_pipeline_layout,NULL);
}


void create_test_pipeline(VkDevice device)
{
    CVM_VK_CHECK(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&test_pipeline_creation_info,NULL,&test_pipeline));
}

void destroy_test_pipeline(VkDevice device)
{
    vkDestroyPipeline(device,test_pipeline,NULL);
}

VkPipeline get_test_pipeline(void)
{
    return test_pipeline;
}

/// need a good way to get VkDevice here, could actually use external initialisation functions, need to work out naming scheme though, initialise/terminate/create/delete/start/stop/end
/// also need to work out how to handle out of date data
/// need a good way to manage screen size, wrt render-passes & sub-passes vs pipelines, maybe only use base struct (VkRect2D? or even self defined width and height struct)
///     ^ rectangle/width/height would allow sub-modules/passes to define their own depth ranges... (probably a good idea!)
/// allowing sub-modules to define their own MSAA settings fits well with above, though both then need a robust way to handle being out of date and recreated
///     ^ maybe as part of render step? or even as separate update step?






