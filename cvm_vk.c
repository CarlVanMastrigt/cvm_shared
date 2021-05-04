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



static uint32_t cvm_vk_update_count=0;///test if render op/buffer intended to be called is up to date

/// copying around some vulkan structs (e.g. device) is invalid, instead just use these static ones

static VkInstance cvm_vk_instance;
static VkPhysicalDevice cvm_vk_physical_device;
static VkDevice cvm_vk_device;///"logical" device
static VkSurfaceKHR cvm_vk_surface;

static VkSwapchainKHR cvm_vk_swapchain=VK_NULL_HANDLE;
static VkSurfaceFormatKHR cvm_vk_surface_format;
static VkPresentModeKHR cvm_vk_surface_present_mode;


static VkRect2D cvm_vk_screen_rectangle;///extent

///make below 2 per-submodule things
//static VkViewport cvm_vk_screen_viewport;
//static VkPipelineViewportStateCreateInfo cvm_vk_screen_viewport_state=(VkPipelineViewportStateCreateInfo)
//{
//    .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
//    .pNext=NULL,
//    .flags=0,
//    .viewportCount=1,
//    .pViewports= &cvm_vk_screen_viewport,
//    .scissorCount=1,
//    .pScissors= &cvm_vk_screen_rectangle
//};

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

///these are the command pools applicable to the main thread, should be provided upon request for a command pool by any submodule that runs in the main thread
static VkCommandPool cvm_vk_transfer_command_pool;
static VkCommandPool cvm_vk_graphics_command_pool;
static VkCommandPool cvm_vk_present_command_pool;///only ever used within this file


/// (for now) transfer queue only used at startup, before first graphics render op, so only need 1 and can call vkDeviceWaitIdle after to avoid need to sync
/// presently all runtime uploads are stream so better to make them all be host visible anyway (ergo no need for transfer in render)


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

tie command buffers to their state's age (use update count to determine if a command is out of date, this alleviates need to wait for commands to finish before recreating pipelines and other resources,
    ^ just wait on fence then increment the update counter, (need to track how many are in flight!) track submit count and wait on fences of completion count to match (can simply be uint that rolls over!
    ^ this paradigm agoids needing to wait on device
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
cvm_vk_presentation_instances[CVM_VK_PRESENTATION_INSTANCE_COUNT];

/// CVM_VK_PRESENTATION_INSTANCE_COUNT should really match swapchain image count, alloc it to be the same


/// need to know the max frame count such that resources can only be used after they are transferred back, should this be matched to the number of swapchain images?
///probably best to match this to swapchain count

static uint32_t cvm_vk_presentation_instance_index=0;

static VkImage * cvm_vk_swapchain_images;
static VkImageView * cvm_vk_swapchain_image_views;
static uint32_t cvm_vk_swapchain_image_count;


static cvm_vk_external_module cvm_vk_external_modules[CVM_VK_MAX_EXTERNAL];
static uint32_t cvm_vk_external_module_count=0;


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






static void cvm_vk_initialise_presentation_intances(void)
{
    uint32_t i;

    VkCommandBufferAllocateInfo command_buffer_allocate_info;
    VkSemaphoreCreateInfo semaphore_create_info;
    VkFenceCreateInfo fence_create_info;
    VkCommandPoolCreateInfo command_pool_create_info;


    command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex=cvm_vk_graphics_queue_family
    };

    CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&cvm_vk_graphics_command_pool));


    command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex=cvm_vk_present_queue_family
    };

    CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&cvm_vk_present_command_pool));


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


//        if(i) fence_create_info=(VkFenceCreateInfo)///all but first should already be signalled
//        {
//            .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
//            .pNext=NULL,
//            .flags=VK_FENCE_CREATE_SIGNALED_BIT
//        };
//        else
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

static void cvm_vk_terminate_presentation_intances(void)
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

    vkDestroyCommandPool(cvm_vk_device,cvm_vk_graphics_command_pool,NULL);
    vkDestroyCommandPool(cvm_vk_device,cvm_vk_present_command_pool,NULL);
}




