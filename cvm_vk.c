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

#include "cvm_shared.h"

///should probably have null initailisation of these vars, though that does obfuscate those that really do need to be statically initialised

static VkInstance cvm_vk_instance;
static VkPhysicalDevice cvm_vk_physical_device;
static VkDevice cvm_vk_device;///"logical" device
static VkSurfaceKHR cvm_vk_surface;
static VkPhysicalDeviceMemoryProperties cvm_vk_memory_properties;
static VkPhysicalDeviceProperties cvm_vk_device_properties;

static VkSwapchainKHR cvm_vk_swapchain=VK_NULL_HANDLE;
static VkSurfaceFormatKHR cvm_vk_surface_format;
static VkPresentModeKHR cvm_vk_surface_present_mode;
static VkRect2D cvm_vk_screen_rectangle;///extent

///these can be the same
static uint32_t cvm_vk_transfer_queue_family;
static uint32_t cvm_vk_graphics_queue_family;
static uint32_t cvm_vk_present_queue_family;
///only support one of each of above (allow these to be the same if above are as well?)
static VkQueue cvm_vk_transfer_queue;
static VkQueue cvm_vk_graphics_queue;
static VkQueue cvm_vk_present_queue;

///these are the command pools applicable to the main thread, should be provided upon request for a command pool by any module that runs in the main thread
static VkCommandPool cvm_vk_transfer_command_pool;///provided as default for same-thread modules
static VkCommandPool cvm_vk_graphics_command_pool;///provided as default for same-thread modules
static VkCommandPool cvm_vk_present_command_pool;///only ever used within this file



///may want to rename cvm_vk_frames, cvm_vk_presentation_instance probably isnt that bad tbh...
///realloc these only if number of swapchain image count changes (it wont really)
static cvm_vk_swapchain_image_acquisition_data * cvm_vk_acquired_images=NULL;///number of these should match swapchain image count
static cvm_vk_swapchain_image_present_data * cvm_vk_presenting_images=NULL;
static uint32_t cvm_vk_swapchain_image_count=0;/// this is also the number of swapchain images
static uint32_t cvm_vk_current_acquired_image_index;
static uint32_t cvm_vk_acquired_image_count=0;///both frames in flight and frames acquired by rendereer
///following used to determine number of swapchain images to allocate
static uint32_t cvm_vk_min_swapchain_images;
static uint32_t cvm_vk_extra_swapchain_images;///thread separation, applicable to transfer as well, name *should* reflect this
static bool cvm_vk_rendering_resources_valid=false;///can this be determined for next frame during critical section?


//static uint32_t cvm_vk_transfer_cycle_count;



static void cvm_vk_create_instance(SDL_Window * window)
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

static void cvm_vk_create_surface(SDL_Window * window)
{
    if(!SDL_Vulkan_CreateSurface(window,cvm_vk_instance,&cvm_vk_surface))
    {
        fprintf(stderr,"COULD NOT CREATE SDL VULKAN SURFACE\n");
        exit(-1);
    }
}

