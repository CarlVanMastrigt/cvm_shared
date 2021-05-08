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

#include "cvm_shared.h"




/// copying around some vulkan structs (e.g. device) is invalid, instead just use these static ones
///     ^ wait, is this actually true?? where did i find this?

static VkInstance cvm_vk_instance;
static VkPhysicalDevice cvm_vk_physical_device;
static VkDevice cvm_vk_device;///"logical" device
static VkSurfaceKHR cvm_vk_surface;

static VkSwapchainKHR cvm_vk_swapchain=VK_NULL_HANDLE;
static VkSurfaceFormatKHR cvm_vk_surface_format;
static VkPresentModeKHR cvm_vk_surface_present_mode;


static VkRect2D cvm_vk_screen_rectangle;///extent



///may make more sense to put just above in struct (useful for "one time" initialisation functions, like vertex buffer)
///ALTERNATIVELY have functions to allocate these external objects? -- probably just use this option






///command pools are externally synchronised! need to allocate one pool per frame!?
/// ^ no, externally synchronised they can only be altered from 1 CPU thread
///     so long as these command pools are only used by the thread that created them,
///     and each module allocates its own command pool per thread (it absolutely should) then everything will be fine

///these can be the same
static uint32_t cvm_vk_transfer_queue_family;
static uint32_t cvm_vk_graphics_queue_family;
static uint32_t cvm_vk_present_queue_family;
///only support one of each of above
static VkQueue cvm_vk_transfer_queue;
static VkQueue cvm_vk_graphics_queue;
static VkQueue cvm_vk_present_queue;

///these are the command pools applicable to the main thread, should be provided upon request for a command pool by any module that runs in the main thread
static VkCommandPool cvm_vk_transfer_command_pool;///provided as default for same-thread modules
static VkCommandPool cvm_vk_graphics_command_pool;///provided as default for same-thread modules
static VkCommandPool cvm_vk_present_command_pool;///only ever used within this file

///submit transfer ops?

/**
shared command pools (one per queue) is fucking terrible, wont work across threads
command pools CAN however be used for command buffers that are submitted to different queues (no they CANNOT!)
    only this file handles the present queue
    need a way for other files to know whether they need a seperate command pool for transfer and graphics ops
        whatever is used MUST support multiple worker thread (able to create multiple command pools) and should be able to know if its operating in the main thread (only 1 command buffer needed)

command pools are only needed for the threads in which command buffers are created and recorded to! this means we only need 1 command pool per worker thread per queue family!

ask for gfx and transfer pools together if thats whats needed, with bool is_main_thread which returns the main thread's queues!
this makes memory management quite tricky, but w/e cross that bridge when we get to it.


note secondary command buffers must be in executable state in order to execute them in a primary command buffer, this reduces their usefulness substantially...

need one command pool per thread per dispatch, things that only exist in the main thread won't need this

tie command buffers to their state's age (use update count to determine if a command is out of date), this alleviates need to wait for commands to finish before recreating pipelines and other resources,
    ^ just wait on fence then increment the update counter, (need to track how many are in flight!) track submit count and wait on fences of completion count to match (can simply be uint that rolls over!)
    ^ this paradigm agoids needing to wait on device



what prevents framebuffer images (that arent the destination swapchain image) being overwritten by other frames in flight?
    ^ external dependencies? semaphores?


could allow 2 sets of "transient" framebuffer (g-buffer) images by creating VkFramebuffers for every possible combo of swapchain image and "transient" (g-buffer) images (i do like this tbh)


can use dependencies to ensure uniform reads


subpasses should know if they're the first or not in the execution hierarchy and have their external dependencies set as such (don't implement single command buffer for layout transition)
modules should mark themselves as clearing the back buffer so that they can be replaced when necessary
*/

///this paradigm sucks and needs review
static struct
{
    VkFence completion_fence;
    bool in_flight;

    VkSemaphore acquire_semaphore;
    VkSemaphore graphics_semaphore;
    VkSemaphore present_semaphore;

    VkCommandBuffer graphics_command_buffer;///allocated from cvm_vk_graphics_command_pool above
    VkCommandBuffer present_command_buffer;///allocated from cvm_vk_present_command_pool above
}
cvm_vk_presentation_instances[CVM_VK_PRESENTATION_INSTANCE_COUNT];///REMOVE

///for now require that number requested be provided, throw error and exit if not (sync issues may occur if this req. isn't met!)

/// CVM_VK_PRESENTATION_INSTANCE_COUNT should really match swapchain image count, alloc it to be the same


/// need to know the max frame count such that resources can only be used after they are transferred back, should this be matched to the number of swapchain images?
///probably best to match this to swapchain count

static uint32_t cvm_vk_presentation_instance_index=0;///REMOVE



///may want to rename cvm_vk_frames, cvm_vk_presentation_instance probably isnt that bad tbh...
static cvm_vk_acquired_frame * cvm_vk_acquired_frames;///number of these should match swapchain image count
static cvm_vk_swapchain_frame * cvm_vk_swapchain_frames;
static uint32_t cvm_vk_current_frame;
static uint32_t cvm_vk_active_frames=0;///both frames in flight and frames acquired by rendereer

static VkImage * cvm_vk_swapchain_images;///can probably make this a function local array (or malloc) that can be discarded after views are created
static VkImageView * cvm_vk_swapchain_image_views;///can probably make part of cvm_vk_frame (though they do get destroyed/recreated separately from frames...)
static uint32_t cvm_vk_swapchain_image_count;
///following used to determine number of swapchain images to allocate
static uint32_t cvm_vk_min_swapchain_images;
static uint32_t cvm_vk_extra_swapchain_images;


static cvm_vk_external_module cvm_vk_external_modules[CVM_VK_MAX_EXTERNAL];///REMOVE
static uint32_t cvm_vk_external_module_count=0;///REMOVE


static void cvm_vk_initialise_instance(SDL_Window * window)
{
    uint32_t extension_count;
    uint32_t api_version;
    const char ** extension_names=NULL;
    const char * layer_names[]={"VK_LAYER_KHRONOS_validation"};///VK_LAYER_LUNARG_standard_validation

    CVM_VK_CHECK(vkEnumerateInstanceVersion(&api_version));

    printf("Vulkan API version: %u.%u.%u - %u\n",(api_version>>22)&0x7F,(api_version>>12)&0x3FF,api_version&0xFFF,api_version>>29);


    if(!SDL_Vulkan_GetInstanceExtensions(window,&extension_count,NULL))
    {
        fprintf(stderr,"COULD NOT GET SDL WINDOW EXTENSION COUNT\n");
        exit(-1);
    }
    extension_names=malloc(sizeof(const char*)*extension_count);
    if(!SDL_Vulkan_GetInstanceExtensions(window,&extension_count,extension_names))
    {
        fprintf(stderr,"COULD NOT GET SDL WINDOW EXTENSIONS\n");
        exit(-1);
    }


    VkApplicationInfo application_info=(VkApplicationInfo)
    {
        .sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext=NULL,
        .pApplicationName=" ",
        .applicationVersion=0,
        .pEngineName="cvm_shared",
        .engineVersion=0,
        .apiVersion=api_version//VK_MAKE_VERSION(1,1,VK_HEADER_VERSION)
    };

    VkInstanceCreateInfo instance_creation_info=(VkInstanceCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo= &application_info,
        .enabledLayerCount=1,///make 0 for release version (no validation), test performance diff
        .ppEnabledLayerNames=layer_names,
        .enabledExtensionCount=extension_count,
        .ppEnabledExtensionNames=extension_names
    };

    CVM_VK_CHECK(vkCreateInstance(&instance_creation_info,NULL,&cvm_vk_instance));

    free(extension_names);
}

