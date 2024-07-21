/**
Copyright 2020,2021,2022,2023,2024 Carl van Mastrigt

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


#define CVM_VK_DEFAULT_TIMEOUT 1000000000
/// ^ 1 second

#define CVM_VK_MAX_QUEUE_FAMILY_COUNT 64


CVM_STACK(VkBufferMemoryBarrier2,cvm_vk_buffer_barrier,4)
CVM_STACK(VkBufferCopy,cvm_vk_buffer_copy,4)


typedef struct cvm_vk_managed_buffer cvm_vk_managed_buffer;
typedef struct cvm_vk_managed_buffer_dismissal_list cvm_vk_managed_buffer_dismissal_list;

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


typedef struct cvm_vk_device cvm_vk_device;

static inline VkDeviceSize cvm_vk_align(VkDeviceSize size, VkDeviceSize alignment)
{
    return (size+alignment-1) & ~(alignment-1);
}


#include "vk/timeline_semaphore.h"
#include "vk/swapchain.h"
#include "vk/command_pool.h"
#include "vk/work_queue.h"




typedef struct cvm_vk_instance_setup
{
    const char ** layer_names;
    uint32_t layer_count;

    const char ** extension_names;
    uint32_t extension_count;

    const char * application_name;
    uint32_t application_version;
}
cvm_vk_instance_setup;


typedef float cvm_vk_device_feature_validation_function(const VkBaseInStructure*, const VkPhysicalDeviceProperties*, const VkPhysicalDeviceMemoryProperties*, const VkExtensionProperties*, uint32_t, const VkQueueFamilyProperties*, uint32_t);
typedef void cvm_vk_device_feature_request_function(VkBaseOutStructure*, bool*, const VkBaseInStructure*, const VkPhysicalDeviceProperties*, const VkPhysicalDeviceMemoryProperties*, const VkExtensionProperties*, uint32_t, const VkQueueFamilyProperties*, uint32_t);

typedef struct cvm_vk_device_setup
{
    VkInstance instance;

    cvm_vk_device_feature_validation_function ** feature_validation;
    uint32_t feature_validation_count;

    cvm_vk_device_feature_request_function ** feature_request;
    uint32_t feature_request_count;

    VkStructureType * device_feature_struct_types;
    size_t * device_feature_struct_sizes;
    uint32_t device_feature_struct_count;

    uint32_t desired_graphics_queues;
    uint32_t desired_transfer_queues;
    uint32_t desired_async_compute_queues;
    ///remove above and default to having just 2 (if possible) : low priority and high priority?
}
cvm_vk_device_setup;


#warning separate device and instance?

typedef struct cvm_vk_device_queue
{
    cvm_vk_timeline_semaphore timeline;
    VkQueue queue;
}
cvm_vk_device_queue;

typedef struct cvm_vk_device_queue_family
{
    cvm_vk_device_queue * queues;
    uint32_t queue_count;
//    const VkQueueFamilyProperties properties;
    VkCommandPool internal_command_pool;
    ///mutex to lock above?
}
cvm_vk_device_queue_family;

struct cvm_vk_device
{
    /// base shared structures
    VkInstance instance;
    #warning remove above

    VkPhysicalDevice physical_device;
    VkDevice device;///"logical" device

    /// capabilities (properties & features)
    const VkPhysicalDeviceProperties properties;
    const VkPhysicalDeviceMemoryProperties memory_properties;

    const VkPhysicalDeviceFeatures2 * features;

    const VkExtensionProperties * extensions;
    uint32_t extension_count;

    const VkQueueFamilyProperties * queue_family_properties;

    cvm_vk_device_queue_family * queue_families;
    uint32_t queue_family_count;


    /// these can be the same, though this should probably change
    /// should have fallbacks...
    /// rename to defaults as thats what they are really...
    uint32_t graphics_queue_family_index;
    uint32_t transfer_queue_family_index;///rename to host_device_transfer ??
    uint32_t async_compute_queue_family_index;

//    uint32_t * queue_family_queue_count;

    uint32_t fallback_present_queue_family_index;
    #warning remove present, this should be per swapchain image

//    VkCommandPool * internal_command_pools;
    /// above used for long lived commands
};





VkInstance cvm_vk_instance_initialise_for_SDL(const char * application_name,uint32_t application_version,bool validation_enabled);
VkInstance cvm_vk_instance_initialise(const cvm_vk_instance_setup * setup);
///above extra is the max extra used by any module
void cvm_vk_instance_terminate(VkInstance instance);

VkSurfaceKHR cvm_vk_create_surface_from_SDL_window(VkInstance instance, SDL_Window * window);
void cvm_vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface);




int cvm_vk_device_initialise(cvm_vk_device * device, const cvm_vk_device_setup * external_device_setup, SDL_Window * window);
///above extra is the max extra used by any module
void cvm_vk_device_terminate(cvm_vk_device * device);


#warning following are placeholders for interoperability with old approach
cvm_vk_device * cvm_vk_device_get(void);
cvm_vk_surface_swapchain * cvm_vk_swapchain_get(void);

VkFence cvm_vk_create_fence(const cvm_vk_device * device,bool initially_signalled);
void cvm_vk_destroy_fence(const cvm_vk_device * device,VkFence fence);
void cvm_vk_wait_on_fence_and_reset(const cvm_vk_device * device, VkFence fence);

VkSemaphore cvm_vk_create_binary_semaphore(const cvm_vk_device * device);
void cvm_vk_destroy_binary_semaphore(const cvm_vk_device * device,VkSemaphore semaphore);

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

void cvm_vk_create_and_bind_memory_for_images(VkDeviceMemory * memory,VkImage * images,uint32_t image_count,VkMemoryPropertyFlags required_properties,VkMemoryPropertyFlags desired_properties);

void cvm_vk_create_image_view(VkImageView * image_view,VkImageViewCreateInfo * info);
void cvm_vk_destroy_image_view(VkImageView image_view);

void cvm_vk_create_sampler(VkSampler * sampler,VkSamplerCreateInfo * info);
void cvm_vk_destroy_sampler(VkSampler sampler);

void cvm_vk_free_memory(VkDeviceMemory memory);

void cvm_vk_create_buffer(VkBuffer * buffer,VkDeviceMemory * memory,VkBufferUsageFlags usage,VkDeviceSize size,void ** mapping,bool * mapping_coherent,VkMemoryPropertyFlags required_properties,VkMemoryPropertyFlags desired_properties);
void cvm_vk_destroy_buffer(VkBuffer buffer,VkDeviceMemory memory,void * mapping);
void cvm_vk_flush_buffer_memory_range(VkMappedMemoryRange * flush_range);
uint32_t cvm_vk_get_buffer_alignment_requirements(VkBufferUsageFlags usage);


VkDeviceSize cvm_vk_buffer_alignment_requirements(const cvm_vk_device * device, VkBufferUsageFlags usage);

typedef struct cvm_vk_buffer_memory_pair_setup
{
    /// in
    VkDeviceSize buffer_size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags required_properties;
    VkMemoryPropertyFlags desired_properties;
    bool map_memory;
    /// out
    VkBuffer buffer;
    VkDeviceMemory memory;
    void * mapping;
    bool mapping_coherent;
}
cvm_vk_buffer_memory_pair_setup;

void cvm_vk_buffer_memory_pair_create(const cvm_vk_device * device, cvm_vk_buffer_memory_pair_setup * setup);
void cvm_vk_buffer_memory_pair_destroy(const cvm_vk_device * device, VkBuffer buffer, VkDeviceMemory memory, bool memory_was_mapped);

VkFormat cvm_vk_get_screen_format(void);///can remove?
uint32_t cvm_vk_get_swapchain_image_count(void);
VkImageView cvm_vk_get_swapchain_image_view(uint32_t index);

VkSemaphoreSubmitInfo cvm_vk_binary_semaphore_submit_info(VkSemaphore semaphore,VkPipelineStageFlags2 stages);
















#define CVM_VK_MAX_QUEUES 16

bool cvm_vk_format_check_optimal_feature_support(VkFormat format,VkFormatFeatureFlags flags);

typedef struct queue_transfer_synchronization_data
{
    atomic_uint_fast32_t spinlock;
    ///COULD have some extra data alongside this that marks data as available, sets an uint8_t from 0 to 1 (available) and marks actual dynamic buffer (if extant) as available
    cvm_vk_buffer_barrier_stack acquire_barriers;
    VkPipelineStageFlags2 wait_stages;///stages that need to waited upon wrt transfer semaphore (stages that have dependence upon QFOT synchronised by semaphore)
    ///is above necessary?? are the stage barriers in the acquire barriers enough?

    ///this is handled by submit only, so could easily be handled by an array in the module/batch itself
    uint64_t transfer_semaphore_wait_value;///transfer semaphore value, value set upon transfer work submission!
    ///need to set this up!

    ///set only at creation time
    uint32_t associated_queue_family_index;///this should probably be on the payload instead... though IS only used in setting up barriers...
}
queue_transfer_synchronization_data;

typedef enum
{
    CVM_VK_PAYLOAD_FIRST_QUEUE_USE=0x00000001,///could technically work this one out...
    CVM_VK_PAYLOAD_LAST_QUEUE_USE=0x00000002,
}
cvm_vk_payload_flags;

#define CVM_VK_PAYLOAD_MAX_WAITS 8

#define CVM_VK_GRAPHICS_QUEUE_INDEX 0
#define CVM_VK_COMPUTE_QUEUE_OFFSET 1
/// ^ such that they come after graphics


///subset of this (CB and transfer_data) required for secondary command buffers!
typedef struct cvm_vk_module_work_payload
{
    ///can reasonably use queue id, stages and semaphore value here instead of a copy of the semaphore itself...

    ///can also store secondary command buffer?
    /// have flags/enum (to assert on) regarding nature of CB, primary? inline? secondary? compute?

    cvm_vk_timeline_semaphore waits[CVM_VK_PAYLOAD_MAX_WAITS];///this also allows host side semaphores so isn't that bad tbh...
    VkPipelineStageFlags2 wait_stages[CVM_VK_PAYLOAD_MAX_WAITS];
    uint32_t wait_count;

    VkPipelineStageFlags2 signal_stages;///singular signal semaphore (the queue's one) supported for now

    VkCommandBuffer command_buffer;

    uint32_t destination_queue:8;///should really be known at init time anyway (for barrier purposes), might as well store for simplicities sake, must be large enough to store CVM_VK_MAX_QUEUES-1
    uint32_t is_graphics:1;/// if false(0) then is asynchronous compute (is this even necessary w/ above queue id??)

    ///presently only support generating 1 signal and submitting 1 command buffer, can easily change should there be justification to do so

    ///makes sense not to have transfer queue in this list as transfer-transfer dependencies make no sense
    queue_transfer_synchronization_data * transfer_data;///the data for THIS queue
}
cvm_vk_module_work_payload;///break this struct up based on intended queue type? (have variants)

typedef struct cvm_vk_module_sub_batch
{
    VkCommandPool graphics_pool;
    VkCommandBuffer * graphics_scbs;///secondary command buffers
    uint32_t graphics_scb_space;
    uint32_t graphics_scb_count;

    ///should not need to query number of scbs allocated, max that ever tries to be used should match number allocated
}
cvm_vk_module_sub_batch;

typedef struct cvm_vk_module_batch
{
    cvm_vk_module_sub_batch * sub_batches;///for distribution to other threads

    VkCommandBuffer * graphics_pcbs;///primary command buffes, allocated from the "main" (0th) sub_batch's pool
    uint32_t graphics_pcb_space;
    uint32_t graphics_pcb_count;

    ///above must be processed at start of frame BEFORE anything might add to it
    ///having one to process and one to fill doesnt sound like a terrible idea to be honest...

    /// have a buffer of pending high priority copies to be executed on the next PCB before any SCB's are generated, this allows SCB only threads to generate copy actions!
    /// would multiple threads writing into the same staging buffer cause issues?
    ///     ^ have staging action that also copies to alleviate this?
    VkCommandPool transfer_pool;/// cannot have transfers in secondary command buffer (scb's need renderpass/subpass) and they arent needed anyway, so have 1 per module batch
    ///all transfer commands actually generated from stored ops in managed buffer(s) and managed texture pool(s)
    VkCommandBuffer transfer_cb;///only for low priority uploads, high priority uploads should go in command buffer/queue that will use them
    uint32_t has_begun_transfer:1;///has begun the transfer command buffer, dont begin the command buffer if it isn't necessary this frame, begin upon first use
    uint32_t has_ended_transfer:1;///has submitted transfer operations
    ///assert that above is only acquired and submitted once a frame!
    uint32_t transfer_affceted_queues_bitmask;


    /// also keep in mind VK_SHARING_MODE_CONCURRENT where viable and profile it
    /// also keep in mind transfer on graphics queue is possible and profile that as well

    queue_transfer_synchronization_data * transfer_data;///array of size CVM_VK_MAX_QUEUES, gets passed down the chain of command to individual payloads via batches

    uint32_t queue_submissions_this_batch:CVM_VK_MAX_QUEUES;///each bit represents which queues have been acquired this frame, upon first acquisition execute barriers immediately
    ///above does bring up an issue, system REQUIRES that every batch is acquired, may be able to circumvent by copying unhandled barriers into next pending group for that queue
}
cvm_vk_module_batch;

typedef struct cvm_vk_module_data
{
    cvm_vk_module_batch * batches;///one per swapchain image

    uint32_t batch_count;///same as swapchain image count used to initialise this
    uint32_t batch_index;

    uint32_t sub_batch_count;///effectively max number of threads this module will use, must have at least 1 (index 0 is the primary thread)

    ///wait a second! -- transfer queue can technically be run alongside graphics! 2 frame delay isnt actually necessary (do we still want it though? does it really add much complexity?)
    queue_transfer_synchronization_data transfer_data[CVM_VK_MAX_QUEUES];/// pending acquire barriers per queue, must be cycled to ensure correct semaphore dependencies exist between transfer ops and these acquires (QFOT's)
}
cvm_vk_module_data;
///must be called after cvm_vk_initialise
void cvm_vk_create_module_data(cvm_vk_module_data * module_data,uint32_t sub_batch_count);///sub batches are basically the number of threads rendering will use
void cvm_vk_resize_module_graphics_data(cvm_vk_module_data * module_data);///this must be called in critical section, rename to resize_module_data
void cvm_vk_destroy_module_data(cvm_vk_module_data * module_data);

cvm_vk_module_batch * cvm_vk_get_module_batch(cvm_vk_module_data * module_data,uint32_t * swapchain_image_index);///have this return bool? (VkCommandBuffer through ref.)
void cvm_vk_end_module_batch(cvm_vk_module_batch * batch);///handles all pending operations (presently only the transfer operation stuff), should be last thing called that uses the batch or its derivatives
VkCommandBuffer cvm_vk_access_batch_transfer_command_buffer(cvm_vk_module_batch * batch,uint32_t affected_queue_bitbask);///returned value should NOT be submitted directly, instead it should be handled by cvm_vk_end_module_batch


void cvm_vk_work_payload_add_wait(cvm_vk_module_work_payload * payload,cvm_vk_timeline_semaphore semaphore,VkPipelineStageFlags2 stages);

void cvm_vk_setup_new_graphics_payload_from_batch(cvm_vk_module_work_payload * payload,cvm_vk_module_batch * batch);
void cvm_vk_submit_graphics_work(cvm_vk_module_work_payload * payload,cvm_vk_payload_flags flags);
VkCommandBuffer cvm_vk_obtain_secondary_command_buffer_from_batch(cvm_vk_module_sub_batch * msb,VkFramebuffer framebuffer,VkRenderPass render_pass,uint32_t sub_pass);

///same as above but for compute (including compute?)

uint32_t cvm_vk_get_transfer_queue_family(void);
uint32_t cvm_vk_get_graphics_queue_family(void);
uint32_t cvm_vk_get_asynchronous_compute_queue_family(void);

#warning move these to the top of this file when possible!
#include "vk/memory.h"
#include "cvm_vk_image.h"
#include "cvm_vk_defaults.h"


#endif
