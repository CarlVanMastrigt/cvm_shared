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


static VkInstance cvm_vk_instance;
static VkPhysicalDevice cvm_vk_physical_device;
static VkDevice cvm_vk_device;///"logical" device
static VkSurfaceKHR cvm_vk_surface;

static VkSwapchainKHR cvm_vk_swapchain;

static VkSurfaceFormatKHR cvm_vk_surface_format;
static VkPresentModeKHR cvm_vk_surface_present_mode;
static VkSurfaceFormatKHR cvm_vk_surface_format;


static VkRect2D cvm_vk_screen_rectangle;///extent
static VkViewport cvm_vk_screen_viewport;

///may make more sense to put just above in struct (useful for "one time" initialisation functions, like vertex buffer)
///ALTERNATIVELY have functions to allocate these external objects? -- probably just use this option








static uint32_t cvm_vk_graphics_queue_family;
static VkCommandPool cvm_vk_graphics_command_pool;
static VkQueue cvm_vk_graphics_queue;

static uint32_t cvm_vk_present_queue_family;
static VkCommandPool cvm_vk_present_command_pool;
static VkQueue cvm_vk_present_queue;

/// (for now) transfer queue only used at startup, before fiirst graphics render op, so only need 1 and can call vkDeviceWaitIdle after to avoid need to sync
/// presently all runtime uploads are stream so better to make them all be host visible anyway (ergo no need for transfer in render)
static uint32_t cvm_vk_transfer_queue_family;
static VkCommandPool cvm_vk_transfer_command_pool;
static VkQueue cvm_vk_transfer_queue;
VkCommandBuffer cvm_vk_transfer_command_buffer;

///submit transfer ops?

static struct
{
    VkFence completion_fence;

    VkSemaphore acquire_semaphore;
    VkSemaphore graphics_semaphore;
    VkSemaphore present_semaphore;

    VkCommandBuffer graphics_command_buffer;///allocated from cvm_vk_graphics_command_pool above
    VkCommandBuffer present_command_buffer;///allocated from cvm_vk_present_command_pool above
}
cvm_vk_presentation_instances[CVM_VK_PRESENTATION_INSTANCE_COUNT];

static uint32_t cvm_vk_presentation_instance_index=0;

static VkImage cvm_vk_swapchain_images[CVM_VK_MAX_SWAPCHAIN_IMAGES];
static VkImageView cvm_vk_swapchain_image_views[CVM_VK_MAX_SWAPCHAIN_IMAGES];
static uint32_t cvm_vk_swapchain_image_count;


/// these MUST be called in critical section, can modify resources
static cvm_vk_external_initialise cvm_vk_initialise_ops[CVM_VK_MAX_EXTERNAL];
static cvm_vk_external_terminate cvm_vk_terminate_ops[CVM_VK_MAX_EXTERNAL];
static cvm_vk_external_render cvm_vk_render_ops[CVM_VK_MAX_EXTERNAL];
static uint32_t external_op_count=0;


static void initialise_cvm_vk_instance(SDL_Window * window)
{
    uint32_t extension_count;
    const char ** extension_names=NULL;
    const char * layer_names[]={"VK_LAYER_LUNARG_standard_validation"};///VK_LAYER_LUNARG_standard_validation

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
        .apiVersion=VK_MAKE_VERSION(1,1,VK_HEADER_VERSION)
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



static void initialise_cvm_vk_surface(SDL_Window * window)
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

    //VkPhysicalDeviceFeatures required_features={VK_FALSE};

    ///add more if more back-end queues are desired (add them to back-end family)
    ///also potentially add transfer queue for quicker data transfer?

    vkGetPhysicalDeviceProperties(cvm_vk_physical_device,&properties);

    if((dedicated_gpu_required)&&(properties.deviceType!=VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))return false;

    printf("testing GPU : %s\n",properties.deviceName);

    #warning test for required features. need comparitor struct to do this?   enable all by default for time being?

    //printf("geometry shader: %d\n",cvm_vk_device_features.geometryShader);

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

        ///for testing purposed comment out this element
        ///if an explicitly a transfer queue family separate from selected graphics queue family exists use it independently
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

    return ((cvm_vk_graphics_queue_family != queue_family_count)&&(cvm_vk_transfer_queue_family != queue_family_count)&&(cvm_vk_present_queue_family != queue_family_count));
    ///return true only if all queue families needed can be satisfied
}

