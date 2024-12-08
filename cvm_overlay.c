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


static VkDescriptorPool cvm_overlay_descriptor_pool_create(const cvm_vk_device * device, uint32_t frame_transient_count)
{
    VkDescriptorPool pool=VK_NULL_HANDLE;
    VkResult result;

    VkDescriptorPoolCreateInfo create_info =
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,///by not specifying individual free must reset whole pool (which is fine)
        .maxSets=frame_transient_count+1,
        .poolSizeCount=2,
        .pPoolSizes=(VkDescriptorPoolSize[2])
        {
            {
                .type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
                .descriptorCount=frame_transient_count
            },
            {
                .type=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount=2*frame_transient_count
            },
        }
    };

    result = vkCreateDescriptorPool(device->device, &create_info, device->host_allocator, &pool);
    assert(result == VK_SUCCESS);

    return pool;
}

static VkDescriptorSetLayout cvm_overlay_descriptor_set_layout_create(const cvm_vk_device * device)
{
    VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
    VkResult result;

    VkDescriptorSetLayoutCreateInfo create_info =
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
                .descriptorCount=2,
                .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=(VkSampler[2]) /// also test w/ null & setting samplers directly
                {
                    device->defaults.fetch_sampler,
                    device->defaults.fetch_sampler
                },
            },
            {
                .binding=1,
                .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here...
                .descriptorCount=1,
                .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers=NULL,
            }
        },
    };

    result = vkCreateDescriptorSetLayout(device->device, &create_info, device->host_allocator, &set_layout);
    assert(result == VK_SUCCESS);

    return set_layout;
}

static VkDescriptorSet cvm_overlay_descriptor_set_allocate(const cvm_vk_device * device, VkDescriptorPool pool, VkDescriptorSetLayout set_layout)
{
    VkDescriptorSet set = VK_NULL_HANDLE;
    VkResult result;

    VkDescriptorSetAllocateInfo allocate_info=
    {
        .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext=NULL,
        .descriptorPool=pool,
        .descriptorSetCount=1,
        .pSetLayouts=&set_layout
    };

    result = vkAllocateDescriptorSets(device->device, &allocate_info, &set);
    assert(result == VK_SUCCESS);

    return set;
}

static void cvm_overlay_descriptor_set_write(const cvm_vk_device * device, VkDescriptorSet descriptor_set, VkImageView colour_atlas_view, VkImageView alpha_atlas_view, VkBuffer uniform_buffer, VkDeviceSize uniform_offset)
{
    VkWriteDescriptorSet writes[2] =
    {
        {
            .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext=NULL,
            .dstSet=descriptor_set,
            .dstBinding=0,
            .dstArrayElement=0,
            .descriptorCount=2,
            .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo=(VkDescriptorImageInfo[2])
            {
                {
                    .sampler=VK_NULL_HANDLE,///using immutable sampler (for now)
                    .imageView=alpha_atlas_view,
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                {
                    .sampler=VK_NULL_HANDLE,///using immutable sampler (for now)
                    .imageView=colour_atlas_view,
                    .imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
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
            .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,///VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER probably preferable here... then use either RGBA8 unorm colour or possibly RGBA16
            .pImageInfo=NULL,
            .pBufferInfo=(VkDescriptorBufferInfo[1])
            {
                {
                    .buffer=uniform_buffer,
                    .offset=uniform_offset,
                    .range=OVERLAY_NUM_COLOURS*4*sizeof(float),
                    #warning make above a struct maybe?
                }
            },
            .pTexelBufferView=NULL
        }
    };

    vkUpdateDescriptorSets(device->device, 2, writes, 0, NULL);
}

static VkPipelineLayout cvm_overlay_pipeline_layout_create(const cvm_vk_device * device, VkDescriptorSetLayout descriptor_set_layout)
{
    VkResult created;
    VkPipelineLayout pipeline_layout=VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo create_info=
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=1,
        .pSetLayouts=(VkDescriptorSetLayout[1])
        {
            descriptor_set_layout,
        },
        .pushConstantRangeCount=1,
        .pPushConstantRanges=(VkPushConstantRange[1])
        {
            {
                .stageFlags=VK_SHADER_STAGE_VERTEX_BIT,
                .offset=0,
                .size=sizeof(float)*2,
            }
        }
    };

    created = vkCreatePipelineLayout(device->device, &create_info, device->host_allocator, &pipeline_layout);
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
                .samples=VK_SAMPLE_COUNT_1_BIT,
                .loadOp=clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout=initial_layout,
                .finalLayout=final_layout,
            },
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
            },
        },
        .dependencyCount=2,
        .pDependencies=(VkSubpassDependency[2])
        {
            {
                .srcSubpass=VK_SUBPASS_EXTERNAL,
                .dstSubpass=0,
                .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            },
            {
                .srcSubpass=0,
                .dstSubpass=VK_SUBPASS_EXTERNAL,
                .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
            },
        },
    };

    created = vkCreateRenderPass(device->device, &create_info, device->host_allocator, &render_pass);
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

    created=vkCreateFramebuffer(device->device, &create_info, device->host_allocator, &framebuffer);
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

    created=vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &create_info, device->host_allocator, &pipeline);
    assert(created == VK_SUCCESS);

    return pipeline;
}

