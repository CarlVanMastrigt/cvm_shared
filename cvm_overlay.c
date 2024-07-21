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

    if(name)printf("%s = %lu nanoseconds\n",name,dt);
}

static VkDescriptorPool cvm_overlay_descriptor_pool_create(const cvm_vk_device * device, uint32_t frame_cycle_count)
{
    VkResult created;
    VkDescriptorPool pool=VK_NULL_HANDLE;

    VkDescriptorPoolCreateInfo create_info =
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,///by not specifying individual free must reset whole pool (which is fine)
        .maxSets=frame_cycle_count+1,
        .poolSizeCount=2,
        .pPoolSizes=(VkDescriptorPoolSize[2])
        {
            {
                .type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
                .descriptorCount=frame_cycle_count
            },
            {
                .type=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount=2
            },
        }
    };

    created = vkCreateDescriptorPool(device->device, &create_info, NULL, &pool);
    assert(created == VK_SUCCESS);

    return pool;
}

static VkDescriptorSetLayout cvm_overlay_image_descriptor_set_layout_create(const cvm_vk_device * device)
{
    VkResult created;
    VkDescriptorSetLayout set_layout=VK_NULL_HANDLE;

    #warning improve the setup regarding samplers (get from device!)
    VkSampler fetch_sampler=cvm_vk_get_fetch_sampler();

    VkDescriptorSetLayoutCreateInfo create_info=
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

    created = vkCreateDescriptorSetLayout(device->device, &create_info, NULL, &set_layout);
    assert(created == VK_SUCCESS);

    return set_layout;
}

static VkDescriptorSet cvm_overlay_image_descriptor_set_allocate(const cvm_vk_device * device, VkDescriptorPool pool, VkDescriptorSetLayout set_layout)
{
    VkResult created;
    VkDescriptorSet set=VK_NULL_HANDLE;

    VkDescriptorSetAllocateInfo allocate_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=pool,
        .descriptorSetCount=1,
        .pSetLayouts=&set_layout
    };

    created = vkAllocateDescriptorSets(device->device, &allocate_info, &set);
    assert(created == VK_SUCCESS);

    return set;
}

static void cvm_overlay_image_descriptor_set_write(const cvm_vk_device * device, VkDescriptorSet descriptor_set, const VkImageView * views)
{
    VkWriteDescriptorSet writes[2]=
    {
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=descriptor_set,
            .dstBinding=0,
            .dstArrayElement=0,
            .descriptorCount=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo=(VkDescriptorImageInfo[1])
            {
                {
                    .sampler=VK_NULL_HANDLE,///using immutable sampler (for now)
                    .imageView=views[0],
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
            },
            .pBufferInfo=NULL,
            .pTexelBufferView=NULL
        },
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=descriptor_set,
            .dstBinding=1,
            .dstArrayElement=0,
            .descriptorCount=1,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo=(VkDescriptorImageInfo[1])
            {
                {
                    .sampler=VK_NULL_HANDLE,///using immutable sampler (for now)
                    .imageView=views[1],
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
            },
            .pBufferInfo=NULL,
            .pTexelBufferView=NULL
        }
    };

    vkUpdateDescriptorSets(device->device, 2, writes, 0, NULL);
}

static VkDescriptorSetLayout cvm_overlay_frame_descriptor_set_layout_create(const cvm_vk_device * device)
{
    VkResult created;
    VkDescriptorSetLayout set_layout=VK_NULL_HANDLE;

    VkDescriptorSetLayoutCreateInfo create_info=
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

    created = vkCreateDescriptorSetLayout(device->device, &create_info, NULL, &set_layout);
    assert(created == VK_SUCCESS);

    return set_layout;
}

