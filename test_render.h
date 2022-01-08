/**
Copyright 2020,2021 Carl van Mastrigt

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

void initialise_test_render_data(void);
void terminate_test_render_data(void);

void initialise_test_swapchain_dependencies(VkSampleCountFlagBits sample_count);
void terminate_test_swapchain_dependencies(void);

cvm_vk_module_batch * test_render_frame(cvm_camera * c);
void test_frame_cleanup(uint32_t swapchain_image_index);

#endif

