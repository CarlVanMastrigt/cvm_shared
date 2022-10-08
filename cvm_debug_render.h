/**
Copyright 2021 Carl van Mastrigt

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

#ifndef CVM_DEBUG_RENDER_H
#define CVM_DEBUG_RENDER_H

typedef struct debug_vertex_data
{
    vec3f pos;
    uint32_t col;///4 unorm8 encoded together
}
debug_vertex_data;

typedef struct debug_render_data
{
    cvm_vk_transient_buffer * transient_buffer;
    debug_vertex_data * vertices;
    VkDeviceSize vertex_offset;
    uint32_t vertex_count;
    uint32_t vertex_space;
}
debug_render_data;

void create_debug_render_data(VkDescriptorSetLayout * descriptor_set_layout);
void destroy_debug_render_data(void);

void create_debug_swapchain_dependent_render_data(VkRenderPass render_pass,uint32_t subpass,VkSampleCountFlagBits sample_count);
void destroy_debug_swapchain_dependent_render_data(void);

static inline uint32_t debug_render_get_transient_buffer_allocation_size(cvm_vk_transient_buffer * tb,uint32_t vert_count)
{
    return cvm_vk_transient_buffer_get_rounded_allocation_size(tb,vert_count*sizeof(debug_vertex_data),sizeof(debug_vertex_data));
}

void prepare_debug_render(debug_render_data * data,cvm_vk_transient_buffer * transient_buffer,uint32_t vert_count);

void add_debug_flat_line(debug_render_data * data,vec3f start_pos,vec3f end_pos,uint32_t col);
void add_debug_gradient_line(debug_render_data * data,vec3f start_pos,vec3f end_pos,uint32_t start_col,uint32_t end_col);

void render_debug(VkCommandBuffer cb,VkDescriptorSet * descriptor_set,debug_render_data * data);

#endif