static void initialise_cvm_vk_physical_device(void)
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

    cvm_vk_surface_format=formats[0];

    free(formats);


    ///select screen present mode
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(cvm_vk_physical_device,cvm_vk_surface,&present_mode_count,NULL));
    present_modes=malloc(sizeof(VkPresentModeKHR)*present_mode_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(cvm_vk_physical_device,cvm_vk_surface,&present_mode_count,present_modes));

    cvm_vk_surface_present_mode=VK_PRESENT_MODE_FIFO_KHR;///guaranteed to exist

    free(present_modes);
}

static void initialise_cvm_vk_logical_device(void)
{
    const char * device_extensions[]={"VK_KHR_swapchain"};
    const char * layer_names[]={"VK_LAYER_LUNARG_standard_validation"};///VK_LAYER_LUNARG_standard_validation
    float queue_priority=1.0;

    ///try copying device_features from gfx until appropriate list of required features found
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


uint32_t add_cvm_vk_swapchain_dependent_functions(cvm_vk_external_initialise initialise,cvm_vk_external_terminate terminate,cvm_vk_external_render render)
{
    if(external_op_count==CVM_VK_MAX_EXTERNAL)
    {
        fprintf(stderr,"INSUFFICIENT EXTERNAL OP SPACE\n");
    }

    cvm_vk_initialise_ops[external_op_count]=initialise;
    cvm_vk_terminate_ops[external_op_count]=terminate;
    cvm_vk_render_ops[external_op_count]=render;

    return external_op_count++;
}



///temp/test function





//static void initialise_cvm_vk_swapchain(void)
//{
//    uint32_t i;
//    VkSurfaceCapabilitiesKHR surface_capabilities;
//
//    ///swapchain
//    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(cvm_vk_physical_device,cvm_vk_surface,&surface_capabilities));
//
//    cvm_vk_screen_rectangle.offset.x=0;
//    cvm_vk_screen_rectangle.offset.y=0;
//    cvm_vk_screen_rectangle.extent=surface_capabilities.currentExtent;
//
//    cvm_vk_screen_viewport=(VkViewport)
//    {
//        .x=0.0,
//        .y=0.0,
//        .width=(float)cvm_vk_screen_rectangle.extent.width,
//        .height=(float)cvm_vk_screen_rectangle.extent.height,
//        .minDepth=0.0,
//        .maxDepth=1.0
//    };
//
//    VkSwapchainCreateInfoKHR swapchain_create_info=(VkSwapchainCreateInfoKHR)
//    {
//        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
//        .pNext=NULL,
//        .flags=0,
//        .surface=cvm_vk_surface,
//        .minImageCount=surface_capabilities.minImageCount,
//        .imageFormat=cvm_vk_surface_format.format,
//        .imageColorSpace=cvm_vk_surface_format.colorSpace,
//        .imageExtent=surface_capabilities.currentExtent,
//        .imageArrayLayers=1,
//        .imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
//        .imageSharingMode=VK_SHARING_MODE_EXCLUSIVE,
//        .queueFamilyIndexCount=0,
//        .pQueueFamilyIndices=NULL,
//        .preTransform=surface_capabilities.currentTransform,
//        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
//        .presentMode=cvm_vk_surface_present_mode,
//        .clipped=VK_TRUE,
//        .oldSwapchain=VK_NULL_HANDLE///maybe find nice way to make this old one, set global static var w/ old one (though this needs to be called before that one is destroyed)
//    };
//
//    CVM_VK_CHECK(vkCreateSwapchainKHR(cvm_vk_device,&swapchain_create_info,NULL,&cvm_vk_swapchain));
//
//
//
//
//    cvm_vk_swapchain_image_count=CVM_VK_MAX_SWAPCHAIN_IMAGES;
//    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&cvm_vk_swapchain_image_count,cvm_vk_swapchain_images));
//
//    VkImageViewCreateInfo image_view_create_info;
//
//    for(i=0;i<cvm_vk_swapchain_image_count;i++)
//    {
//        image_view_create_info=(VkImageViewCreateInfo)
//        {
//            .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
//            .pNext=NULL,
//            .flags=0,
//            .image=cvm_vk_swapchain_images[i],
//            .viewType=VK_IMAGE_VIEW_TYPE_2D,
//            .format=cvm_vk_surface_format.format,
//            .components=(VkComponentMapping)
//            {
//                .r=VK_COMPONENT_SWIZZLE_IDENTITY,
//                .g=VK_COMPONENT_SWIZZLE_IDENTITY,
//                .b=VK_COMPONENT_SWIZZLE_IDENTITY,
//                .a=VK_COMPONENT_SWIZZLE_IDENTITY
//            },
//            .subresourceRange=(VkImageSubresourceRange)
//            {
//                .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
//                .baseMipLevel=0,
//                .levelCount=1,
//                .baseArrayLayer=0,
//                .layerCount=1
//            }
//        };
//
//        CVM_VK_CHECK(vkCreateImageView(cvm_vk_device,&image_view_create_info,NULL,cvm_vk_swapchain_image_views+i));
//    }
//}

//static void free_cvm_vk_swapchain_images(void)
//{
//    uint32_t i;
//
//    for(i=0;i<cvm_vk_swapchain_image_count;i++)
//    {
//        vkDestroyImageView(cvm_vk_device,cvm_vk_swapchain_image_views[i],NULL);
//    }
//}





static void initialise_presentation_intances(void)
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


        if(i)fence_create_info=(VkFenceCreateInfo)///all but first should already be signalled
        {
            .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext=NULL,
            .flags=VK_FENCE_CREATE_SIGNALED_BIT
        };
        else fence_create_info=(VkFenceCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        CVM_VK_CHECK(vkCreateFence(cvm_vk_device,&fence_create_info,NULL,&cvm_vk_presentation_instances[i].completion_fence));
    }
}