static bool check_physical_device_appropriate(bool dedicated_gpu_required,bool sync_compute_required)
{
    uint32_t i,queue_family_count;
    VkQueueFamilyProperties * queue_family_properties=NULL;
    VkBool32 surface_supported;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(cvm_vk_physical_device,&cvm_vk_device_properties);

    if((dedicated_gpu_required)&&(cvm_vk_device_properties.deviceType!=VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))return false;

    printf("testing GPU : %s\n",cvm_vk_device_properties.deviceName);

    vkGetPhysicalDeviceFeatures(cvm_vk_physical_device,&features);

    #warning test for required features. need comparitor struct to do this?   enable all by default for time being?

    ///SIMPLE EXAMPLE TEST: printf("geometry shader: %d\n",features.geometryShader);

    vkGetPhysicalDeviceQueueFamilyProperties(cvm_vk_physical_device,&queue_family_count,NULL);
    queue_family_properties=malloc(sizeof(VkQueueFamilyProperties)*queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(cvm_vk_physical_device,&queue_family_count,queue_family_properties);

    cvm_vk_graphics_queue_family=queue_family_count;
    cvm_vk_transfer_queue_family=queue_family_count;
    cvm_vk_present_queue_family=queue_family_count;

    for(i=0;i<queue_family_count;i++)
    {
        //printf("queue family %u available queue count: %u\n",i,queue_family_properties[i].queueCount);
        #warning add support for a-sync compute

        ///If an implementation exposes any queue family that supports graphics operations,
        /// at least one queue family of at least one physical device exposed by the implementation must support both graphics and compute operations.

        if((cvm_vk_graphics_queue_family==queue_family_count)&&(queue_family_properties[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)&&(queue_family_properties[i].queueFlags&VK_QUEUE_COMPUTE_BIT || !sync_compute_required))
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

static void cvm_vk_create_physical_device(bool sync_compute_required)
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
        if(check_physical_device_appropriate(true,sync_compute_required)) break;
    }

    if(i==device_count)for(i=0;i<device_count;i++)///check for non-dedicated (integrated) gfx cards next
    {
        cvm_vk_physical_device=physical_devices[i];
        if(check_physical_device_appropriate(false,sync_compute_required)) break;
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

    vkGetPhysicalDeviceMemoryProperties(cvm_vk_physical_device,&cvm_vk_memory_properties);
    printf("memory types: %u\nheap count: %u\n",cvm_vk_memory_properties.memoryTypeCount,cvm_vk_memory_properties.memoryHeapCount);
//    for(i=0;i<memory_properties.memoryTypeCount;i++)
//    {
//        if(memory_properties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
//        {
//            printf("device local memory type: %u using heap: %u\n",i,memory_properties.memoryTypes[i].heapIndex);
//        }
//    }

    free(present_modes);
}

static void cvm_vk_create_logical_device(void)
{
    const char * device_extensions[]={"VK_KHR_swapchain"};
    const char * layer_names[]={"VK_LAYER_KHRONOS_validation"};///VK_LAYER_LUNARG_standard_validation
    float queue_priority=1.0;
    ///if implementing separate transfer queue use a lower priority

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



void cvm_vk_create_swapchain(void)
{
    uint32_t i;
    VkSurfaceCapabilitiesKHR surface_capabilities;

    VkSwapchainKHR old_swapchain=cvm_vk_swapchain;
    uint32_t old_swapchain_image_count=cvm_vk_swapchain_image_count;

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
        .oldSwapchain=old_swapchain
    };

    CVM_VK_CHECK(vkCreateSwapchainKHR(cvm_vk_device,&swapchain_create_info,NULL,&cvm_vk_swapchain));

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&i,NULL));
    if(i!=cvm_vk_swapchain_image_count)
    {
        fprintf(stderr,"COULD NOT CREATE REQUESTED NUMBER OF SWAPCHAIN IMAGES\n");
        exit(-1);
    }

    VkImage * swapchain_images=malloc(sizeof(VkImage)*cvm_vk_swapchain_image_count);

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&cvm_vk_swapchain_image_count,swapchain_images));



    cvm_vk_current_acquired_image_index=0;///need to reset in case number of swapchain images goes down

    if(old_swapchain_image_count!=cvm_vk_swapchain_image_count)
    {
        cvm_vk_acquired_images=realloc(cvm_vk_acquired_images,cvm_vk_swapchain_image_count*sizeof(cvm_vk_swapchain_image_acquisition_data));
        cvm_vk_presenting_images=realloc(cvm_vk_presenting_images,cvm_vk_swapchain_image_count*sizeof(cvm_vk_swapchain_image_present_data));
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        cvm_vk_acquired_images[i].image_index=CVM_VK_INVALID_IMAGE_INDEX;

        VkSemaphoreCreateInfo semaphore_create_info=(VkSemaphoreCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_acquired_images[i].acquire_semaphore));


        /// swapchain frames
        cvm_vk_presenting_images[i].in_flight=false;
        cvm_vk_presenting_images[i].image=swapchain_images[i];

        VkFenceCreateInfo fence_create_info=(VkFenceCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        CVM_VK_CHECK(vkCreateFence(cvm_vk_device,&fence_create_info,NULL,&cvm_vk_presenting_images[i].completion_fence));

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

        CVM_VK_CHECK(vkCreateImageView(cvm_vk_device,&image_view_create_info,NULL,&cvm_vk_presenting_images[i].image_view));

        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_presenting_images[i].present_semaphore));


        ///queue ownership transfer stuff
        if(cvm_vk_present_queue_family!=cvm_vk_graphics_queue_family)
        {
            CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&semaphore_create_info,NULL,&cvm_vk_presenting_images[i].graphics_relinquish_semaphore));


            VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext=NULL,
                .commandPool=0,///set later
                .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount=1
            };

            command_buffer_allocate_info.commandPool=cvm_vk_graphics_command_pool;
            CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&cvm_vk_presenting_images[i].graphics_relinquish_command_buffer));

            command_buffer_allocate_info.commandPool=cvm_vk_present_command_pool;
            CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&cvm_vk_presenting_images[i].present_acquire_command_buffer));

            VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext=NULL,
                .flags=0,///not one time use
                .pInheritanceInfo=NULL
            };

            /// would really like to swap to VkImageMemoryBarrier2KHR here, is much nicer way to organise data



            CVM_VK_CHECK(vkBeginCommandBuffer(cvm_vk_presenting_images[i].graphics_relinquish_command_buffer,&command_buffer_begin_info));

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

            vkCmdPipelineBarrier(cvm_vk_presenting_images[i].graphics_relinquish_command_buffer,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,0,NULL,0,NULL,1,&graphics_relinquish_barrier);///dstStageMask=VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT ??

            CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_presenting_images[i].graphics_relinquish_command_buffer));



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

            CVM_VK_CHECK(vkBeginCommandBuffer(cvm_vk_presenting_images[i].present_acquire_command_buffer,&command_buffer_begin_info));

            vkCmdPipelineBarrier(cvm_vk_presenting_images[i].present_acquire_command_buffer,0,0,0,0,NULL,0,NULL,1,&present_acquire_barrier);
            /// from wiki: no srcStage/AccessMask or dstStage/AccessMask is needed, waiting for a semaphore does that automatically.

            CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_presenting_images[i].present_acquire_command_buffer));
        }
    }

    free(swapchain_images);

    if(old_swapchain!=VK_NULL_HANDLE)vkDestroySwapchainKHR(cvm_vk_device,old_swapchain,NULL);

    cvm_vk_rendering_resources_valid=true;
}

