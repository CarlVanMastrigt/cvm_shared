/**
Copyright 2020,2021,2022,2023 Carl van Mastrigt

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



static VkRenderPass overlay_render_pass;

///if uniform paradigm is based on max per frame being respected then the descrptor sets can be pre baked with offsets per swapchain image
/// separate uniform buffer for fixed data (colour, screen size &c.) ?
static VkDescriptorSetLayout overlay_descriptor_set_layout;///rename this set to uniform
static VkDescriptorPool overlay_descriptor_pool;///make separate pool for swapchain dependent and swapchain independent resources
static VkDescriptorSet * overlay_descriptor_sets;

/// swapchain generation probably useful, needs a way to clean up last used swapchain generation
static VkPipelineLayout overlay_pipeline_layout;///relevant descriptors, share as much as possible
static VkPipeline overlay_pipeline;









void test_timing(bool start,char * name)
{
    static struct timespec tso,tsn;
    uint64_t dt;

    clock_gettime(CLOCK_REALTIME,&tsn);

    dt=(tsn.tv_sec-tso.tv_sec)*1000000000 + tsn.tv_nsec-tso.tv_nsec;

    if(start) tso=tsn;

    if(name)printf("%s = %lu\n",name,dt);
}


static void create_overlay_descriptor_set_layouts(cvm_overlay_renderer * renderer)
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

    cvm_vk_create_descriptor_set_layout(&renderer->image_descriptor_set_layout,&layout_create_info);
}

static void create_overlay_image_descriptors(cvm_overlay_renderer * renderer)
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

    cvm_vk_create_descriptor_pool(&renderer->image_descriptor_pool,&pool_create_info);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=renderer->image_descriptor_pool,
        .descriptorSetCount=1,
        .pSetLayouts=&renderer->image_descriptor_set_layout
    };

    cvm_vk_allocate_descriptor_sets(&renderer->image_descriptor_set,&descriptor_set_allocate_info);

    VkWriteDescriptorSet writes[2]=
    {
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=renderer->image_descriptor_set,
            .dstBinding=0,
            .dstArrayElement=0,
            .descriptorCount=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo=(VkDescriptorImageInfo[1])
            {
                {
                    .sampler=VK_NULL_HANDLE,///using immutable sampler (for now)
                    .imageView=renderer->transparent_image_view,
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
            },
            .pBufferInfo=NULL,
            .pTexelBufferView=NULL
        },
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=renderer->image_descriptor_set,
            .dstBinding=1,
            .dstArrayElement=0,
            .descriptorCount=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo=(VkDescriptorImageInfo[1])
            {
                {
                    .sampler=VK_NULL_HANDLE,///using immutable sampler (for now)
                    .imageView=renderer->colour_image_view,
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

static void update_overlay_uniforms(uint32_t swapchain_image,VkBuffer buffer,VkDeviceSize offset)
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
                    .buffer=buffer,
                    .offset=offset,
                    .range=OVERLAY_NUM_COLOURS*4*sizeof(float)
                }
            },
            .pTexelBufferView=NULL
        }
    };

    cvm_vk_write_descriptor_sets(writes,1);
}

static void create_overlay_pipeline_layouts(cvm_overlay_renderer * renderer)
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
            renderer->image_descriptor_set_layout
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
//            {
//                .flags=0,
//                .format=cvm_vk_get_screen_format(),
//                .samples=VK_SAMPLE_COUNT_1_BIT,///sample_count not relevant for actual render target (swapchain image)
//                .loadOp=VK_ATTACHMENT_LOAD_OP_LOAD,
//                .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
//                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
//                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
//                .initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,/// is first render pass (ergo the clear above) thus the VK_IMAGE_LAYOUT_UNDEFINED rather than VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
//                .finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR///is last render pass thus the VK_IMAGE_LAYOUT_PRESENT_SRC_KHR rather than VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
//            }
            {
                .flags=0,
                .format=cvm_vk_get_screen_format(),
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
            cvm_vk_get_default_colour_attachment_dependency(VK_SUBPASS_EXTERNAL,0),
            cvm_vk_get_default_colour_attachment_dependency(0,VK_SUBPASS_EXTERNAL)
        }
    };

    cvm_vk_create_render_pass(&overlay_render_pass,&create_info);
}

static void create_overlay_pipelines(cvm_overlay_renderer * renderer)
{
    VkGraphicsPipelineCreateInfo create_info=
    {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .stageCount=2,
        .pStages=(VkPipelineShaderStageCreateInfo[2])
        {
            renderer->vertex_stage,
            renderer->fragment_stage
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

static void create_overlay_images(cvm_overlay_renderer * renderer,uint32_t t_w,uint32_t t_h,uint32_t c_w,uint32_t c_h)
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
    cvm_vk_create_image(&renderer->transparent_image,&image_creation_info);

    image_creation_info.format=VK_FORMAT_R8G8B8A8_UNORM;
    image_creation_info.usage=VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;///conditionally VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ???
    image_creation_info.extent.width=c_w;
    image_creation_info.extent.height=c_h;
    cvm_vk_create_image(&renderer->colour_image,&image_creation_info);

    VkImage images[2]={renderer->transparent_image,renderer->colour_image};
    cvm_vk_create_and_bind_memory_for_images(&renderer->image_memory,images,2,0,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);



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
    view_creation_info.image=renderer->transparent_image;
    cvm_vk_create_image_view(&renderer->transparent_image_view,&view_creation_info);

    view_creation_info.format=VK_FORMAT_R8G8B8A8_UNORM;
    view_creation_info.image=renderer->colour_image;
    cvm_vk_create_image_view(&renderer->colour_image_view,&view_creation_info);


    cvm_vk_create_image_atlas(&renderer->transparent_image_atlas,renderer->transparent_image,renderer->transparent_image_view,sizeof(uint8_t),t_w,t_h,true,&renderer->shunt_buffer);
    cvm_vk_create_image_atlas(&renderer->colour_image_atlas,renderer->colour_image,renderer->colour_image_view,sizeof(uint8_t)*4,c_w,c_h,false,&renderer->shunt_buffer);

    #warning as images ALWAYS need staging (even on uma) could make staging a creation parameter?? -- YES
    ///perhaps not as it limits the ability to use it for a more generalised purpose...
    ///could move general section out and use a meta structure encompassing the generalised image atlas?
}

void initialise_overlay_render_data(cvm_overlay_renderer * renderer)
{
    create_overlay_render_pass();

    create_overlay_descriptor_set_layouts(renderer);

    create_overlay_pipeline_layouts(renderer);

    cvm_vk_create_shader_stage_info(&renderer->vertex_stage,"cvm_shared/shaders/overlay.vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(&renderer->fragment_stage,"cvm_shared/shaders/overlay.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    create_overlay_images(renderer,1024,1024,1024,1024);

    create_overlay_image_descriptors(renderer);/// move into above?

    cvm_overlay_open_freetype();
}

void terminate_overlay_render_data(cvm_overlay_renderer * renderer)
{
    cvm_overlay_close_freetype();

    cvm_vk_destroy_image_atlas(&renderer->transparent_image_atlas);
    cvm_vk_destroy_image_atlas(&renderer->colour_image_atlas);

    cvm_vk_destroy_image_view(renderer->transparent_image_view);
    cvm_vk_destroy_image_view(renderer->colour_image_view);

    cvm_vk_destroy_image(renderer->transparent_image);
    cvm_vk_destroy_image(renderer->colour_image);

    cvm_vk_free_memory(renderer->image_memory);




    cvm_vk_destroy_render_pass(overlay_render_pass);

    cvm_vk_destroy_shader_stage_info(&renderer->vertex_stage);
    cvm_vk_destroy_shader_stage_info(&renderer->fragment_stage);

    cvm_vk_destroy_pipeline_layout(overlay_pipeline_layout);

    cvm_vk_destroy_descriptor_pool(renderer->image_descriptor_pool);

    cvm_vk_destroy_descriptor_set_layout(overlay_descriptor_set_layout);
    cvm_vk_destroy_descriptor_set_layout(renderer->image_descriptor_set_layout);
}


void initialise_overlay_swapchain_dependencies(cvm_overlay_renderer * renderer)
{
    #warning REMOVE
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();

    create_overlay_pipelines(renderer);

    create_overlay_descriptor_sets(swapchain_image_count);
}

void terminate_overlay_swapchain_dependencies(void)
{
    #warning REMOVE
    uint32_t swapchain_image_count=cvm_vk_get_swapchain_image_count();
    uint32_t i;

    cvm_vk_destroy_pipeline(overlay_pipeline);

    cvm_vk_destroy_descriptor_pool(overlay_descriptor_pool);

    free(overlay_descriptor_sets);
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






static inline VkFramebuffer cvm_overlay_renderer_framebuffer_acquire(cvm_overlay_renderer * renderer, cvm_vk_device * device, cvm_vk_swapchain_presentable_image * presentable_image)
{
    uint32_t i;
    VkResult created;
    cvm_overlay_framebuffer * framebuffer;
    bool removed;

    ///first, prune unused framebuffers
    for(i=0;i<renderer->framebuffers.count;  )
    {
        framebuffer=renderer->framebuffers.stack+i;
        if(framebuffer->swapchain_generation!=presentable_image->parent_swapchain->generation && !framebuffer->in_flight)
        {
            /// this framebuffer is out of date and no longer in use, delete it!
            vkDestroyFramebuffer(device->device, framebuffer->framebuffer, NULL);
            ///following is a hacky trick to remove `framebuffer` from the stack (overwrite it with the last entry)
            removed=cvm_overlay_framebuffer_stack_get(&renderer->framebuffers,framebuffer);
            assert(removed);
            puts("overlay framebuffer deleted");
        }
        else
        {
            i++;
        }
    }



    for(i=0;i<renderer->framebuffers.count;i++)
    {
        framebuffer = renderer->framebuffers.stack+i;
        if(presentable_image->unique_image_identifier == framebuffer->unique_swapchain_image_identifier)
        {
            assert(framebuffer->swapchain_generation == presentable_image->parent_swapchain->generation);
            assert( ! framebuffer->in_flight);
            framebuffer->in_flight=true;
            return framebuffer->framebuffer;
        }
    }

    framebuffer=cvm_overlay_framebuffer_stack_new(&renderer->framebuffers);

    VkFramebufferCreateInfo framebuffer_creation_info=
    {
        .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .renderPass=overlay_render_pass,
        #warning move above into renderer
        .attachmentCount=1,
        .pAttachments=&presentable_image->image_view,
        .width=presentable_image->parent_swapchain->surface_capabilities.currentExtent.width,
        .height=presentable_image->parent_swapchain->surface_capabilities.currentExtent.height,
        #warning not sure above is best way to get image size
        .layers=1
    };

    created=vkCreateFramebuffer(device->device, &framebuffer_creation_info, NULL, &framebuffer->framebuffer);
    assert(created==VK_SUCCESS);///don't have a good way to handle

    framebuffer->in_flight=true;
    framebuffer->swapchain_generation=presentable_image->parent_swapchain->generation;
    framebuffer->unique_swapchain_image_identifier=presentable_image->unique_image_identifier;

    return framebuffer->framebuffer;
}

static inline void cvm_overlay_renderer_framebuffers_initialise(cvm_overlay_renderer * renderer)
{
    cvm_overlay_framebuffer_stack_initialise(&renderer->framebuffers);
}

static inline void cvm_overlay_renderer_framebuffers_terminate(cvm_overlay_renderer * renderer, cvm_vk_device * device)
{
    cvm_overlay_framebuffer framebuffer;

    while(cvm_overlay_framebuffer_stack_get(&renderer->framebuffers,&framebuffer))
    {
        assert( ! framebuffer.in_flight);
        vkDestroyFramebuffer(device->device, framebuffer.framebuffer, NULL);
    }
    cvm_overlay_framebuffer_stack_terminate(&renderer->framebuffers);
}







typedef struct cvm_overlay_renderer_work_entry
{
    cvm_vk_command_pool command_pool;

    ///descriptor sets?? can descriptor sets not be shared???

    ///used to identify images
    uint16_t unique_swapchain_image_identifier;
}
cvm_overlay_renderer_work_entry;

void cvm_overlay_renderer_work_entry_initialise(void * shared_ptr, void * entry_ptr)
{
    cvm_overlay_renderer * renderer=shared_ptr;
    cvm_overlay_renderer_work_entry * entry=entry_ptr;

    cvm_vk_command_pool_initialise(renderer->device,&entry->command_pool,renderer->device->graphics_queue_family_index,0);
}

void cvm_overlay_renderer_work_entry_terminate(void * shared_ptr, void * entry_ptr)
{
    cvm_overlay_renderer * renderer=shared_ptr;
    cvm_overlay_renderer_work_entry * entry=entry_ptr;

    cvm_vk_command_pool_terminate(renderer->device,&entry->command_pool);
}

void cvm_overlay_renderer_work_entry_reset(void * shared_ptr, void * entry_ptr)
{
    uint32_t i;
    cvm_overlay_renderer * renderer=shared_ptr;
    cvm_overlay_renderer_work_entry * entry=entry_ptr;
    cvm_overlay_framebuffer * framebuffer;

    cvm_vk_command_pool_reset(renderer->device,&entry->command_pool);

    for(i=0;i<renderer->framebuffers.count;i++)
    {
        framebuffer=renderer->framebuffers.stack+i;
        if(entry->unique_swapchain_image_identifier == framebuffer->unique_swapchain_image_identifier)
        {
            assert(framebuffer->in_flight);
            framebuffer->in_flight=false;
            break;
        }
    }
    assert(i<renderer->framebuffers.count);///NOT FOUND!
}

void cvm_overlay_renderer_initialise(cvm_overlay_renderer * renderer, cvm_vk_device * device, cvm_vk_staging_buffer_ * staging_buffer, uint32_t renderer_cycle_count)
{
    renderer->device=device;
    renderer->cycle_count=renderer_cycle_count;
    renderer->staging_buffer = staging_buffer;

    cvm_vk_staging_shunt_buffer_initialise(&renderer->shunt_buffer, staging_buffer->alignment);
    cvm_overlay_render_data_stack_initialise(&renderer->element_render_stack);

    cvm_vk_work_queue_setup work_queue_setup=
    {
        .shared_data=renderer,///may want this to become an allocated struct so more can ve shared
        .entry_size=sizeof(cvm_overlay_renderer_work_entry),
        .entry_count=renderer_cycle_count,

        .entry_initialise_func=cvm_overlay_renderer_work_entry_initialise,
        .entry_terminate_func=cvm_overlay_renderer_work_entry_terminate,
        .entry_reset_func=cvm_overlay_renderer_work_entry_reset,
    };

    cvm_vk_work_queue_initialise(&renderer->work_queue,&work_queue_setup);

    cvm_overlay_renderer_framebuffers_initialise(renderer);
}

void cvm_overlay_renderer_terminate(cvm_overlay_renderer * renderer, cvm_vk_device * device)
{
    cvm_vk_work_queue_terminate(device,&renderer->work_queue);
    cvm_vk_staging_shunt_buffer_terminate(&renderer->shunt_buffer);
    cvm_overlay_render_data_stack_terminate(&renderer->element_render_stack);
    cvm_overlay_renderer_framebuffers_terminate(renderer, device);
}







void overlay_render_to_image(cvm_vk_device * device, cvm_overlay_renderer * renderer, cvm_vk_swapchain_presentable_image * presentable_image, int screen_w,int screen_h,widget * menu_widget)
{
    #warning get screen res from presentable image
    uint32_t swapchain_image_index;
    cvm_overlay_renderer_work_entry * work_entry;
    cvm_vk_command_buffer cb;
    cvm_vk_timeline_semaphore_moment completion_moment;
    cvm_vk_staging_buffer_allocation staging_buffer_allocation;
    VkDeviceSize upload_offset,instance_offset,uniform_offset,staging_space;
    cvm_overlay_render_data_stack * element_render_stack;
    cvm_vk_staging_shunt_buffer * shunt_buffer;
    cvm_vk_staging_buffer_ * staging_buffer;
    VkFramebuffer framebuffer;

    VkDeviceSize vertex_offset;//replace with instance_offset


    /// definitely want to clean up targets &c.
    /// "resultant" (swapchain) attachment goes at end
    /// actually loop framebuffers, look into benefit
    /// define state that varies between attachments in base, referenceable struct that can be shared across functions and invocations (inc. clear values?)
    /// dont bother storing framebuffer images/data in enumed array, store individually

    /// could have a single image atlas and use different aspects of part of it (r,g,b,a via image views) for different alpha/transparent image_atlases
    /// this would be fairly inefficient to retrieve though..., probably better not to.

    if(presentable_image)
    {
        element_render_stack = &renderer->element_render_stack;
        cvm_overlay_render_data_stack_reset(element_render_stack);

        shunt_buffer = &renderer->shunt_buffer;
        cvm_vk_staging_shunt_buffer_reset(shunt_buffer);

        staging_buffer = renderer->staging_buffer;

        #warning temporary hack!
        swapchain_image_index=presentable_image - presentable_image->parent_swapchain->presenting_images;

        work_entry = cvm_vk_work_queue_acquire_entry(device,&renderer->work_queue);
        cvm_vk_command_pool_acquire_command_buffer(device,&work_entry->command_pool,&cb);
        cvm_vk_command_buffer_wait_on_presentable_image_acquisition(&cb,presentable_image);

        work_entry->unique_swapchain_image_identifier = presentable_image->unique_image_identifier;

        framebuffer=cvm_overlay_renderer_framebuffer_acquire(renderer,device,presentable_image);


        ///cvm_vk_staging_buffer_allocation_align_offset

        #warning this uses the shunt buffer!
        render_widget_overlay(element_render_stack,menu_widget);

        #warning make copy and size functions on a stack!


        /// upload all staged resources needed by this frame
        ///if ever using a dedicated transfer queue (probably don't want tbh) change command buffers in these

        uniform_offset  = 0;
        upload_offset   = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, uniform_offset + sizeof(float)*4*OVERLAY_NUM_COLOURS);
        instance_offset = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, upload_offset  + shunt_buffer->offset);
        staging_space   = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, instance_offset + cvm_overlay_render_data_stack_size(element_render_stack));

        staging_buffer_allocation=cvm_vk_staging_buffer_reserve_allocation(staging_buffer, device, staging_space, true);

        memcpy(staging_buffer_allocation.mapping+uniform_offset,overlay_colours,sizeof(float)*4*OVERLAY_NUM_COLOURS);

        #warning shunt buffer needs its own mutex!
        cvm_vk_staging_shunt_buffer_copy(shunt_buffer, staging_buffer_allocation.mapping+upload_offset);
        cvm_vk_image_atlas_submit_all_pending_copy_actions(&renderer->transparent_image_atlas,cb.buffer,staging_buffer->buffer, staging_buffer_allocation.acquired_offset+upload_offset);
        cvm_vk_image_atlas_submit_all_pending_copy_actions(&renderer->colour_image_atlas,cb.buffer,staging_buffer->buffer, staging_buffer_allocation.acquired_offset+upload_offset);
        #warning move these memcpy to functions!

        cvm_overlay_render_data_stack_copy(element_render_stack, staging_buffer_allocation.mapping+instance_offset);
        ///end of transfer



        ///start of graphics


        update_overlay_uniforms(swapchain_image_index,staging_buffer->buffer,staging_buffer_allocation.acquired_offset+uniform_offset);///should really build buffer acquisition into update uniforms function


        cvm_vk_staging_buffer_flush_allocation(renderer->staging_buffer,device,&staging_buffer_allocation,0,staging_space);

        float screen_dimensions[4]={2.0/((float)screen_w),2.0/((float)screen_h),(float)screen_w,(float)screen_h};

        vkCmdPushConstants(cb.buffer,overlay_pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,4*sizeof(float),screen_dimensions);

        vkCmdBindDescriptorSets(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline_layout,0,1,overlay_descriptor_sets+swapchain_image_index,0,NULL);

        vkCmdBindDescriptorSets(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline_layout,1,1,&renderer->image_descriptor_set,0,NULL);


        VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext=NULL,
            .renderPass=overlay_render_pass,
            .framebuffer=framebuffer,
            .renderArea=cvm_vk_get_default_render_area(),
            .clearValueCount=1,
            #warning overlay shouldnt clear, temporary measure
            .pClearValues=(VkClearValue[1]){{.color=(VkClearColorValue){.float32={0.0f,0.0f,0.0f,0.0f}}}},
        };

        vkCmdBeginRenderPass(cb.buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

        vkCmdBindPipeline(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,overlay_pipeline);

        instance_offset+=staging_buffer_allocation.acquired_offset;
        vkCmdBindVertexBuffers(cb.buffer,0,1,&renderer->staging_buffer->buffer,&instance_offset);

        vkCmdDraw(cb.buffer,4,renderer->element_render_stack.count,0,0);

        vkCmdEndRenderPass(cb.buffer);///================

        cvm_vk_command_buffer_signal_presenting_image_complete(&cb,presentable_image);
        cvm_vk_command_buffer_submit(device,&work_entry->command_pool,&cb,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,&completion_moment);

        cvm_vk_staging_buffer_complete_allocation(renderer->staging_buffer,staging_buffer_allocation.segment_index,completion_moment);

        cvm_vk_work_queue_release_entry(&renderer->work_queue,work_entry,&completion_moment);
    }
}