static void cvm_overlay_images_initialise(cvm_overlay_images * images, const cvm_vk_device * device,
                                          uint32_t alpha_image_width , uint32_t alpha_image_height ,
                                          uint32_t colour_image_width, uint32_t colour_image_height)
{
    VkResult created;
    /// images must be powers of 2
    assert((alpha_image_width   & (alpha_image_width  -1)) == 0);
    assert((alpha_image_height  & (alpha_image_height -1)) == 0);
    assert((colour_image_width  & (colour_image_width -1)) == 0);
    assert((colour_image_height & (colour_image_height-1)) == 0);

    VkImageCreateInfo image_creation_info[2]=
    {
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .imageType=VK_IMAGE_TYPE_2D,
            .format=VK_FORMAT_R8_UNORM,
            .extent=(VkExtent3D)
            {
                .width=alpha_image_width,
                .height=alpha_image_height,
                .depth=1
            },
            .mipLevels=1,
            .arrayLayers=1,
            .samples=1,
            .tiling=VK_IMAGE_TILING_OPTIMAL,
            .usage=VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount=0,
            .pQueueFamilyIndices=NULL,
            .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        },
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .imageType=VK_IMAGE_TYPE_2D,
            .format=VK_FORMAT_R8G8B8A8_UNORM,
            .extent=(VkExtent3D)
            {
                .width=colour_image_width,
                .height=colour_image_height,
                .depth=1
            },
            .mipLevels=1,
            .arrayLayers=1,
            .samples=1,
            .tiling=VK_IMAGE_TILING_OPTIMAL,
            .usage=VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount=0,
            .pQueueFamilyIndices=NULL,
            .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        }
    };

    created = cvm_vk_create_images(device, image_creation_info, 2, &images->memory, images->images, images->views);
    assert(created == VK_SUCCESS);

    cvm_vk_create_image_atlas(&images->alpha_atlas , images->images[0], images->views[0], sizeof(uint8_t)  , alpha_image_width , alpha_image_height , false);
    cvm_vk_create_image_atlas(&images->colour_atlas, images->images[1], images->views[1], sizeof(uint8_t)*4, colour_image_width, colour_image_height, false);
}

static void cvm_overlay_images_terminate(const cvm_vk_device * device, cvm_overlay_images * images)
{
    cvm_vk_destroy_image_atlas(&images->alpha_atlas);
    cvm_vk_destroy_image_atlas(&images->colour_atlas);

    vkDestroyImageView(device->device, images->views[0], device->host_allocator);
    vkDestroyImageView(device->device, images->views[1], device->host_allocator);

    vkDestroyImage(device->device, images->images[0], device->host_allocator);
    vkDestroyImage(device->device, images->images[1], device->host_allocator);

    cvm_vk_free_memory(images->memory);
}



