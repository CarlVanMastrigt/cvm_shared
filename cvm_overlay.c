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


///if uniform paradigm is based on max per frame being respected then the descrptor sets can be pre baked with offsets per swapchain image
/// separate uniform buffer for fixed data (colour, screen size &c.) ?


/// swapchain generation probably useful, needs a way to clean up last used swapchain generation
///relevant descriptors, share as much as possible










void test_timing(bool start,char * name)
{
    static struct timespec tso,tsn;
    uint64_t dt;

    clock_gettime(CLOCK_REALTIME,&tsn);

    dt=(tsn.tv_sec-tso.tv_sec)*1000000000 + tsn.tv_nsec-tso.tv_nsec;

    if(start) tso=tsn;

    if(name)printf("%s = %lu\n",name,dt);
}


static void cvm_overlay_descriptors_initialise(cvm_overlay_renderer * renderer, uint32_t frame_cycle_count)
{
    VkDescriptorSetLayoutCreateInfo frame_descriptor_set_layout_create_info=
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
        },
    };
    cvm_vk_create_descriptor_set_layout(&renderer->frame_descriptor_set_layout,&frame_descriptor_set_layout_create_info);

    VkDescriptorPoolCreateInfo frame_descriptor_pool_create_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,///by not specifying individual free must reset whole pool (which is fine)
        .maxSets=frame_cycle_count,
        .poolSizeCount=1,
        .pPoolSizes=(VkDescriptorPoolSize[1])
        {
            {
                .type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
                .descriptorCount=frame_cycle_count
            }
        }
    };
    cvm_vk_create_descriptor_pool(&renderer->frame_descriptor_pool,&frame_descriptor_pool_create_info);


    #warning improve the layout/setup of this! also remove genericized function calls
    VkSampler fetch_sampler=cvm_vk_get_fetch_sampler();
    VkDescriptorSetLayoutCreateInfo image_descriptor_set_layout_create_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .bindingCount=2,
        .pBindings=(VkDescriptorSetLayoutBinding[2])
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
        },
    };
    cvm_vk_create_descriptor_set_layout(&renderer->image_descriptor_set_layout,&image_descriptor_set_layout_create_info);

    VkDescriptorPoolCreateInfo image_descriptor_pool_create_info=
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
    cvm_vk_create_descriptor_pool(&renderer->image_descriptor_pool,&image_descriptor_pool_create_info);
}

static void cvm_overlay_descriptors_terminate(cvm_overlay_renderer * renderer)
{
    cvm_vk_destroy_descriptor_pool(renderer->frame_descriptor_pool);
    cvm_vk_destroy_descriptor_set_layout(renderer->frame_descriptor_set_layout);

    cvm_vk_destroy_descriptor_pool(renderer->image_descriptor_pool);
    cvm_vk_destroy_descriptor_set_layout(renderer->image_descriptor_set_layout);
}

static void cvm_overlay_image_descriptor_set_allocate(cvm_overlay_renderer * renderer)
{
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

static void cvm_overlay_image_descriptor_set_free(cvm_overlay_renderer * renderer)
{
}

static VkDescriptorSet cvm_overlay_frame_descriptor_set_allocate(cvm_overlay_renderer * renderer)
{
    VkDescriptorSet descriptor_set;
    ///separate pool for image descriptor sets? (so that they dont need to be reallocated/recreated upon swapchain changes)

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=renderer->frame_descriptor_pool,
        .descriptorSetCount=1,
        .pSetLayouts=&renderer->frame_descriptor_set_layout,
    };

    cvm_vk_allocate_descriptor_sets(&descriptor_set,&descriptor_set_allocate_info);

    return descriptor_set;
}

static VkDescriptorSet cvm_overlay_frame_descriptor_set_free(cvm_overlay_renderer * renderer)
{
}

