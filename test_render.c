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
static VkPipelineShaderStageCreateInfo test_stages[2];

static VkFramebuffer * test_framebuffers;
static VkImageView test_framebuffer_attachments[1];///first reserved for swapchain, rest are views of locally created images


static cvm_vk_managed_buffer test_buffer;
static cvm_vk_dynamic_buffer_allocation * test_index_allocation;
static cvm_vk_dynamic_buffer_allocation * test_vertex_allocation;

/// actually we want a way to handle uploads from here, and as dynamics are the same as instance data its probably worth having a dedicated buffer for them and uniforms AND upload/transfers
static cvm_vk_ring_buffer test_uniform_buffer;
static uint32_t * test_ring_buffer_acquisitions;

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
                .stageFlags=VK_SHADER_STAGE_VERTEX_BIT,
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
                .buffer=test_uniform_buffer.buffer,
                .offset=offset,
                .range=sizeof(float)*4
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
                .size=sizeof(float)*16
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
                        .format=VK_FORMAT_R8G8B8A8_UNORM,
                        .offset=offsetof(test_render_data,col)
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
                .depthTestEnable=VK_FALSE,
                .depthWriteEnable=VK_FALSE,
                .depthCompareOp=VK_COMPARE_OP_GREATER,
                .depthBoundsTestEnable=VK_TRUE,
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

static void create_test_vertex_buffer(void)
{
    int i,j;
    test_render_data * vertex_data;

    uint16_t index_data[36]=
    {
        0,1,3,
        0,3,2,
        0,4,5,
        0,5,1,
        1,5,7,
        1,7,3,
        2,6,4,
        2,4,0,
        3,7,6,
        3,6,2,
        4,6,7,
        4,7,5
    };

    test_index_allocation=cvm_vk_acquire_dynamic_buffer_allocation(&test_buffer,sizeof(uint16_t)*36);

    memcpy(cvm_vk_get_dynamic_buffer_allocation_mapping(&test_buffer,test_index_allocation),index_data,sizeof(uint16_t)*36);






    test_vertex_allocation=cvm_vk_acquire_dynamic_buffer_allocation(&test_buffer,sizeof(test_render_data)*8);

    vertex_data=cvm_vk_get_dynamic_buffer_allocation_mapping(&test_buffer,test_vertex_allocation);

    for(i=0;i<8;i++)
    {
        for(j=0;j<3;j++)
        {
            vertex_data[i].col[j]=!!(i&1<<j)*255;
            vertex_data[i].pos[j]=((float)(!!(i&1<<j)*2-1))*0.7;
            printf("%f,",vertex_data[i].pos[j]);
        }
        printf("\n");
    }

    cvm_vk_flush_managed_buffer(&test_buffer);
}

void initialise_test_render_data()
{
    create_test_render_pass(cvm_vk_get_screen_format(),VK_SAMPLE_COUNT_1_BIT);

    create_test_descriptor_set_layouts();

    create_test_pipeline_layouts();

    cvm_vk_create_shader_stage_info(test_stages+0,"shaders/test_vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(test_stages+1,"shaders/test_frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    cvm_vk_create_module_data(&test_module_data,false);

    cvm_vk_create_managed_buffer(&test_buffer,65536,10,16,64,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT,false,true);

    cvm_vk_create_ring_buffer(&test_uniform_buffer,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,1024);

    create_test_vertex_buffer();
}

void terminate_test_render_data()
{
    cvm_vk_destroy_render_pass(test_render_pass);

    cvm_vk_destroy_shader_stage_info(test_stages+0);
    cvm_vk_destroy_shader_stage_info(test_stages+1);

    cvm_vk_destroy_pipeline_layout(test_pipeline_layout);

    cvm_vk_destroy_descriptor_set_layout(test_descriptor_set_layout);

    cvm_vk_destroy_module_data(&test_module_data,false);


    cvm_vk_relinquish_dynamic_buffer_allocation(&test_buffer,test_vertex_allocation);
    cvm_vk_relinquish_dynamic_buffer_allocation(&test_buffer,test_index_allocation);

    cvm_vk_destroy_managed_buffer(&test_buffer);

    cvm_vk_destroy_ring_buffer(&test_uniform_buffer);
}


void initialise_test_swapchain_dependencies(void)
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    VkRect2D screen_rectangle=cvm_vk_get_screen_rectangle();

    create_test_pipelines(screen_rectangle,VK_SAMPLE_COUNT_1_BIT,1.0);

    create_test_framebuffers(screen_rectangle,swapchain_image_count);

    create_test_descriptor_sets(swapchain_image_count);

    cvm_vk_resize_module_graphics_data(&test_module_data,0);

    test_ring_buffer_acquisitions=calloc(swapchain_image_count,sizeof(uint32_t));
}