static void cvm_vk_initialise_surface(SDL_Window * window)
{
    if(!SDL_Vulkan_CreateSurface(window,cvm_vk_instance,&cvm_vk_surface))
    {
        fprintf(stderr,"COULD NOT CREATE SDL VULKAN SURFACE\n");
        exit(-1);
    }
}

static bool check_physical_device_appropriate(bool dedicated_gpu_required)
{
    VkPhysicalDeviceProperties properties;
    uint32_t i,queue_family_count;
    VkQueueFamilyProperties * queue_family_properties=NULL;
    VkBool32 surface_supported;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(cvm_vk_physical_device,&properties);

    if((dedicated_gpu_required)&&(properties.deviceType!=VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))return false;

    printf("testing GPU : %s\n",properties.deviceName);

    vkGetPhysicalDeviceFeatures(cvm_vk_physical_device,&features);

    #warning test for required features. need comparitor struct to do this?   enable all by default for time being?

    ///SIMPLE EXAMPLE TEST: printf("geometry shader: %d\n",features.geometryShader);

    vkGetPhysicalDeviceQueueFamilyProperties(cvm_vk_physical_device,&queue_family_count,NULL);
    queue_family_properties=malloc(sizeof(VkQueueFamilyProperties)*queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(cvm_vk_physical_device,&queue_family_count,queue_family_properties);

    ///use first queue family to support each desired operation

    #warning find_other queue families

    cvm_vk_graphics_queue_family=queue_family_count;
    cvm_vk_transfer_queue_family=queue_family_count;
    cvm_vk_present_queue_family=queue_family_count;

    for(i=0;i<queue_family_count;i++)
    {
        if((cvm_vk_graphics_queue_family==queue_family_count)&&(queue_family_properties[i].queueFlags&VK_QUEUE_GRAPHICS_BIT))
        {
            cvm_vk_graphics_queue_family=i;
            if(cvm_vk_transfer_queue_family==queue_family_count)cvm_vk_transfer_queue_family=i;///graphics must support transfer, even if not specified
        }

        ///if an explicit transfer queue family separate from selected graphics queue family exists use it independently
        if((i != cvm_vk_graphics_queue_family)&&(queue_family_properties[i].queueFlags&VK_QUEUE_TRANSFER_BIT))
        {
            cvm_vk_transfer_queue_family=i;
        }

        if(cvm_vk_present_queue_family==queue_family_count)
        {
            CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(cvm_vk_physical_device,i,cvm_vk_surface,&surface_supported));
            if(surface_supported==VK_TRUE)cvm_vk_present_queue_family=i;
        }
    }

    free(queue_family_properties);

    printf("Queue Families count:%u g:%u t:%u p:%u\n",queue_family_count,cvm_vk_graphics_queue_family,cvm_vk_transfer_queue_family,cvm_vk_present_queue_family);

    return ((cvm_vk_graphics_queue_family != queue_family_count)&&(cvm_vk_transfer_queue_family != queue_family_count)&&(cvm_vk_present_queue_family != queue_family_count));
    ///return true only if all queue families needed can be satisfied
}

static void cvm_vk_initialise_physical_device(void)
{
    ///pick best physical device with all required features

    uint32_t i,device_count,format_count,present_mode_count;
    VkPhysicalDevice * physical_devices;
    VkSurfaceFormatKHR * formats=NULL;
    VkPresentModeKHR * present_modes=NULL;

    CVM_VK_CHECK(vkEnumeratePhysicalDevices(cvm_vk_instance,&device_count,NULL));
    physical_devices=malloc(sizeof(VkPhysicalDevice)*device_count);
    CVM_VK_CHECK(vkEnumeratePhysicalDevices(cvm_vk_instance,&device_count,physical_devices));

    for(i=0;i<device_count;i++)///check for dedicated gfx cards first
    {
        cvm_vk_physical_device=physical_devices[i];
        if(check_physical_device_appropriate(true)) break;
    }

    if(i==device_count)for(i=0;i<device_count;i++)///check for non-dedicated (integrated) gfx cards next
    {
        cvm_vk_physical_device=physical_devices[i];
        if(check_physical_device_appropriate(false)) break;
    }

    free(physical_devices);

    if(i==device_count)
    {
        fprintf(stderr,"NONE OF %d PHYSICAL DEVICES MEET REQUIREMENTS\n",device_count);
        exit(-1);
    }


    ///select screen image format
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(cvm_vk_physical_device,cvm_vk_surface,&format_count,NULL));
    formats=malloc(sizeof(VkSurfaceFormatKHR)*format_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(cvm_vk_physical_device,cvm_vk_surface,&format_count,formats));

    cvm_vk_surface_format=formats[0];///search for preferred/fallback instead of taking first?

    free(formats);


    ///select screen present mode
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(cvm_vk_physical_device,cvm_vk_surface,&present_mode_count,NULL));
    present_modes=malloc(sizeof(VkPresentModeKHR)*present_mode_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(cvm_vk_physical_device,cvm_vk_surface,&present_mode_count,present_modes));

    ///maybe want do do something meaningful here in later versions, below should be fine for now
    cvm_vk_surface_present_mode=VK_PRESENT_MODE_FIFO_KHR;///guaranteed to exist

    free(present_modes);
}

static void cvm_vk_initialise_logical_device(void)
{
    const char * device_extensions[]={"VK_KHR_swapchain"};
    const char * layer_names[]={"VK_LAYER_KHRONOS_validation"};///VK_LAYER_LUNARG_standard_validation
    float queue_priority=1.0;

    ///try copying device_features from gfx until appropriate list of required features found
    ///can possibly do this internally for engine based o features requested (though that would disallow custom things to be done by user, so is probably better to AND anything like that with user defined feature list)
    VkPhysicalDeviceFeatures features=(VkPhysicalDeviceFeatures){};
    features.geometryShader=VK_TRUE;
    features.multiDrawIndirect=VK_TRUE;
    features.sampleRateShading=VK_TRUE;
    features.fillModeNonSolid=VK_TRUE;
    features.fragmentStoresAndAtomics=VK_TRUE;
//    features=cvm_vk_device_features;

    VkDeviceQueueCreateInfo device_queue_creation_infos[3];///3 is max queues
    uint32_t queue_family_count=0;

    ///graphics queue family is "base" from which transfer and present can deviate (or be the same as)
    device_queue_creation_infos[queue_family_count++]=(VkDeviceQueueCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=cvm_vk_graphics_queue_family,
        .queueCount=1,
        .pQueuePriorities= &queue_priority
    };

    if(cvm_vk_transfer_queue_family != cvm_vk_graphics_queue_family)
    device_queue_creation_infos[queue_family_count++]=(VkDeviceQueueCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=cvm_vk_transfer_queue_family,
        .queueCount=1,
        .pQueuePriorities= &queue_priority
    };

    if(cvm_vk_present_queue_family != cvm_vk_graphics_queue_family)
    device_queue_creation_infos[queue_family_count++]=(VkDeviceQueueCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=cvm_vk_present_queue_family,
        .queueCount=1,
        .pQueuePriorities= &queue_priority
    };



    VkDeviceCreateInfo device_creation_info=(VkDeviceCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueCreateInfoCount=queue_family_count,
        .pQueueCreateInfos= device_queue_creation_infos,
        .enabledLayerCount=1,///make 0 for release version (no validation), test performance diff
        .ppEnabledLayerNames=layer_names,
        .enabledExtensionCount=1,
        .ppEnabledExtensionNames=device_extensions,
        .pEnabledFeatures= &features
    };

    CVM_VK_CHECK(vkCreateDevice(cvm_vk_physical_device,&device_creation_info,NULL,&cvm_vk_device));

    vkGetDeviceQueue(cvm_vk_device,cvm_vk_graphics_queue_family,0,&cvm_vk_graphics_queue);
    vkGetDeviceQueue(cvm_vk_device,cvm_vk_transfer_queue_family,0,&cvm_vk_transfer_queue);
    vkGetDeviceQueue(cvm_vk_device,cvm_vk_present_queue_family,0,&cvm_vk_present_queue);
}