static void update_overlay_uniforms(VkDescriptorSet descriptor_set,VkBuffer buffer,VkDeviceSize offset)
{
    ///investigate whether its necessary to update this every frame if its basically unchanging data and war hazards are a problem. may want to avoid just to stop validation from picking it up.
    VkWriteDescriptorSet writes[1]=
    {
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=descriptor_set,
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

static void cvm_overlay_pipeline_layout_create(cvm_vk_device * device, cvm_overlay_renderer * renderer)
{
    VkResult created;

    VkPipelineLayoutCreateInfo create_info=
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=2,
        .pSetLayouts=(VkDescriptorSetLayout[2])
        {
            renderer->frame_descriptor_set_layout,
            renderer->image_descriptor_set_layout,
        },
        .pushConstantRangeCount=1,
        .pPushConstantRanges=(VkPushConstantRange[1])
        {
            {
                .stageFlags=VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset=0,
                .size=sizeof(float)*4,
            }
        }
    };

    created = vkCreatePipelineLayout(device->device, &create_info, NULL, &renderer->pipeline_layout);
    assert(created == VK_SUCCESS);
}

static void cvm_overlay_pipeline_layout_destroy(cvm_vk_device * device, cvm_overlay_renderer * renderer)
{
    vkDestroyPipelineLayout(device->device, renderer->pipeline_layout, NULL);
}

static VkRenderPass cvm_overlay_render_pass_create(cvm_vk_device * device)
{
    VkRenderPass render_pass;
    VkResult created;

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

    created=vkCreateRenderPass(device->device,&create_info,NULL,&render_pass);
    assert(created == VK_SUCCESS);

    return render_pass;
}

static VkPipeline cvm_overlay_pipeline_create(cvm_vk_device * device, cvm_overlay_renderer * renderer, VkRenderPass render_pass)
{
    VkResult created;
    VkPipeline pipeline;

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
        .layout=renderer->pipeline_layout,
        .renderPass=render_pass,
        .subpass=0,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };

    ///can pass above into multiple functions as parameter
    created=vkCreateGraphicsPipelines(device->device,VK_NULL_HANDLE,1,&create_info,NULL,&pipeline);
    assert(created == VK_SUCCESS);

    return pipeline;
}

static void cvm_overlay_images_initialise(cvm_vk_device * device, cvm_overlay_renderer * renderer,uint32_t t_w,uint32_t t_h,uint32_t c_w,uint32_t c_h)
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

