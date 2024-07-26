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

static VkRenderPass cvm_overlay_render_pass_create(const cvm_vk_device * device,VkFormat target_format, VkImageLayout initial_layout, VkImageLayout final_layout, bool clear)
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
            {
                .flags=0,
                .format=target_format,
                .samples=VK_SAMPLE_COUNT_1_BIT,///sample_count not relevant for actual render target (swapchain image)
                .loadOp=clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=initial_layout,
                .finalLayout=final_layout,
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

static VkPipeline cvm_overlay_pipeline_create(const cvm_vk_device * device, const VkPipelineShaderStageCreateInfo * stages, VkPipelineLayout pipeline_layout, VkRenderPass render_pass, VkExtent2D extent)
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
        .pVertexInputState=&(VkPipelineVertexInputStateCreateInfo)
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
        },
        .pInputAssemblyState=&(VkPipelineInputAssemblyStateCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,///not the default
            .primitiveRestartEnable=VK_FALSE
        },
        .pTessellationState=NULL,///not needed (yet)
        .pViewportState=&(VkPipelineViewportStateCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .viewportCount=1,
            .pViewports=(VkViewport[1])
            {
                {
                .x=0.0,
                .y=0.0,
                .width=(float)extent.width,
                .height=(float)extent.height,
                .minDepth=0.0,
                .maxDepth=1.0,
                },
            },
            .scissorCount=1,
            .pScissors= (VkRect2D[1])
            {
                {
                    .offset=(VkOffset2D){.x=0,.y=0},
                    .extent=extent,
                },
            },
        },
        .pRasterizationState=&(VkPipelineRasterizationStateCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .depthClampEnable=VK_FALSE,
            .rasterizerDiscardEnable=VK_FALSE,
            .polygonMode=VK_POLYGON_MODE_FILL,
            .cullMode=VK_CULL_MODE_NONE,
            .frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable=VK_FALSE,
            .depthBiasConstantFactor=0.0,
            .depthBiasClamp=0.0,
            .depthBiasSlopeFactor=0.0,
            .lineWidth=1.0
        },
        .pMultisampleState=&(VkPipelineMultisampleStateCreateInfo)
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
        },
        .pDepthStencilState=NULL,
        .pColorBlendState=&(VkPipelineColorBlendStateCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .logicOpEnable=VK_FALSE,
            .logicOp=VK_LOGIC_OP_COPY,
            .attachmentCount=1,///must equal colorAttachmentCount in subpass
            .pAttachments= (VkPipelineColorBlendAttachmentState[1])
            {
                {
                    .blendEnable=VK_TRUE,
                    .srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA,
                    .dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .colorBlendOp=VK_BLEND_OP_ADD,
                    .srcAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,
                    .dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,
                    .alphaBlendOp=VK_BLEND_OP_ADD,
                    .colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
                },
            },
            .blendConstants={0.0,0.0,0.0,0.0},
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


    cvm_vk_create_image_atlas(&images->alpha_atlas , images->images[0], images->views[0], sizeof(uint8_t)  , alpha_image_width , alpha_image_height , false, shunt_buffer);
    cvm_vk_create_image_atlas(&images->colour_atlas, images->images[1], images->views[1], sizeof(uint8_t)*4, colour_image_width, colour_image_height, false, shunt_buffer);
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




struct cvm_overlay_frame_resource_set
{
    struct cvm_overlay_target_resources* target_resources;
    struct cvm_overlay_frame_resources* frame_resources;
};


static inline void cvm_overlay_frame_resources_initialise(struct cvm_overlay_frame_resources* frame_resources, const cvm_vk_device* device,
    const struct cvm_overlay_target_resources* target_resources, const cvm_overlay_target * target)
{
    frame_resources->image_view = target->image_view;

    frame_resources->framebuffer = cvm_overlay_framebuffer_create(device, target->extent, target_resources->render_pass, target->image_view);

    frame_resources->last_use_moment = cvm_vk_timeline_semaphore_moment_null;
}

static inline void cvm_overlay_frame_resources_terminate(struct cvm_overlay_frame_resources* frame_resources, const cvm_vk_device* device)
{
    vkDestroyFramebuffer(device->device, frame_resources->framebuffer, NULL);
}

static inline void cvm_overlay_target_resources_initialise(struct cvm_overlay_target_resources* target_resources, const cvm_vk_device* device,
    const cvm_overlay_renderer * renderer, const cvm_overlay_target * target)
{
    /// set cache key information
    target_resources->extent = target->extent;
    target_resources->format = target->format;
    target_resources->color_space = target->color_space;
    target_resources->initial_layout = target->initial_layout;
    target_resources->final_layout = target->final_layout;
    target_resources->clear_image = target->clear_image;

    target_resources->render_pass = cvm_overlay_render_pass_create(device, target->format, target->initial_layout, target->final_layout, target->clear_image);
    target_resources->pipeline = cvm_overlay_pipeline_create(device, renderer->pipeline_stages, renderer->pipeline_layout, target_resources->render_pass, target->extent);

    cvm_overlay_frame_resources_cache_initialise(&target_resources->frame_resources);
}

static inline void cvm_overlay_target_resources_terminate(struct cvm_overlay_target_resources* target_resources, const cvm_vk_device* device)
{
    struct cvm_overlay_frame_resources* frame_resources;

    while((frame_resources = cvm_overlay_frame_resources_cache_evict(&target_resources->frame_resources)))
    {
        ///these both MUST exist right?
        assert(frame_resources->last_use_moment.semaphore != VK_NULL_HANDLE);
        cvm_vk_timeline_semaphore_moment_wait(device, &frame_resources->last_use_moment);
        vkDestroyFramebuffer(device->device, frame_resources->framebuffer, NULL);
    }
    cvm_overlay_frame_resources_cache_terminate(&target_resources->frame_resources);

    vkDestroyRenderPass(device->device, target_resources->render_pass, NULL);
    vkDestroyPipeline(device->device, target_resources->pipeline, NULL);
}



static inline struct cvm_overlay_frame_resource_set cvm_overlay_renderer_frame_resource_set_acquire(cvm_overlay_renderer * renderer, const cvm_vk_device * device, const cvm_overlay_target * target)
{
    struct cvm_overlay_target_resources* target_resources;
    struct cvm_overlay_frame_resources* frame_resources;
    bool evicted;


    target_resources = cvm_overlay_target_resources_cache_find(&renderer->target_resources, target);
    if(target_resources==NULL)
    {
        target_resources = cvm_overlay_target_resources_cache_new(&renderer->target_resources, &evicted);
        if(evicted)
        {
            cvm_overlay_target_resources_terminate(target_resources, device);
        }

        cvm_overlay_target_resources_initialise(target_resources, device, renderer, target);
    }

    frame_resources = cvm_overlay_frame_resources_cache_find(&target_resources->frame_resources, target);
    if(frame_resources==NULL)
    {
        frame_resources = cvm_overlay_frame_resources_cache_new(&target_resources->frame_resources, &evicted);
        if(evicted)
        {
            cvm_overlay_frame_resources_terminate(frame_resources, device);
        }
        cvm_overlay_frame_resources_initialise(frame_resources, device, target_resources, target);
    }


    return (struct cvm_overlay_frame_resource_set)
    {
        .target_resources = target_resources,
        .frame_resources = frame_resources,
    };
}






typedef struct cvm_overlay_renderer_work_entry
{
    cvm_vk_command_pool command_pool;

    VkDescriptorSet frame_descriptor_set;
}
cvm_overlay_renderer_work_entry;

void cvm_overlay_renderer_work_entry_initialise(void * shared_ptr, void * entry_ptr)
{
    cvm_overlay_renderer * renderer = shared_ptr;
    cvm_overlay_renderer_work_entry * entry = entry_ptr;

    cvm_vk_command_pool_initialise(&entry->command_pool, renderer->device, renderer->device->graphics_queue_family_index, 0);
    entry->frame_descriptor_set = cvm_overlay_frame_descriptor_set_allocate(renderer);
}

void cvm_overlay_renderer_work_entry_terminate(void * shared_ptr, void * entry_ptr)
{
    cvm_overlay_renderer * renderer = shared_ptr;
    cvm_overlay_renderer_work_entry * entry = entry_ptr;

    cvm_vk_command_pool_terminate(&entry->command_pool, renderer->device);
}

void cvm_overlay_renderer_work_entry_reset(void * shared_ptr, void * entry_ptr)
{
    uint32_t i;
    cvm_overlay_renderer * renderer = shared_ptr;
    cvm_overlay_renderer_work_entry * entry=entry_ptr;


    cvm_vk_command_pool_reset(&entry->command_pool, renderer->device);
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


    cvm_vk_staging_shunt_buffer_initialise(&renderer->shunt_buffer, staging_buffer->alignment, 1<<18, false);
    cvm_overlay_element_render_data_stack_initialise(&renderer->element_render_stack);
    cvm_overlay_target_resources_cache_initialise(&renderer->target_resources);
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
    struct cvm_overlay_target_resources * target_resources;


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

    while(target_resources = cvm_overlay_target_resources_cache_evict(&renderer->target_resources))
    {
        cvm_overlay_target_resources_terminate(target_resources, device);
    }
    cvm_overlay_target_resources_cache_terminate(&renderer->target_resources);
}

static inline void cvm_overlay_add_target_acquire_instructions(cvm_vk_command_buffer * command_buffer, const cvm_overlay_target * target)
{
    #warning make this a function;
    assert(command_buffer->wait_count + target->wait_semaphore_count < 11);
    memcpy(command_buffer->wait_info + command_buffer->wait_count, target->wait_semaphores, sizeof(VkSemaphoreSubmitInfo) * target->wait_semaphore_count);
    command_buffer->wait_count += target->wait_semaphore_count;

    if(target->acquire_barrier_count)
    {
        VkDependencyInfo dependencies =
        {
            .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext=NULL,
            .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT,
            .memoryBarrierCount=0,
            .pMemoryBarriers=NULL,
            .bufferMemoryBarrierCount=0,
            .pBufferMemoryBarriers=NULL,
            .imageMemoryBarrierCount=target->acquire_barrier_count,
            .pImageMemoryBarriers=target->acquire_barriers,
        };

        vkCmdPipelineBarrier2(command_buffer->buffer, &dependencies);
    }
}

static inline void cvm_overlay_add_target_release_instructions(cvm_vk_command_buffer * command_buffer, const cvm_overlay_target * target)
{
    #warning make this a function;
    assert(command_buffer->signal_count + target->signal_semaphore_count < 4);
    memcpy(command_buffer->signal_info + command_buffer->signal_count, target->signal_semaphores, sizeof(VkSemaphoreSubmitInfo) * target->signal_semaphore_count);
    command_buffer->signal_count += target->signal_semaphore_count;

    if(target->release_barrier_count)
    {
        VkDependencyInfo dependencies =
        {
            .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext=NULL,
            .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT,
            .memoryBarrierCount=0,
            .pMemoryBarriers=NULL,
            .bufferMemoryBarrierCount=0,
            .pBufferMemoryBarriers=NULL,
            .imageMemoryBarrierCount=target->release_barrier_count,
            .pImageMemoryBarriers=target->release_barriers,
        };

        vkCmdPipelineBarrier2(command_buffer->buffer, &dependencies);
    }
}

cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_target(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, const cvm_overlay_target* target)
{
    cvm_overlay_renderer_work_entry * work_entry;
    cvm_vk_command_buffer cb;
    cvm_vk_timeline_semaphore_moment completion_moment;
    cvm_vk_staging_buffer_allocation staging_buffer_allocation;
    VkDeviceSize upload_offset,instance_offset,uniform_offset,staging_space;
    cvm_overlay_element_render_data_stack * element_render_stack;
    cvm_vk_staging_shunt_buffer * shunt_buffer;
    cvm_vk_staging_buffer_ * staging_buffer;
    struct cvm_overlay_frame_resource_set frame_resources_set;
    VkDeviceSize staging_offset;
    char * staging_mapping;
    float screen_w,screen_h;


    element_render_stack = &renderer->element_render_stack;
    shunt_buffer = &renderer->shunt_buffer;
    staging_buffer = renderer->staging_buffer;

    /// move this somewhere?? theme data? allow non-destructive mutation based on these?
    #warning make it so this is used in uniform texel buffer, good to test that
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

    #warning renderer needs to know queue familt to present on!

    cvm_overlay_element_render_data_stack_reset(element_render_stack);
    cvm_vk_staging_shunt_buffer_reset(shunt_buffer);
    /// acting on the shunt buffer directly in this way feels a little off

    work_entry = cvm_vk_work_queue_entry_acquire(&renderer->work_queue, device);
    cvm_vk_command_pool_acquire_command_buffer(&work_entry->command_pool, device, &cb);


    frame_resources_set = cvm_overlay_renderer_frame_resource_set_acquire(renderer, device, target);

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

    /// copy necessary changes to the overlay images, using the rendering command buffer
    cvm_vk_staging_shunt_buffer_copy(shunt_buffer, staging_mapping+upload_offset);
    cvm_vk_image_atlas_submit_all_pending_copy_actions(&renderer->images.alpha_atlas , cb.buffer, staging_buffer->buffer, staging_offset+upload_offset);
    cvm_vk_image_atlas_submit_all_pending_copy_actions(&renderer->images.colour_atlas, cb.buffer, staging_buffer->buffer, staging_offset+upload_offset);

    cvm_overlay_element_render_data_stack_copy(element_render_stack, staging_mapping+instance_offset);
    ///end of transfer


    ///start of graphics
    cvm_overlay_add_target_acquire_instructions(&cb, target);

    cvm_overlay_frame_descriptor_set_write(device, work_entry->frame_descriptor_set, staging_buffer->buffer, staging_offset+uniform_offset);///should really build buffer acquisition into update uniforms function


    cvm_vk_staging_buffer_flush_allocation(renderer->staging_buffer, device, &staging_buffer_allocation, 0, staging_space);

    /// get resolution of target from the swapchain
    screen_w=(float)target->extent.width;
    screen_h=(float)target->extent.height;
    float screen_dimensions[4]={2.0/screen_w,2.0/screen_h,screen_w,screen_h};
    vkCmdPushConstants(cb.buffer,renderer->pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,4*sizeof(float),screen_dimensions);
    vkCmdBindDescriptorSets(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,renderer->pipeline_layout,0,1,&work_entry->frame_descriptor_set,0,NULL);
    vkCmdBindDescriptorSets(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,renderer->pipeline_layout,1,1,&renderer->image_descriptor_set,0,NULL);


    VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext=NULL,
        .renderPass=frame_resources_set.target_resources->render_pass,
        .framebuffer=frame_resources_set.frame_resources->framebuffer,
        .renderArea=cvm_vk_get_default_render_area(),
        .clearValueCount=1,
        #warning overlay shouldnt clear (normally), temporary measure
        .pClearValues=(VkClearValue[1]){{.color=(VkClearColorValue){.float32={0.0f,0.0f,0.0f,0.0f}}}},
    };
    vkCmdBeginRenderPass(cb.buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

    vkCmdBindPipeline(cb.buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,frame_resources_set.target_resources->pipeline);
    vkCmdBindVertexBuffers(cb.buffer, 0, 1, &renderer->staging_buffer->buffer, &(VkDeviceSize){instance_offset+staging_offset});///little bit of hacky stuff to create lvalue
    vkCmdDraw(cb.buffer,4,renderer->element_render_stack.count,0,0);

    vkCmdEndRenderPass(cb.buffer);///================

    cvm_overlay_add_target_release_instructions(&cb, target);

    cvm_vk_command_pool_submit_command_buffer(&work_entry->command_pool, device, &cb, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, &completion_moment);

    cvm_vk_staging_buffer_complete_allocation(renderer->staging_buffer, staging_buffer_allocation.segment_index, completion_moment);

    cvm_vk_work_queue_entry_release(&renderer->work_queue, work_entry, &completion_moment);
    frame_resources_set.frame_resources->last_use_moment = completion_moment;///somewhat hacky but w/e

    return completion_moment;
}


cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_presentable_image(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, cvm_vk_swapchain_presentable_image * presentable_image, bool last_use)
{
    bool can_present, first_use;
    uint32_t overlay_queue_family_index;
    cvm_overlay_target target;

    first_use = (presentable_image->layout == VK_IMAGE_LAYOUT_UNDEFINED);

    target.color_space = presentable_image->parent_swapchain_instance->surface_format.colorSpace;
    target.format = presentable_image->parent_swapchain_instance->surface_format.format;
    target.extent = presentable_image->parent_swapchain_instance->surface_capabilities.currentExtent;
    target.image_view = presentable_image->image_view;

    target.wait_semaphore_count = 0;
    target.acquire_barrier_count = 0;

    target.signal_semaphore_count = 0;
    target.release_barrier_count = 0;

    target.initial_layout = presentable_image->layout;

    target.clear_image = first_use;

    if(first_use)/// can figure out from state
    {
        assert(presentable_image->state == cvm_vk_presentable_image_state_acquired);///also indicates first use
        presentable_image->state = cvm_vk_presentable_image_state_started;
        target.wait_semaphores[target.wait_semaphore_count++] = cvm_vk_binary_semaphore_submit_info(presentable_image->acquire_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    assert(presentable_image->state == cvm_vk_presentable_image_state_started);

    if(last_use)/// don't have a perfect way to figure out, but could infer from
    {
        overlay_queue_family_index = device->graphics_queue_family_index;

        presentable_image->last_use_queue_family = overlay_queue_family_index;
        presentable_image->layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        target.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        can_present = presentable_image->parent_swapchain_instance->queue_family_presentable_mask | (1<<overlay_queue_family_index);

        if(can_present)
        {
            presentable_image->state = cvm_vk_presentable_image_state_complete;
            /// signal after any stage that could modify the image contents
            target.signal_semaphores[target.signal_semaphore_count++] = cvm_vk_binary_semaphore_submit_info(presentable_image->present_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        }
        else
        {
            presentable_image->state = cvm_vk_presentable_image_state_tranferred;

            target.signal_semaphores[target.signal_semaphore_count++] = cvm_vk_binary_semaphore_submit_info(presentable_image->qfot_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

            target.release_barriers[target.release_barrier_count++] = (VkImageMemoryBarrier2)
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = NULL,
                .srcStageMask  =  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask  =  VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                /// ignored by QFOT??
                .dstStageMask = 0,///no relevant stage representing present... (afaik), maybe VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT ??
                .dstAccessMask = 0,///should be 0 by spec
                .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,///colour attachment optimal? modify  renderpasses as necessary to accommodate this (must match present acquire)
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = overlay_queue_family_index,
                .dstQueueFamilyIndex = presentable_image->parent_swapchain_instance->fallback_present_queue_family,
                .image = presentable_image->image,
                .subresourceRange = (VkImageSubresourceRange)
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
        }
    }
    else
    {
        presentable_image->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        target.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    presentable_image->last_use_moment = cvm_overlay_render_to_target(device, renderer, menu_widget, &target);

    return presentable_image->last_use_moment;
}