void cvm_vk_destroy_swapchain(void)
{
    uint32_t i;

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        vkDestroySemaphore(cvm_vk_device,cvm_vk_acquired_images[i].acquire_semaphore,NULL);

        /// swapchain frames
        vkDestroyImageView(cvm_vk_device,cvm_vk_presenting_images[i].image_view,NULL);

        vkDestroyFence(cvm_vk_device,cvm_vk_presenting_images[i].completion_fence,NULL);

        vkDestroySemaphore(cvm_vk_device,cvm_vk_presenting_images[i].present_semaphore,NULL);

        ///queue ownership transfer stuff
        if(cvm_vk_present_queue_family!=cvm_vk_graphics_queue_family)
        {
            vkDestroySemaphore(cvm_vk_device,cvm_vk_presenting_images[i].graphics_relinquish_semaphore,NULL);

            vkFreeCommandBuffers(cvm_vk_device,cvm_vk_graphics_command_pool,1,&cvm_vk_presenting_images[i].graphics_relinquish_command_buffer);
            vkFreeCommandBuffers(cvm_vk_device,cvm_vk_graphics_command_pool,1,&cvm_vk_presenting_images[i].present_acquire_command_buffer);
        }
    }
}


VkRect2D cvm_vk_get_screen_rectangle(void)
{
    return cvm_vk_screen_rectangle;
}

VkFormat cvm_vk_get_screen_format(void)
{
    return cvm_vk_surface_format.format;
}

uint32_t cvm_vk_get_swapchain_image_count(void)
{
    return cvm_vk_swapchain_image_count;
}

VkImageView cvm_vk_get_swapchain_image_view(uint32_t index)
{
    return cvm_vk_presenting_images[index].image_view;
}

void cvm_vk_initialise(SDL_Window * window,uint32_t min_swapchain_images,uint32_t extra_swapchain_images,bool sync_compute_required)
{
    cvm_vk_min_swapchain_images=min_swapchain_images;
    cvm_vk_extra_swapchain_images=extra_swapchain_images;

    cvm_vk_create_instance(window);
    cvm_vk_create_surface(window);
    cvm_vk_create_physical_device(sync_compute_required);
    cvm_vk_create_logical_device();
    cvm_vk_create_default_command_pools();
}

void cvm_vk_terminate(void)
{
    cvm_vk_destroy_default_command_pools();

    free(cvm_vk_acquired_images);
    free(cvm_vk_presenting_images);

    vkDestroySwapchainKHR(cvm_vk_device,cvm_vk_swapchain,NULL);
    vkDestroyDevice(cvm_vk_device,NULL);
    vkDestroySurfaceKHR(cvm_vk_instance,cvm_vk_surface,NULL);
    vkDestroyInstance(cvm_vk_instance,NULL);
}