static VkDescriptorSet cvm_overlay_frame_descriptor_set_allocate(cvm_overlay_renderer * renderer)
{
    VkDescriptorSet descriptor_set;
    ///separate pool for image descriptor sets? (so that they dont need to be reallocated/recreated upon swapchain changes)

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=renderer->descriptor_pool,
        .descriptorSetCount=1,
        .pSetLayouts=&renderer->frame_descriptor_set_layout,
    };

    cvm_vk_allocate_descriptor_sets(&descriptor_set,&descriptor_set_allocate_info);

    return descriptor_set;
}

static void cvm_overlay_frame_descriptor_set_write(const cvm_vk_device * device, VkDescriptorSet descriptor_set,VkBuffer buffer,VkDeviceSize offset)
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
                    .range=OVERLAY_NUM_COLOURS*4*sizeof(float),
                    #warning make above a struct maybe?
                }
            },
            .pTexelBufferView=NULL
        }
    };

    vkUpdateDescriptorSets(device->device, 1, writes, 0, NULL);
}

static VkPipelineLayout cvm_overlay_pipeline_layout_create(const cvm_vk_device * device, VkDescriptorSetLayout frame_descriptor_set_layout, VkDescriptorSetLayout image_descriptor_set_layout)
{
    VkResult created;
    VkPipelineLayout pipeline_layout=VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo create_info=
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=2,
        .pSetLayouts=(VkDescriptorSetLayout[2])
        {
            frame_descriptor_set_layout,
            image_descriptor_set_layout,
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

    created = vkCreatePipelineLayout(device->device, &create_info, NULL, &pipeline_layout);
    assert(created == VK_SUCCESS);

    return pipeline_layout;
}

static VkRenderPass cvm_overlay_render_pass_create(const cvm_vk_device * device,VkFormat swapchain_format)
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
            #warning make this change dependent upon whether we clear or not
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
            cvm_vk_get_default_colour_attachment_dependency(VK_SUBPASS_EXTERNAL,0),
            cvm_vk_get_default_colour_attachment_dependency(0,VK_SUBPASS_EXTERNAL)
        }
    };

    created = vkCreateRenderPass(device->device,&create_info,NULL,&render_pass);
    assert(created == VK_SUCCESS);

    return render_pass;
}

static VkFramebuffer cvm_overlay_framebuffer_create(const cvm_vk_device * device, VkExtent2D extent, VkRenderPass render_pass, VkImageView swapchain_image_view)
{
    VkFramebuffer framebuffer;
    VkResult created;

    VkFramebufferCreateInfo create_info=
    {
        .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .renderPass=render_pass,
        .attachmentCount=1,
        .pAttachments=&swapchain_image_view,
        .width=extent.width,
        .height=extent.height,
        .layers=1
    };

    created=vkCreateFramebuffer(device->device, &create_info, NULL, &framebuffer);
    assert(created == VK_SUCCESS);

    return framebuffer;
}

static VkPipeline cvm_overlay_pipeline_create(const cvm_vk_device * device, const VkPipelineShaderStageCreateInfo * stages, VkPipelineLayout pipeline_layout, VkRenderPass render_pass)
{
    VkResult created;
    VkPipeline pipeline;

    VkGraphicsPipelineCreateInfo create_info=
    {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .stageCount=2,
        .pStages=stages,
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
                        .stride=sizeof(cvm_overlay_element_render_data),
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
                        .offset=offsetof(cvm_overlay_element_render_data,data0)
                    },
                    {
                        .location=1,
                        .binding=0,
                        .format=VK_FORMAT_R32G32_UINT,
                        .offset=offsetof(cvm_overlay_element_render_data,data1)
                    },
                    {
                        .location=2,
                        .binding=0,
                        .format=VK_FORMAT_R16G16B16A16_UINT,
                        .offset=offsetof(cvm_overlay_element_render_data,data2)
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
        .layout=pipeline_layout,
        .renderPass=render_pass,
        .subpass=0,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };

    created=vkCreateGraphicsPipelines(device->device,VK_NULL_HANDLE,1,&create_info,NULL,&pipeline);
    assert(created == VK_SUCCESS);

    return pipeline;
}