static inline void cvm_overlay_frame_resources_initialise(struct cvm_overlay_frame_resources* frame_resources, const cvm_vk_device* device,
    const struct cvm_overlay_target_resources* target_resources, const struct cvm_overlay_target * target)
{
    frame_resources->image_view = target->image_view;
    frame_resources->image_view_unique_identifier = target->image_view_unique_identifier;

    frame_resources->framebuffer = cvm_overlay_framebuffer_create(device, target->extent, target_resources->render_pass, target->image_view);
}

static inline void cvm_overlay_frame_resources_terminate(struct cvm_overlay_frame_resources* frame_resources, const cvm_vk_device* device)
{
    vkDestroyFramebuffer(device->device, frame_resources->framebuffer, device->host_allocator);
}

static inline struct cvm_overlay_frame_resources* cvm_overlay_frame_resources_acquire(struct cvm_overlay_target_resources* target_resources, const cvm_vk_device * device, const struct cvm_overlay_target * target)
{
    struct cvm_overlay_frame_resources* frame_resources;
    bool evicted;

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

    return frame_resources;
}

static inline void cvm_overlay_frame_resources_release(struct cvm_overlay_frame_resources* frame_resources, cvm_vk_timeline_semaphore_moment completion_moment)
{
    /// still exists in the cache, needn't do anything
}



static inline void cvm_overlay_target_resources_initialise(struct cvm_overlay_target_resources* target_resources, const cvm_vk_device* device,
    const struct cvm_overlay_rendering_static_resources * static_resources, const struct cvm_overlay_target * target)
{
    /// set cache key information
    target_resources->extent = target->extent;
    target_resources->format = target->format;
    target_resources->color_space = target->color_space;
    target_resources->initial_layout = target->initial_layout;
    target_resources->final_layout = target->final_layout;
    target_resources->clear_image = target->clear_image;

    target_resources->render_pass = cvm_overlay_render_pass_create(device, target->format, target->initial_layout, target->final_layout, target->clear_image);
    target_resources->pipeline = cvm_overlay_pipeline_create(device, static_resources->pipeline_stages, static_resources->pipeline_layout, target_resources->render_pass, target->extent);

    cvm_overlay_frame_resources_cache_initialise(&target_resources->frame_resources, 8);

    target_resources->last_use_moment = CVM_VK_TIMELINE_SEMAPHORE_MOMENT_NULL;
}

static inline void cvm_overlay_target_resources_terminate(struct cvm_overlay_target_resources* target_resources, const cvm_vk_device* device)
{
    struct cvm_overlay_frame_resources* frame_resources;

    assert(target_resources->last_use_moment.semaphore != VK_NULL_HANDLE);
    cvm_vk_timeline_semaphore_moment_wait(device, &target_resources->last_use_moment);

    while((frame_resources = cvm_overlay_frame_resources_cache_evict(&target_resources->frame_resources)))
    {
        cvm_overlay_frame_resources_terminate(frame_resources, device);
    }
    cvm_overlay_frame_resources_cache_terminate(&target_resources->frame_resources);

    vkDestroyRenderPass(device->device, target_resources->render_pass, device->host_allocator);
    vkDestroyPipeline(device->device, target_resources->pipeline, device->host_allocator);
}

static inline bool cvm_overlay_target_resources_compatible_with_target(const struct cvm_overlay_target_resources* target_resources, const struct cvm_overlay_target * target)
{
    return target_resources != NULL &&
        target_resources->extent.width == target->extent.width &&
        target_resources->extent.height == target->extent.height &&
        target_resources->format == target->format &&
        target_resources->color_space == target->color_space &&
        target_resources->initial_layout == target->initial_layout &&
        target_resources->final_layout == target->final_layout &&
        target_resources->clear_image == target->clear_image;
}

static inline void cvm_overlay_target_resources_prune(cvm_overlay_renderer * renderer, const cvm_vk_device * device)
{
    struct cvm_overlay_target_resources* target_resources;
    /// prune out of date resources
    while(renderer->target_resources.count > 1)
    {
        ///deletion queue, get the first entry ready to be deleted
        target_resources = cvm_overlay_target_resources_queue_get_front_ptr(&renderer->target_resources);
        assert(target_resources->last_use_moment.semaphore != VK_NULL_HANDLE);
        if(cvm_vk_timeline_semaphore_moment_query(device, &target_resources->last_use_moment))
        {
            cvm_overlay_target_resources_terminate(target_resources, device);
            cvm_overlay_target_resources_queue_dequeue(&renderer->target_resources, NULL);
        }
        else break;
    }
}