static void delete_presentation_intances(void)
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




//void recreate_cvm_vk_display_data(void)
//{
//    CVM_VK_CHECK(vkDeviceWaitIdle(cvm_vk_device));
//
////    VkSwapchainKHR old_swapchain=gfx->swapchain;
//
//
//    ///call terminate functions
//
//
//
//    initialise_cvm_vk_swapchain();
//}




void initialise_cvm_vk_swapchain_and_dependents(void)
{
    uint32_t i;
    VkSurfaceCapabilitiesKHR surface_capabilities;

    ///swapchain
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(cvm_vk_physical_device,cvm_vk_surface,&surface_capabilities));

    cvm_vk_screen_rectangle.offset.x=0;
    cvm_vk_screen_rectangle.offset.y=0;
    cvm_vk_screen_rectangle.extent=surface_capabilities.currentExtent;

    cvm_vk_screen_viewport=(VkViewport)
    {
        .x=0.0,
        .y=0.0,
        .width=(float)cvm_vk_screen_rectangle.extent.width,
        .height=(float)cvm_vk_screen_rectangle.extent.height,
        .minDepth=0.0,
        .maxDepth=1.0
    };

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
        .oldSwapchain=VK_NULL_HANDLE///maybe find nice way to make this old one, set global static var w/ old one (though this needs to be called before that one is destroyed)
    };

    CVM_VK_CHECK(vkCreateSwapchainKHR(cvm_vk_device,&swapchain_create_info,NULL,&cvm_vk_swapchain));




    cvm_vk_swapchain_image_count=CVM_VK_MAX_SWAPCHAIN_IMAGES;
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


    ///initialise dependents
}
//void reinitialise_cvm_vk_swapchain_and_dependents(void)
//{
//}
void terminate_cvm_vk_swapchain_and_dependents(void)
{
    uint32_t i;

    ///terminate dependents

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        vkDestroyImageView(cvm_vk_device,cvm_vk_swapchain_image_views[i],NULL);
    }

    vkDestroySwapchainKHR(cvm_vk_device,cvm_vk_swapchain,NULL);
}






void initialise_cvm_vk_data(SDL_Window * window)
{
    initialise_cvm_vk_instance(window);
    initialise_cvm_vk_surface(window);
    initialise_cvm_vk_physical_device();
    initialise_cvm_vk_logical_device();
    initialise_presentation_intances();
}

void terminate_cvm_vk_data(void)
{
    vkDeviceWaitIdle(cvm_vk_device);

    delete_presentation_intances();

    terminate_cvm_vk_swapchain_and_dependents();

    vkDestroyDevice(cvm_vk_device,NULL);
    vkDestroyInstance(cvm_vk_instance,NULL);
}