static void cvm_overlay_images_initialise(cvm_overlay_images * images, const cvm_vk_device * device,
                                          uint32_t alpha_image_width, uint32_t alpha_image_height,
                                          uint32_t colour_image_width, uint32_t colour_image_height,
                                          cvm_vk_staging_shunt_buffer * shunt_buffer)
{
    VkResult created;

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
    image_creation_info.extent.width=alpha_image_width;
    image_creation_info.extent.height=alpha_image_height;
    created = vkCreateImage(device->device, &image_creation_info, NULL, images->images+0);
    assert(created == VK_SUCCESS);

    image_creation_info.format=VK_FORMAT_R8G8B8A8_UNORM;
    image_creation_info.usage=VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;///conditionally VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ???
    image_creation_info.extent.width=colour_image_width;
    image_creation_info.extent.height=colour_image_height;
    created = vkCreateImage(device->device, &image_creation_info, NULL, images->images+1);
    assert(created == VK_SUCCESS);

    cvm_vk_create_and_bind_memory_for_images(&images->memory, images->images, 2, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);



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
    view_creation_info.image=images->images[0];
    created = vkCreateImageView(device->device, &view_creation_info, NULL, images->views+0);
    assert(created == VK_SUCCESS);

    view_creation_info.format=VK_FORMAT_R8G8B8A8_UNORM;
    view_creation_info.image=images->images[1];
    created = vkCreateImageView(device->device, &view_creation_info, NULL, images->views+1);
    assert(created == VK_SUCCESS);


    cvm_vk_create_image_atlas(&images->alpha_atlas , images->images[0], images->views[0], sizeof(uint8_t)  , alpha_image_width , alpha_image_height , true, shunt_buffer);
    cvm_vk_create_image_atlas(&images->colour_atlas, images->images[1], images->views[1], sizeof(uint8_t)*4, colour_image_width, colour_image_height, true, shunt_buffer);
}

static void cvm_overlay_images_terminate(const cvm_vk_device * device, cvm_overlay_images * images)
{
    cvm_vk_destroy_image_atlas(&images->alpha_atlas);
    cvm_vk_destroy_image_atlas(&images->colour_atlas);

    vkDestroyImageView(device->device, images->views[0], NULL);
    vkDestroyImageView(device->device, images->views[1], NULL);

    vkDestroyImage(device->device, images->images[0], NULL);
    vkDestroyImage(device->device, images->images[1], NULL);

    cvm_vk_free_memory(images->memory);
}




typedef struct cvm_overlay_frame_resource_set
{
    VkFramebuffer framebuffer;
    VkPipeline pipeline;
    VkRenderPass render_pass;
    uint32_t swapchain_resource_index;
}
cvm_overlay_frame_resource_set;



static inline void cvm_overlay_swapchain_resources_terminate(cvm_overlay_swapchain_resources * resources, const cvm_vk_device * device)
{
    uint16_t i;
    assert(resources->in_flight_frame_count==0);

    for(i=0;i<resources->frame_count;i++)
    {
        if(resources->frame_resources_[i].framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device->device, resources->frame_resources_[i].framebuffer, NULL);
        }
    }
    free(resources->frame_resources_);

    vkDestroyRenderPass(device->device, resources->render_pass, NULL);
    vkDestroyPipeline(device->device, resources->pipeline, NULL);
}