static void cvm_vk_create_default_command_pools()
{
    VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex=cvm_vk_transfer_queue_family
    };

    CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&cvm_vk_transfer_command_pool));

    if(cvm_vk_graphics_queue_family==cvm_vk_transfer_queue_family) cvm_vk_graphics_command_pool=cvm_vk_transfer_command_pool;
    else
    {
        command_pool_create_info.queueFamilyIndex=cvm_vk_graphics_queue_family;
        CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&cvm_vk_graphics_command_pool));
    }

    if(cvm_vk_present_queue_family==cvm_vk_transfer_queue_family) cvm_vk_present_command_pool=cvm_vk_transfer_command_pool;
    else
    {
        command_pool_create_info.queueFamilyIndex=cvm_vk_present_queue_family;
        CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&cvm_vk_present_command_pool));
    }
}

static void cvm_vk_destroy_default_command_pools()
{
    vkDestroyCommandPool(cvm_vk_device,cvm_vk_transfer_command_pool,NULL);
    if(cvm_vk_graphics_queue_family!=cvm_vk_transfer_queue_family) vkDestroyCommandPool(cvm_vk_device,cvm_vk_graphics_command_pool,NULL);
    if(cvm_vk_present_queue_family!=cvm_vk_transfer_queue_family) vkDestroyCommandPool(cvm_vk_device,cvm_vk_present_command_pool,NULL);
}

static void cvm_vk_create_presentation_frames(VkImage * swapchain_images)
{
    uint32_t i;

    cvm_vk_current_frame=0;///need to reset in case number of swapchain images goes down

    cvm_vk_acquired_frames=malloc(cvm_vk_swapchain_image_count*sizeof(cvm_vk_acquired_frame));
    cvm_vk_swapchain_frames=malloc(cvm_vk_swapchain_image_count*sizeof(cvm_vk_swapchain_frame));

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        cvm_vk_acquired_frames[i].acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;
        cvm_vk_acquired_frames[i].in_flight=false;

        VkSemaphoreCreateInfo semaphore_create_info=(VkSemaphoreCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        VkFenceCreateInfo fence_create_info=(VkFenceCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        CVM_VK_CHECK(vkCreateFence(cvm_vk_device,&fence_create_info,NULL,&cvm_vk_acquired_frames[i].completion_fence));

        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_acquired_frames[i].acquire_semaphore));


        /// swapchain frames
        cvm_vk_swapchain_frames[i].image=swapchain_images[i];

        VkImageViewCreateInfo image_view_create_info=(VkImageViewCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .image=swapchain_images[i],
            .viewType=VK_IMAGE_VIEW_TYPE_2D,
            .format=cvm_vk_surface_format.format,
            .components=(VkComponentMapping)
            {
                .r=VK_COMPONENT_SWIZZLE_IDENTITY,
                .g=VK_COMPONENT_SWIZZLE_IDENTITY,
                .b=VK_COMPONENT_SWIZZLE_IDENTITY,
                .a=VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange=(VkImageSubresourceRange)
            {
                .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel=0,
                .levelCount=1,
                .baseArrayLayer=0,
                .layerCount=1
            }
        };

        CVM_VK_CHECK(vkCreateImageView(cvm_vk_device,&image_view_create_info,NULL,&cvm_vk_swapchain_frames[i].image_view));

        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_swapchain_frames[i].present_semaphore));


        ///queue ownership transfer stuff
        if(cvm_vk_present_queue_family!=cvm_vk_graphics_queue_family)
        {
            CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_swapchain_frames[i].graphics_relinquish_semaphore));


            VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext=NULL,
                .commandPool=0,///set later
                .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount=1
            };

            command_buffer_allocate_info.commandPool=cvm_vk_graphics_command_pool;
            CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&cvm_vk_swapchain_frames[i].graphics_relinquish_command_buffer));

            command_buffer_allocate_info.commandPool=cvm_vk_present_command_pool;
            CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&cvm_vk_swapchain_frames[i].present_acquire_command_buffer));

            VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext=NULL,
                .flags=0,///not one time use
                .pInheritanceInfo=NULL
            };

            /// would really like to swap to VkImageMemoryBarrier2KHR here, is much nicer way to organise data



            CVM_VK_CHECK(vkBeginCommandBuffer(cvm_vk_swapchain_frames[i].graphics_relinquish_command_buffer,&command_buffer_begin_info));

            VkImageMemoryBarrier graphics_relinquish_barrier=(VkImageMemoryBarrier)
            {
                .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext=NULL,
                .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,///stage where data is written BEFORE the transfer/barrier
                .dstAccessMask=0,///should be 0 by spec
                .oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex=cvm_vk_graphics_queue_family,
                .dstQueueFamilyIndex=cvm_vk_present_queue_family,
                .image=swapchain_images[i],
                .subresourceRange=(VkImageSubresourceRange)
                {
                    .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel=0,
                    .levelCount=1,
                    .baseArrayLayer=0,
                    .layerCount=1
                }
            };

            vkCmdPipelineBarrier(cvm_vk_swapchain_frames[i].graphics_relinquish_command_buffer,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,0,NULL,0,NULL,1,&graphics_relinquish_barrier);///dstStageMask=VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ??

            CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_swapchain_frames[i].graphics_relinquish_command_buffer));



            VkImageMemoryBarrier present_acquire_barrier=(VkImageMemoryBarrier)
            {
                .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext=NULL,
                .srcAccessMask=0,///ignored anyway
                .dstAccessMask=0,
                .oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex=cvm_vk_graphics_queue_family,
                .dstQueueFamilyIndex=cvm_vk_present_queue_family,
                .image=swapchain_images[i],
                .subresourceRange=(VkImageSubresourceRange)
                {
                    .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel=0,
                    .levelCount=1,
                    .baseArrayLayer=0,
                    .layerCount=1
                }
            };

            CVM_VK_CHECK(vkBeginCommandBuffer(cvm_vk_swapchain_frames[i].present_acquire_command_buffer,&command_buffer_begin_info));

            vkCmdPipelineBarrier(cvm_vk_swapchain_frames[i].present_acquire_command_buffer,0,0,0,0,NULL,0,NULL,1,&present_acquire_barrier);
            /// from wiki: no srcStage/AccessMask or dstStage/AccessMask is needed, waiting for a semaphore does that automatically.

            CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_swapchain_frames[i].present_acquire_command_buffer));
        }
    }
}