void cvm_vk_initialise_swapchain(void)
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

    //printf("Minimum swapchain image count: %d\n",surface_capabilities.minImageCount);
    //printf("Maximum swapchain image count: %d\n",surface_capabilities.maxImageCount);

    /// the contents of this dictate explicit transfer of the swapchain images between the graphics and present queues!
    VkSwapchainCreateInfoKHR swapchain_create_info=(VkSwapchainCreateInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext=NULL,
        .flags=0,
        .surface=cvm_vk_surface,
        .minImageCount=surface_capabilities.minImageCount,
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

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&cvm_vk_swapchain_image_count,NULL));
    cvm_vk_swapchain_images=malloc(cvm_vk_swapchain_image_count*sizeof(VkImage));
    printf("swapchain image count: %u\n",cvm_vk_swapchain_image_count);
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&cvm_vk_swapchain_image_count,cvm_vk_swapchain_images));

    VkImageViewCreateInfo image_view_create_info;
    cvm_vk_swapchain_image_views=malloc(cvm_vk_swapchain_image_count*sizeof(VkImageView));

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

    vkDestroySwapchainKHR(cvm_vk_device,cvm_vk_swapchain,NULL);
    cvm_vk_swapchain=VK_NULL_HANDLE;
}


uint32_t cvm_vk_add_external_module(cvm_vk_external_module module)
{
    if(cvm_vk_external_module_count==CVM_VK_MAX_EXTERNAL)
    {
        fprintf(stderr,"INSUFFICIENT EXTERNAL OP SPACE\n");
    }

    cvm_vk_external_modules[cvm_vk_external_module_count]=module;

    return cvm_vk_external_module_count++;
}





void cvm_vk_initialise(SDL_Window * window,uint32_t frame_count)
{
    cvm_vk_initialise_instance(window);
    cvm_vk_initialise_surface(window);
    cvm_vk_initialise_physical_device();
    cvm_vk_initialise_logical_device();
    cvm_vk_initialise_presentation_intances();
}

void cvm_vk_terminate(void)
{
    cvm_vk_terminate_presentation_intances();
    cvm_vk_terminate_swapchain();

    vkDestroyDevice(cvm_vk_device,NULL);
    vkDestroySurfaceKHR(cvm_vk_instance,cvm_vk_surface,NULL);
    vkDestroyInstance(cvm_vk_instance,NULL);
}

void cvm_vk_wait(void)
{
    uint32_t i;
    for(i=0;i<CVM_VK_PRESENTATION_INSTANCE_COUNT;i++)if(cvm_vk_presentation_instances[i].in_flight)
    {
        CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&cvm_vk_presentation_instances[i].completion_fence,VK_TRUE,1000000000));
        CVM_VK_CHECK(vkResetFences(cvm_vk_device,1,&cvm_vk_presentation_instances[i].completion_fence));
        cvm_vk_presentation_instances[i].in_flight=false;///has finished
    }
    //vkDeviceWaitIdle(cvm_vk_device);///move to controller func, like recreate?
}



