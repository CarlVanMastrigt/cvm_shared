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






///REMOVE
#define CVM_VK_MAX_EXTERNAL 256
///REMOVE
#define CVM_VK_PRESENTATION_INSTANCE_COUNT 3
///need 1 more than this for each extra layer of cycling (e.g. thread-sync-separation, that is written in thread then passed to main thread before being rendered with)

#define CVM_VK_INVALID_IMAGE_INDEX 0xFFFFFFFF


/**
features to be cognisant of:

fragmentStoresAndAtomics



(maybe)

vertexPipelineStoresAndAtomics

could move swapchain/resource rebuilding outside of critical section if atomic locking mechanism is employed
    ^ cas to gain control, VK thread must gain absolute control (a==0 -> a=0xFFFFFFFF) whereas any thread using those resources can gain read permissions (a!=0xFFFFFFFF -> a=a+1)
        ^ this probably isnt woth it as some systems might handle busy waiting poorly, also swapchain/render_resource recreation should be a rare operation anyway!

going to have to rely on acquire op failing to know when to recreate swapchain
    ^ also need to take settings changes into account, i.e. manually do same thing as out of date would and also prevent swapchain image acquisition
*/

///won't be supporting modules having msaa output

///render system, upon swapchain creation/recreation should forward acquire swapchain images
///move this to be thread static declaration?

///use swapchain image to index into these? yes
///this should encompass swapchain images? (or at least it could...)

///could move these structs to the c file? - they should only ever be used by cvm_vk internally
///this struct contains the data needed to be known upfront when acquiring swapchain image (cvm_vk_swapchain_frame), some data could go in either struct though...
typedef struct cvm_vk_swapchain_image_acquisition_data
{
    ///would be good to have a way to specify other semaphores &c. here for more complex interdependency

    VkSemaphore acquire_semaphore;///first module should test against this in its submit
    /// need a way to signal this semaphore when we aren't going to render with this image for some reason (e.g. swapchain destruction)
    ///     ^ WAIT, are semaphores used at submit time and can 2 semaphores be used to cycle module submissions if they affect the same stage???)
    ///         ^ probably not possible, multiple frames can be in flight at once! (could do 2 semaphores per frame?? even then probably needless)
    uint32_t image_index;/// set to CVM_VK_INVALID_IMAGE_INDEX to indicate failure to acquire
}
cvm_vk_swapchain_image_acquisition_data;


typedef struct cvm_vk_swapchain_image_present_data
{
    VkImage image;///theese are provided by the WSI
    VkImageView image_view;

    VkFence completion_fence;
    bool in_flight;///error checking

    ///following only used if present and graphics are different
    ///     ^ test as best as possible with bool that forces behaviour, and maybe try different queue/queue_family when possible (o.e. when available hardware allows)
    /// timeline semaphore in renderer instead?
    VkCommandBuffer graphics_relinquish_command_buffer;///graphics finalise will almost certianly always be the same, so create it at startup with multi submit
    VkCommandBuffer present_acquire_command_buffer;

    VkSemaphore graphics_relinquish_semaphore;///only necessary when transferring to a dedicated present queue, used as part of queue transfer
    VkSemaphore present_semaphore;///needed by VkPresentInfoKHR
}
cvm_vk_swapchain_image_present_data;


/// this is basically the low priority transfer function, should be used for low priority work (bulk long lived data that isnt needed immediately and for defragging memory/images)


//typedef enum
//{
//    CVM_VK_QUEUE_TYPE_TRANSFER,
//    CVM_VK_QUEUE_TYPE_GRAPHICS,
//    CVM_VK_QUEUE_TYPE_COMPUTE, /// need to add support for this one
//    CVM_VK_QUEUE_TYPE_PRESENT, /// not sure this one really necessary tbh
//    CVM_VK_QUEUE_TYPE_COUNT,
//}
//cvm_vk_queue_type;


/// panel, slice, block, unit, frame
/// frame for graphics, slice/unit/block for transfer?


///misc data the rendering system needs from each module, as well as container for module's per frame rendering data, should be static var in module file
typedef struct cvm_vk_module_data cvm_vk_module_data;