/// MUST be called AFTER present for the frame
/// need robust input parameter(s?) to control prevention of frame acquisition (can probably assume recreation needed if game_running is still true) also allows other reasons to recreate, e.g. settings change
///      ^ this paradigm might even avoid swapchain recreation when not changing things that affect it! (e.g. 1 modules MSAA settings)
/// need a better name for this
/// rely on this func to detect swapchain resize? couldn't hurt to double check based on screen resize
void cvm_vk_prepare_for_next_frame(bool rendering_resources_invalid)
{
    if(rendering_resources_invalid)cvm_vk_rendering_resources_valid=false;

    cvm_vk_swapchain_image_acquisition_data * acquired_image = cvm_vk_acquired_images + (cvm_vk_current_acquired_image_index+cvm_vk_extra_swapchain_images+1)%cvm_vk_swapchain_image_count;
    /// next frame (+1) written to by module detached the most from presentation (+cvm_vk_extra_swapchain_images)
    /// this modulo op should be used everywhere a particular frame offset (offset from cvm_vk_current_image_acquisition_index) is needed

    if(acquired_image->image_index!=CVM_VK_INVALID_IMAGE_INDEX)
    {
        cvm_vk_swapchain_image_present_data * presenting_image = cvm_vk_presenting_images + acquired_image->image_index;

        if(presenting_image->in_flight)
        {
            CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&presenting_image->completion_fence,VK_TRUE,1000000000));
            CVM_VK_CHECK(vkResetFences(cvm_vk_device,1,&presenting_image->completion_fence));
            presenting_image->in_flight=false;///has finished
            cvm_vk_acquired_image_count--;
        }
        else
        {
            fprintf(stderr,"SHOULD BE IN FLIGHT IF IMAGE WAS ACQUIRED\n");
        }
    }

    if(cvm_vk_rendering_resources_valid)
    {
        VkResult r=vkAcquireNextImageKHR(cvm_vk_device,cvm_vk_swapchain,1000000000,acquired_image->acquire_semaphore,VK_NULL_HANDLE,&acquired_image->image_index);
        if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
        {
            puts("swapchain out of date");
            acquired_image->image_index=CVM_VK_INVALID_IMAGE_INDEX;
            cvm_vk_rendering_resources_valid=false;
        }
        else if(r!=VK_SUCCESS)
        {
            fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",r);
            acquired_image->image_index=CVM_VK_INVALID_IMAGE_INDEX;
            cvm_vk_rendering_resources_valid=false;
        }
        else
        {
            cvm_vk_acquired_image_count++;
        }
    }
    else
    {
        acquired_image->image_index=CVM_VK_INVALID_IMAGE_INDEX;
    }
}

void cvm_vk_transition_frame(void)///must be called in critical section!
{
    cvm_vk_current_acquired_image_index = (cvm_vk_current_acquired_image_index+1)%cvm_vk_swapchain_image_count;
}