static inline cvm_overlay_frame_resource_set cvm_overlay_renderer_frame_resource_set_acquire(cvm_overlay_renderer * renderer, const cvm_vk_device * device, const cvm_vk_swapchain_presentable_image * presentable_image)
{
    cvm_overlay_swapchain_resources * swapchain_resources;
    cvm_overlay_frame_resources * frame_resources;
    uint32_t i,swapchain_resource_index;
    bool frame_resources_found;


    /// first, prune unused resources, assuming the oldest will end first
    while((swapchain_resources = cvm_overlay_swapchain_resources_queue_get_front_ptr(&renderer->swapchain_resources)) &&
        (swapchain_resources->swapchain_generation != presentable_image->parent_swapchain->generation) && (swapchain_resources->in_flight_frame_count == 0))
    {
        /// these resources are out of date and no longer in use, delete them!
        cvm_overlay_swapchain_resources_terminate(swapchain_resources, device);
        cvm_overlay_swapchain_resources_queue_dequeue(&renderer->swapchain_resources, NULL);///remove from the queue
    }


    swapchain_resources = cvm_overlay_swapchain_resources_queue_get_back_ptr(&renderer->swapchain_resources);

    if( swapchain_resources==NULL || swapchain_resources->swapchain_generation != presentable_image->parent_swapchain->generation)
    {
        swapchain_resources = cvm_overlay_swapchain_resources_queue_new(&renderer->swapchain_resources);

        swapchain_resources->swapchain_generation = presentable_image->parent_swapchain->generation;
        swapchain_resources->in_flight_frame_count = 0;
        swapchain_resources->render_pass = cvm_overlay_render_pass_create(device,presentable_image->parent_swapchain->surface_format.format);
        swapchain_resources->pipeline = cvm_overlay_pipeline_create(device, renderer->pipeline_stages, renderer->pipeline_layout, swapchain_resources->render_pass);
        swapchain_resources->frame_count = presentable_image->parent_swapchain->image_count;
        swapchain_resources->frame_resources_ = malloc(sizeof(cvm_overlay_frame_resources)*presentable_image->parent_swapchain->image_count);
        for(i=0;i<swapchain_resources->frame_count;i++)
        {
            swapchain_resources->frame_resources_[i].framebuffer = VK_NULL_HANDLE;
        }
    }

    swapchain_resource_index = cvm_overlay_swapchain_resources_queue_back_index(&renderer->swapchain_resources);
    /// record that we're using this swapchain
    swapchain_resources->in_flight_frame_count++;


    /// get or create appropriate frame resources
    assert(presentable_image->index < swapchain_resources->frame_count);
    frame_resources = swapchain_resources->frame_resources_ + presentable_image->index;
    if(frame_resources->framebuffer == VK_NULL_HANDLE)
    {
        /// frame resources not init because they haven't been used yet
        frame_resources->framebuffer = cvm_overlay_framebuffer_create(device,presentable_image->parent_swapchain->surface_capabilities.currentExtent,swapchain_resources->render_pass,presentable_image->image_view);
    }


    return (cvm_overlay_frame_resource_set)
    {
        .render_pass = swapchain_resources->render_pass,
        .pipeline = swapchain_resources->pipeline,
        .framebuffer = frame_resources->framebuffer,
        .swapchain_resource_index = swapchain_resource_index,
    };
}



typedef struct cvm_overlay_renderer_work_entry
{
    cvm_vk_command_pool command_pool;

    VkDescriptorSet frame_descriptor_set;

    ///descriptor sets?? can descriptor sets not be shared???

    ///used to identify swapchain_dependent_resources in use
    uint32_t swapchain_generation;/// purely for debugging
    uint32_t swapchain_resource_index;
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
    cvm_overlay_swapchain_resources * swapchain_resources;

    cvm_vk_command_pool_reset(renderer->device,&entry->command_pool);

    swapchain_resources = cvm_overlay_swapchain_resources_queue_get_ptr(&renderer->swapchain_resources, entry->swapchain_resource_index);
    assert(swapchain_resources->swapchain_generation == entry->swapchain_generation);
    assert(swapchain_resources->in_flight_frame_count > 0);
    swapchain_resources->in_flight_frame_count--;
}