static void cvm_vk_destroy_presentation_frames()
{
    uint32_t i;

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        vkDestroyFence(cvm_vk_device,cvm_vk_acquired_frames[i].completion_fence,NULL);
        vkDestroySemaphore(cvm_vk_device,cvm_vk_acquired_frames[i].acquire_semaphore,NULL);

        /// swapchain frames
        vkDestroyImageView(cvm_vk_device,cvm_vk_swapchain_frames[i].image_view,NULL);

        vkDestroySemaphore(cvm_vk_device,cvm_vk_swapchain_frames[i].present_semaphore,NULL);

        ///queue ownership transfer stuff
        if(cvm_vk_present_queue_family!=cvm_vk_graphics_queue_family)
        {
            vkDestroySemaphore(cvm_vk_device,cvm_vk_swapchain_frames[i].graphics_relinquish_semaphore,NULL);

            vkFreeCommandBuffers(cvm_vk_device,cvm_vk_graphics_command_pool,1,&cvm_vk_swapchain_frames[i].graphics_relinquish_command_buffer);
            vkFreeCommandBuffers(cvm_vk_device,cvm_vk_graphics_command_pool,1,&cvm_vk_swapchain_frames[i].present_acquire_command_buffer);
        }
    }

    free(cvm_vk_acquired_frames);
    free(cvm_vk_swapchain_frames);
}

void cvm_vk_initialise_swapchain(void) ///merge with swapchain initialisation?
{
    uint32_t i;
    VkSurfaceCapabilitiesKHR surface_capabilities;

    ///swapchain
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(cvm_vk_physical_device,cvm_vk_surface,&surface_capabilities));

    cvm_vk_screen_rectangle=(VkRect2D)
    {
        .offset=(VkOffset2D)
        {
            .x=0,
            .y=0
        },
        .extent=surface_capabilities.currentExtent
    };

    cvm_vk_swapchain_image_count=((surface_capabilities.minImageCount > cvm_vk_min_swapchain_images) ? surface_capabilities.minImageCount : cvm_vk_min_swapchain_images)+cvm_vk_extra_swapchain_images;

    if((surface_capabilities.maxImageCount)&&(surface_capabilities.maxImageCount < cvm_vk_swapchain_image_count))
    {
        fprintf(stderr,"REQUESTED SWAPCHAIN IMAGE COUNT NOT SUPPORTED\n");
        exit(-1);
    }

    cvm_vk_swapchain_images=malloc(sizeof(VkImage)*cvm_vk_swapchain_image_count);
    cvm_vk_swapchain_image_views=malloc(sizeof(VkImageView)*cvm_vk_swapchain_image_count);

    /// the contents of this dictate explicit transfer of the swapchain images between the graphics and present queues!
    VkSwapchainCreateInfoKHR swapchain_create_info=(VkSwapchainCreateInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext=NULL,
        .flags=0,
        .surface=cvm_vk_surface,
        .minImageCount=cvm_vk_swapchain_image_count,
        .imageFormat=cvm_vk_surface_format.format,
        .imageColorSpace=cvm_vk_surface_format.colorSpace,
        .imageExtent=surface_capabilities.currentExtent,
        .imageArrayLayers=1,
        .imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL,
        .preTransform=surface_capabilities.currentTransform,
        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode=cvm_vk_surface_present_mode,
        .clipped=VK_TRUE,
        .oldSwapchain=cvm_vk_swapchain
    };

    CVM_VK_CHECK(vkCreateSwapchainKHR(cvm_vk_device,&swapchain_create_info,NULL,&cvm_vk_swapchain));

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&i,NULL));
    if(i!=cvm_vk_swapchain_image_count)
    {
        fprintf(stderr,"COULD NOT CREATE REQUESTED NUMBER OF SWAPCHAIN IMAGES\n");
        exit(-1);
    }
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&cvm_vk_swapchain_image_count,cvm_vk_swapchain_images));

    VkImageViewCreateInfo image_view_create_info;

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        image_view_create_info=(VkImageViewCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .image=cvm_vk_swapchain_images[i],
            .viewType=VK_IMAGE_VIEW_TYPE_2D,
            .format=cvm_vk_surface_format.format,
            .components=(VkComponentMapping)
            {
                .r=VK_COMPONENT_SWIZZLE_IDENTITY,
                .g=VK_COMPONENT_SWIZZLE_IDENTITY,
                .b=VK_COMPONENT_SWIZZLE_IDENTITY,
                .a=VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange=(VkImageSubresourceRange)
            {
                .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel=0,
                .levelCount=1,
                .baseArrayLayer=0,
                .layerCount=1
            }
        };

        CVM_VK_CHECK(vkCreateImageView(cvm_vk_device,&image_view_create_info,NULL,cvm_vk_swapchain_image_views+i));
    }

    for(i=0;i<cvm_vk_external_module_count;i++) cvm_vk_external_modules[i].initialise(
        cvm_vk_device,cvm_vk_physical_device,cvm_vk_screen_rectangle,cvm_vk_swapchain_image_count,cvm_vk_swapchain_image_views);

    cvm_vk_create_presentation_frames(cvm_vk_swapchain_images);///move this to a parent function for both this and cvm_vk_initialise_swapchain
}

void cvm_vk_reinitialise_swapchain(void)///actually does substantially improve performance over terminate -> initialise
{
    uint32_t i;
    VkSwapchainKHR old_swapchain;

    cvm_vk_wait();

    /// is terminating external modules here actually necessary? could we do that after in an update step and just set swapchain as having changed here for those modules to reference?

    for(i=0;i<cvm_vk_external_module_count;i++) cvm_vk_external_modules[i].terminate(cvm_vk_device);

    for(i=0;i<cvm_vk_swapchain_image_count;i++) vkDestroyImageView(cvm_vk_device,cvm_vk_swapchain_image_views[i],NULL);

    free(cvm_vk_swapchain_images);
    free(cvm_vk_swapchain_image_views);

    cvm_vk_destroy_presentation_frames(); ///move this to a parent function or redesign this whole structure

    old_swapchain=cvm_vk_swapchain;

    cvm_vk_initialise_swapchain();

    vkDestroySwapchainKHR(cvm_vk_device,old_swapchain,NULL);
}

static void cvm_vk_terminate_swapchain(void)
{
    uint32_t i;

    for(i=0;i<cvm_vk_external_module_count;i++) cvm_vk_external_modules[i].terminate(cvm_vk_device);

    for(i=0;i<cvm_vk_swapchain_image_count;i++) vkDestroyImageView(cvm_vk_device,cvm_vk_swapchain_image_views[i],NULL);

    free(cvm_vk_swapchain_images);
    free(cvm_vk_swapchain_image_views);

    cvm_vk_destroy_presentation_frames();

    vkDestroySwapchainKHR(cvm_vk_device,cvm_vk_swapchain,NULL);
    cvm_vk_swapchain=VK_NULL_HANDLE;
}

static void cvm_vk_initialise_presentation_intances(void)///REMOVE
{
    uint32_t i;

    VkCommandBufferAllocateInfo command_buffer_allocate_info;
    VkSemaphoreCreateInfo semaphore_create_info;
    VkFenceCreateInfo fence_create_info;

    for(i=0;i<CVM_VK_PRESENTATION_INSTANCE_COUNT;i++)
    {
        command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext=NULL,
            .commandPool=cvm_vk_graphics_command_pool,
            .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount=1
        };

        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&cvm_vk_presentation_instances[i].graphics_command_buffer));


        command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext=NULL,
            .commandPool=cvm_vk_present_command_pool,
            .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount=1
        };

        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&cvm_vk_presentation_instances[i].present_command_buffer));


        semaphore_create_info=(VkSemaphoreCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_presentation_instances[i].acquire_semaphore));
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_presentation_instances[i].graphics_semaphore));
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_presentation_instances[i].present_semaphore));

        fence_create_info=(VkFenceCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        CVM_VK_CHECK(vkCreateFence(cvm_vk_device,&fence_create_info,NULL,&cvm_vk_presentation_instances[i].completion_fence));
        cvm_vk_presentation_instances[i].in_flight=false;
    }
}