void cvm_vk_present_current_frame(cvm_vk_module_graphics_block ** graphics_blocks, uint32_t graphics_block_count)
{
    cvm_vk_swapchain_image_acquisition_data * acquired_image;
    cvm_vk_swapchain_image_present_data * presenting_image;
    ///add extra ones if async/sync compute can be waited upon as well?
    VkPipelineStageFlags wait_stage_flags;
    uint32_t i;

    acquired_image=cvm_vk_acquired_images+cvm_vk_current_acquired_image_index;

    if(acquired_image->image_index==CVM_VK_INVALID_IMAGE_INDEX)
    {
        ///check frame isnt in flight?
        ///also check no module frame has graphics work?
        return;
    }

    presenting_image=cvm_vk_presenting_images+acquired_image->image_index;

    #warning need error checking that every module has work to do, also a way to exit early

    ///i do prefer design of VkSubmitInfo2KHR here instead
    VkSubmitInfo submit_info=(VkSubmitInfo) /// whether submission has associated fence depends on need for family ownership transfer, so actually submit in branched code section
    {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext=NULL,
        .waitSemaphoreCount=0,
        .pWaitSemaphores=NULL,
        .pWaitDstStageMask=&wait_stage_flags,
        .commandBufferCount=1,///always just 1
        .pCommandBuffers= NULL,///not set yet
        .signalSemaphoreCount=0,///only changes to 1 for last module, 0 otherwise
        .pSignalSemaphores=NULL
    };

    for(i=0;i<graphics_block_count;i++)
    {
        ///semaphores in between submits in same queue probably arent necessary, can likely rely on external subpass dependencies, this needs looking into
        /// ^yes this looks to be the case
        submit_info.waitSemaphoreCount=0;

        if(i==0)///wait on image acquisition
        {
            submit_info.waitSemaphoreCount=1;
            submit_info.pWaitSemaphores=&acquired_image->acquire_semaphore;

            wait_stage_flags=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;///is this correct?
        }

        ///may also want to wait on async compute? (if so, reimplement allowing multiple semaphore waits)

        submit_info.pCommandBuffers=&graphics_blocks[i]->work;

        if((i==(graphics_block_count-1))&&(cvm_vk_present_queue_family==cvm_vk_graphics_queue_family))///no queue ownership transfer required, signal present semaphore as this is the last frame
        {
            submit_info.signalSemaphoreCount=1;
            submit_info.pSignalSemaphores=&presenting_image->present_semaphore;

            CVM_VK_CHECK(vkQueueSubmit(cvm_vk_graphics_queue,1,&submit_info,presenting_image->completion_fence));
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
        submit_info.pSignalSemaphores=&presenting_image->graphics_relinquish_semaphore;
        submit_info.pCommandBuffers=&presenting_image->graphics_relinquish_command_buffer;
        CVM_VK_CHECK(vkQueueSubmit(cvm_vk_graphics_queue,1,&submit_info,VK_NULL_HANDLE));

        ///acquire
        wait_stage_flags=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit_info.waitSemaphoreCount=1;
        submit_info.pWaitSemaphores=&presenting_image->graphics_relinquish_semaphore;
        submit_info.signalSemaphoreCount=1;
        submit_info.pSignalSemaphores=&presenting_image->present_semaphore;
        submit_info.pCommandBuffers=&presenting_image->present_acquire_command_buffer;
        CVM_VK_CHECK(vkQueueSubmit(cvm_vk_present_queue,1,&submit_info,presenting_image->completion_fence));
    }

    VkPresentInfoKHR present_info=(VkPresentInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext=NULL,
        .waitSemaphoreCount=1,
        .pWaitSemaphores=&presenting_image->present_semaphore,
        .swapchainCount=1,
        .pSwapchains=&cvm_vk_swapchain,
        .pImageIndices=&acquired_image->image_index,
        .pResults=NULL
    };


    VkResult r=vkQueuePresentKHR(cvm_vk_present_queue,&present_info);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't present (out of date)");
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);

    presenting_image->in_flight=true;
}

bool cvm_vk_recreate_rendering_resources(void) /// this should be made unnecessary id change noted in cvm_vk_transition_frame is implemented
{
    return ((cvm_vk_acquired_image_count == 0)&&(!cvm_vk_rendering_resources_valid));
}

void cvm_vk_wait(void)
{
    uint32_t i;

    ///this will leave some images acquired... (and some command buffers in the pending state)

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        if(cvm_vk_presenting_images[i].in_flight)
        {
            CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&cvm_vk_presenting_images[i].completion_fence,VK_TRUE,1000000000));
            cvm_vk_presenting_images[i].in_flight=false;///has finished
            cvm_vk_acquired_image_count--;
        }
    }

    #warning do same for submits made to transfer queue
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