static void cvm_overlay_images_terminate(cvm_vk_device * device, cvm_overlay_renderer * renderer)
{
    cvm_vk_destroy_image_atlas(&renderer->transparent_image_atlas);
    cvm_vk_destroy_image_atlas(&renderer->colour_image_atlas);

    cvm_vk_destroy_image_view(renderer->transparent_image_view);
    cvm_vk_destroy_image_view(renderer->colour_image_view);

    cvm_vk_destroy_image(renderer->transparent_image);
    cvm_vk_destroy_image(renderer->colour_image);

    cvm_vk_free_memory(renderer->image_memory);
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




typedef struct cvm_overlay_frame_resource_set
{
    VkFramebuffer framebuffer;
    VkPipeline pipeline;
    VkRenderPass render_pass;
}
cvm_overlay_frame_resource_set;




static inline cvm_overlay_swapchain_resources_initialise(cvm_overlay_renderer * renderer)
{
    cvm_overlay_swapchain_resources_stack_initialise(&renderer->swapchain_dependent_resources);
}

static inline cvm_overlay_swapchain_resources_terminate_entry(cvm_overlay_swapchain_resources * resources, cvm_vk_device * device)
{
    cvm_overlay_framebuffer framebuffer;

    assert(resources->in_flight_count==0);

    while(cvm_overlay_framebuffer_stack_get(&resources->framebuffers,&framebuffer))
    {
        vkDestroyFramebuffer(device->device, framebuffer.framebuffer, NULL);
    }
    cvm_overlay_framebuffer_stack_terminate(&resources->framebuffers);

    vkDestroyRenderPass(device->device, resources->render_pass, NULL);
    vkDestroyPipeline(device->device, resources->pipeline, NULL);
}

static inline void cvm_overlay_swapchain_resources_terminate(cvm_overlay_renderer * renderer, cvm_vk_device * device)
{
    cvm_overlay_swapchain_resources resources;

    while(cvm_overlay_swapchain_resources_stack_get(&renderer->swapchain_dependent_resources,&resources))
    {
        cvm_overlay_swapchain_resources_terminate_entry(&resources, device);
    }
    cvm_overlay_swapchain_resources_stack_terminate(&renderer->swapchain_dependent_resources);
}

static inline cvm_overlay_frame_resource_set cvm_overlay_swapchain_resources_acquire(cvm_overlay_renderer * renderer, cvm_vk_device * device, cvm_vk_swapchain_presentable_image * presentable_image)
{
    cvm_overlay_swapchain_resources * swapchain_resources;
    cvm_overlay_framebuffer * framebuffer;
    uint32_t i;
    VkResult created;
    bool removed;

    ///first, prune unused resources
    for(i=0;i<renderer->swapchain_dependent_resources.count;i++)
    {
        swapchain_resources=renderer->swapchain_dependent_resources.stack+i;
        if(swapchain_resources->swapchain_generation != presentable_image->parent_swapchain->generation && swapchain_resources->in_flight_count==0)
        {
            /// these resources are out of date and no longer in use, delete them!
            cvm_overlay_swapchain_resources_terminate_entry(swapchain_resources, device);
            ///following is a hacky trick to remove `swapchain_resources` from the stack (overwrite it with the last entry)
            removed=cvm_overlay_framebuffer_stack_get(&renderer->swapchain_dependent_resources, swapchain_resources);
            assert(removed);
            puts("overlay swapchain resources deleted");
            i--;/// decrement to step back, such that we can assess the entry that replaces the deleted one
        }
    }

    for(i=0;i<renderer->swapchain_dependent_resources.count;i++)
    {
        swapchain_resources = renderer->swapchain_dependent_resources.stack+i;
        if(presentable_image->parent_swapchain->generation == swapchain_resources->swapchain_generation)
        {
            /// appropriate swapchain resources found
            swapchain_resources->in_flight_count++;
            break;
        }
    }
    if(i==renderer->swapchain_dependent_resources.count)///didnt find a swapchain
    {
        swapchain_resources=cvm_overlay_swapchain_resources_stack_new(&renderer->swapchain_dependent_resources);

        swapchain_resources->swapchain_generation=presentable_image->parent_swapchain->generation;
        swapchain_resources->in_flight_count=1;///will be used this frame
        cvm_overlay_framebuffer_stack_initialise(&swapchain_resources->framebuffers);
        swapchain_resources->render_pass=cvm_overlay_render_pass_create(device);
        swapchain_resources->pipeline=cvm_overlay_pipeline_create(device,renderer,swapchain_resources->render_pass);
    }


    for(i=0;i<swapchain_resources->framebuffers.count;i++)
    {
        framebuffer=swapchain_resources->framebuffers.stack+i;
        if(presentable_image->unique_image_identifier==framebuffer->unique_swapchain_image_identifier)
        {
            /// appropriate framebuffer found
            break;
        }
    }
    if(i==swapchain_resources->framebuffers.count)///didnt find a framebuffer
    {
        framebuffer=cvm_overlay_framebuffer_stack_new(&swapchain_resources->framebuffers);

        #warning move this to a function?
        VkFramebufferCreateInfo framebuffer_creation_info=
        {
            .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .renderPass=swapchain_resources->render_pass,
            .attachmentCount=1,
            .pAttachments=&presentable_image->image_view,
            .width=presentable_image->parent_swapchain->surface_capabilities.currentExtent.width,
            .height=presentable_image->parent_swapchain->surface_capabilities.currentExtent.height,
            #warning not sure above is best way to get image size
            .layers=1
        };

        created=vkCreateFramebuffer(device->device, &framebuffer_creation_info, NULL, &framebuffer->framebuffer);
        assert(created == VK_SUCCESS);

        framebuffer->unique_swapchain_image_identifier=presentable_image->unique_image_identifier;
    }

    return (cvm_overlay_frame_resource_set)
    {
        .render_pass=swapchain_resources->render_pass,
        .pipeline=swapchain_resources->pipeline,
        .framebuffer=framebuffer->framebuffer,
    };
}






typedef struct cvm_overlay_renderer_work_entry
{
    cvm_vk_command_pool command_pool;

    VkDescriptorSet frame_descriptor_set;

    ///descriptor sets?? can descriptor sets not be shared???

    ///used to identify swapchain_dependent_resources in use
    uint16_t swapchain_generation;
}
cvm_overlay_renderer_work_entry;

void cvm_overlay_renderer_work_entry_initialise(void * shared_ptr, void * entry_ptr)
{
    cvm_overlay_renderer * renderer=shared_ptr;
    cvm_overlay_renderer_work_entry * entry=entry_ptr;

    cvm_vk_command_pool_initialise(renderer->device,&entry->command_pool,renderer->device->graphics_queue_family_index,0);
    entry->frame_descriptor_set=cvm_overlay_frame_descriptor_set_allocate(renderer);
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
    cvm_overlay_swapchain_resources * swapchain_dependent_resources;

    cvm_vk_command_pool_reset(renderer->device,&entry->command_pool);

    for(i=0;i<renderer->swapchain_dependent_resources.count;i++)
    {
        swapchain_dependent_resources=renderer->swapchain_dependent_resources.stack+i;
        if(entry->swapchain_generation == swapchain_dependent_resources->swapchain_generation)
        {
            assert(swapchain_dependent_resources->in_flight_count > 0);
            swapchain_dependent_resources->in_flight_count--;
            break;
        }
    }
    assert(i < renderer->swapchain_dependent_resources.count);///TARGET RESOURCES IN USE NOT FOUND!
}

void cvm_overlay_renderer_initialise(cvm_overlay_renderer * renderer, cvm_vk_device * device, cvm_vk_staging_buffer_ * staging_buffer, uint32_t renderer_cycle_count)
{
    cvm_overlay_open_freetype();


    renderer->device=device;
    renderer->cycle_count=renderer_cycle_count;
    renderer->staging_buffer = staging_buffer;

    cvm_overlay_descriptors_initialise(renderer, renderer_cycle_count);

    cvm_vk_create_shader_stage_info(&renderer->vertex_stage,"cvm_shared/shaders/overlay.vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(&renderer->fragment_stage,"cvm_shared/shaders/overlay.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);


    cvm_vk_staging_shunt_buffer_initialise(&renderer->shunt_buffer, staging_buffer->alignment);
    cvm_overlay_render_data_stack_initialise(&renderer->element_render_stack);

    cvm_overlay_swapchain_resources_initialise(renderer);

    cvm_overlay_pipeline_layout_create(device, renderer);

    cvm_overlay_images_initialise(device,renderer,1024,1024,1024,1024);

    cvm_overlay_image_descriptor_set_allocate(renderer);

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
}

void cvm_overlay_renderer_terminate(cvm_overlay_renderer * renderer, cvm_vk_device * device)
{
    cvm_vk_work_queue_terminate(device,&renderer->work_queue);

    cvm_overlay_images_terminate(device, renderer);

    cvm_vk_destroy_shader_stage_info(&renderer->vertex_stage);
    cvm_vk_destroy_shader_stage_info(&renderer->fragment_stage);

    cvm_overlay_descriptors_terminate(renderer);




    cvm_vk_staging_shunt_buffer_terminate(&renderer->shunt_buffer);

    cvm_overlay_render_data_stack_terminate(&renderer->element_render_stack);

    cvm_overlay_swapchain_resources_terminate(renderer, device);

    cvm_overlay_pipeline_layout_destroy(device, renderer);

    cvm_overlay_close_freetype();
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
    cvm_overlay_frame_resource_set frame_resources;

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

        work_entry->swapchain_generation = presentable_image->parent_swapchain->generation;

        frame_resources=cvm_overlay_swapchain_resources_acquire(renderer, device, presentable_image);


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


        update_overlay_uniforms(work_entry->frame_descriptor_set, staging_buffer->buffer, staging_buffer_allocation.acquired_offset+uniform_offset);///should really build buffer acquisition into update uniforms function


        cvm_vk_staging_buffer_flush_allocation(renderer->staging_buffer,device,&staging_buffer_allocation,0,staging_space);

        float screen_dimensions[4]={2.0/((float)screen_w),2.0/((float)screen_h),(float)screen_w,(float)screen_h};

        vkCmdPushConstants(cb.buffer,renderer->pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,4*sizeof(float),screen_dimensions);

        vkCmdBindDescriptorSets(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,renderer->pipeline_layout,0,1,&work_entry->frame_descriptor_set,0,NULL);

        vkCmdBindDescriptorSets(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,renderer->pipeline_layout,1,1,&renderer->image_descriptor_set,0,NULL);


        VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext=NULL,
            .renderPass=frame_resources.render_pass,
            .framebuffer=frame_resources.framebuffer,
            .renderArea=cvm_vk_get_default_render_area(),
            .clearValueCount=1,
            #warning overlay shouldnt clear, temporary measure
            .pClearValues=(VkClearValue[1]){{.color=(VkClearColorValue){.float32={0.0f,0.0f,0.0f,0.0f}}}},
        };

        vkCmdBeginRenderPass(cb.buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

        vkCmdBindPipeline(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,frame_resources.pipeline);

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