static void cvm_vk_terminate_presentation_intances(void)///REMOVE
{
    uint32_t i;

    for(i=0;i<CVM_VK_PRESENTATION_INSTANCE_COUNT;i++)
    {
        vkDestroyFence(cvm_vk_device,cvm_vk_presentation_instances[i].completion_fence,NULL);

        vkDestroySemaphore(cvm_vk_device,cvm_vk_presentation_instances[i].acquire_semaphore,NULL);
        vkDestroySemaphore(cvm_vk_device,cvm_vk_presentation_instances[i].graphics_semaphore,NULL);
        vkDestroySemaphore(cvm_vk_device,cvm_vk_presentation_instances[i].present_semaphore,NULL);

        vkFreeCommandBuffers(cvm_vk_device,cvm_vk_graphics_command_pool,1,&cvm_vk_presentation_instances[i].graphics_command_buffer);
        vkFreeCommandBuffers(cvm_vk_device,cvm_vk_present_command_pool,1,&cvm_vk_presentation_instances[i].present_command_buffer);
    }
}




uint32_t cvm_vk_add_external_module(cvm_vk_external_module module)///REMOVE
{
    if(cvm_vk_external_module_count==CVM_VK_MAX_EXTERNAL)
    {
        fprintf(stderr,"INSUFFICIENT EXTERNAL OP SPACE\n");
        exit(-1);
    }

    cvm_vk_external_modules[cvm_vk_external_module_count]=module;

    return cvm_vk_external_module_count++;
}





void cvm_vk_initialise(SDL_Window * window,uint32_t min_swapchain_images,uint32_t extra_swapchain_images)
{
    cvm_vk_min_swapchain_images=min_swapchain_images;
    cvm_vk_extra_swapchain_images=extra_swapchain_images;

    cvm_vk_initialise_instance(window);
    cvm_vk_initialise_surface(window);
    cvm_vk_initialise_physical_device();
    cvm_vk_initialise_logical_device();
    cvm_vk_create_default_command_pools();
    cvm_vk_initialise_presentation_intances();
}

void cvm_vk_terminate(void)
{
    cvm_vk_terminate_swapchain();
    cvm_vk_terminate_presentation_intances();
    cvm_vk_destroy_default_command_pools();

    vkDestroyDevice(cvm_vk_device,NULL);
    vkDestroySurfaceKHR(cvm_vk_instance,cvm_vk_surface,NULL);
    vkDestroyInstance(cvm_vk_instance,NULL);
}

///new frame paradigm removes need for this, let presentation engine/render loop handle flushing frames
void cvm_vk_wait(void)
{
    uint32_t i;
    for(i=0;i<CVM_VK_PRESENTATION_INSTANCE_COUNT;i++)if(cvm_vk_presentation_instances[i].in_flight)
    {
        CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&cvm_vk_presentation_instances[i].completion_fence,VK_TRUE,1000000000));
        CVM_VK_CHECK(vkResetFences(cvm_vk_device,1,&cvm_vk_presentation_instances[i].completion_fence));
        cvm_vk_presentation_instances[i].in_flight=false;///has finished
    }
}

///also do module frame invalidation here, pass in settings update counter or similar
/// success, fail but not ready to recreate, fail and ready to recreate
/// enen though it'd be possible to not recreate swapchain when some settings change and just use out of date checking to avoid rendering...
///         its much simpler and easier to just hijack swapchain recreation to rebuild rendering resources and avoid presenting out of date data

///returns true when rebuild is required and possible, frame images in should be CVM_VK_INVALID_IMAGE_INDEX (and be that for ALL frames)
/// CANNOT alter the contents of any presenation frame being read from, should add some kind of error checking (though that will be difficult to do in multithreading situations efficiently)
/// MUST be called AFTER present for the frame
/// need static bool here, or using input to control prevention of frame acquisition (can probably assume recreation needed if game_running is still true) also allows other reasons to recreate, e.g. settings change
///      ^ this paradigm might even avoid swapchain recreation when not changing things that affect it! (e.g. 1 modules MSAA settings)
/// need a better name for this
/// rely on this func to detect swapchain resize? couldn't hurt to double check based on screen resize
bool cvm_vk_prepare_for_next_frame(bool acquire)
{
    cvm_vk_acquired_frame * frame = cvm_vk_acquired_frames + (cvm_vk_current_frame+cvm_vk_extra_swapchain_images+1)%cvm_vk_swapchain_image_count;///next frame written to by module detached the most from presentation
    /// this modulo op should be used everywhere its acted upon instead

    if(frame->in_flight)
    {
        CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&frame->completion_fence,VK_TRUE,1000000000));
        CVM_VK_CHECK(vkResetFences(cvm_vk_device,1,&frame->completion_fence));
        frame->in_flight=false;///has finished
        cvm_vk_active_frames--;
    }

    if(acquire)
    {
        VkResult r=vkAcquireNextImageKHR(cvm_vk_device,cvm_vk_swapchain,1000000000,frame->acquire_semaphore,VK_NULL_HANDLE,&frame->acquired_image_index);
        if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
        {
            puts("swapchain out of date");
            frame->acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;
            return false;
        }
        else if(r!=VK_SUCCESS)
        {
            fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",r);
            frame->acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;
            return false;
        }
    }
    else
    {
        frame->acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;
        return false;
    }

    cvm_vk_active_frames++;
    return true;
}

void cvm_vk_transition_frame(void)///must be called in critical section!
{
    cvm_vk_current_frame = (cvm_vk_current_frame+1)%cvm_vk_swapchain_image_count;
}

