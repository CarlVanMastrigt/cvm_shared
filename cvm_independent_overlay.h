/**
Copyright 2024 Carl van Mastrigt

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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_INDEPENDENT_OVERLAY_H
#define CVM_INDEPENDENT_OVERLAY_H

/// for rendering independently of any particular render pass or rendering pipeline
/// can provide enough for a UI only application
/// can act as a starting point for using the general ui/rendering capabilities of the library
/// can be a drop in renderer after other rendering and simply render over the top of other work







/// target information and synchronization requirements
struct cvm_overlay_target
{
    /// framebuffer depends on these
    VkImageView image_view;
    cvm_vk_resource_identifier image_view_unique_identifier;
    /// render pass depends on these
    VkExtent2D extent;
    VkFormat format;
    // start respecting colour space, to do so must support different input colour space (i believe) and must have information on which space blending is done in and how
    VkColorSpaceKHR color_space;/// respecting the colour space is NYI

    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    bool clear_image;

    /// in / out synchronization setup data
    uint32_t wait_semaphore_count;
    uint32_t acquire_barrier_count;
    VkSemaphoreSubmitInfo wait_semaphores[4];
    VkImageMemoryBarrier2 acquire_barriers[4];

    uint32_t signal_semaphore_count;
    uint32_t release_barrier_count;
    VkSemaphoreSubmitInfo signal_semaphores[4];
    VkImageMemoryBarrier2 release_barriers[4];
};





struct cvm_overlay_frame_resources
{
    #warning does this require a last use moment!?
    /// copy of target image view, used as key to the framebuffer cache
    VkImageView image_view;
    cvm_vk_resource_identifier image_view_unique_identifier;

    /// data to cache
    VkFramebuffer framebuffer;
};

#define CVM_CACHE_CMP( entry , key ) (entry->image_view_unique_identifier == key->image_view_unique_identifier) && (entry->image_view == key->image_view)
CVM_CACHE(struct cvm_overlay_frame_resources, struct cvm_overlay_target*, cvm_overlay_frame_resources)
#undef CVM_CACHE_CMP

/// needs a better name
struct cvm_overlay_target_resources
{
    /// used for finding extant resources in cache
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR color_space;
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    bool clear_image;

    /// data to cache
    VkRenderPass render_pass;
    VkPipeline pipeline;

    /// rely on all frame resources being deleted to ensure not in use
    cvm_overlay_frame_resources_cache frame_resources;

    /// moment when this cache entry is no longer in use and can thus be evicted
    cvm_vk_timeline_semaphore_moment last_use_moment;
};

CVM_QUEUE(struct cvm_overlay_target_resources, cvm_overlay_target_resources, 8)
/// queue as is done above might not be best if it's desirable to maintain multiple targets as renderable

/// resources used in a per cycle/frame fashion
struct cvm_overlay_transient_resources
{
    cvm_vk_command_pool command_pool;

    VkDescriptorSet descriptor_set;

    cvm_vk_timeline_semaphore_moment last_use_moment;
};

CVM_QUEUE(struct cvm_overlay_transient_resources*,cvm_overlay_transient_resources,8)

/// fixed sized queue of these?
/// queue init at runtime? (custom size)
/// make cache init at runtime too? (not great but w/e)

struct cvm_overlay_rendering_static_resources
{
    struct cvm_overlay_image_atlases image_atlases;

    VkDescriptorPool descriptor_pool;

    ///these descriptors don't change (only need 1 set, always has the same bindings)
    VkDescriptorSetLayout descriptor_set_layout;

    /// the actual descriptor sets will exist on the work entry (per frame)

    /// for creating pipelines
    VkPipelineLayout pipeline_layout;
    VkPipelineShaderStageCreateInfo pipeline_stages[2];//vertex,fragment
};

#warning this should NOT require a shunt buffer
void cvm_overlay_rendering_static_resources_initialise(struct cvm_overlay_rendering_static_resources * static_resources, const cvm_vk_device * device, uint32_t renderer_transient_count);
void cvm_overlay_rendering_static_resources_terminate(struct cvm_overlay_rendering_static_resources * static_resources, const cvm_vk_device * device);

typedef struct cvm_overlay_renderer
{
    /// for uploading to images, is NOT locally owned
    struct cvm_vk_staging_buffer_ * staging_buffer;


    uint32_t transient_count;
    uint32_t transient_count_initialised;
    struct cvm_overlay_transient_resources* transient_resources_backing;
    cvm_overlay_transient_resources_queue transient_resources_queue;

    /// are separate shunt buffers even the best way to do this??
//    cvm_overlay_element_render_data_stack element_render_stack;
    struct cvm_overlay_render_batch* render_batch;/// <- temporary, move elsewhere
//    cvm_vk_shunt_buffer shunt_buffer;

    struct cvm_overlay_rendering_static_resources static_resources;

    cvm_overlay_target_resources_queue target_resources;
}
cvm_overlay_renderer;

typedef struct cvm_overlay_setup
{
    struct cvm_vk_staging_buffer_ * staging_buffer;

    VkImageLayout initial_target_layout;
    VkImageLayout final_target_layout;

    uint32_t renderer_resource_queue_count;///effectively max frames in flight, size of work queue
    uint32_t target_resource_cache_size;/// size of cache for target dependent resources, effectively max swapchain size, should be lightweight so 8 or 16 is reasonable
}
cvm_overlay_setup;


void cvm_overlay_renderer_initialise(cvm_overlay_renderer * renderer, cvm_vk_device * device, struct cvm_vk_staging_buffer_ * staging_buffer, uint32_t renderer_cycle_count);
void cvm_overlay_renderer_terminate(cvm_overlay_renderer * renderer, cvm_vk_device * device);

cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_target(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, const struct cvm_overlay_target* target);

cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_presentable_image(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, cvm_vk_swapchain_presentable_image * presentable_image, bool last_use);




#endif