///combine concepts and call them work units?
///using same names as vulkan would probably be best, graphics command, transfer command or just cvm_vk_commands
typedef struct cvm_vk_module_work_block
{
    /// synchronisation handled by the acquired frame/image
    cvm_vk_module_data * parent_module;

    VkCommandPool graphics_pool;
    VkCommandBuffer graphics_work;///for now stack everything on graphics queue, later make a work block support many interdependent command buffers with semaphores as necessary

    ///make these the same when they actually share a queue family/queue
    VkCommandPool transfer_pool;
    VkCommandBuffer transfer_work;

    /// need intra-frame and inter-frame (the structurally difficult one) semaphores to do ownership transfers, and structure to handle that
    /// support for multiple command buffers per frame each on different queue when necessary will allow for much more fine grained control and better GPU utilization (structure needs to support this)
    /// above allows a queue(family) to be working on (have ownership of) resources NOT utilised by by another queue(family) and be doing work in "tandem" with that queue which is using different resources at that time
    /// a good example of this is performing game transfers during overlay rendering and vice-versa

    /// also keep in mind VK_SHARING_MODE_CONCURRENT where viable and profile it
    /// also keep in mind transfer on graphics queue is possible and profile that as well

    /// do inter-block semaphores invalidate any benefit for multiple framebuffer images (semaphores in chain do affect only specific stages...)
    /// ^ will depend on usage patterns but i think in general the answer is yes, is worth looking at example of intended use

    /// break into multiple submits/command_buffers to break up dependencies?? -- THIS

    /// ^ also i have no way to test any of this on my current hardware...

    /// could/should add async compute, but it has same dependency problems as transfer unless using VK_SHARING_MODE_CONCURRENT (as mentioned above)

    /// also probably worth investigating whether per-frame resources are worth transferring to device local (double)buffer before using on the gpu

    uint32_t has_work:1;///if this frame doesnt have work then dont issue commands! (mainly for transfer, graphics should ALWAYS have work as its responsible for transitioning images)
    uint32_t in_flight:1;///used for error checking
}
cvm_vk_module_work_block;

struct cvm_vk_module_data
{
    cvm_vk_module_work_block * work_blocks;

    uint32_t block_count;
    uint32_t block_index;
};

///gfx_data can ONLY change in main thread and ONLY be changed when not in use by other threads (inside main sync mutexes or before starting other threads that use it)

void cvm_vk_initialise(SDL_Window * window,uint32_t min_swapchain_images,uint32_t extra_swapchain_images,bool sync_compute_required);///this extra is the max extra used by any module
void cvm_vk_terminate(void);///also terminates swapchain dependant data at same time

void cvm_vk_create_swapchain(void);
void cvm_vk_destroy_swapchain(void);






void cvm_vk_create_render_pass(VkRenderPass * render_pass,VkRenderPassCreateInfo * info);
void cvm_vk_destroy_render_pass(VkRenderPass render_pass);

void cvm_vk_create_framebuffer(VkFramebuffer * framebuffer,VkFramebufferCreateInfo * info);
void cvm_vk_destroy_framebuffer(VkFramebuffer framebuffer);

void cvm_vk_create_pipeline_layout(VkPipelineLayout * pipeline_layout,VkPipelineLayoutCreateInfo * info);
void cvm_vk_destroy_pipeline_layout(VkPipelineLayout pipeline_layout);

void cvm_vk_create_graphics_pipeline(VkPipeline * pipeline,VkGraphicsPipelineCreateInfo * info);
void cvm_vk_destroy_pipeline(VkPipeline pipeline);

void cvm_vk_create_shader_stage_info(VkPipelineShaderStageCreateInfo * stage_info,const char * filename,VkShaderStageFlagBits stage);
void cvm_vk_destroy_shader_stage_info(VkPipelineShaderStageCreateInfo * stage_info);



void cvm_vk_create_descriptor_set_layout(VkDescriptorSetLayout * descriptor_set_layout,VkDescriptorSetLayoutCreateInfo * info);
void cvm_vk_destroy_descriptor_set_layout(VkDescriptorSetLayout descriptor_set_layout);