void cvm_overlay_renderer_initialise(cvm_overlay_renderer * renderer, cvm_vk_device * device, cvm_vk_staging_buffer_ * staging_buffer, uint32_t renderer_cycle_count)
{
    renderer->device=device;
    renderer->cycle_count=renderer_cycle_count;
    renderer->staging_buffer = staging_buffer;

    renderer->descriptor_pool = cvm_overlay_descriptor_pool_create(device, renderer_cycle_count);
    renderer->frame_descriptor_set_layout = cvm_overlay_frame_descriptor_set_layout_create(device);
    renderer->image_descriptor_set_layout = cvm_overlay_image_descriptor_set_layout_create(device);

    renderer->pipeline_layout = cvm_overlay_pipeline_layout_create(device, renderer->frame_descriptor_set_layout, renderer->image_descriptor_set_layout);

    cvm_vk_create_shader_stage_info(renderer->pipeline_stages+0,"cvm_shared/shaders/overlay.vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(renderer->pipeline_stages+1,"cvm_shared/shaders/overlay.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);


    cvm_vk_staging_shunt_buffer_initialise(&renderer->shunt_buffer, staging_buffer->alignment);
    cvm_overlay_element_render_data_stack_initialise(&renderer->element_render_stack);
    cvm_overlay_swapchain_resources_queue_initialise(&renderer->swapchain_resources);
    cvm_overlay_images_initialise(&renderer->images, device, 1024, 1024, 1024, 1024, &renderer->shunt_buffer);

    renderer->image_descriptor_set = cvm_overlay_image_descriptor_set_allocate(device, renderer->descriptor_pool, renderer->image_descriptor_set_layout); /// no matching free necessary
    cvm_overlay_image_descriptor_set_write(device, renderer->image_descriptor_set, renderer->images.views);

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
    cvm_overlay_swapchain_resources * resources;


    cvm_vk_work_queue_terminate(device,&renderer->work_queue);

    cvm_overlay_images_terminate(device, &renderer->images);

    cvm_vk_destroy_shader_stage_info(renderer->pipeline_stages+0);
    cvm_vk_destroy_shader_stage_info(renderer->pipeline_stages+1);

    vkDestroyPipelineLayout(device->device, renderer->pipeline_layout, NULL);

    vkDestroyDescriptorPool(device->device, renderer->descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device->device, renderer->frame_descriptor_set_layout, NULL);
    vkDestroyDescriptorSetLayout(device->device, renderer->image_descriptor_set_layout, NULL);

    cvm_vk_staging_shunt_buffer_terminate(&renderer->shunt_buffer);

    cvm_overlay_element_render_data_stack_terminate(&renderer->element_render_stack);

    while((resources = cvm_overlay_swapchain_resources_queue_dequeue_ptr(&renderer->swapchain_resources)))
    {
        cvm_overlay_swapchain_resources_terminate(resources, device);
    }
    cvm_overlay_swapchain_resources_queue_terminate(&renderer->swapchain_resources);
}



