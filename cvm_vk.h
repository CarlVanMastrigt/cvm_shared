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


for unified memory architecture (UMA) systems, its probably worth it to be able to avoid staging altogether
    ^ have the memory system upload directly and just do appropriate barriers/semaphores (they will likely match barriers/semaphores used when transferring from staging memory anyway)


could move swapchain/resource rebuilding outside of critical section if atomic locking mechanism is employed
    ^ cas to gain control, VK thread must gain absolute control (a==0 -> a=0xFFFFFFFF) whereas any thread using those resources can gain read permissions (a!=0xFFFFFFFF -> a=a+1)
        ^ this probably isnt woth it as some systems might handle busy waiting poorly, also swapchain/render_resource recreation should be a rare operation anyway!

going to have to rely on acquire op failing to know when to recreate swapchain
    ^ also need to take settings changes into account, i.e. manually do same thing as out of date would and also prevent swapchain image acquisition


    rename module to just module
*/

///won't be supporting modules having msaa output


///render system, upon swapchain creation/recreation should forward acquire swapchain images
///move this to be thread static declaration?

///use swapchain image to index into these? yes
///this should encompass swapchain images? (or at least it could...)

///this struct contains the data needed to be known upfront when acquiring swapchain image (cvm_vk_swapchain_frame), some data could go in either struct though...
typedef struct cvm_vk_acquired_frame
{
    ///would be good to have a way to specify other semaphores &c. here for more complex interdependency

    VkSemaphore acquire_semaphore;///first module should test against this in its submit
    /// need a way to signal this semaphore when we aren't going to render with this image for some reason (e.g. swapchain destruction)
    ///     ^ WAIT, are semaphores used at submit time and can 2 semaphores be used to cycle module submissions if they affect the same stage???)
    ///         ^ probably not possible, multiple frames can be in flight at once! (could do 2 semaphores per frame?? even then probably needless)
    uint32_t acquired_image_index;/// set to 0xFFFFFFFF to indicate failure to acquire


    ///used internally
    VkFence completion_fence;
    bool in_flight;
}
cvm_vk_acquired_frame;


typedef struct cvm_vk_swapchain_frame
{
    VkImage image;///theese are provided by the WSI
    VkImageView image_view;

    ///following only used if present and graphics are different
    ///     ^ test as best as possible with bool that forces behaviour, and maybe try different queue/queue_family when possible (o.e. when available hardware allows)
    /// timeline semaphore in renderer instead?
    VkCommandBuffer graphics_relinquish_command_buffer;///graphics finalise will almost certianly always be the same, so create it at startup with multi submit
    VkCommandBuffer present_acquire_command_buffer;

    VkSemaphore graphics_relinquish_semaphore;///only necessary when transferring to a dedicated present queue, used as part of queue transfer
    VkSemaphore present_semaphore;///needed by VkPresentInfoKHR
}
cvm_vk_swapchain_frame;


/// if using a linked list paradigm, the looking forward in linked list becomes method to acquire appropriate frame
/// also likely requires acquiring at least (or probably exactly) enough frames for all modules (i.e. extra_frames+1) at swapchain creation/recreation
/// non existence of acquired frame (or next for second thread) is method by which existence of frame to render to can be determined across threads
///         ^ only alter linked list inside critical section!
/// linked list wont work either! have to have semaphore at time of acquisition! -- use cycling array with same idea




///misc data the rendering system needs from each module, as well as container for module's per frame rendering data, should be static var in module file
typedef struct cvm_vk_module_data cvm_vk_module_data;