void cvm_vk_present_current_frame(cvm_vk_module_frame_data ** module_frames, uint32_t module_frame_count)
{
    cvm_vk_acquired_frame * acquired_frame;
    cvm_vk_swapchain_frame * swapchain_frame;
    ///add extra ones if async/sync compute can be waited upon as well?
    VkSemaphore semaphores[4];
    VkPipelineStageFlags wait_stage_flags[4];
    uint32_t i;

    acquired_frame=cvm_vk_acquired_frames+cvm_vk_current_frame;
    swapchain_frame=cvm_vk_swapchain_frames+acquired_frame->acquired_image_index;

    if(acquired_frame->acquired_image_index==CVM_VK_INVALID_IMAGE_INDEX)
    {
        ///check frame isnt in flight?
        ///also check no module frame has graphics work?
        return;
    }


    #warning need error checking that every module has work to do, also a way to exit early


    ///i do prefer design of VkSubmitInfo2KHR here instead
    VkSubmitInfo submit_info=(VkSubmitInfo) /// whether submission has associated fence depends on need for family ownership transfer, so actually submit in branched code section
    {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext=NULL,
        .waitSemaphoreCount=0,
        .pWaitSemaphores=semaphores,
        .pWaitDstStageMask=wait_stage_flags,
        .commandBufferCount=1,///always just 1
        .pCommandBuffers= NULL,
        .signalSemaphoreCount=0,///only changes to 1 for last module, 0 otherwise
        .pSignalSemaphores=NULL
    };

    for(i=0;i<module_frame_count;i++)
    {
        ///semaphores in between submits in same queue probably arent necessary, can likely rely on external subpass dependencies, this needs looking into
        /// ^yes this looks to be the case
        submit_info.waitSemaphoreCount=0;

        if(i==0)///wait on image acquisition
        {
            semaphores[submit_info.waitSemaphoreCount]=acquired_frame->acquire_semaphore;
            wait_stage_flags[submit_info.waitSemaphoreCount]=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submit_info.waitSemaphoreCount++;
        }

        if(module_frames[i]->has_transfer_work)///wait on upload of data
        {
            #warning semaphore probably only needed when queues are different! (needs further work)
            semaphores[submit_info.waitSemaphoreCount]=module_frames[i]->transfer_semaphore;
            wait_stage_flags[submit_info.waitSemaphoreCount]=module_frames[i]->transfer_flags;
            submit_info.waitSemaphoreCount++;
        }

        ///may also want to wait on async compute?

        submit_info.pCommandBuffers=&module_frames[i]->graphics_work;

        if((i==module_frame_count)&&(cvm_vk_present_queue_family==cvm_vk_graphics_queue_family))///no queue ownership transfer required, signal present semaphore as this is the last frame
        {
            submit_info.signalSemaphoreCount=1;
            submit_info.pSignalSemaphores=&swapchain_frame->present_semaphore;

            CVM_VK_CHECK(vkQueueSubmit(cvm_vk_graphics_queue,1,&submit_info,acquired_frame->completion_fence));
        }
        else
        {
            CVM_VK_CHECK(vkQueueSubmit(cvm_vk_graphics_queue,1,&submit_info,VK_NULL_HANDLE));
        }
    }

    if(cvm_vk_present_queue_family!=cvm_vk_graphics_queue_family)///perform queue ownership transfer
    {
        ///relinquish
        submit_info.waitSemaphoreCount=0;
        submit_info.pWaitSemaphores=NULL;
        submit_info.signalSemaphoreCount=1;
        submit_info.pSignalSemaphores=&swapchain_frame->graphics_relinquish_semaphore;
        submit_info.pCommandBuffers=&swapchain_frame->graphics_relinquish_command_buffer;
        CVM_VK_CHECK(vkQueueSubmit(cvm_vk_graphics_queue,1,&submit_info,VK_NULL_HANDLE));

        ///acquire
        wait_stage_flags[0]=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit_info.waitSemaphoreCount=1;
        submit_info.pWaitSemaphores=&swapchain_frame->graphics_relinquish_semaphore;
        submit_info.signalSemaphoreCount=1;
        submit_info.pSignalSemaphores=&swapchain_frame->present_semaphore;
        submit_info.pCommandBuffers=&swapchain_frame->present_acquire_command_buffer;
        CVM_VK_CHECK(vkQueueSubmit(cvm_vk_present_queue,1,&submit_info,acquired_frame->completion_fence));
    }

    VkPresentInfoKHR present_info=(VkPresentInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext=NULL,
        .waitSemaphoreCount=1,
        .pWaitSemaphores=&swapchain_frame->present_semaphore,
        .swapchainCount=1,
        .pSwapchains=&cvm_vk_swapchain,
        .pImageIndices=&acquired_frame->acquired_image_index,
        .pResults=NULL
    };


    VkResult r=vkQueuePresentKHR(cvm_vk_present_queue,&present_info);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't present (out of date)");
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);

    acquired_frame->in_flight=true;
}

void cvm_vk_present(void)
{
    uint32_t i,image_index;
    VkResult r;

    ///these types could probably be re-used, but as they're passed as pointers its better to be sure?
    VkImageMemoryBarrier end_graphics_barrier,present_barrier;
    VkSubmitInfo graphics_submit_info,present_submit_info;
    VkPipelineStageFlags graphics_wait_dst_stage_mask,present_wait_dst_stage_mask;
    VkPresentInfoKHR present_info;



    r=vkAcquireNextImageKHR(cvm_vk_device,cvm_vk_swapchain,1000000000,cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].acquire_semaphore,VK_NULL_HANDLE,&image_index);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't use swapchain");//recreate_gfx_swapchain(gfx);///rebuild swapchain dependent elements for this buffer
        ///could try this again after swapchain recreation? (probably within while loop?)
        ///     ^ NO! - wouldn't have correct screen size from sdl event!
        return;
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",r);



    ///separate command buffer for each render element (game/overlay) ? (i like this, no function call per module, just a command buffer)

    ///fence to prevent re-submission of same command buffer ( command buffer wasn't created with VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT )




    ///command buffer filling

    VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext=NULL,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT  necessary? in tutorial bet shouldn't be used?
        .pInheritanceInfo=NULL
    };



    #warning move render pass to completely separate function (function pointer from list called by this function as part of command recording)

    ///also need barrier for uploaded vertex data from instance data pumped over transfer queue



    ///this also resets the command buffer
    CVM_VK_CHECK(vkBeginCommandBuffer(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_command_buffer,&command_buffer_begin_info));


    for(i=0;i<cvm_vk_external_module_count;i++)cvm_vk_external_modules[i].render(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_command_buffer,image_index,cvm_vk_screen_rectangle);

    CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_command_buffer));




    graphics_wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    graphics_submit_info=(VkSubmitInfo) /// whether submission has associated fence depends on need for family ownership transfer, so actually submit in branched code section
    {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext=NULL,
        .waitSemaphoreCount=1,
        .pWaitSemaphores= &cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].acquire_semaphore,
        .pWaitDstStageMask= &graphics_wait_dst_stage_mask,
        .commandBufferCount=1,
        .pCommandBuffers= &cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_command_buffer,
        .signalSemaphoreCount=1,
        .pSignalSemaphores= &cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_semaphore
    };



    if(cvm_vk_present_queue_family==cvm_vk_graphics_queue_family)///just present, waiting on graphics semaphore
    {
        CVM_VK_CHECK(vkQueueSubmit(cvm_vk_graphics_queue,1,&graphics_submit_info,cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].completion_fence));
        ///if families the same this is last submit so fence goes here

        present_info=(VkPresentInfoKHR)
        {
            .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext=NULL,
            .waitSemaphoreCount=1,
            .pWaitSemaphores= &cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_semaphore,
            .swapchainCount=1,
            .pSwapchains= &cvm_vk_swapchain,
            .pImageIndices= &image_index,
            .pResults=NULL
        };
    }
    else
    {
        #warning implement this!
        fprintf(stderr,"QUEUE OWNERSIP TRANSFER NYI\n");
        exit(-1);
    }

    ///either of above moves presentation instance to in flight
    cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].in_flight=true;

    ///following transfers image to present_queue family



    r=vkQueuePresentKHR(cvm_vk_present_queue,&present_info);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't present (out of date)");//recreate_gfx_swapchain(gfx);///rebuild swapchain dependent elements for this buffer
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);

    cvm_vk_presentation_instance_index=(cvm_vk_presentation_instance_index+1)%CVM_VK_PRESENTATION_INSTANCE_COUNT;

    /// finish/sync "next" (oldest in queue) presentation instance after this (new) one is fully submitted
    if(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].in_flight)
    {
        CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].completion_fence,VK_TRUE,1000000000));
        CVM_VK_CHECK(vkResetFences(cvm_vk_device,1,&cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].completion_fence));
        cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].in_flight=false;///has finished
    }
}








VkFormat cvm_vk_get_screen_format(void)
{
    return cvm_vk_surface_format.format;
}