void overlay_render_to_image(const cvm_vk_device * device, cvm_overlay_renderer * renderer, cvm_vk_swapchain_presentable_image * presentable_image, widget * menu_widget)
{
    cvm_overlay_renderer_work_entry * work_entry;
    cvm_vk_command_buffer cb;
    cvm_vk_timeline_semaphore_moment completion_moment;
    cvm_vk_staging_buffer_allocation staging_buffer_allocation;
    VkDeviceSize upload_offset,instance_offset,uniform_offset,staging_space;
    cvm_overlay_element_render_data_stack * element_render_stack;
    cvm_vk_staging_shunt_buffer * shunt_buffer;
    cvm_vk_staging_buffer_ * staging_buffer;
    cvm_overlay_frame_resource_set frame_resources;
    VkDeviceSize staging_offset;
    char * staging_mapping;
    float screen_w,screen_h;

    element_render_stack = &renderer->element_render_stack;
    shunt_buffer = &renderer->shunt_buffer;
    staging_buffer = renderer->staging_buffer;

    const float overlay_colours[OVERLAY_NUM_COLOURS*4]=
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


    if(presentable_image)
    {
        cvm_overlay_element_render_data_stack_reset(element_render_stack);
        cvm_vk_staging_shunt_buffer_reset(shunt_buffer);
        /// acting on the shunt buffer directly in this way feels a little off

        work_entry = cvm_vk_work_queue_entry_acquire(&renderer->work_queue, device);
        cvm_vk_command_pool_acquire_command_buffer(device,&work_entry->command_pool,&cb);
        cvm_vk_command_buffer_wait_on_presentable_image_acquisition(&cb,presentable_image);

        frame_resources=cvm_overlay_renderer_frame_resource_set_acquire(renderer, device, presentable_image);

        work_entry->swapchain_generation = presentable_image->parent_swapchain->generation;
        work_entry->swapchain_resource_index = frame_resources.swapchain_resource_index;

        ///this uses the shunt buffer!
        render_widget_overlay(element_render_stack,menu_widget);

        /// upload all staged resources needed by this frame
        uniform_offset  = 0;
        upload_offset   = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, uniform_offset + sizeof(float)*4*OVERLAY_NUM_COLOURS);
        instance_offset = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, upload_offset  + shunt_buffer->offset);
        staging_space   = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, instance_offset + cvm_overlay_element_render_data_stack_size(element_render_stack));

        staging_buffer_allocation=cvm_vk_staging_buffer_reserve_allocation(staging_buffer, device, staging_space, true);
        staging_offset = staging_buffer_allocation.acquired_offset;
        staging_mapping = staging_buffer_allocation.mapping;

        memcpy(staging_mapping+uniform_offset,overlay_colours,sizeof(float)*4*OVERLAY_NUM_COLOURS);

        #warning shunt buffer needs its own mutex!
        /// copy necessary changes to the overlay images, using the rendering command buffer
        cvm_vk_staging_shunt_buffer_copy(shunt_buffer, staging_mapping+upload_offset);
        cvm_vk_image_atlas_submit_all_pending_copy_actions(&renderer->images.alpha_atlas , cb.buffer, staging_buffer->buffer, staging_offset+upload_offset);
        cvm_vk_image_atlas_submit_all_pending_copy_actions(&renderer->images.colour_atlas, cb.buffer, staging_buffer->buffer, staging_offset+upload_offset);

        cvm_overlay_element_render_data_stack_copy(element_render_stack, staging_mapping+instance_offset);
        ///end of transfer


        ///start of graphics
        cvm_overlay_frame_descriptor_set_write(device, work_entry->frame_descriptor_set, staging_buffer->buffer, staging_offset+uniform_offset);///should really build buffer acquisition into update uniforms function


        cvm_vk_staging_buffer_flush_allocation(renderer->staging_buffer, device, &staging_buffer_allocation, 0, staging_space);

        /// get resolution of target from the swapchain
        screen_w=(float)presentable_image->parent_swapchain->surface_capabilities.currentExtent.width;
        screen_h=(float)presentable_image->parent_swapchain->surface_capabilities.currentExtent.height;
        float screen_dimensions[4]={2.0/screen_w,2.0/screen_h,screen_w,screen_h};
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
            #warning overlay shouldnt clear (normally), temporary measure
            .pClearValues=(VkClearValue[1]){{.color=(VkClearColorValue){.float32={0.0f,0.0f,0.0f,0.0f}}}},
        };
        vkCmdBeginRenderPass(cb.buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

        vkCmdBindPipeline(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,frame_resources.pipeline);
        vkCmdBindVertexBuffers(cb.buffer, 0, 1, &renderer->staging_buffer->buffer, &(VkDeviceSize){instance_offset+staging_offset});///little bit of hacky stuff to create lvalue
        vkCmdDraw(cb.buffer,4,renderer->element_render_stack.count,0,0);

        vkCmdEndRenderPass(cb.buffer);///================


        cvm_vk_command_buffer_signal_presenting_image_complete(&cb,presentable_image);
        cvm_vk_command_buffer_submit(device, &work_entry->command_pool, &cb, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, &completion_moment);

        cvm_vk_staging_buffer_complete_allocation(renderer->staging_buffer,staging_buffer_allocation.segment_index,completion_moment);
        cvm_vk_work_queue_entry_release(&renderer->work_queue, work_entry, &completion_moment);
    }
}





