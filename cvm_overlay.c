/**
Copyright 2020,2021,2022 Carl van Mastrigt

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

static VkImage overlay_transparent_image;
static VkImageView overlay_transparent_image_view;///views of single array slice from image
static cvm_vk_image_atlas overlay_transparent_image_atlas;

static VkImage overlay_colour_image;
static VkImageView overlay_colour_image_view;///views of single array slice from image
static cvm_vk_image_atlas overlay_colour_image_atlas;

static cvm_vk_transient_buffer overlay_transient_buffer;
static cvm_vk_staging_buffer overlay_staging_buffer;
static uint32_t max_overlay_elements=4096;

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
                    .imageView=overlay_transparent_image_view,
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
                    .range=OVERLAY_NUM_COLOURS*4*sizeof(float)
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

static void create_overlay_render_pass(void)
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
                .format=cvm_vk_get_screen_format(),
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
            cvm_vk_get_default_colour_attachment_dependency(VK_SUBPASS_EXTERNAL,0),
            cvm_vk_get_default_colour_attachment_dependency(0,VK_SUBPASS_EXTERNAL)
        }
    };

    cvm_vk_create_render_pass(&overlay_render_pass,&create_info);
}

static void create_overlay_pipelines(void)
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
                .vertexBindingDescriptionCount=1,
                .pVertexBindingDescriptions= (VkVertexInputBindingDescription[1])
                {
                    {
                        .binding=0,
                        .stride=sizeof(cvm_overlay_render_data),
                        .inputRate=VK_VERTEX_INPUT_RATE_INSTANCE
                    }
                },
                .vertexAttributeDescriptionCount=3,
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
                        .format=VK_FORMAT_R32G32_UINT,
                        .offset=offsetof(cvm_overlay_render_data,data1)
                    },
                    {
                        .location=2,
                        .binding=0,
                        .format=VK_FORMAT_R16G16B16A16_UINT,
                        .offset=offsetof(cvm_overlay_render_data,data2)
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
                .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,///not the default
                .primitiveRestartEnable=VK_FALSE
            }
        },
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
                .pAttachments= (VkPipelineColorBlendAttachmentState[])
                {
                    cvm_vk_get_default_alpha_blend_state()
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

static void create_overlay_framebuffers(uint32_t swapchain_image_count)
{
    uint32_t i;
    VkImageView attachments[1];

    overlay_framebuffers=malloc(swapchain_image_count*sizeof(VkFramebuffer));

    for(i=0;i<swapchain_image_count;i++)
    {
        attachments[0]=cvm_vk_get_swapchain_image_view(i);

        cvm_vk_create_default_framebuffer(overlay_framebuffers+i,overlay_render_pass,attachments,1);
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
    cvm_vk_create_image(&overlay_transparent_image,&image_creation_info);

    image_creation_info.format=VK_FORMAT_R8G8B8A8_UNORM;
    image_creation_info.usage=VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;///conditionally VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ???
    image_creation_info.extent.width=c_w;
    image_creation_info.extent.height=c_h;
    cvm_vk_create_image(&overlay_colour_image,&image_creation_info);

    VkImage images[2]={overlay_transparent_image,overlay_colour_image};
    cvm_vk_create_and_bind_memory_for_images(&overlay_image_memory,images,2,0);



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
    view_creation_info.image=overlay_transparent_image;
    cvm_vk_create_image_view(&overlay_transparent_image_view,&view_creation_info);

    view_creation_info.format=VK_FORMAT_R8G8B8A8_UNORM;
    view_creation_info.image=overlay_colour_image;
    cvm_vk_create_image_view(&overlay_colour_image_view,&view_creation_info);


    cvm_vk_create_image_atlas(&overlay_transparent_image_atlas,overlay_transparent_image,overlay_transparent_image_view,sizeof(uint8_t),t_w,t_h,true);
    cvm_vk_create_image_atlas(&overlay_colour_image_atlas,overlay_colour_image,overlay_colour_image_view,sizeof(uint8_t)*4,c_w,c_h,false);

    overlay_transparent_image_atlas.staging_buffer=&overlay_staging_buffer;
    overlay_colour_image_atlas.staging_buffer=&overlay_staging_buffer;

    #warning as images ALWAYS need staging (even on uma) could make staging a creation parameter??
    ///perhaps not as it limits the ability to use it for a more generalised purpose...
    ///could move general section out and use a meta structure encompassing the generalised image atlas?
}

void initialise_overlay_render_data(void)
{
    create_overlay_render_pass();

    create_overlay_descriptor_set_layouts();

    create_overlay_pipeline_layouts();

    cvm_vk_create_shader_stage_info(&overlay_vertex_stage,"cvm_shared/shaders/overlay.vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(&overlay_fragment_stage,"cvm_shared/shaders/overlay.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    cvm_vk_create_module_data(&overlay_module_data,1);

    cvm_vk_transient_buffer_create(&overlay_transient_buffer,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cvm_vk_staging_buffer_create(&overlay_staging_buffer,VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    create_overlay_images(1024,1024,1024,1024);

    create_overlay_consistent_descriptors();

    cvm_overlay_open_freetype();
}

void terminate_overlay_render_data(void)
{
    cvm_overlay_close_freetype();

    cvm_vk_destroy_image_view(overlay_transparent_image_view);
    cvm_vk_destroy_image_view(overlay_colour_image_view);

    cvm_vk_destroy_image(overlay_transparent_image);
    cvm_vk_destroy_image(overlay_colour_image);

    cvm_vk_free_memory(overlay_image_memory);

    cvm_vk_destroy_image_atlas(&overlay_transparent_image_atlas);
    cvm_vk_destroy_image_atlas(&overlay_colour_image_atlas);


    cvm_vk_destroy_render_pass(overlay_render_pass);

    cvm_vk_destroy_shader_stage_info(&overlay_vertex_stage);
    cvm_vk_destroy_shader_stage_info(&overlay_fragment_stage);

    cvm_vk_destroy_pipeline_layout(overlay_pipeline_layout);

    cvm_vk_destroy_descriptor_pool(overlay_consistent_descriptor_pool);

    cvm_vk_destroy_descriptor_set_layout(overlay_descriptor_set_layout);
    cvm_vk_destroy_descriptor_set_layout(overlay_consistent_descriptor_set_layout);

    cvm_vk_destroy_module_data(&overlay_module_data);

    cvm_vk_transient_buffer_destroy(&overlay_transient_buffer);
    cvm_vk_staging_buffer_destroy(&overlay_staging_buffer);
}


void initialise_overlay_swapchain_dependencies(void)
{
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();

    create_overlay_pipelines();

    create_overlay_framebuffers(swapchain_image_count);

    create_overlay_descriptor_sets(swapchain_image_count);

    cvm_vk_resize_module_graphics_data(&overlay_module_data);

    uint32_t uniform_size=0;
    uniform_size+=cvm_vk_transient_buffer_get_rounded_allocation_size(&overlay_transient_buffer,sizeof(float)*4*OVERLAY_NUM_COLOURS,0);
    uniform_size+=cvm_vk_transient_buffer_get_rounded_allocation_size(&overlay_transient_buffer,max_overlay_elements*sizeof(cvm_overlay_render_data),0);
    cvm_vk_transient_buffer_update(&overlay_transient_buffer,uniform_size,swapchain_image_count);

    uint32_t staging_size=65536;///arbitrary
    cvm_vk_staging_buffer_update(&overlay_staging_buffer,staging_size,swapchain_image_count);
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

float overlay_colours[OVERLAY_NUM_COLOURS*4]=
{
    1.0,0.1,0.1,1.0,///OVERLAY_NO_COLOUR (error)
    0.24,0.24,0.6,0.9,///OVERLAY_BACKGROUND_COLOUR
    0.12,0.12,0.36,0.85,///OVERLAY_MAIN_COLOUR
    0.12,0.12,0.48,0.85,///OVERLAY_MAIN_ALTERNATE_COLOUR
    0.3,0.3,1.0,0.2,///OVERLAY_HIGHLIGHTING_COLOUR
    0.4,0.6,0.9,0.3,///OVERLAY_TEXT_HIGHLIGHT_COLOUR
    0.2,0.3,1.0,0.8,///OVERLAY_TEXT_COMPOSITION_COLOUR_0
    0.4,0.6,0.9,0.8,///OVERLAY_TEXT_COLOUR_0
};

void overlay_render_frame(int screen_w,int screen_h,widget * menu_widget)
{
    cvm_vk_module_batch * batch;
    uint32_t swapchain_image_index;

    cvm_vk_module_work_payload payload;

    cvm_overlay_element_render_buffer element_render_buffer;
    VkDeviceSize uniform_offset,vertex_offset;

    ///perhaps this should return previous image index as well, such that that value can be used in cleanup (e.g. relinquishing ring buffer space)
    batch = cvm_vk_get_module_batch(&overlay_module_data,&swapchain_image_index);



    /// definitely want to clean up targets &c.
    /// "resultant" (swapchain) attachment goes at end
    /// actually loop framebuffers, look into benefit
    /// define state that varies between attachments in base, referenceable struct that can be shared across functions and invocations (inc. clear values?)
    /// dont bother storing framebuffer images/data in enumed array, store individually

    /// could have a single image atlas and use different aspects of part of it (r,g,b,a via image views) for different alpha/transparent image_atlases
    /// this would be fairly inefficient to retrieve though..., probably better not to.

    if(batch)
    {
        cvm_vk_init_batch_primary_payload(batch,&payload);

        cvm_vk_staging_buffer_begin(&overlay_staging_buffer);///build barriers into begin/end paradigm maybe???
        cvm_vk_transient_buffer_begin(&overlay_transient_buffer,swapchain_image_index);
        #warning need to use the appropriate queue for all following transfer ops
        ///     ^ possibly detect when they're different and use gfx directly to avoid double submission?
        ///         ^ have cvm_vk_get_module_batch return same command buffer?
        ///             ^could have meta level function that inserts both barriers to appropriate queues simultaneously as necessary??? (VK_NULL_HANDLE when unit-transfer?)
        ///   module handles semaphore when necessary, perhaps it can also handle some aspect(s) of barriers?

        element_render_buffer.buffer=cvm_vk_transient_buffer_get_allocation(&overlay_transient_buffer,max_overlay_elements*sizeof(cvm_overlay_render_data),0,&vertex_offset);
        element_render_buffer.space=max_overlay_elements;
        element_render_buffer.count=0;

        render_widget_overlay(&element_render_buffer,menu_widget);

        /// upload all staged resources needed by this frame
        ///if ever using a dedicated transfer queue (probably don't want tbh) change command buffers in these
        cvm_vk_image_atlas_submit_all_pending_copy_actions(&overlay_transparent_image_atlas,payload.command_buffer);
        cvm_vk_image_atlas_submit_all_pending_copy_actions(&overlay_colour_image_atlas,payload.command_buffer);

        ///end of transfer

        ///start of graphics

        float * colours = cvm_vk_transient_buffer_get_allocation(&overlay_transient_buffer,sizeof(float)*4*OVERLAY_NUM_COLOURS,0,&uniform_offset);

        memcpy(colours,overlay_colours,sizeof(float)*4*OVERLAY_NUM_COLOURS);

        update_overlay_uniforms(swapchain_image_index,uniform_offset);///should really build buffer acquisition into update uniforms function


        float screen_dimensions[4]={2.0/((float)screen_w),2.0/((float)screen_h),(float)screen_w,(float)screen_h};

        vkCmdPushConstants(payload.command_buffer,overlay_pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,4*sizeof(float),screen_dimensions);

        vkCmdBindDescriptorSets(payload.command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline_layout,0,1,overlay_descriptor_sets+swapchain_image_index,0,NULL);

        vkCmdBindDescriptorSets(payload.command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline_layout,1,1,&overlay_consistent_descriptor_set,0,NULL);

        VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext=NULL,
            .renderPass=overlay_render_pass,
            .framebuffer=overlay_framebuffers[swapchain_image_index],
            .renderArea=cvm_vk_get_default_render_area(),
            .clearValueCount=0,
            .pClearValues=NULL
        };

        vkCmdBeginRenderPass(payload.command_buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

        vkCmdBindPipeline(payload.command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline);

        cvm_vk_transient_buffer_bind_as_vertex(payload.command_buffer,&overlay_transient_buffer,0,vertex_offset);

        vkCmdDraw(payload.command_buffer,4,element_render_buffer.count,0,0);

        vkCmdEndRenderPass(payload.command_buffer);///================

        cvm_vk_staging_buffer_end(&overlay_staging_buffer,swapchain_image_index);
        cvm_vk_transient_buffer_end(&overlay_transient_buffer);

        cvm_vk_submit_graphics_work(&payload,CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE);
    }
}

void overlay_frame_cleanup(uint32_t swapchain_image_index)
{
    if(swapchain_image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        cvm_vk_staging_buffer_release_space(&overlay_staging_buffer,swapchain_image_index);
    }
}



cvm_vk_image_atlas_tile * overlay_create_transparent_image_tile_with_staging(void ** staging,uint32_t w, uint32_t h)
{
    return cvm_vk_acquire_image_atlas_tile_with_staging(&overlay_transparent_image_atlas,w,h,staging);
}

void overlay_destroy_transparent_image_tile(cvm_vk_image_atlas_tile * tile)
{
    cvm_vk_relinquish_image_atlas_tile(&overlay_transparent_image_atlas,tile);
}

cvm_vk_image_atlas_tile * overlay_create_colour_image_tile_with_staging(void ** staging,uint32_t w, uint32_t h)
{
    return cvm_vk_acquire_image_atlas_tile_with_staging(&overlay_colour_image_atlas,w,h,staging);
}

void overlay_destroy_colour_image_tile(cvm_vk_image_atlas_tile * tile)
{
    cvm_vk_relinquish_image_atlas_tile(&overlay_colour_image_atlas,tile);
}