///data passed to rendering engine by each module for rendering as well as getting data back for next render (should be passed around via a pointer, with null indicating module has nothing to provide)
/// if first of these provided to present is null, use a clear command buffer to fill in
typedef struct cvm_vk_module_frame_data
{
    cvm_vk_module_data * parent_module;
    ///cvm_vk_presentation_frame * parent_frame;//

    /// used for tracking whether this work is still up to date (resources it uses haven't changed since creation)
    /// should also be able to track whether swapchain image is up to date, compare against creation_update_id in parent_module
    ///set this at same time as swapchain_image_index

    VkCommandBuffer transfer_work;
    bool has_transfer_work;

    VkPipelineStageFlags transfer_flags;///stages relevant to data uploaded by transfer queue

    VkCommandBuffer graphics_work;
    bool has_graphics_work;

    VkSemaphore transfer_semaphore;///used to ensure data uploaded by transfer queue for this frame is available to graphics operations
    /// probaly want another semaphore/fence for staging buffer data that wont be needed immediately
    ///should be able to mark transfer ops as high low priority, probably also with high/low priority command buffers
    /// alternatively "this frame data" could be handled by graphics queue? (graphics queue guaranteed to be transfer right?)
    ///only potentially necessary when transfer and graphics queues are different
    ///     ^ if they are different do we still need the barrier/dependency on the subpass?
    /// only submit this semaphore if there is graphics work to wait on it as well!

    /// pass back to module to tell it which render pass to use
    //uint32_t swapchain_image_index;/// set to 0xFFFFFFFF to indicate failure to acquire, set based on acquired image
    ///doesnt need acquire semaphore as that'\'ll be handled at's only needed at time of queue submission

    bool in_flight;///used for error checking
}
cvm_vk_module_frame_data;

struct cvm_vk_module_data
{
    VkCommandPool transfer_pool;
    VkCommandPool graphics_pool;

    cvm_vk_module_frame_data * frames;
    uint32_t frame_count;///also used to tell how many fewer command sets are needed relative to frame count (effectively swapchain_image_offset)
    /// ^ e.g. for sinle threaded app will always be 0, for double threaded, 1 for modules on main thread and 0 for modules on second thread

    uint32_t current_frame;/// the current index of frames to use, internal to this module
};

///gfx_data can ONLY change in main thread and ONLY be changed when not in use by other threads (inside main sync mutexes or before starting other threads that use it)

void cvm_vk_initialise(SDL_Window * window,uint32_t min_swapchain_images,uint32_t extra_swapchain_images);
void cvm_vk_terminate(void);///also terminates swapchain dependant data at same time

void cvm_vk_initialise_swapchain(void);
void cvm_vk_reinitialise_swapchain(void);






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





bool cvm_vk_prepare_for_next_frame(bool acquire);
void cvm_vk_transition_frame(void);///must be called in critical section!
void cvm_vk_present_current_frame(cvm_vk_module_frame_data ** module_frames, uint32_t module_count);
bool cvm_vk_are_frames_active(void);

VkRect2D cvm_vk_get_screen_rectangle(void);
VkFormat cvm_vk_get_screen_format(void);///can remove?
uint32_t cvm_vk_get_swapchain_image_count(void);
VkImageView cvm_vk_get_swapchain_image_view(uint32_t index);

/// (NYI) these are needed for cases where modules delegate rendering to a worker thread by way of secondary command buffers, which then need their own command pool
/// also need functions to allocate command buffers from these pools
//void cvm_vk_create_graphics_command_pool(VkCommandPool * command_pool);
//void cvm_vk_create_transfer_command_pool(VkCommandPool * command_pool);
//void cvm_vk_destroy_command_pool(VkCommandPool * command_pool);

///must be called after cvm_vk_initialise
void cvm_vk_create_module_data(cvm_vk_module_data * module_data,bool in_separate_thread,uint32_t active_frame_count);
void cvm_vk_destroy_module_data(cvm_vk_module_data * module_data,bool in_separate_thread);

///combine following 2 functions? or separate the frame end (though that does complicate things, could return the frame itself?
VkCommandBuffer cvm_vk_begin_module_frame_transfer(cvm_vk_module_data * module_data,bool has_work);///always valid to call? if not return bool
VkCommandBuffer cvm_vk_begin_module_frame_graphics(cvm_vk_module_data * module_data,bool has_work,uint32_t frame_offset,uint32_t * swapchain_image_index);
cvm_vk_module_frame_data * cvm_vk_end_module_frame(cvm_vk_module_data * module_data);





///test stuff
void initialise_test_render_data(void);
void terminate_test_render_data(void);

void terminate_test_swapchain_dependencies(VkDevice device);
void initialise_test_swapchain_dependencies(VkDevice device,VkPhysicalDevice physical_device,VkRect2D screen_rectangle,uint32_t swapchain_image_count,VkImageView * swapchain_image_views);

VkBuffer * get_test_buffer(void);///remove after memory mangement system is implemented









/// put memory stuff in separate file?






#endif
