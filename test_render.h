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
    float pos[3];
    uint8_t col[3];
}
test_render_data;

void initialise_test_render_data(void);
void terminate_test_render_data(void);

//void create_test_render_pass(VkFormat swapchain_format);

/// applications responsibility to handle recreation of module resources (unless i can come up with a better approach)

void initialise_test_swapchain_dependencies(void);
void terminate_test_swapchain_dependencies(void);

cvm_vk_module_work_block * test_render_frame(camera * c);
void test_frame_cleanup(uint32_t swapchain_image_index);

#endif