void present_cvm_vk_data(void)
{
    uint32_t image_index;
    VkResult r;

    ///these types could probably be re-used, but as they're passed as pointers its better to be sure?
    VkImageMemoryBarrier graphics_barrier,present_barrier;
    VkSubmitInfo graphics_submit_info,present_submit_info;
    VkPipelineStageFlags graphics_wait_dst_stage_mask,present_wait_dst_stage_mask;
    VkPresentInfoKHR present_info;




    r=vkAcquireNextImageKHR(cvm_vk_device,cvm_vk_swapchain,1000000000,cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].acquire_semaphore,VK_NULL_HANDLE,&image_index);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't use swapchain");//recreate_gfx_swapchain(gfx);///rebuild swapchain dependent elements for this buffer
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",r);



    ///seperate command bufer for each render element (game/overlay) ?

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





    CVM_VK_CHECK(vkBeginCommandBuffer(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].graphics_command_buffer,&command_buffer_begin_info));









    ///this shouldn't be necessary when any external unit is used (possibly have fallback blank screen renderer though?)

    VkImageMemoryBarrier temp_barrier=(VkImageMemoryBarrier)
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext=NULL,
        .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,///stage where data is written BEFORE the transfer/barrier
        .dstAccessMask=0,///ignored anyway
        .oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex=cvm_vk_graphics_queue_family,
        .dstQueueFamilyIndex=cvm_vk_graphics_queue_family,
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
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&temp_barrier);






    /// example/test render pass, put in separate function
    #warning call render function here




    graphics_barrier=(VkImageMemoryBarrier) /// does layout transition  &  if queue families are different relinquishes swapchain image
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext=NULL,
        .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,///stage where data is written BEFORE the transfer/barrier
        .dstAccessMask=0,///ignored anyway
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
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&graphics_barrier);

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

        vkCmdPipelineBarrier(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].present_command_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,NULL,0,NULL,1,&present_barrier);

        CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].present_command_buffer));

        present_wait_dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

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

    ///following transfers image to present_queue family



    r=vkQueuePresentKHR(cvm_vk_present_queue,&present_info);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't present (out of date)");//recreate_gfx_swapchain(gfx);///rebuild swapchain dependent elements for this buffer
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);




    cvm_vk_presentation_instance_index=(cvm_vk_presentation_instance_index+1)%CVM_VK_PRESENTATION_INSTANCE_COUNT;

    /// finish/sync "next" (oldest in queue) presentation instance after this (new) one is fully submitted

    CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].completion_fence,VK_TRUE,1000000000));
    CVM_VK_CHECK(vkResetFences(cvm_vk_device,1,&cvm_vk_presentation_instances[cvm_vk_presentation_instance_index].completion_fence));
}



























///TEST FUNCTIONS FROM HERE, STRIP DOWN / USE AS BASIS FOR REAL VERSIONS

typedef struct test_render_data
{
    vec3f pos;
    vec3f c;
}
test_render_data;

static VkPipeline test_pipeline;
static VkBuffer test_buffer;
static VkDeviceMemory test_buffer_memory;
static VkRenderPass test_render_pass;
static VkFramebuffer test_framebuffers[16];
static uint32_t test_framebuffer_count;




VkShaderModule load_shader_data(const char * filename)
{
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

    return shader_module;
}