void cvm_vk_create_render_pass(VkRenderPass * render_pass,VkRenderPassCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateRenderPass(cvm_vk_device,info,NULL,render_pass));
}

void cvm_vk_destroy_render_pass(VkRenderPass render_pass)
{
    vkDestroyRenderPass(cvm_vk_device,render_pass,NULL);
}

void cvm_vk_create_framebuffer(VkFramebuffer * framebuffer,VkFramebufferCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateFramebuffer(cvm_vk_device,info,NULL,framebuffer));
}

void cvm_vk_destroy_framebuffer(VkFramebuffer framebuffer)
{
    vkDestroyFramebuffer(cvm_vk_device,framebuffer,NULL);
}


void cvm_vk_create_pipeline_layout(VkPipelineLayout * pipeline_layout,VkPipelineLayoutCreateInfo * info)
{
    CVM_VK_CHECK(vkCreatePipelineLayout(cvm_vk_device,info,NULL,pipeline_layout));
}

void cvm_vk_destroy_pipeline_layout(VkPipelineLayout pipeline_layout)
{
    vkDestroyPipelineLayout(cvm_vk_device,pipeline_layout,NULL);
}


void cvm_vk_create_graphics_pipeline(VkPipeline * pipeline,VkGraphicsPipelineCreateInfo * info)
{
    #warning use pipeline cache!?
    CVM_VK_CHECK(vkCreateGraphicsPipelines(cvm_vk_device,VK_NULL_HANDLE,1,info,NULL,pipeline));
}

void cvm_vk_destroy_pipeline(VkPipeline pipeline)
{
    vkDestroyPipeline(cvm_vk_device,pipeline,NULL);
}


///return VkPipelineShaderStageCreateInfo, but hold on to VkShaderModule (passed by ptr) for deletion at program cleanup ( that module can be kept inside the creation info! )
void cvm_vk_create_shader_stage_info(VkPipelineShaderStageCreateInfo * stage_info,const char * filename,VkShaderStageFlagBits stage)
{
    static char * entrypoint="main";

    FILE * f;
    size_t length;
    char * data_buffer;

    VkShaderModule shader_module;

    if((f=fopen(filename,"rb")))
    {
        fseek(f,0,SEEK_END);
        length=ftell(f);
        data_buffer=malloc(length);
        rewind(f);
        fread(data_buffer,1,length,f);
        fclose(f);

        VkShaderModuleCreateInfo shader_module_create_info=(VkShaderModuleCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .codeSize=length,
            .pCode=(uint32_t*)data_buffer
        };

        if(vkCreateShaderModule(cvm_vk_device,&shader_module_create_info,NULL,&shader_module)!=VK_SUCCESS)
        {
            fprintf(stderr,"ERROR CREATING SHADER MODULE FROM FILE: %s\n",filename);
            exit(-1);
        }

        free(data_buffer);
    }
    else
    {
        fprintf(stderr,"COULD NOT LOAD SHADER: %s\n",filename);
        exit(-1);
    }

    *stage_info=(VkPipelineShaderStageCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,///not supported
        .stage=stage,
        .module=shader_module,
        .pName=entrypoint,///always use main as entrypoint, use a static string such that this address is still valid after function return
        .pSpecializationInfo=NULL
    };
}

void cvm_vk_destroy_shader_stage_info(VkPipelineShaderStageCreateInfo * stage_info)
{
    vkDestroyShaderModule(cvm_vk_device,stage_info->module,NULL);
}






void cvm_vk_create_module_data(cvm_vk_module_data * module_data,bool in_separate_thread,uint32_t active_frame_count)
{
    uint32_t i;

    module_data->current_frame=0;

    module_data->frame_count=active_frame_count;
    module_data->frames=malloc(sizeof(cvm_vk_module_frame_data)*module_data->frame_count);


    if(in_separate_thread)
    {
        VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext=NULL,
            .flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex=cvm_vk_transfer_queue_family
        };

        CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&module_data->transfer_pool));

        if(cvm_vk_transfer_queue_family==cvm_vk_graphics_queue_family) module_data->graphics_pool=module_data->transfer_pool;
        else
        {
            command_pool_create_info.queueFamilyIndex=cvm_vk_graphics_queue_family;
            CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&module_data->graphics_pool));
        }
    }
    else
    {
        module_data->transfer_pool=cvm_vk_transfer_command_pool;
        module_data->graphics_pool=cvm_vk_graphics_command_pool;
    }


    for(i=0;i<module_data->frame_count;i++)
    {
        module_data->frames[i].parent_module=module_data;
        module_data->frames[i].in_flight=false;
        //module_data->frames[i].swapchain_image_index=CVM_VK_INVALID_IMAGE_INDEX;///basically just for error checking

        module_data->frames[i].has_transfer_work=false;
        module_data->frames[i].has_graphics_work=false;
        module_data->frames[i].transfer_flags=VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;///probably want to set this properly, i.e. 0 and have module set it


        VkSemaphoreCreateInfo semaphore_create_info=(VkSemaphoreCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&module_data->frames[i].transfer_semaphore));


        VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext=NULL,
            .commandPool=module_data->transfer_pool,
            .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount=1
        };

        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&module_data->frames[i].transfer_work));

        command_buffer_allocate_info.commandPool=module_data->graphics_pool;
        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&module_data->frames[i].graphics_work));
    }
}

void cvm_vk_destroy_module_data(cvm_vk_module_data * module_data,bool in_separate_thread)
{
    uint32_t i;

    for(i=0;i<module_data->frame_count;i++)
    {
        if(module_data->frames[i].in_flight)
        {
            fprintf(stderr,"TRYING TO DESTROY module FRAME THAT IS IN FLIGHT\n");
            exit(-1);
        }

        vkFreeCommandBuffers(cvm_vk_device,module_data->transfer_pool,1,&module_data->frames[i].transfer_work);
        vkFreeCommandBuffers(cvm_vk_device,module_data->graphics_pool,1,&module_data->frames[i].graphics_work);

        vkDestroySemaphore(cvm_vk_device,module_data->frames[i].transfer_semaphore,NULL);
    }

    if(in_separate_thread)
    {
        vkDestroyCommandPool(cvm_vk_device,module_data->transfer_pool,NULL);

        if(cvm_vk_transfer_queue_family!=cvm_vk_graphics_queue_family)
        {
            vkDestroyCommandPool(cvm_vk_device,module_data->graphics_pool,NULL);
        }
    }

    free(module_data->frames);
}


VkCommandBuffer cvm_vk_begin_module_frame_transfer(cvm_vk_module_data * module_data,bool has_work)
{
    cvm_vk_module_frame_data * frame=module_data->frames+module_data->current_frame;

    frame->has_transfer_work=has_work;

    VkCommandBuffer command_buffer=VK_NULL_HANDLE;

    if(frame->has_transfer_work)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext=NULL,
            .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT  necessary? in tutorial bet shouldn't be used?
            .pInheritanceInfo=NULL
        };

        CVM_VK_CHECK(vkBeginCommandBuffer(frame->transfer_work,&command_buffer_begin_info));
        command_buffer=frame->transfer_work;
    }

    return command_buffer;
}

