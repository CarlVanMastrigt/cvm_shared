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


#ifndef CVM_VK_H
#define CVM_VK_H

#ifndef CVM_VK_CHECK
#define CVM_VK_CHECK(f)                                                         \
{                                                                               \
    VkResult r=f;                                                               \
    if(r!=VK_SUCCESS)                                                           \
    {                                                                           \
        fprintf(stderr,"VULKAN FUNCTION FAILED : %d :%s\n",r,#f);               \
        exit(-1);                                                               \
    }                                                                           \
}

#endif

#define CVM_VK_MAX_EXTERNAL 256
///256 better?

#define CVM_VK_PRESENTATION_INSTANCE_COUNT 2
///need 1 more than this for each extra layer of cycling (e.g. thread-sync-separation, that is written in thread then passed to main thread before being rendered with)

#define CVM_VK_MAX_SWAPCHAIN_IMAGES 16




typedef void(*cvm_vk_external_initialise)(VkDevice,VkPhysicalDevice,VkRect2D,VkViewport,uint32_t,VkImageView*);
typedef void(*cvm_vk_external_terminate)(VkDevice);
typedef void(*cvm_vk_external_render)(VkCommandBuffer,uint32_t,VkRect2D);///primary command buffer into which to render,  (framebuffer/swapchain_image) index to use for rendering, render_area
///first uint is queue_family, second is swapchain image count









///gfx_data can ONLY change in main thread and ONLY be changed when not in use by other threads (inside main sync mutexes or before starting other threads that use it)


void initialise_cvm_vk(SDL_Window * window);
void terminate_cvm_vk(void);


uint32_t add_cvm_vk_swapchain_dependent_functions(cvm_vk_external_initialise initialise,cvm_vk_external_terminate terminate,cvm_vk_external_render render);


void initialise_cvm_vk_swapchain_and_dependents(void);
void reinitialise_cvm_vk_swapchain_and_dependents(void);
//void terminate_cvm_vk_swapchain_and_dependents(void);

void present_cvm_vk_data(void);

void cvm_vk_wait(void);
VkFormat cvm_vk_get_screen_format(void);
void cvm_vk_create_render_pass(VkRenderPassCreateInfo * render_pass_creation_info,VkRenderPass * render_pass);




void initialise_test_render_data(void);
void terminate_test_render_data(void);

#endif