void cvm_vk_present(void)
{
    uint32_t i,image_index;
    VkResult r;

    ///these types could probably be re-used, but as they're passed as pointers its better to be sure?
    VkImageMemoryBarrier start_graphics_barrier,end_graphics_barrier,present_barrier;
    VkSubmitInfo graphics_submit_info,present_submit_info;
    VkPipelineStageFlags graphics_wait_dst_stage_mask,present_wait_dst_stage_mask;
    VkPresentInfoKHR present_info;



    r=vkAcquireNextImageKHR(cvm_vk_device,cvm_vk_swapchain,1000000000,cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].acquire_semaphore,VK_NULL_HANDLE,&image_index);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't use swapchain");//recreate_gfx_swapchain(gfx);///rebuild swapchain dependent elements for this buffer
        return;
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",r);



    ///separate command buffer for each render element (game/overlay) ? (i like this, no function call per submodule, just a command buffer)

    ///fence to prevent re-submission of same command buffer ( command buffer wasn't created with VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT )


    ///before  submit/present  ensure update_count associated with incoming command buffer matches the current value (so as to ensure that pipelines &c. have not been deleted)
    ///otherwise just have primary command buffer clear screen

    ///alter primary command buffer

    ///

    ///only delete pipelines & display images between mutexes in main thread (assuming multi-threading)








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



    start_graphics_barrier=(VkImageMemoryBarrier) /// does layout transition  &  if queue families are different relinquishes swapchain image, is this valid when queue families are the same?
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext=NULL,
        .srcAccessMask=0,/// 0 because its effectively a queue transfer or b/c
        .dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,///VK_QUEUE_FAMILY_IGNORED when not describing ownership transfer?
        .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
        .image=cvm_vk_swapchain_images[image_index],
        .subresourceRange=(VkImageSubresourceRange)
        {
            .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel=0,
            .levelCount=1,
            .baseArrayLayer=0,
            .layerCount=1
        }
    };

    vkCmdPipelineBarrier(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_command_buffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,NULL,0,NULL,1,&start_graphics_barrier);



    for(i=0;i<cvm_vk_external_module_count;i++)cvm_vk_external_modules[i].render(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_command_buffer,image_index,cvm_vk_screen_rectangle);

    ///only do following inside branch where the queue families are not the same? though this does handle layout transitions anyway right?
    end_graphics_barrier=(VkImageMemoryBarrier) /// does layout transition  &  if queue families are different relinquishes swapchain image, is this valid when queue families are the same?
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext=NULL,
        .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,///stage where data is written BEFORE the transfer/barrier
        .dstAccessMask=0,///should be 0 by spec
        .oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex=cvm_vk_graphics_queue_family,
        .dstQueueFamilyIndex=cvm_vk_present_queue_family,
        .image=cvm_vk_swapchain_images[image_index],
        .subresourceRange=(VkImageSubresourceRange)
        {
            .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel=0,
            .levelCount=1,
            .baseArrayLayer=0,
            .layerCount=1
        }
    };

    vkCmdPipelineBarrier(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_command_buffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&end_graphics_barrier);

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
    else ///barrier above instead releases subresource, here present queue family acquires it (with "matching" barrier) and presents, waiting on ownership transfer (present) semaphore
    {
        #warning this needs to be tested on appropriate hardware
        #warning APPARENTLY THIS HARDWARE THAT HS THIS DOES NOT EVEN EXIST !!!

        ///could use swapchain images with VK_SHARING_MODE_CONCURRENT to avoid this issue

        CVM_VK_CHECK(vkQueueSubmit(cvm_vk_graphics_queue,1,&graphics_submit_info,VK_NULL_HANDLE));///if families different fence goes after transfer op




        ///=========== this section transfers ownership (start) ==============
        present_barrier=(VkImageMemoryBarrier)
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext=NULL,
            .srcAccessMask=0,///ignored anyway
            .dstAccessMask=0,
            .oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex=cvm_vk_graphics_queue_family,
            .dstQueueFamilyIndex=cvm_vk_present_queue_family,
            .image=cvm_vk_swapchain_images[image_index],
            .subresourceRange=(VkImageSubresourceRange)
            {
                .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel=0,
                .levelCount=1,
                .baseArrayLayer=0,
                .layerCount=1
            }
        };

        CVM_VK_CHECK(vkBeginCommandBuffer(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].present_command_buffer,&command_buffer_begin_info));

        vkCmdPipelineBarrier(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].present_command_buffer,0,0,0,0,NULL,0,NULL,1,&present_barrier);
        /// from wiki: no srcStage/AccessMask or dstStage/AccessMask is needed, waiting for a semaphore does that automatically.

        CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].present_command_buffer));

        present_wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        present_submit_info=(VkSubmitInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext=NULL,
            .waitSemaphoreCount=1,
            .pWaitSemaphores= &cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_semaphore,
            .pWaitDstStageMask= &present_wait_dst_stage_mask,
            .commandBufferCount=1,
            .pCommandBuffers= &cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].present_command_buffer,
            .signalSemaphoreCount=1,
            .pSignalSemaphores= &cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].present_semaphore
        };

        CVM_VK_CHECK(vkQueueSubmit(cvm_vk_present_queue,1,&present_submit_info,cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].completion_fence));///if families different this is last submit so fence goes here
        ///=========== this section transfers ownership (end) ==============
        ///if vkQueueSubmit fails here it will be necessary to invalidate the command buffer contents




        present_info=(VkPresentInfoKHR)
        {
            .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext=NULL,
            .waitSemaphoreCount=1,
            .pWaitSemaphores= &cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].present_semaphore,
            .swapchainCount=1,
            .pSwapchains= &cvm_vk_swapchain,
            .pImageIndices= &image_index,
            .pResults=NULL
        };
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


    //cvm_vk_presentation_instances[i].in_flight

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



















///TEST FUNCTIONS FROM HERE, STRIP DOWN / USE AS BASIS FOR REAL VERSIONS



static VkBuffer test_buffer;
static VkDeviceMemory test_buffer_memory;








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





