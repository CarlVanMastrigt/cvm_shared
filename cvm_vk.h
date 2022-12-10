/**
Copyright 2020,2021,2022 Carl van Mastrigt

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
    assert(r==VK_SUCCESS ||                                                     \
           !fprintf(stderr,"VULKAN FUNCTION FAILED : %d : %s\n",r,#f));         \
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

typedef struct cvm_vk_timeline_semaphore
{
    VkSemaphore semaphore;
    uint64_t value;
}
cvm_vk_timeline_semaphore;

typedef struct cvm_vk_swapchain_image_present_data
{
    VkImage image;///theese are provided by the WSI
    VkImageView image_view;

    VkSemaphore acquire_semaphore;///held temporarily by this struct, not owner, not created or destroyed as part of it

    //bool in_flight;///error checking
    uint32_t successfully_acquired:1;
    uint32_t successfully_submitted:1;

    ///following only used if present and graphics are different
    ///     ^ test as best as possible with bool that forces behaviour, and maybe try different queue/queue_family when possible (o.e. when available hardware allows)
    /// timeline semaphore in renderer instead?
    VkCommandBuffer present_acquire_command_buffer;

    VkSemaphore present_semaphore;///needed by VkPresentInfoKHR

    ///semaphore values
    uint64_t graphics_wait_value;
    uint64_t present_wait_value;
    uint64_t transfer_wait_value;///should move this to the transferchain when that becomes a thing

//    VkFence completion_fence;
}
cvm_vk_swapchain_image_present_data;





void cvm_vk_initialise(SDL_Window * window,uint32_t min_swapchain_images,bool sync_compute_required,const char ** requested_extensions,int requested_extension_count);
///above extra is the max extra used by any module
void cvm_vk_terminate(void);///also terminates swapchain dependant data at same time

void cvm_vk_create_swapchain(void);
void cvm_vk_destroy_swapchain(void);



//void cvm_vk_create_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore);
//void cvm_vk_destroy_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore);
void cvm_vk_wait_on_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore,uint64_t value,uint64_t timeout_ns);


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

void cvm_vk_create_and_bind_memory_for_images(VkDeviceMemory * memory,VkImage * images,uint32_t image_count,VkMemoryPropertyFlags extra_flags);

void cvm_vk_create_image_view(VkImageView * image_view,VkImageViewCreateInfo * info);
void cvm_vk_destroy_image_view(VkImageView image_view);

void cvm_vk_create_sampler(VkSampler * sampler,VkSamplerCreateInfo * info);
void cvm_vk_destroy_sampler(VkSampler sampler);

void cvm_vk_free_memory(VkDeviceMemory memory);

void * cvm_vk_create_buffer(VkBuffer * buffer,VkDeviceMemory * memory,VkBufferUsageFlags usage,VkDeviceSize size,bool require_host_visible);
void cvm_vk_destroy_buffer(VkBuffer buffer,VkDeviceMemory memory,void * mapping);
void cvm_vk_flush_buffer_memory_range(VkMappedMemoryRange * flush_range);
uint32_t cvm_vk_get_buffer_alignment_requirements(VkBufferUsageFlags usage);


VkPhysicalDeviceFeatures2 * cvm_vk_get_device_features(void);

VkFormat cvm_vk_get_screen_format(void);///can remove?
uint32_t cvm_vk_get_swapchain_image_count(void);
VkImageView cvm_vk_get_swapchain_image_view(uint32_t index);
VkImage cvm_vk_get_swapchain_image(uint32_t index);



bool cvm_vk_format_check_optimal_feature_support(VkFormat format,VkFormatFeatureFlags flags);

typedef enum
{
    CVM_VK_PAYLOAD_FIRST_SWAPCHAIN_USE=0x00000001,
    CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE=0x00000002,
}
cvm_vk_payload_flags;

#define CVM_VK_PAYLOAD_MAX_WAITS 8
#define CVM_VK_PAYLOAD_MAX_SIGNALS 4

typedef struct cvm_vk_module_work_payload
{
    cvm_vk_timeline_semaphore waits[CVM_VK_PAYLOAD_MAX_WAITS];
    VkPipelineStageFlags2 wait_stages[CVM_VK_PAYLOAD_MAX_WAITS];
    uint32_t wait_count;

    ///consider making signal singular... signalling pipeline stages out of order has the potential for bad results, and allowing different stages has very little benefit
    /// output/results, used as input to subsequent commands, will all have the same semaphore, just different values
    cvm_vk_timeline_semaphore signals[CVM_VK_PAYLOAD_MAX_SIGNALS];
    VkPipelineStageFlags2 signal_stages[CVM_VK_PAYLOAD_MAX_SIGNALS];
    uint32_t signal_count;

    VkCommandBuffer command_buffer;


    ///presently only support generating 1 signal and submitting 1 command buffer, can easily change should there be justification to do so
}
cvm_vk_module_work_payload;

///have these return semaphores (which will be set upon their completion)
void cvm_vk_submit_graphics_work(cvm_vk_module_work_payload * payload,cvm_vk_payload_flags flags);
void cvm_vk_submit_transfer_work(cvm_vk_module_work_payload * payload);

///misc data the rendering system needs from each module, as well as container for module's per frame rendering data, should be static var in module file
typedef struct cvm_vk_module_sub_batch
{
    VkCommandPool graphics_pool;
    VkCommandBuffer * graphics_scbs;///secondary command buffers
    uint32_t graphics_scb_count;
    uint32_t graphics_scbs_used;

    ///should not need to query number of scbs allocated, max that ever tries to be used should match number allocated
}
cvm_vk_module_sub_batch;

typedef struct cvm_vk_module_batch
{
    cvm_vk_module_sub_batch * sub_batches;///for distribution to other threads

    /// might consider removing the singular command buffer paradigm above entirely, allocate new command buffers as necessary (is this vaible)
    VkCommandBuffer * graphics_pcbs;///primary command buffes, allocated from main_sub_batch pool
    uint32_t graphics_pcb_count;
    uint32_t graphics_pcbs_used;

    uint32_t graphics_pcb_created;///error checking only, used to ensure all scb's were returned

    /// should have/use array of primary command buffers for each queue type w/ async being submitted in order w/ implicit preference for queue family


    VkCommandPool transfer_pool;/// cannot have transfers in secondary command buffer (scb's need renderpass/subpass) and they arent needed anyway, so have 1 per module batch
    ///all transfer commands actually generated from stored ops in managed buffer(s) and managed texture pool(s)
    VkCommandBuffer transfer_pcb;///only for low priority uploads, high priority uploads should go in command buffer/queue that will use them
    /// should not need to ask module how many of these there are
    /// should use same/similar logic to distribute them that was used to create them
    /// effectively allocated is max, so can have variant number actually used in any given frame depending on workload

    /// need intra-frame and inter-frame (the structurally difficult one) semaphores to do ownership transfers, and structure to handle that
    /// support for multiple command buffers per frame each on different queue when necessary will allow for much more fine grained control and better GPU utilization (structure needs to support this)
    /// above allows a queue(family) to be working on (have ownership of) resources NOT utilised by by another queue(family) and be doing work in "tandem" with that queue which is using different resources at that time
    /// a good example of this is performing game transfers during overlay rendering and vice-versa

    /// also keep in mind VK_SHARING_MODE_CONCURRENT where viable and profile it
    /// also keep in mind transfer on graphics queue is possible and profile that as well

    /// do inter-batch semaphores invalidate any benefit for multiple framebuffer images (semaphores in chain do affect only specific stages...)
    /// ^ will depend on usage patterns but i think in general the answer is yes, is worth looking at example of intended use

    /// break into multiple submits/command_buffers to break up dependencies?? -- THIS

    /// ^ also i have no way to test any of this on my current hardware...

    /// could/should add async compute, but it has same dependency problems as transfer unless using VK_SHARING_MODE_CONCURRENT (as mentioned above)

    /// also probably worth investigating whether per-frame resources are worth transferring to device local (double)buffer before using on the gpu
}
cvm_vk_module_batch;

typedef struct cvm_vk_module_data
{
    cvm_vk_module_batch * batches;///one per swapchain image

    uint32_t batch_count;///same as swapchain image count used to initialise this
    uint32_t batch_index;

    uint32_t sub_batch_count;///effectively max number of threads this module will use, must have at least 1 (index 0 is the primary thread)

    ///need an array of fixed, recyclable primary command buffers here and a way to sequence them with the dynamic command buffers
}
cvm_vk_module_data;
///must be called after cvm_vk_initialise
void cvm_vk_create_module_data(cvm_vk_module_data * module_data,uint32_t sub_batch_count);///sub batches are basically the number of threads rendering will use
void cvm_vk_resize_module_graphics_data(cvm_vk_module_data * module_data);///this must be called in critical section, rename to resize_module_data
void cvm_vk_destroy_module_data(cvm_vk_module_data * module_data);

cvm_vk_module_batch * cvm_vk_get_module_batch(cvm_vk_module_data * module_data,uint32_t * swapchain_image_index);///have this return bool? (VkCommandBuffer through ref.)

VkCommandBuffer cvm_vk_get_batch_secondary_command_buffer(cvm_vk_module_sub_batch * msb,VkFramebuffer framebuffer,VkRenderPass render_pass,uint32_t sub_pass);
void cvm_vk_init_batch_primary_payload(cvm_vk_module_batch * mb,cvm_vk_module_work_payload * payload);


static inline bool cvm_vk_availability_token_check(uint16_t token_counter,uint16_t current_counter,uint16_t delay)
{
    return ((current_counter-token_counter)&0xFFFF)>=delay;
}

uint32_t cvm_vk_get_transfer_queue_family(void);
uint32_t cvm_vk_get_graphics_queue_family(void);


uint32_t cvm_vk_prepare_for_next_frame(bool rendering_resources_invalid);
bool cvm_vk_recreate_rendering_resources(void);///this and operations resulting from it returning true, must be called in critical section
bool cvm_vk_check_for_remaining_frames(uint32_t * completed_frame_index);

#include "cvm_vk_memory.h"
#include "cvm_vk_image.h"
#include "cvm_vk_defaults.h"


#endif
