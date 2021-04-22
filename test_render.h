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

void initialise_test_render_data_ext(VkDevice device,VkRenderPass render_pass,VkPipelineViewportStateCreateInfo * viewport_state_info);//,VkPipelineMultisampleStateCreateInfo * multisample_state_info
void terminate_test_render_data_ext(VkDevice device);

void create_test_pipeline(VkDevice device);
void destroy_test_pipeline(VkDevice device);
VkPipeline get_test_pipeline(void);

#endif