///unlike other functions, this one takes abstract/resultant data rather than just generic creation info
void * cvm_vk_create_buffer(VkBuffer * buffer,VkDeviceMemory * memory,VkBufferUsageFlags usage,VkDeviceSize size,bool require_host_visible)
{
    /// prefer_host_local for staging buffer that is written to dynamically? maybe in the future
    /// instead have flags for usage that are used to determine which heap to use?

    /// exclusive ownership means explicit ownership transfers are required, but this may have better performance
    /// need for these can be indicated by flags on buffer ranges (for staging)

    VkBufferCreateInfo buffer_create_info=(VkBufferCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .size=size,
        .usage=usage,
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL
    };

    CVM_VK_CHECK(vkCreateBuffer(cvm_vk_device,&buffer_create_info,NULL,buffer));

    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(cvm_vk_device,*buffer,&buffer_memory_requirements);

    uint32_t i;

    /// integrated graphics ONLY has shared heaps! (UMA) means there's no good way to test everything is working, but also means there's room for optimisation
    ///     ^ if staging is detected as not needed then don't do it? (but still need a way to test staging, circumvent_uma define? this won't really test things properly though...)
    ///     ^ also I'm pretty sure staging is needed for uploading textures

    for(i=0;i<cvm_vk_memory_properties.memoryTypeCount;i++)
    {
        if(( buffer_memory_requirements.memoryTypeBits & 1<<i ) && ( !require_host_visible || cvm_vk_memory_properties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ))
        {
            void * mapping=NULL;

            VkMemoryAllocateInfo memory_allocate_info=(VkMemoryAllocateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext=NULL,
                .allocationSize=buffer_memory_requirements.size,
                .memoryTypeIndex=i
            };

            CVM_VK_CHECK(vkAllocateMemory(cvm_vk_device,&memory_allocate_info,NULL,memory));

            CVM_VK_CHECK(vkBindBufferMemory(cvm_vk_device,*buffer,*memory,0));///offset/alignment kind of irrelevant because of 1 buffer per allocation paradigm

            if(cvm_vk_memory_properties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            {
                CVM_VK_CHECK(vkMapMemory(cvm_vk_device,*memory,0,VK_WHOLE_SIZE,0,&mapping));
            }

            return mapping;
        }
    }

    fprintf(stderr,"COULD NOT FIND APPROPRIATE MEMORY FOR ALLOCATION\n");
    exit(-1);
}

void cvm_vk_destroy_buffer(VkBuffer buffer,VkDeviceMemory memory,void * mapping)
{
    if(mapping)
    {
        vkUnmapMemory(cvm_vk_device,memory);
    }

    vkDestroyBuffer(cvm_vk_device,buffer,NULL);
    vkFreeMemory(cvm_vk_device,memory,NULL);
}

void cvm_vk_flush_buffer_memory_range(VkMappedMemoryRange * flush_range)
{
    CVM_VK_CHECK(vkFlushMappedMemoryRanges(cvm_vk_device,1,flush_range));
}

uint32_t cvm_vk_get_buffer_alignment_requirements(VkBufferUsageFlags usage)
{
    uint32_t alignment=1;

    /// need specialised functions for vertex buffers and index buffers (or leave it up to user)
    /// vertex: alignment = size largest primitive/type used in vertex inputs
    /// index: size of index type used
    /// indirect: 4

    if(usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT) && alignment < cvm_vk_device_properties.limits.optimalBufferCopyOffsetAlignment)
        alignment=cvm_vk_device_properties.limits.optimalBufferCopyOffsetAlignment;

    if(usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) && alignment < cvm_vk_device_properties.limits.minTexelBufferOffsetAlignment)
        alignment = cvm_vk_device_properties.limits.minTexelBufferOffsetAlignment;

    if(usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT && alignment < cvm_vk_device_properties.limits.minUniformBufferOffsetAlignment)
        alignment = cvm_vk_device_properties.limits.minUniformBufferOffsetAlignment;

    if(usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT && alignment < cvm_vk_device_properties.limits.minStorageBufferOffsetAlignment)
        alignment = cvm_vk_device_properties.limits.minStorageBufferOffsetAlignment;

    return alignment;
}


void cvm_vk_create_module_data(cvm_vk_module_data * module_data,bool in_separate_thread)
{
    module_data->graphics_block_count=0;
    module_data->graphics_blocks=NULL;

    module_data->graphics_block_index=0;


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
}