VkCommandBuffer cvm_vk_begin_module_frame_graphics(cvm_vk_module_data * module_data,bool has_work,uint32_t frame_offset,uint32_t * swapchain_image_index)
{
    *swapchain_image_index = cvm_vk_acquired_frames[(cvm_vk_current_frame+frame_offset)%cvm_vk_swapchain_image_count].acquired_image_index;///value passed back by rendering engine

    cvm_vk_module_frame_data * frame=module_data->frames+module_data->current_frame;

    VkCommandBuffer command_buffer=VK_NULL_HANDLE;

    /// if recreating module relevant data might want to pass in first, new acquired swapchain image index so this frames correct index can be passed back to modules
    /// this may warrant making get swapchain image index a separate function that operates on same list of submodle frames (and can fail accordingly &c.)
    if(*swapchain_image_index != CVM_VK_INVALID_IMAGE_INDEX)///data passed in is valid and everything is up to date
    {
        if(frame->in_flight)
        {
            fprintf(stderr,"TRYING TO WRITE module FRAME THAT IS STILL IN FLIGHT\n");
            exit(-1);
        }

        frame->has_graphics_work=has_work;

        VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext=NULL,
            .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT  necessary? in tutorial bet shouldn't be used?
            .pInheritanceInfo=NULL
        };

        if(frame->has_graphics_work)
        {
            CVM_VK_CHECK(vkBeginCommandBuffer(frame->graphics_work,&command_buffer_begin_info));
            command_buffer=frame->graphics_work;
        }
    }
    else frame->has_graphics_work=false;

    return command_buffer;
}

cvm_vk_module_frame_data * cvm_vk_end_module_frame(cvm_vk_module_data * module_data)
{
    cvm_vk_module_frame_data * frame=module_data->frames + module_data->current_frame++;
    if(module_data->current_frame==module_data->frame_count)module_data->current_frame=0;

    if(frame->has_transfer_work) CVM_VK_CHECK(vkEndCommandBuffer(frame->transfer_work));
    if(frame->has_graphics_work) CVM_VK_CHECK(vkEndCommandBuffer(frame->graphics_work));

    return frame;
}














///TEST FUNCTIONS FROM HERE, STRIP DOWN / USE AS BASIS FOR REAL VERSIONS



static VkBuffer test_buffer;
static VkDeviceMemory test_buffer_memory;

VkBuffer * get_test_buffer(void)
{
    return &test_buffer;
}








static void create_vertex_buffer(void)
{
    test_render_data d[4];
    d[0].c=(vec3f){1,0,0};
    d[0].pos=(vec3f){-0.7,-0.7,0};

    d[1].c=(vec3f){0,1,0};
    d[1].pos=(vec3f){-0.7,0.7,0};

    d[2].c=(vec3f){0,0,1};
    d[2].pos=(vec3f){0.7,-0.7,0};

    d[3].c=(vec3f){1,1,1};
    d[3].pos=(vec3f){0.7,0.7,0};

//    VkBufferCreateInfo tbc=(VkBufferCreateInfo)
//    {
//        .sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
//        .pNext=NULL,
//        .flags=0,
//        .size=sizeof(test_render_data)*4,
//        .usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
//        .queueFamilyIndexCount=0,
//        .pQueueFamilyIndices=NULL
//    };
//
//    VkBuffer tb;
//
//    CVM_VK_CHECK(vkCreateBuffer(cvm_vk_device,&tbc,NULL,&tb));
//
//    VkMemoryRequirements mr;
//    vkGetBufferMemoryRequirements(cvm_vk_device,tb,&mr);
//
//    printf("a: %u\n",mr.alignment);
//
//    vkDestroyBuffer(cvm_vk_device,tb,NULL);

    ///creation of buffer may be done wrong here, valgrind reports jump based on uninitialised value


    VkBufferCreateInfo buffer_create_info=(VkBufferCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .size=sizeof(test_render_data)*4,
        .usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL
    };

    CVM_VK_CHECK(vkCreateBuffer(cvm_vk_device,&buffer_create_info,NULL,&test_buffer));


    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(cvm_vk_device,test_buffer,&buffer_memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;///move this to gfx ? prevents calling it every time a buffer is allocated
    vkGetPhysicalDeviceMemoryProperties(cvm_vk_physical_device,&memory_properties);


    uint32_t i;
    VkMemoryAllocateInfo memory_allocate_info;

    /// integrated graphics ONLY has shared heaps! means theres no good way to test everything is working, but also means there's room for optimisation
    ///     ^ could FORCE staging buffer for now
    ///     ^ if staging is detected as not needed then dont do it? (staging is useful for managing resources though...)
    ///         ^ or perhaps if it would be copying in anyway then the managed transfer paradigm could work but without the staging transfer, just move in directly
    ///             ^ does make synchronization (particularly delete more challenging though, will have to schedule things for deletion)
//    printf("memory types: %u\nheap count: %u\n",memory_properties.memoryTypeCount,memory_properties.memoryHeapCount);
//
//    for(i=0;i<memory_properties.memoryTypeCount;i++)
//    {
//        if(memory_properties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
//        {
//            printf("device local memory type: %u using heap: %u\n",i,memory_properties.memoryTypes[i].heapIndex);
//        }
//    }

    for(i=0;i<memory_properties.memoryTypeCount;i++)
    {
        if((buffer_memory_requirements.memoryTypeBits&(1<<i))&&(memory_properties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            memory_allocate_info=(VkMemoryAllocateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext=NULL,
                .allocationSize=buffer_memory_requirements.size,
                .memoryTypeIndex=i
            };

            CVM_VK_CHECK(vkAllocateMemory(cvm_vk_device,&memory_allocate_info,NULL,&test_buffer_memory));
            break;
        }
    }

    CVM_VK_CHECK(vkBindBufferMemory(cvm_vk_device,test_buffer,test_buffer_memory,0))


    void * data;
    CVM_VK_CHECK(vkMapMemory(cvm_vk_device,test_buffer_memory,0,sizeof(test_render_data)*4,0,&data));

    memcpy(data,d,sizeof(test_render_data)*4);

    VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
    {
        .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .pNext=NULL,
        .memory=test_buffer_memory,
        .offset=0,
        .size=VK_WHOLE_SIZE
    };

    CVM_VK_CHECK(vkFlushMappedMemoryRanges(cvm_vk_device,1,&flush_range));

    vkUnmapMemory(cvm_vk_device,test_buffer_memory);
}



static void initialise_test_swapchain_dependencies(VkDevice device,VkPhysicalDevice physical_device,VkRect2D screen_rectangle,uint32_t swapchain_image_count,VkImageView * swapchain_image_views)
{
    initialise_test_swapchain_dependencies_ext(cvm_vk_screen_rectangle,swapchain_image_count,swapchain_image_views);
}

static void render_test(VkCommandBuffer command_buffer,uint32_t swapchain_image_index,VkRect2D screen_rectangle)
{
    test_render_ext(command_buffer,swapchain_image_index,screen_rectangle,&test_buffer);
}

static void terminate_test_swapchain_dependencies(VkDevice device)
{
    terminate_test_swapchain_dependencies_ext();
}




static cvm_vk_external_module test_module=
(cvm_vk_external_module)
{
    .initialise =   initialise_test_swapchain_dependencies,
    .render     =   render_test,
    .terminate  =   terminate_test_swapchain_dependencies
};

void initialise_test_render_data()
{
    create_vertex_buffer();

    initialise_test_render_data_ext();

    cvm_vk_add_external_module(test_module);
}

void terminate_test_render_data()
{
    vkDestroyBuffer(cvm_vk_device,test_buffer,NULL);
    vkFreeMemory(cvm_vk_device,test_buffer_memory,NULL);

    terminate_test_render_data_ext();
}