#warning "target_resources" needs a better name
static inline struct cvm_overlay_target_resources* cvm_overlay_target_resources_acquire(cvm_overlay_renderer * renderer, const cvm_vk_device * device, const struct cvm_overlay_target * target)
{
    struct cvm_overlay_target_resources* target_resources;

    target_resources = cvm_overlay_target_resources_queue_get_back_ptr(&renderer->target_resources);
    if(!cvm_overlay_target_resources_compatible_with_target(target_resources, target))
    {
        target_resources = cvm_overlay_target_resources_queue_new(&renderer->target_resources);

        cvm_overlay_target_resources_initialise(target_resources, device, &renderer->static_resources, target);
    }

    cvm_overlay_target_resources_prune(renderer, device);

    return target_resources;
}

static inline void cvm_overlay_target_resources_release(struct cvm_overlay_target_resources* target_resources, cvm_vk_timeline_semaphore_moment completion_moment)
{
    target_resources->last_use_moment = completion_moment;
    /// still exists in the cache, needn't do anything more
}


static inline void cvm_overlay_add_target_acquire_instructions(cvm_vk_command_buffer * command_buffer, const struct cvm_overlay_target * target)
{
    cvm_vk_command_buffer_add_wait_info(command_buffer, target->wait_semaphores, target->wait_semaphore_count);

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

static inline void cvm_overlay_add_target_release_instructions(cvm_vk_command_buffer * command_buffer, const struct cvm_overlay_target * target)
{
    cvm_vk_command_buffer_add_signal_info(command_buffer, target->signal_semaphores, target->signal_semaphore_count);

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


static inline void cvm_overlay_transient_resources_initialise(struct cvm_overlay_transient_resources* transient_resources, const cvm_vk_device* device, const cvm_overlay_renderer * renderer)
{
    transient_resources->last_use_moment = CVM_VK_TIMELINE_SEMAPHORE_MOMENT_NULL;

    cvm_vk_command_pool_initialise(&transient_resources->command_pool, device, device->graphics_queue_family_index, 0);
    transient_resources->descriptor_set = cvm_overlay_descriptor_set_allocate(device, renderer->static_resources.descriptor_pool, renderer->static_resources.descriptor_set_layout);
}

static inline void cvm_overlay_transient_resources_terminate(struct cvm_overlay_transient_resources* transient_resources, const cvm_vk_device* device)
{
    cvm_vk_timeline_semaphore_moment_wait(device, &transient_resources->last_use_moment);
    cvm_vk_command_pool_terminate(&transient_resources->command_pool, device);
}

/// may stall
static inline struct cvm_overlay_transient_resources* cvm_overlay_transient_resources_acquire(cvm_overlay_renderer * renderer, const cvm_vk_device* device)
{
    struct cvm_overlay_transient_resources* transient_resources;
    bool dequeued;

    if(renderer->transient_count_initialised < renderer->transient_count)
    {
        transient_resources = renderer->transient_resources_backing + renderer->transient_count_initialised;

        cvm_overlay_transient_resources_initialise(transient_resources, device, renderer);

        renderer->transient_count_initialised++;
    }
    else
    {
        dequeued = cvm_overlay_transient_resources_queue_dequeue(&renderer->transient_resources_queue, &transient_resources);
        assert(dequeued);

        cvm_vk_timeline_semaphore_moment_wait(device, &transient_resources->last_use_moment);

        /// reset resources
        cvm_vk_command_pool_reset(&transient_resources->command_pool, device);
    }

    return transient_resources;
}

static inline void cvm_overlay_transient_resources_release(cvm_overlay_renderer * renderer, struct cvm_overlay_transient_resources* transient_resources, cvm_vk_timeline_semaphore_moment completion_moment)
{
    transient_resources->last_use_moment = completion_moment;
    cvm_overlay_transient_resources_queue_enqueue(&renderer->transient_resources_queue, transient_resources);
}


void cvm_overlay_rendering_static_resources_initialise(struct cvm_overlay_rendering_static_resources * static_resources, const cvm_vk_device * device, uint32_t renderer_transient_count)
{
    static_resources->descriptor_pool = cvm_overlay_descriptor_pool_create(device, renderer_transient_count);
    static_resources->descriptor_set_layout = cvm_overlay_descriptor_set_layout_create(device);

    static_resources->pipeline_layout = cvm_overlay_pipeline_layout_create(device, static_resources->descriptor_set_layout);
    cvm_vk_create_shader_stage_info(static_resources->pipeline_stages+0,"cvm_shared/shaders/overlay.vert.spv",VK_SHADER_STAGE_VERTEX_BIT);
    cvm_vk_create_shader_stage_info(static_resources->pipeline_stages+1,"cvm_shared/shaders/overlay.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

    cvm_overlay_images_initialise(&static_resources->images, device, 1024, 1024, 1024, 1024);
}
void cvm_overlay_rendering_static_resources_terminate(struct cvm_overlay_rendering_static_resources * static_resources, const cvm_vk_device * device)
{
    cvm_overlay_images_terminate(device, &static_resources->images);

    cvm_vk_destroy_shader_stage_info(static_resources->pipeline_stages+0);
    cvm_vk_destroy_shader_stage_info(static_resources->pipeline_stages+1);

    vkDestroyPipelineLayout(device->device, static_resources->pipeline_layout, device->host_allocator);

    vkDestroyDescriptorPool(device->device, static_resources->descriptor_pool, device->host_allocator);
    vkDestroyDescriptorSetLayout(device->device, static_resources->descriptor_set_layout, device->host_allocator);
}



void cvm_overlay_render_batch_initialise(struct cvm_overlay_render_batch* batch, const struct cvm_vk_device* device, VkDeviceSize shunt_buffer_max_size)
{
    VkDeviceSize shunt_buffer_alignment = cvm_vk_buffer_alignment_requirements(device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    cvm_overlay_element_render_data_stack_initialise(&batch->render_elements);
    cvm_vk_shunt_buffer_initialise(&batch->upload_shunt_buffer, shunt_buffer_alignment, shunt_buffer_max_size, false);///256k, plenty for per frame

    cvm_vk_buffer_image_copy_stack_initialise(&batch->alpha_atlas_copy_actions);
    cvm_vk_buffer_image_copy_stack_initialise(&batch->colour_atlas_copy_actions);
}

void cvm_overlay_render_batch_terminate(struct cvm_overlay_render_batch* batch)
{
    cvm_vk_buffer_image_copy_stack_terminate(&batch->alpha_atlas_copy_actions);
    cvm_vk_buffer_image_copy_stack_terminate(&batch->colour_atlas_copy_actions);

    cvm_vk_shunt_buffer_terminate(&batch->upload_shunt_buffer);
    cvm_overlay_element_render_data_stack_terminate(&batch->render_elements);
}



/// can actually easily sub in/out atlases, will just require re-filling them from cpu data (or can even manually copy over data between atlases, which would defragment in the process)
void cvm_overlay_render_batch_build(struct cvm_overlay_render_batch* batch, widget * menu_widget, struct cvm_vk_image_atlas* colour_atlas, struct cvm_vk_image_atlas* alpha_atlas)
{
    /// copy actions should be reset when copied, this must have been done before resetting the batch (overlay system relies on these entries having been staged and uploaded)
    assert(batch->alpha_atlas_copy_actions.count == 0);
    assert(batch->colour_atlas_copy_actions.count == 0);

    cvm_vk_shunt_buffer_reset(&batch->upload_shunt_buffer);
    cvm_overlay_element_render_data_stack_reset(&batch->render_elements);

    batch->colour_atlas = colour_atlas;
    batch->alpha_atlas  = alpha_atlas;

    render_widget_overlay(batch, menu_widget);

    batch->screen_w = menu_widget->base.r.x2;
    batch->screen_h = menu_widget->base.r.y2;
}

void cvm_overlay_render_batch_stage(const struct cvm_vk_device * device, struct cvm_overlay_render_batch* batch, struct cvm_vk_staging_buffer_* staging_buffer, const float* colour_array, VkDescriptorSet descriptor_set)
{
    VkDeviceSize upload_offset, elements_offset, uniform_offset, staging_space;

    /// upload all staged resources needed by this frame (uniforms, uploaded data, elements)
    uniform_offset  = 0;
    upload_offset   = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, uniform_offset + sizeof(float)*4*OVERLAY_NUM_COLOURS);
    elements_offset = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, upload_offset  + cvm_vk_shunt_buffer_get_space_used(&batch->upload_shunt_buffer));
    staging_space   = cvm_vk_staging_buffer_allocation_align_offset(staging_buffer, elements_offset + cvm_overlay_element_render_data_stack_size(&batch->render_elements));

    batch->staging_buffer_allocation = cvm_vk_staging_buffer_allocation_acquire(staging_buffer, device, staging_space, true);

    char* const staging_mapping = batch->staging_buffer_allocation.mapping;
    const VkDeviceSize staging_offset = batch->staging_buffer_allocation.acquired_offset;

    batch->element_offset = staging_offset + elements_offset;
    batch->upload_offset  = staging_offset + upload_offset;

    memcpy(staging_mapping + uniform_offset, colour_array, sizeof(float)*4*OVERLAY_NUM_COLOURS);
    cvm_vk_shunt_buffer_copy(&batch->upload_shunt_buffer, staging_mapping + upload_offset);
    cvm_overlay_element_render_data_stack_copy(&batch->render_elements, staging_mapping + elements_offset);

    ///flush all uploads
    cvm_vk_staging_buffer_allocation_flush_range(staging_buffer, device, &batch->staging_buffer_allocation, 0, staging_space);

    cvm_overlay_descriptor_set_write(device, descriptor_set, batch->colour_atlas->supervised_image.view, batch->alpha_atlas->supervised_image.view, staging_buffer->buffer, staging_offset+uniform_offset);
    batch->descriptor_set = descriptor_set;
}

///copy staged data and apply barriers to atlas images
void cvm_overlay_render_batch_prepare_for_rendering(struct cvm_overlay_render_batch* batch, VkCommandBuffer command_buffer)
{
    const VkBuffer staging_buffer = batch->staging_buffer_allocation.parent->buffer;
    cvm_vk_image_atlas_submit_all_pending_copy_actions(batch->colour_atlas, command_buffer, staging_buffer, batch->upload_offset, &batch->colour_atlas_copy_actions);
    cvm_vk_image_atlas_submit_all_pending_copy_actions(batch->alpha_atlas , command_buffer, staging_buffer, batch->upload_offset, &batch->alpha_atlas_copy_actions);

    /// make sure the atlases are ready for rendering
    cvm_vk_image_atlas_barrier(batch->colour_atlas, command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
    cvm_vk_image_atlas_barrier(batch->alpha_atlas , command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
}

/// pass in `struct cvm_overlay_rendering_resources*` instead of pipeline layout
/**
pipeline_layout: completely static, singular
pipeline: changes with target, singular
descriptor set (from batch): must come from managed per-frame resources, dynamic and numerous
*/
void cvm_overlay_render_batch_render(struct cvm_overlay_render_batch* batch, VkPipelineLayout pipeline_layout, VkCommandBuffer command_buffer, VkPipeline pipeline)
{
    #warning can get screen dimensions from batch OR assert the ones in the batch are correct
    float push_constants[2]={2.0/(float)batch->screen_w, 2.0/(float)batch->screen_h};
    vkCmdPushConstants(command_buffer,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(float) * 2, push_constants);
    /// set index (firstSet) is defined in pipeline creation (index in array)
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &batch->descriptor_set, 0, NULL);

    vkCmdBindPipeline(command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &batch->staging_buffer_allocation.parent->buffer, &batch->element_offset);///little bit of hacky stuff to create lvalue
    vkCmdDraw(command_buffer, 4, batch->render_elements.count, 0, 0);
}


void cvm_overlay_render_batch_complete(struct cvm_overlay_render_batch* batch, cvm_vk_timeline_semaphore_moment completion_moment)
{
    cvm_vk_staging_buffer_allocation_release(&batch->staging_buffer_allocation, completion_moment);
}

/**
render batch has 4 stages:
build: fill out element buffer and figure out what to upload, no real vulkan/GPU stuff
stage: all stuff prior to rendering ops, get space in staging buffer (this wants to happen as close to rendering as possible) and upload all required data into this staging buffer space
        ^ requires command buffer (but could be split into commabd buffer and non command buffer parts)
render: simple, bind resources and submit the render ops
complete: provide a completion moment to resources as necessary

might be good to track this with an enum of state/stage
*/



void cvm_overlay_renderer_initialise(cvm_overlay_renderer * renderer, cvm_vk_device * device, struct cvm_vk_staging_buffer_ * staging_buffer, uint32_t renderer_cycle_count)
{
    renderer->transient_count = renderer_cycle_count;
    renderer->transient_count_initialised = 0;
    renderer->transient_resources_backing = malloc(sizeof(struct cvm_overlay_transient_resources) * renderer_cycle_count);
    cvm_overlay_transient_resources_queue_initialise(&renderer->transient_resources_queue);
    renderer->staging_buffer = staging_buffer;

//    cvm_vk_shunt_buffer_initialise(&renderer->shunt_buffer, staging_buffer->alignment, 1<<18, false);

    cvm_overlay_rendering_static_resources_initialise(&renderer->static_resources, device, renderer_cycle_count);


    renderer->render_batch = malloc(sizeof(struct cvm_overlay_render_batch));
    cvm_overlay_render_batch_initialise(renderer->render_batch, device, 1<<18);


    cvm_overlay_target_resources_queue_initialise(&renderer->target_resources);
}

void cvm_overlay_renderer_terminate(cvm_overlay_renderer * renderer, cvm_vk_device * device)
{
    struct cvm_overlay_target_resources * target_resources;
    uint32_t i;

    for(i=0;i<renderer->transient_count_initialised;i++)
    {
        cvm_overlay_transient_resources_terminate(renderer->transient_resources_backing + i, device);
    }
    free(renderer->transient_resources_backing);
    cvm_overlay_transient_resources_queue_terminate(&renderer->transient_resources_queue);

    cvm_overlay_rendering_static_resources_terminate(&renderer->static_resources, device);


    cvm_overlay_render_batch_terminate(renderer->render_batch);
    free(renderer->render_batch);

    while((target_resources = cvm_overlay_target_resources_queue_dequeue_ptr(&renderer->target_resources)))
    {
        cvm_overlay_target_resources_terminate(target_resources, device);
    }
    cvm_overlay_target_resources_queue_terminate(&renderer->target_resources);
}






cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_target(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, const struct cvm_overlay_target* target)
{
    cvm_vk_command_buffer cb;
    cvm_vk_timeline_semaphore_moment completion_moment;
//    cvm_vk_staging_buffer_allocation staging_buffer_allocation;
//    VkDeviceSize upload_offset,instance_offset,uniform_offset,staging_space;
    struct cvm_overlay_render_batch* render_batch;
    struct cvm_vk_staging_buffer_ * staging_buffer;
    struct cvm_overlay_target_resources* target_resources;
    struct cvm_overlay_frame_resources* frame_resources;
    struct cvm_overlay_transient_resources* transient_resources;
    VkDeviceSize staging_offset;
    char * staging_mapping;
    float screen_w,screen_h;

    staging_buffer = renderer->staging_buffer;

    render_batch = renderer->render_batch;

    /// setup/reset the render batch
    cvm_overlay_render_batch_build(render_batch, menu_widget, &renderer->static_resources.images.colour_atlas, &renderer->static_resources.images.alpha_atlas);



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


    /// acting on the shunt buffer directly in this way feels a little off

    transient_resources = cvm_overlay_transient_resources_acquire(renderer, device);
    target_resources = cvm_overlay_target_resources_acquire(renderer, device, target);
    frame_resources = cvm_overlay_frame_resources_acquire(target_resources, device, target);

    cvm_vk_command_pool_acquire_command_buffer(&transient_resources->command_pool, device, &cb);

    ///this uses the shunt buffer!

    cvm_overlay_render_batch_stage(device, render_batch, staging_buffer, overlay_colours, transient_resources->descriptor_set);
    cvm_overlay_render_batch_prepare_for_rendering(render_batch, cb.buffer);




    ///start of graphics
    cvm_overlay_add_target_acquire_instructions(&cb, target);


    VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext=NULL,
        .renderPass=target_resources->render_pass,
        .framebuffer=frame_resources->framebuffer,
        .renderArea=cvm_vk_get_default_render_area(),
        .clearValueCount=1,
        .pClearValues=(VkClearValue[1]){{.color=(VkClearColorValue){.float32={0.0f,0.0f,0.0f,0.0f}}}},
        /// ^ do we even wat to clear? or should we assume all pixels will be rendered?
    };

    vkCmdBeginRenderPass(cb.buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

    cvm_overlay_render_batch_render(render_batch, renderer->static_resources.pipeline_layout, cb.buffer, target_resources->pipeline);

    vkCmdEndRenderPass(cb.buffer);///================


    cvm_overlay_add_target_release_instructions(&cb, target);

    completion_moment = cvm_vk_command_pool_submit_command_buffer(&transient_resources->command_pool, device, &cb, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    cvm_overlay_render_batch_complete(render_batch, completion_moment);

    cvm_overlay_target_resources_release(target_resources, completion_moment);
    cvm_overlay_frame_resources_release(frame_resources, completion_moment);
    cvm_overlay_transient_resources_release(renderer, transient_resources, completion_moment);

    return completion_moment;
}


cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_presentable_image(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, cvm_vk_swapchain_presentable_image * presentable_image, bool last_use)
{
    uint32_t overlay_queue_family_index;
    cvm_vk_timeline_semaphore_moment completion_moment;

    bool first_use = (presentable_image->layout == VK_IMAGE_LAYOUT_UNDEFINED);

    struct cvm_overlay_target target =
    {
        .image_view = presentable_image->image_view,
        .image_view_unique_identifier = presentable_image->image_view_unique_identifier,

        .extent = presentable_image->parent_swapchain_instance->surface_capabilities.currentExtent,
        .format = presentable_image->parent_swapchain_instance->surface_format.format,
        .color_space = presentable_image->parent_swapchain_instance->surface_format.colorSpace,

        .initial_layout = presentable_image->layout,
        .final_layout = last_use ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .clear_image = first_use,

        .wait_semaphore_count = 0,
        .acquire_barrier_count = 0,

        .signal_semaphore_count = 0,
        .release_barrier_count = 0,
    };


    #warning genericize the following with a function

    if(first_use)/// can figure out from state
    {
        assert(presentable_image->state == CVM_VK_PRESENTABLE_IMAGE_STATE_ACQUIRED);///also indicates first use
        presentable_image->state = CVM_VK_PRESENTABLE_IMAGE_STATE_STARTED;
        target.wait_semaphores[target.wait_semaphore_count++] = cvm_vk_binary_semaphore_submit_info(presentable_image->acquire_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    assert(presentable_image->state == CVM_VK_PRESENTABLE_IMAGE_STATE_STARTED);

    if(last_use)/// don't have a perfect way to figure out, but could infer from
    {
        overlay_queue_family_index = device->graphics_queue_family_index;

        presentable_image->last_use_queue_family = overlay_queue_family_index;

        // can present on this queue family
        if(presentable_image->parent_swapchain_instance->queue_family_presentable_mask | (1<<overlay_queue_family_index))
        {
            presentable_image->state = CVM_VK_PRESENTABLE_IMAGE_STATE_COMPLETE;
            /// signal after any stage that could modify the image contents
            target.signal_semaphores[target.signal_semaphore_count++] = cvm_vk_binary_semaphore_submit_info(presentable_image->present_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        }
        else
        {
            presentable_image->state = CVM_VK_PRESENTABLE_IMAGE_STATE_TRANSFERRED;

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

    completion_moment = cvm_overlay_render_to_target(device, renderer, menu_widget, &target);

    presentable_image->last_use_moment = completion_moment;
    presentable_image->layout = target.final_layout;
    /// must record changes made to layout

    return completion_moment;
}