void cvm_vk_resize_module_graphics_data(cvm_vk_module_data * module_data,uint32_t extra_frame_count)
{
    uint32_t i;

    if(extra_frame_count>cvm_vk_extra_swapchain_images)
    {
        fprintf(stderr,"TRYING TO CREATE MODULE WITH MORE EXTRA FRAMES THAN ARE AVAILABLE IN THE SWAPCHAIN\n");
        exit(-1);
    }

    uint32_t new_block_count=(cvm_vk_swapchain_image_count-cvm_vk_extra_swapchain_images)+extra_frame_count;

    if(new_block_count==module_data->graphics_block_count)return;///no resizing necessary

    for(i=new_block_count;i<module_data->graphics_block_count;i++)///clean up unnecessary blocks
    {
        if(module_data->graphics_blocks[i].in_flight)
        {
            fprintf(stderr,"TRYING TO DESTROY module FRAME THAT IS IN FLIGHT\n");
            exit(-1);
        }

        vkFreeCommandBuffers(cvm_vk_device,module_data->transfer_pool,1,&module_data->graphics_blocks[i].work);
    }


    module_data->graphics_blocks=realloc(module_data->graphics_blocks,sizeof(cvm_vk_module_graphics_block)*new_block_count);


    for(i=module_data->graphics_block_count;i<new_block_count;i++)///create new necessary frames
    {
        module_data->graphics_blocks[i].parent_module=module_data;
        module_data->graphics_blocks[i].in_flight=false;

        module_data->graphics_blocks[i].has_work=false;

        VkSemaphoreCreateInfo semaphore_create_info=(VkSemaphoreCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext=NULL,
            .flags=0
        };

        VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext=NULL,
            .commandPool=module_data->graphics_pool,
            .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount=1
        };

        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&module_data->graphics_blocks[i].work));
    }

    module_data->graphics_block_index=0;
    module_data->graphics_block_count=new_block_count;
}

void cvm_vk_destroy_module_data(cvm_vk_module_data * module_data,bool in_separate_thread)
{
    uint32_t i;

    for(i=0;i<module_data->graphics_block_count;i++)
    {
        if(module_data->graphics_blocks[i].in_flight)
        {
            fprintf(stderr,"TRYING TO DESTROY module FRAME THAT IS IN FLIGHT\n");
            exit(-1);
        }

        vkFreeCommandBuffers(cvm_vk_device,module_data->transfer_pool,1,&module_data->graphics_blocks[i].work);
    }

    if(in_separate_thread)
    {
        vkDestroyCommandPool(cvm_vk_device,module_data->transfer_pool,NULL);

        if(cvm_vk_transfer_queue_family!=cvm_vk_graphics_queue_family)
        {
            vkDestroyCommandPool(cvm_vk_device,module_data->graphics_pool,NULL);
        }
    }

    free(module_data->graphics_blocks);
}

//VkCommandBuffer cvm_vk_begin_module_frame_transfer(cvm_vk_module_data * module_data,bool has_work)
//{
//    cvm_vk_module_frame_data * frame=module_data->frames+module_data->current_frame_index;
//
//    frame->has_transfer_work=has_work;
//
//    VkCommandBuffer command_buffer=VK_NULL_HANDLE;
//
//    if(frame->has_transfer_work)
//    {
//        VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
//        {
//            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//            .pNext=NULL,
//            .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT  necessary? in tutorial bet shouldn't be used?
//            .pInheritanceInfo=NULL
//        };
//
//        CVM_VK_CHECK(vkBeginCommandBuffer(frame->transfer_work,&command_buffer_begin_info));
//        command_buffer=frame->transfer_work;
//    }
//
//    return command_buffer;
//}

VkCommandBuffer cvm_vk_begin_module_graphics_block(cvm_vk_module_data * module_data,uint32_t frame_offset,uint32_t * swapchain_image_index)
{
    *swapchain_image_index = cvm_vk_acquired_images[(cvm_vk_current_acquired_image_index+frame_offset)%cvm_vk_swapchain_image_count].image_index;///value passed back by rendering engine

    cvm_vk_module_graphics_block * block=module_data->graphics_blocks+module_data->graphics_block_index;

    if(*swapchain_image_index != CVM_VK_INVALID_IMAGE_INDEX)///data passed in is valid and everything is up to date
    {
        if(block->in_flight)
        {
            fprintf(stderr,"TRYING TO WRITE MODULE FRAME THAT IS STILL IN FLIGHT\n");
            exit(-1);
        }

        block->has_work=true;

        VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext=NULL,
            .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT  necessary? in tutorial bet shouldn't be used?
            .pInheritanceInfo=NULL
        };


        CVM_VK_CHECK(vkBeginCommandBuffer(block->work,&command_buffer_begin_info));
        return block->work;
    }
    else
    {
        block->has_work=false;
        return VK_NULL_HANDLE;
    }
}

cvm_vk_module_graphics_block * cvm_vk_end_module_graphics_block(cvm_vk_module_data * module_data)
{
    cvm_vk_module_graphics_block * block=module_data->graphics_blocks + module_data->graphics_block_index;

    if(block->has_work)
    {
        CVM_VK_CHECK(vkEndCommandBuffer(block->work));
        module_data->graphics_block_index = (module_data->graphics_block_index+1)%module_data->graphics_block_count;
    }

    return block;
}