void terminate_test_swapchain_dependencies()
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    uint32_t i;

    cvm_vk_destroy_pipeline(test_pipeline);

    for(i=0;i<swapchain_image_count;i++)
    {
        cvm_vk_destroy_framebuffer(test_framebuffers[i]);
        cvm_vk_relinquish_ring_buffer_space(&test_uniform_buffer,test_ring_buffer_acquisitions+i);
    }

    cvm_vk_destroy_descriptor_pool(test_descriptor_pool);

    free(test_descriptor_sets);
    free(test_framebuffers);
    free(test_ring_buffer_acquisitions);
}

VkPipeline get_test_pipeline(void)
{
    return test_pipeline;
}

VkFramebuffer get_test_framebuffer(uint32_t swapchain_image_index)
{
    return test_framebuffers[swapchain_image_index];
}


cvm_vk_module_graphics_block * test_render_frame(camera * c)
{
    VkCommandBuffer command_buffer;
    uint32_t swapchain_image_index;

//    command_buffer = cvm_vk_begin_module_frame_transfer(&test_module_data,false);
//    if(command_buffer!=VK_NULL_HANDLE)
//    {
//        ///do transfers
//    }


    ///perhaps this should return previous image index as well, such that that value can be used in cleanup (e.g. relinquishing ring buffer space)
    command_buffer = cvm_vk_begin_module_graphics_block(&test_module_data,0,&swapchain_image_index);


    ///float multipliers[4];


    if(command_buffer!=VK_NULL_HANDLE)
    {
        cvm_vk_begin_ring_buffer(&test_uniform_buffer);

        VkDeviceSize uniforms_offset;
        float * uniforms = cvm_vk_get_ring_buffer_allocation(&test_uniform_buffer,sizeof(float)*4,&uniforms_offset);
        //printf("C %u\n",atomic_load(&test_uniform_buffer.space_remaining));
        if(uniforms==NULL)puts("FAILED");

//        uniforms[0]=0.2*sin(SDL_GetTicks()*0.001);
//        uniforms[1]=0.2*cos(SDL_GetTicks()*0.001);
//        uniforms[2]=0.0;
//        uniforms[3]=0.0;

        uniforms[0]=1.0;//0.7+0.3*cos(SDL_GetTicks()*0.005);
        uniforms[1]=1.0;//0.7+0.3*cos(SDL_GetTicks()*0.007);
        uniforms[2]=1.0;//0.7+0.3*cos(SDL_GetTicks()*0.011);
        uniforms[3]=1.0;

        update_and_bind_test_uniforms(swapchain_image_index,uniforms_offset);



        vkCmdPushConstants(command_buffer,test_pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(float)*16,get_view_matrix_buffer(c));

        vkCmdBindDescriptorSets(command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,test_pipeline_layout,0,1,test_descriptor_sets+swapchain_image_index,0,NULL);

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


        ///put following in a function?
        //VkDeviceSize offset=test_vertex_allocation->offset<<test_buffer.base_dynamic_allocation_size_factor;
        //vkCmdBindVertexBuffers(command_buffer,0,1,&test_buffer.buffer,&offset);
        cvm_vk_bind_dymanic_allocation_index(command_buffer,&test_buffer,test_index_allocation,VK_INDEX_TYPE_UINT16);
        cvm_vk_bind_dymanic_allocation_vertex(command_buffer,&test_buffer,test_vertex_allocation,0);

        //vkCmdDraw(command_buffer,4,1,0,0);
        vkCmdDrawIndexed(command_buffer,36,1,0,0,0);

        vkCmdEndRenderPass(command_buffer);///================


        test_ring_buffer_acquisitions[swapchain_image_index]=cvm_vk_end_ring_buffer(&test_uniform_buffer);
    }

    return cvm_vk_end_module_graphics_block(&test_module_data);
}

void test_frame_cleanup(uint32_t swapchain_image_index)
{
    if(swapchain_image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        cvm_vk_relinquish_ring_buffer_space(&test_uniform_buffer,test_ring_buffer_acquisitions+swapchain_image_index);
    }
}



