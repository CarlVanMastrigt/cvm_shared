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





VkResult cvm_overlay_image_atlases_initialise(struct cvm_overlay_image_atlases* image_atlases, const struct cvm_vk_device* device, uint32_t alpha_w, uint32_t alpha_h, uint32_t colour_w, uint32_t colour_h, bool multithreaded)
{
    VkResult result;
    VkImage images[2];
    VkImageView views[2];
    /// images must be powers of 2
    assert((alpha_w  & (alpha_w  -1)) == 0);
    assert((alpha_h  & (alpha_h  -1)) == 0);
    assert((colour_w & (colour_w -1)) == 0);
    assert((colour_h & (colour_h -1)) == 0);

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
                .width  = alpha_w,
                .height = alpha_h,
                .depth  = 1
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
                .width  = colour_w,
                .height = colour_h,
                .depth  = 1
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

    result = cvm_vk_create_images(device, image_creation_info, 2, &image_atlases->memory, images, views);
    if(result != VK_SUCCESS)
    {
        return result;
    }

    cvm_vk_create_image_atlas(&image_atlases->alpha_atlas, images[0], views[0], sizeof(uint8_t), alpha_w, alpha_h, multithreaded);
    image_atlases->alpha_image = images[0];
    image_atlases->alpha_view = views[0];

    cvm_vk_create_image_atlas(&image_atlases->colour_atlas, images[1], views[1], sizeof(uint8_t)*4, colour_w, colour_h, multithreaded);
    image_atlases->colour_image = images[1];
    image_atlases->colour_view = views[1];

    return result;
}

void cvm_overlay_image_atlases_terminate(struct cvm_overlay_image_atlases* image_atlases, const struct cvm_vk_device* device)
{
    cvm_vk_destroy_image_atlas(&image_atlases->alpha_atlas);
    cvm_vk_destroy_image_atlas(&image_atlases->colour_atlas);

    vkDestroyImageView(device->device, image_atlases->alpha_view, device->host_allocator);
    vkDestroyImageView(device->device, image_atlases->colour_view, device->host_allocator);

    vkDestroyImage(device->device, image_atlases->alpha_image, device->host_allocator);
    vkDestroyImage(device->device, image_atlases->colour_image, device->host_allocator);

    vkFreeMemory(device->device, image_atlases->memory, device->host_allocator);
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
#warning replace atlases with cvm_overlay_image_atlases
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

void cvm_overlay_render_batch_stage(struct cvm_overlay_render_batch* batch, const struct cvm_vk_device * device, struct cvm_vk_staging_buffer_* staging_buffer, const float* colour_array, VkDescriptorSet descriptor_set)
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
void cvm_overlay_render_batch_upload(struct cvm_overlay_render_batch* batch, VkCommandBuffer command_buffer)
{
    const VkBuffer staging_buffer = batch->staging_buffer_allocation.parent->buffer;
    cvm_vk_image_atlas_submit_all_pending_copy_actions(batch->colour_atlas, command_buffer, staging_buffer, batch->upload_offset, &batch->colour_atlas_copy_actions);
    cvm_vk_image_atlas_submit_all_pending_copy_actions(batch->alpha_atlas , command_buffer, staging_buffer, batch->upload_offset, &batch->alpha_atlas_copy_actions);
}

void cvm_overlay_render_batch_ready(struct cvm_overlay_render_batch* batch, VkCommandBuffer command_buffer)
{
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


void cvm_overlay_render_batch_finish(struct cvm_overlay_render_batch* batch, cvm_vk_timeline_semaphore_moment completion_moment)
{
    cvm_vk_staging_buffer_allocation_release(&batch->staging_buffer_allocation, completion_moment);
}

/**
render batch has 6 stages:
build: fill out element buffer and figure out what to upload, no real vulkan/GPU stuff
stage: all stuff prior to rendering ops, get space in staging buffer (this wants to happen as close to rendering as possible) and upload all required data into this staging buffer space
        ^ requires command buffer (but could be split into commabd buffer and non command buffer parts)
render: simple, bind resources and submit the render ops
complete: provide a completion moment to resources as necessary

might be good to track this with an enum of state/stage
*/