static void create_pipeline(VkRenderPass render_pass,VkRect2D screen_rectangle,VkViewport screen_viewport)
{
    ///load test shaders

    /// "shaders/test_vert.spv" "shaders/test_frag.spv"

    VkShaderModule stage_modules[16];
    VkPipelineShaderStageCreateInfo stage_creation_info[16];
    VkPipelineLayout layout;
    uint32_t i,stage_count=0;

    //if(vert_file)
    {
        stage_modules[stage_count]=load_shader_data("shaders/test_vert.spv");
        stage_creation_info[stage_count]=(VkPipelineShaderStageCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .stage=VK_SHADER_STAGE_VERTEX_BIT,
            .module=stage_modules[stage_count],
            .pName="main",
            .pSpecializationInfo=NULL
        };
        stage_count++;
    }

    //if(frag_file)
    {
        stage_modules[stage_count]=load_shader_data("shaders/test_frag.spv");
        stage_creation_info[stage_count]=(VkPipelineShaderStageCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .stage=VK_SHADER_STAGE_FRAGMENT_BIT,
            .module=stage_modules[stage_count],
            .pName="main",
            .pSpecializationInfo=NULL
        };
        stage_count++;
    }

    VkVertexInputBindingDescription input_bindings[1]=
    {
        (VkVertexInputBindingDescription)
        {
            .binding=0,
            .stride=sizeof(test_render_data),
            .inputRate=VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    VkVertexInputAttributeDescription input_attributes[2]=
    {
        (VkVertexInputAttributeDescription)
        {
            .location=0,
            .binding=0,
            .format=VK_FORMAT_R32G32B32_SFLOAT,
            .offset=offsetof(test_render_data,pos)
        },
        (VkVertexInputAttributeDescription)
        {
            .location=1,
            .binding=0,
            .format=VK_FORMAT_R32G32B32_SFLOAT,
            .offset=offsetof(test_render_data,c)
        }
    };

    VkPipelineVertexInputStateCreateInfo vert_input_info=(VkPipelineVertexInputStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .vertexBindingDescriptionCount=1,
        .pVertexBindingDescriptions=input_bindings,
        .vertexAttributeDescriptionCount=2,
        .pVertexAttributeDescriptions=input_attributes
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info=(VkPipelineInputAssemblyStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,///VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        .primitiveRestartEnable=VK_FALSE
    };


    VkPipelineViewportStateCreateInfo viewport_state_info=(VkPipelineViewportStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .viewportCount=1,
        .pViewports= &screen_viewport,
        .scissorCount=1,
        .pScissors= &screen_rectangle
    };


    VkPipelineRasterizationStateCreateInfo rasterization_state_info=(VkPipelineRasterizationStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .depthClampEnable=VK_FALSE,
        .rasterizerDiscardEnable=VK_FALSE,
        .polygonMode=VK_POLYGON_MODE_FILL,
        .cullMode=VK_CULL_MODE_BACK_BIT,
        .frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable=VK_FALSE,
        .depthBiasConstantFactor=0.0,
        .depthBiasClamp=0.0,
        .depthBiasSlopeFactor=0.0,
        .lineWidth=1.0
    };

    VkPipelineMultisampleStateCreateInfo multisample_state_info=(VkPipelineMultisampleStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .rasterizationSamples=VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable=VK_FALSE,
        .minSampleShading=1.0,
        .pSampleMask=NULL,
        .alphaToCoverageEnable=VK_FALSE,
        .alphaToOneEnable=VK_FALSE
    };

    VkPipelineColorBlendAttachmentState blend_attachment_state_info=(VkPipelineColorBlendAttachmentState)
    {
        .blendEnable=VK_FALSE,
        .srcColorBlendFactor=VK_BLEND_FACTOR_ONE,//VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor=VK_BLEND_FACTOR_ZERO,//VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
        .colorBlendOp=VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE,//VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO,//VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
        .alphaBlendOp=VK_BLEND_OP_ADD,
        .colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo blend_state_info=(VkPipelineColorBlendStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .logicOpEnable=VK_FALSE,
        .logicOp=VK_LOGIC_OP_COPY,
        .attachmentCount=1,///must equal colorAttachmentCount in subpass
        .pAttachments= &blend_attachment_state_info,
        .blendConstants={0.0,0.0,0.0,0.0}
    };

    VkPipelineLayoutCreateInfo layout_creation_info=(VkPipelineLayoutCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .setLayoutCount=0,
        .pSetLayouts=NULL,
        .pushConstantRangeCount=0,
        .pPushConstantRanges=NULL
    };

    CVM_VK_CHECK(vkCreatePipelineLayout(cvm_vk_device,&layout_creation_info,NULL,&layout));

    VkGraphicsPipelineCreateInfo pipeline_creation_info=(VkGraphicsPipelineCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .stageCount=stage_count,
        .pStages=stage_creation_info,
        .pVertexInputState= &vert_input_info,
        .pInputAssemblyState= &input_assembly_info,
        .pTessellationState=NULL,
        .pViewportState= &viewport_state_info,
        .pRasterizationState= &rasterization_state_info,
        .pMultisampleState= &multisample_state_info,
        .pDepthStencilState=NULL,
        .pColorBlendState= &blend_state_info,
        .pDynamicState=NULL,
        .layout=layout,
        .renderPass=render_pass,
        .subpass=0,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };

    CVM_VK_CHECK(vkCreateGraphicsPipelines(cvm_vk_device,VK_NULL_HANDLE,1,&pipeline_creation_info,NULL,&test_pipeline));

    vkDestroyPipelineLayout(cvm_vk_device,layout,NULL);

    for(i=0;i<stage_count;i++)vkDestroyShaderModule(cvm_vk_device,stage_modules[i],NULL);
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



static void initialise_test_external(VkDevice device,VkPhysicalDevice physical_device,VkRect2D screen_rectangle,VkViewport screen_viewport,uint32_t swapchain_image_count,VkImageView * swapchain_image_views)
{
    uint32_t i;
    VkFramebufferCreateInfo framebuffer_create_info;
    ///other per-presentation_instance images defined by rest of program should be used here

    create_pipeline(test_render_pass,screen_rectangle,screen_viewport);

    test_framebuffer_count=swapchain_image_count;

    for(i=0;i<test_framebuffer_count;i++)
    {
        framebuffer_create_info=(VkFramebufferCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .renderPass=test_render_pass,
            .attachmentCount=1,
            .pAttachments= swapchain_image_views+i,
            .width=screen_rectangle.extent.width,
            .height=screen_rectangle.extent.height,
            .layers=1
        };

        CVM_VK_CHECK(vkCreateFramebuffer(device,&framebuffer_create_info,NULL,test_framebuffers+i));
    }
}

static void terminate_test_external(VkDevice device)
{
    uint32_t i;
    vkDestroyPipeline(device,test_pipeline,NULL);

    for(i=0;i<test_framebuffer_count;i++)vkDestroyFramebuffer(device,test_framebuffers[i],NULL);
}



static void render_test_external(VkCommandBuffer command_buffer,uint32_t swapchain_image_index,VkRect2D screen_rectangle)
{
    VkClearValue clear_value;///other clear colours should probably be provided by other chunks of application
    clear_value.color=(VkClearColorValue){{0.95f,0.5f,0.75f,1.0f}};

    VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext=NULL,
        .renderPass=test_render_pass,
        .framebuffer=test_framebuffers[swapchain_image_index],
        .renderArea=screen_rectangle,
        .clearValueCount=1,
        .pClearValues= &clear_value
    };

    vkCmdBeginRenderPass(command_buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);///================

    vkCmdBindPipeline(command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,test_pipeline);

    VkDeviceSize offset=0;
    vkCmdBindVertexBuffers(command_buffer,0,1,&test_buffer,&offset);

    vkCmdDraw(command_buffer,4,1,0,0);

    vkCmdEndRenderPass(command_buffer);///================
}






void initialise_test_render_data()
{
    ///default attachment (swapchain image)
    VkAttachmentDescription attachment_description=(VkAttachmentDescription)
    {
        .flags=0,
        .format=cvm_vk_surface_format.format,
        .samples=VK_SAMPLE_COUNT_1_BIT,
        .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference attachment_reference=(VkAttachmentReference)
    {
        .attachment=0,
        .layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass_description=(VkSubpassDescription)
    {
        .flags=0,
        .pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount=0,
        .pInputAttachments=NULL,
        .colorAttachmentCount=1,
        .pColorAttachments= &attachment_reference,
        .pResolveAttachments=NULL,
        .pDepthStencilAttachment=NULL,
        .preserveAttachmentCount=0,
        .pPreserveAttachments=NULL
    };

    VkSubpassDependency subpass_dependencies[2];

    ///if doing pipeline barries these external subpass demendencies probably aren't necessary
    subpass_dependencies[0]=(VkSubpassDependency)
    {
        .srcSubpass=VK_SUBPASS_EXTERNAL,
        .dstSubpass=0,
        .srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,///must be the same as wait stage in submit (the one associated w/ the relevant semaphore)
        .dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask=0,
        .dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
    };

    subpass_dependencies[1]=(VkSubpassDependency)
    {
        .srcSubpass=0,
        .dstSubpass=VK_SUBPASS_EXTERNAL,
        .srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,///dont know which stage in present uses image data so use all
        .srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask=VK_ACCESS_MEMORY_READ_BIT,
        .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo render_pass_creation_info=(VkRenderPassCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .attachmentCount=1,
        .pAttachments= &attachment_description,
        .subpassCount=1,
        .pSubpasses= &subpass_description,
        .dependencyCount=2,
        .pDependencies=subpass_dependencies
    };

    CVM_VK_CHECK(vkCreateRenderPass(cvm_vk_device,&render_pass_creation_info,NULL,&test_render_pass));

    create_vertex_buffer();

    add_cvm_vk_swapchain_dependent_functions(initialise_test_external,terminate_test_external,render_test_external);
}

void terminate_test_render_data()
{
    vkDestroyRenderPass(cvm_vk_device,test_render_pass,NULL);

    vkDestroyBuffer(cvm_vk_device,test_buffer,NULL);
    vkFreeMemory(cvm_vk_device,test_buffer_memory,NULL);
}