void cvm_vk_create_descriptor_pool(VkDescriptorPool * descriptor_pool,VkDescriptorPoolCreateInfo * info);
void cvm_vk_destroy_descriptor_pool(VkDescriptorPool descriptor_pool);

void cvm_vk_allocate_descriptor_sets(VkDescriptorSet * descriptor_sets,VkDescriptorSetAllocateInfo * info);
void cvm_vk_write_descriptor_sets(VkWriteDescriptorSet * writes,uint32_t count);

void cvm_vk_create_image(VkImage * image,VkImageCreateInfo * info);
void cvm_vk_destroy_image(VkImage image);

void cvm_vk_create_and_bind_memory_for_images(VkDeviceMemory * memory,VkImage * images,uint32_t image_count);

void cvm_vk_create_image_view(VkImageView * image_view,VkImageViewCreateInfo * info);
void cvm_vk_destroy_image_view(VkImageView image_view);

void cvm_vk_free_memory(VkDeviceMemory memory);

void * cvm_vk_create_buffer(VkBuffer * buffer,VkDeviceMemory * memory,VkBufferUsageFlags usage,VkDeviceSize size,bool require_host_visible);
void cvm_vk_destroy_buffer(VkBuffer buffer,VkDeviceMemory memory,void * mapping);
void cvm_vk_flush_buffer_memory_range(VkMappedMemoryRange * flush_range);
uint32_t cvm_vk_get_buffer_alignment_requirements(VkBufferUsageFlags usage);


uint32_t cvm_vk_prepare_for_next_frame(bool rendering_resources_invalid);
void cvm_vk_transition_frame(void);///must be called in critical section!
void cvm_vk_present_current_frame(cvm_vk_module_work_block ** work_blocks, uint32_t work_block_count);
bool cvm_vk_recreate_rendering_resources(void);///this and operations resulting from it returning true, must be called in critical section
bool cvm_vk_check_for_remaining_frames(uint32_t * completed_frame_index);


uint32_t cvm_vk_get_transfer_queue_family(void);
uint32_t cvm_vk_get_graphics_queue_family(void);

VkRect2D cvm_vk_get_screen_rectangle(void);
VkFormat cvm_vk_get_screen_format(void);///can remove?
uint32_t cvm_vk_get_swapchain_image_count(void);
VkImageView cvm_vk_get_swapchain_image_view(uint32_t index);


VkSampler cvm_vk_get_fetch_sampler(void);

/// (NYI) these are needed for cases where modules delegate rendering to a worker thread by way of secondary command buffers, which then need their own command pool
/// also need functions to allocate command buffers from these pools
//void cvm_vk_create_graphics_command_pool(VkCommandPool * command_pool);
//void cvm_vk_create_transfer_command_pool(VkCommandPool * command_pool);
//void cvm_vk_destroy_command_pool(VkCommandPool * command_pool);

///must be called after cvm_vk_initialise
void cvm_vk_create_module_data(cvm_vk_module_data * module_data,bool in_separate_thread);///extra_transfer_slots should be equal to number passed in to extra_frame_count elsewhere
void cvm_vk_resize_module_graphics_data(cvm_vk_module_data * module_data,uint32_t extra_frame_count);///this must be called in critical section
void cvm_vk_destroy_module_data(cvm_vk_module_data * module_data,bool in_separate_thread);

cvm_vk_module_work_block * cvm_vk_begin_module_work_block(cvm_vk_module_data * module_data,uint32_t frame_offset,uint32_t * swapchain_image_index);///have this return bool? (VkCommandBuffer through ref.)
cvm_vk_module_work_block * cvm_vk_end_module_work_block(cvm_vk_module_data * module_data);



/// temporary functions to replave extensions

/// not correct outside single queue per family paradigm, may wish to use module to avoid this problem? no particularly clean way to do this when queue families in barriers are specified externally
void cvm_vk_module_add_dependency(cvm_vk_module_work_block * work_block,VkDependencyInfoKHR * dependency,VkCommandBuffer from, VkCommandBuffer to);
void CVM_TMP_vkCmdPipelineBarrier2KHR(VkCommandBuffer commandBuffer,const VkDependencyInfoKHR* pDependencyInfo);

#include "cvm_vk_memory.h"
#include "cvm_vk_image.h"



#endif
