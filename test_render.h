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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif


#ifndef CVM_TEST_RENDER_H
#define CVM_TEST_RENDER_H

typedef struct test_render_data
{
    vec3f pos;
    vec3f c;
}
test_render_data;

void initialise_test_render_data_ext(void);
void terminate_test_render_data_ext(void);

void create_test_render_pass(VkFormat swapchain_format);
//

void initialise_test_swapchain_dependencies_ext(VkRect2D screen_rectangle,VkRenderPass render_pas,uint32_t swapchain_image_count,VkImageView * swapchain_image_views);
void terminate_test_swapchain_dependencies_ext(void);
VkPipeline get_test_pipeline(void);

#endif

