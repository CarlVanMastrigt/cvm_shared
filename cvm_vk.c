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

#include "cvm_shared.h"

///should probably have null initailisation of these vars, though that does obfuscate those that really do need to be statically initialised

static VkInstance cvm_vk_instance;
static VkPhysicalDevice cvm_vk_physical_device;
static VkDevice cvm_vk_device;///"logical" device
static VkSurfaceKHR cvm_vk_surface;
static VkPhysicalDeviceMemoryProperties cvm_vk_memory_properties;
static VkPhysicalDeviceProperties cvm_vk_device_properties;
static VkPhysicalDeviceFeatures2 cvm_vk_device_features;
static VkPhysicalDeviceVulkan11Features cvm_vk_device_features_11;
static VkPhysicalDeviceVulkan12Features cvm_vk_device_features_12;
static VkPhysicalDeviceVulkan13Features cvm_vk_device_features_13;

static VkSwapchainKHR cvm_vk_swapchain=VK_NULL_HANDLE;
static VkSurfaceFormatKHR cvm_vk_surface_format;
static VkPresentModeKHR cvm_vk_surface_present_mode;

///these can be the same
static uint32_t cvm_vk_transfer_queue_family;
static uint32_t cvm_vk_graphics_queue_family;
static uint32_t cvm_vk_present_queue_family;
///only support one of each of above (allow these to be the same if above are as well?)
static VkQueue cvm_vk_transfer_queue;
static VkQueue cvm_vk_graphics_queue;
static VkQueue cvm_vk_present_queue;

///these command pools should only ever be used to create command buffers that are submitted multiple times, and only accessed internally (only ever used within this file)
static VkCommandPool cvm_vk_transfer_command_pool;
static VkCommandPool cvm_vk_graphics_command_pool;
static VkCommandPool cvm_vk_present_command_pool;


///may want to rename cvm_vk_frames, cvm_vk_presentation_instance probably isnt that bad tbh...
///realloc these only if number of swapchain image count changes (it wont really)
static VkSemaphore * cvm_vk_image_acquisition_semaphores=NULL;///number of these should match swapchain image count
static cvm_vk_swapchain_image_present_data * cvm_vk_presenting_images=NULL;
static uint32_t cvm_vk_swapchain_image_count=0;/// this is also the number of swapchain images
static uint32_t cvm_vk_current_acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;
static uint32_t cvm_vk_acquired_image_count=0;///both frames in flight and frames acquired by rendereer
//static bool cvm_vk_work_finished=true;
///following used to determine number of swapchain images to allocate
static uint32_t cvm_vk_min_swapchain_images;
static bool cvm_vk_rendering_resources_valid=false;///can this be determined for next frame during critical section?





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

    vkGetPhysicalDeviceProperties(cvm_vk_physical_device,&cvm_vk_device_properties);

    if((dedicated_gpu_required)&&(cvm_vk_device_properties.deviceType!=VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))return false;

    if(strstr(cvm_vk_device_properties.deviceName,"LLVM"))return false;///temp fix to exclude buggy LLVM implementation, need a better/more general way to avoid such "software" devices
    //if(!strstr(cvm_vk_device_properties.deviceName,"LLVM"))return false;

    printf("testing GPU : %s\n",cvm_vk_device_properties.deviceName);

    printf("standard sample locations: %d\n",cvm_vk_device_properties.limits.standardSampleLocations);

    VkPhysicalDeviceDynamicRenderingFeatures dr=
    {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext=NULL,
        .dynamicRendering=2
    };

    VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT mrstss=
    {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT,
        .pNext=&dr,
        .multisampledRenderToSingleSampled=2
    };

    VkPhysicalDeviceSynchronization2Features s2=
    {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext=&mrstss,
        .synchronization2=2
    };




    cvm_vk_device_features.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    cvm_vk_device_features.pNext=&cvm_vk_device_features_11;

    cvm_vk_device_features_11.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    cvm_vk_device_features_11.pNext=&cvm_vk_device_features_12;

    cvm_vk_device_features_12.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    cvm_vk_device_features_12.pNext=&cvm_vk_device_features_13;

    cvm_vk_device_features_13.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    cvm_vk_device_features_13.pNext=&s2;
    //cvm_vk_device_features_11.pNext=NULL;



//    vkGetPhysicalDeviceFeatures(cvm_vk_physical_device,&cvm_vk_device_features);
    vkGetPhysicalDeviceFeatures2(cvm_vk_physical_device,&cvm_vk_device_features);

    ///VkPhysicalDeviceLineRasterizationFeaturesEXT
    puts("");
    puts("EXTENSIONS");
    printf("synchronization2: %d\n",s2.synchronization2);
    printf("multisample render to single sampled: %d\n",mrstss.multisampledRenderToSingleSampled);
    printf("dynamic rendering: %d\n",dr.dynamicRendering);
    puts("");

    #warning test for required features. need comparitor struct to do this?   enable all by default for time being?
    puts("CORE");
    ///SIMPLE EXAMPLE TEST: printf("geometry shader: %d\n",features.geometryShader);
    printf("geometry shader: %d\n",cvm_vk_device_features.features.geometryShader);
    printf("sample rate shading: %d\n",cvm_vk_device_features.features.sampleRateShading);
    printf("wide lines: %d\n",cvm_vk_device_features.features.wideLines);
    printf("timeline semaphores: %d\n",cvm_vk_device_features_12.timelineSemaphore);
    puts("");

    vkGetPhysicalDeviceQueueFamilyProperties(cvm_vk_physical_device,&queue_family_count,NULL);
    queue_family_properties=malloc(sizeof(VkQueueFamilyProperties)*queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(cvm_vk_physical_device,&queue_family_count,queue_family_properties);

    cvm_vk_graphics_queue_family=queue_family_count;
    cvm_vk_transfer_queue_family=queue_family_count;
    cvm_vk_present_queue_family=queue_family_count;

    for(i=0;i<queue_family_count;i++)
    {
        printf("queue family %u available queue count: %u\n",i,queue_family_properties[i].queueCount);
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

    printf("present modes: %u\n",present_mode_count);
    for(i=0;i<present_mode_count;i++)
    {
        if(present_modes[i]==VK_PRESENT_MODE_FIFO_KHR)break;
    }

    if(i==present_mode_count)
    {
        fprintf(stderr,"VK_PRESENT_MODE_FIFO_KHR NOT AVAILABLE\n");
        exit(-1);
    }

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

static void cvm_vk_create_logical_device(const char ** requested_extensions,int requested_extension_count)
{
    const char * layer_names[]={"VK_LAYER_KHRONOS_validation"};///VK_LAYER_LUNARG_standard_validation
    float queue_priority=1.0;
    VkExtensionProperties * possible_extensions;
    uint32_t possible_extension_count,i,j,found_extensions;
    ///if implementing separate transfer queue use a lower priority?
    /// should also probably try putting transfers on separate queue (when possible) even when they require the same queue family

    VkPhysicalDeviceFeatures features=(VkPhysicalDeviceFeatures){};
    features.geometryShader=VK_TRUE;
    features.multiDrawIndirect=VK_TRUE;
    features.sampleRateShading=VK_TRUE;
    features.fillModeNonSolid=VK_TRUE;
    features.fragmentStoresAndAtomics=VK_TRUE;
    features.wideLines=VK_TRUE;
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


    vkEnumerateDeviceExtensionProperties(cvm_vk_physical_device,NULL,&possible_extension_count,NULL);
    possible_extensions=malloc(sizeof(VkExtensionProperties)*possible_extension_count);
    vkEnumerateDeviceExtensionProperties(cvm_vk_physical_device,NULL,&possible_extension_count,possible_extensions);


    for(i=0;i<requested_extension_count && strcmp(requested_extensions[i],"VK_KHR_swapchain");i++);
    if(i==requested_extension_count)
    {
        fprintf(stderr,"REQUESTING VK_KHR_swapchain IS MANDATORY\n");
        exit(-1);
    }

    for(found_extensions=0,i=0;i<possible_extension_count && found_extensions<requested_extension_count;i++)
    {
        for(j=0;j<requested_extension_count;j++)found_extensions+=(0==strcmp(possible_extensions[i].extensionName,requested_extensions[j]));
    }
    if(i==possible_extension_count)
    {
        fprintf(stderr,"REQUESTED EXTENSION NOT FOUND\n");
        exit(-1);
    }

//    for(i=0;i<possible_extension_count;i++)
//    {
//        puts(possible_extensions[i].extensionName);
//    }

    free(possible_extensions);



    VkPhysicalDeviceSynchronization2Features s2=
    {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext=NULL,
        .synchronization2=VK_TRUE
    };

    VkPhysicalDeviceVulkan12Features features_12=
    {
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext=&s2,
//        .pNext=NULL,
        .timelineSemaphore=VK_TRUE
    };

    VkDeviceCreateInfo device_creation_info=(VkDeviceCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext=&features_12,
//        .pNext=NULL,
        .flags=0,
        .queueCreateInfoCount=queue_family_count,
        .pQueueCreateInfos= device_queue_creation_infos,
        .enabledLayerCount=1,///make 0 for release version (no validation), test performance diff
        .ppEnabledLayerNames=layer_names,
        .enabledExtensionCount=requested_extension_count,
        .ppEnabledExtensionNames=requested_extensions,
        .pEnabledFeatures= &features
    };

    CVM_VK_CHECK(vkCreateDevice(cvm_vk_physical_device,&device_creation_info,NULL,&cvm_vk_device));

    vkGetDeviceQueue(cvm_vk_device,cvm_vk_graphics_queue_family,0,&cvm_vk_graphics_queue);
    vkGetDeviceQueue(cvm_vk_device,cvm_vk_transfer_queue_family,0,&cvm_vk_transfer_queue);
    vkGetDeviceQueue(cvm_vk_device,cvm_vk_present_queue_family,0,&cvm_vk_present_queue);
}

static void cvm_vk_create_internal_command_pools(void)
{
    VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
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

static void cvm_vk_destroy_internal_command_pools(void)
{
    vkDestroyCommandPool(cvm_vk_device,cvm_vk_transfer_command_pool,NULL);
    if(cvm_vk_graphics_queue_family!=cvm_vk_transfer_queue_family) vkDestroyCommandPool(cvm_vk_device,cvm_vk_graphics_command_pool,NULL);
    if(cvm_vk_present_queue_family!=cvm_vk_transfer_queue_family) vkDestroyCommandPool(cvm_vk_device,cvm_vk_present_command_pool,NULL);
}



static void cvm_vk_create_transfer_chain(void)
{
    //
}

static void cvm_vk_destroy_transfer_chain(void)
{
    //
}

void cvm_vk_create_swapchain(void)
{
    uint32_t i;
    VkSurfaceCapabilitiesKHR surface_capabilities;

    VkSwapchainKHR old_swapchain=cvm_vk_swapchain;
    uint32_t old_swapchain_image_count=cvm_vk_swapchain_image_count;

    ///swapchain
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(cvm_vk_physical_device,cvm_vk_surface,&surface_capabilities));

    cvm_vk_swapchain_image_count=((surface_capabilities.minImageCount > cvm_vk_min_swapchain_images) ? surface_capabilities.minImageCount : cvm_vk_min_swapchain_images);

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
        .imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
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
        fprintf(stderr,"COULD NOT CREATE REQUESTED NUMBER OF SWAPCHAIN IMAGES (%u CREATED)\n",i);
        cvm_vk_swapchain_image_count=i;
        exit(-1);
    }

    VkImage * swapchain_images=malloc(sizeof(VkImage)*cvm_vk_swapchain_image_count);

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&cvm_vk_swapchain_image_count,swapchain_images));



    cvm_vk_current_acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;///no longer have a valid swapchain image

    if(old_swapchain_image_count!=cvm_vk_swapchain_image_count)
    {
        printf("swapchain image count: %u\n",cvm_vk_swapchain_image_count);
        cvm_vk_image_acquisition_semaphores=realloc(cvm_vk_image_acquisition_semaphores,(cvm_vk_swapchain_image_count+1)*sizeof(VkSemaphore));
        cvm_vk_presenting_images=realloc(cvm_vk_presenting_images,cvm_vk_swapchain_image_count*sizeof(cvm_vk_swapchain_image_present_data));
    }

    VkSemaphoreCreateInfo binary_semaphore_create_info=(VkSemaphoreCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=NULL,
        .flags=0
    };

    for(i=0;i<cvm_vk_swapchain_image_count+1;i++)
    {
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&binary_semaphore_create_info,NULL,cvm_vk_image_acquisition_semaphores+i));
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&binary_semaphore_create_info,NULL,&cvm_vk_presenting_images[i].present_semaphore));
        cvm_vk_create_timeline_semaphore(&cvm_vk_presenting_images[i].graphics_work_tracking);
        cvm_vk_create_timeline_semaphore(&cvm_vk_presenting_images[i].transfer_work_tracking);
        cvm_vk_presenting_images[i].acquire_semaphore=VK_NULL_HANDLE;

//        VkFenceCreateInfo fence_create_info=(VkFenceCreateInfo)
//        {
//            .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
//            .pNext=NULL,
//            .flags=0
//        };
//        CVM_VK_CHECK(vkCreateFence(cvm_vk_device,&fence_create_info,NULL,&cvm_vk_presenting_images[i].completion_fence));


        /// swapchain frames
        cvm_vk_presenting_images[i].successfully_acquired=false;
        cvm_vk_presenting_images[i].successfully_submitted=false;
        cvm_vk_presenting_images[i].image=swapchain_images[i];

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




        ///queue ownership transfer stuff (needs testing)
        if(cvm_vk_present_queue_family!=cvm_vk_graphics_queue_family)
        {
            VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext=NULL,
                .commandPool=cvm_vk_present_command_pool,
                .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount=1
            };

            CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&cvm_vk_presenting_images[i].present_acquire_command_buffer));

            VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext=NULL,
                .flags=0,///not one time use
                .pInheritanceInfo=NULL
            };

            VkDependencyInfo present_acquire_dependencies=
            {
                .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext=NULL,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT,
                .memoryBarrierCount=0,
                .pMemoryBarriers=NULL,
                .bufferMemoryBarrierCount=0,
                .pBufferMemoryBarriers=NULL,
                .imageMemoryBarrierCount=1,
                .pImageMemoryBarriers=(VkImageMemoryBarrier2[1])
                {
                    {
                        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .pNext=NULL,
                        .srcStageMask=0,/// from examles: no srcStage/AccessMask or dstStage/AccessMask is needed, waiting for a semaphore does that automatically.
                        .srcAccessMask=0,
                        .dstStageMask=0,
                        .dstAccessMask=0,
                        .oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,///colour attachment optimal? modify  renderpasses as necessary to accommodate this (must match graphics relinquish)
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
                    }
                }
            };


            CVM_VK_CHECK(vkBeginCommandBuffer(cvm_vk_presenting_images[i].present_acquire_command_buffer,&command_buffer_begin_info));

            vkCmdPipelineBarrier2_cvm_test(cvm_vk_presenting_images[i].present_acquire_command_buffer,&present_acquire_dependencies);

            CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_presenting_images[i].present_acquire_command_buffer));
        }
    }

    free(swapchain_images);

    if(old_swapchain!=VK_NULL_HANDLE)vkDestroySwapchainKHR(cvm_vk_device,old_swapchain,NULL);

    cvm_vk_create_swapchain_dependednt_defaults(surface_capabilities.currentExtent.width,surface_capabilities.currentExtent.height);

    cvm_vk_rendering_resources_valid=true;
}

void cvm_vk_destroy_swapchain(void)
{
    uint32_t i;

    cvm_vk_destroy_swapchain_dependednt_defaults();

    if(cvm_vk_acquired_image_count)
    {
        fprintf(stderr,"MUST WAIT TILL ALL IMAGES ARE RETURNED BEFORE TERMINATING SWAPCHAIN\n");
        exit(-1);
    }

    for(i=0;i<cvm_vk_swapchain_image_count+1;i++)
    {
        vkDestroySemaphore(cvm_vk_device,cvm_vk_image_acquisition_semaphores[i],NULL);
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        vkDestroySemaphore(cvm_vk_device,cvm_vk_presenting_images[i].present_semaphore,NULL);
        cvm_vk_destroy_timeline_semaphore(&cvm_vk_presenting_images[i].graphics_work_tracking);
        cvm_vk_destroy_timeline_semaphore(&cvm_vk_presenting_images[i].transfer_work_tracking);

        //vkDestroyFence(cvm_vk_device,cvm_vk_presenting_images[i].completion_fence,NULL);

        /// swapchain frames
        vkDestroyImageView(cvm_vk_device,cvm_vk_presenting_images[i].image_view,NULL);


        ///queue ownership transfer stuff
        if(cvm_vk_present_queue_family!=cvm_vk_graphics_queue_family)
        {
            vkFreeCommandBuffers(cvm_vk_device,cvm_vk_present_command_pool,1,&cvm_vk_presenting_images[i].present_acquire_command_buffer);
        }
    }
}


void cvm_vk_create_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore)
{
    VkSemaphoreCreateInfo timeline_semaphore_create_info=(VkSemaphoreCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=(VkSemaphoreTypeCreateInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                .pNext=NULL,
                .semaphoreType=VK_SEMAPHORE_TYPE_TIMELINE,
                .initialValue=0,
            }
        },
        .flags=0
    };

    CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&timeline_semaphore_create_info,NULL,&timeline_semaphore->semaphore));
    timeline_semaphore->value=0;
}

void cvm_vk_destroy_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore)
{
    vkDestroySemaphore(cvm_vk_device,timeline_semaphore->semaphore,NULL);
}

void cvm_vk_wait_on_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore,uint64_t timeout_ns)
{
    VkSemaphoreWaitInfo graphics_wait=
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext=NULL,
        .flags=0,
        .semaphoreCount=1,
        .pSemaphores=&timeline_semaphore->semaphore,
        .pValues=&timeline_semaphore->value
    };
    CVM_VK_CHECK(vkWaitSemaphores(cvm_vk_device,&graphics_wait,timeout_ns));
}


VkPhysicalDeviceFeatures2 * cvm_vk_get_device_features(void)
{
    return &cvm_vk_device_features;
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

VkImage cvm_vk_get_swapchain_image(uint32_t index)
{
    return cvm_vk_presenting_images[index].image;
}


void cvm_vk_initialise(SDL_Window * window,uint32_t min_swapchain_images,bool sync_compute_required,const char ** requested_extensions,int requested_extension_count)
{
    cvm_vk_min_swapchain_images=min_swapchain_images;

    cvm_vk_create_instance(window);
    cvm_vk_create_surface(window);
    cvm_vk_create_physical_device(sync_compute_required);
    cvm_vk_create_logical_device(requested_extensions,requested_extension_count);
    cvm_vk_create_internal_command_pools();
    cvm_vk_create_transfer_chain();

    cvm_vk_create_defaults();
}

void cvm_vk_terminate(void)
{
    cvm_vk_destroy_defaults();

    cvm_vk_destroy_transfer_chain();
    cvm_vk_destroy_internal_command_pools();

    free(cvm_vk_image_acquisition_semaphores);
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
/// returns newly finished frames image index so that data waiting on it can be cleaned up for threads in upcoming critical section
uint32_t cvm_vk_prepare_for_next_frame(bool rendering_resources_invalid)
{
    if(rendering_resources_invalid)cvm_vk_rendering_resources_valid=false;

    uint32_t old_acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;
    cvm_vk_swapchain_image_present_data * presenting_image;

    if(cvm_vk_rendering_resources_valid)
    {
        VkSemaphore acquire_semaphore=cvm_vk_image_acquisition_semaphores[cvm_vk_acquired_image_count++];

        VkResult r=vkAcquireNextImageKHR(cvm_vk_device,cvm_vk_swapchain,1000000000,acquire_semaphore,VK_NULL_HANDLE,&cvm_vk_current_acquired_image_index);

        if(r>=VK_SUCCESS)
        {
            if(r==VK_SUBOPTIMAL_KHR)
            {
                fprintf(stderr,"acquired swapchain image suboptimal\n");
                cvm_vk_rendering_resources_valid=false;
            }
            else if(r!=VK_SUCCESS)fprintf(stderr,"ACQUIRE NEXT IMAGE SUCCEEDED WITH RESULT : %d\n",r);

            presenting_image = cvm_vk_presenting_images + cvm_vk_current_acquired_image_index;

            if(presenting_image->successfully_submitted)
            {
                if(!presenting_image->successfully_acquired)
                {
                    fprintf(stderr,"SOMEHOW PREPARING/CLEANING UP FRAME THAT WAS SUBMITTED BUT NOT ACQUIRED\n");
                    exit(-1);
                }
                cvm_vk_wait_on_timeline_semaphore(&presenting_image->graphics_work_tracking,1000000000);
                cvm_vk_wait_on_timeline_semaphore(&presenting_image->transfer_work_tracking,1000000000);
//                CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&presenting_image->completion_fence,VK_TRUE,1000000000));
//                CVM_VK_CHECK(vkResetFences(cvm_vk_device,1,&presenting_image->completion_fence));
            }
            if(presenting_image->successfully_acquired)///if it was acquired, cleanup is in order
            {
                cvm_vk_image_acquisition_semaphores[--cvm_vk_acquired_image_count]=presenting_image->acquire_semaphore;
                old_acquired_image_index=cvm_vk_current_acquired_image_index;
            }

            presenting_image->successfully_acquired=true;
            presenting_image->successfully_submitted=false;
            presenting_image->acquire_semaphore=acquire_semaphore;
        }
        else
        {
            if(r==VK_ERROR_OUT_OF_DATE_KHR) fprintf(stderr,"acquired swapchain image out of date\n");
            else fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",r);
            cvm_vk_current_acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;
            cvm_vk_rendering_resources_valid=false;
            cvm_vk_image_acquisition_semaphores[--cvm_vk_acquired_image_count]=acquire_semaphore;
        }
    }
    else
    {
        cvm_vk_current_acquired_image_index=CVM_VK_INVALID_IMAGE_INDEX;
    }

    return old_acquired_image_index;
}

void cvm_vk_submit_graphics_work(cvm_vk_module_work_payload * payload,cvm_vk_payload_flags flags)
{
    static VkSemaphoreSubmitInfo wait_semaphores[16];
    static VkSemaphoreSubmitInfo signal_semaphores[2];
    static VkCommandBufferSubmitInfo command_buffer;
    static VkSubmitInfo2 submit_info;
    static bool initialised=false;

    uint32_t i;

    cvm_vk_swapchain_image_present_data * presenting_image;

    ///check if there is actually a frame in flight...

    if(cvm_vk_current_acquired_image_index==CVM_VK_INVALID_IMAGE_INDEX)
    {
        fprintf(stderr,"SHOULDN'T BE SUBMITTING WORK WHEN NO VALID SWAPCHAIN IMAGE WAS ACQUIRED THIS FRAME\n");
        exit(-1);
    }

    presenting_image=cvm_vk_presenting_images+cvm_vk_current_acquired_image_index;

    if(!initialised)
    {
        initialised=true;
        command_buffer=(VkCommandBufferSubmitInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext=NULL,
            .commandBuffer=VK_NULL_HANDLE,
            .deviceMask=0,
        };
        for(i=0;i<2;i++)signal_semaphores[i]=(VkSemaphoreSubmitInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext=NULL,
            .semaphore=VK_NULL_HANDLE,
            .value=0,
            .stageMask=0,
            .deviceIndex=0,
        };
        for(i=0;i<16;i++)wait_semaphores[i]=(VkSemaphoreSubmitInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext=NULL,
            .semaphore=VK_NULL_HANDLE,
            .value=0,
            .stageMask=0,
            .deviceIndex=0,
        };
        submit_info=(VkSubmitInfo2)
        {
            .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext=NULL,
            .flags=0,
            .waitSemaphoreInfoCount=0,
            .pWaitSemaphoreInfos=wait_semaphores,
            .commandBufferInfoCount=1,
            .pCommandBufferInfos=&command_buffer,
            .signalSemaphoreInfoCount=0,
            .pSignalSemaphoreInfos=signal_semaphores
        };
    }

    submit_info.waitSemaphoreInfoCount=0;
    submit_info.signalSemaphoreInfoCount=0;


    if(flags&CVM_VK_PAYLOAD_FIRST_SWAPCHAIN_USE)
    {
        //vkDeviceWaitIdle(cvm_vk_device);
        ///add initial semaphore
        wait_semaphores[submit_info.waitSemaphoreInfoCount].semaphore=presenting_image->acquire_semaphore;
        wait_semaphores[submit_info.waitSemaphoreInfoCount].value=0;///binary semaphore
        wait_semaphores[submit_info.waitSemaphoreInfoCount].stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit_info.waitSemaphoreInfoCount++;
    }

    if(flags&CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE)
    {
        ///add final semaphore and other synchronization
        if(cvm_vk_present_queue_family==cvm_vk_graphics_queue_family)
        {
            signal_semaphores[submit_info.signalSemaphoreInfoCount].semaphore=presenting_image->present_semaphore;
            signal_semaphores[submit_info.signalSemaphoreInfoCount].value=0;///binary semaphore
            signal_semaphores[submit_info.signalSemaphoreInfoCount].stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;///is this correct?
            submit_info.signalSemaphoreInfoCount++;
        }
        else ///requires QFOT
        {
            VkDependencyInfo graphics_relinquish_dependencies=
            {
                .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext=NULL,
                .dependencyFlags=VK_DEPENDENCY_BY_REGION_BIT,
                .memoryBarrierCount=0,
                .pMemoryBarriers=NULL,
                .bufferMemoryBarrierCount=0,
                .pBufferMemoryBarriers=NULL,
                .imageMemoryBarrierCount=1,
                .pImageMemoryBarriers=(VkImageMemoryBarrier2[1])
                {
                    {
                        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .pNext=NULL,
                        .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .srcAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                        .dstStageMask=0,///no relevant stage representing present... (afaik), maybe VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT ??
                        .dstAccessMask=0,///should be 0 by spec
                        .oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,///colour attachment optimal? modify  renderpasses as necessary to accommodate this (must match present acquire)
                        .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        .srcQueueFamilyIndex=cvm_vk_graphics_queue_family,
                        .dstQueueFamilyIndex=cvm_vk_present_queue_family,
                        .image=presenting_image->image,
                        .subresourceRange=(VkImageSubresourceRange)
                        {
                            .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel=0,
                            .levelCount=1,
                            .baseArrayLayer=0,
                            .layerCount=1
                        }
                    }
                }
            };

            vkCmdPipelineBarrier2_cvm_test(payload->command_buffer,&graphics_relinquish_dependencies);
        }

        ///either semaphore used to determine work has finished or one used to acquire ownership on
        signal_semaphores[submit_info.signalSemaphoreInfoCount].semaphore=presenting_image->graphics_work_tracking.semaphore;
        signal_semaphores[submit_info.signalSemaphoreInfoCount].value=++presenting_image->graphics_work_tracking.value;
        signal_semaphores[submit_info.signalSemaphoreInfoCount].stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;///is this correct?
        submit_info.signalSemaphoreInfoCount++;

        //printf("%u\n",presenting_image->graphics_work_tracking.value);
    }



    ///load payload data into submit info!
    if(submit_info.waitSemaphoreInfoCount+payload->wait_count>16)
    {
        fprintf(stderr,"TRYING TO USE TOO MANY WAITS\n");
        exit(-1);
    }

    for(i=0;i<payload->wait_count;i++)
    {
        wait_semaphores[submit_info.waitSemaphoreInfoCount].semaphore=payload->waits[i].semaphore;
        wait_semaphores[submit_info.waitSemaphoreInfoCount].value=payload->waits[i].value;
        wait_semaphores[submit_info.waitSemaphoreInfoCount].stageMask=payload->wait_stages[i];
        submit_info.waitSemaphoreInfoCount++;
    }

    if(payload->signal)
    {
        signal_semaphores[submit_info.signalSemaphoreInfoCount].semaphore=payload->signal->semaphore;
        signal_semaphores[submit_info.signalSemaphoreInfoCount].value=++payload->signal->value;
        signal_semaphores[submit_info.signalSemaphoreInfoCount].stageMask=payload->signal_stages;///is this correct?
        submit_info.signalSemaphoreInfoCount++;
    }

    command_buffer.commandBuffer=payload->command_buffer;
    CVM_VK_CHECK(vkEndCommandBuffer(payload->command_buffer));///must be ended here in case QFOT barrier needed to be injected!

//    CVM_VK_CHECK(vkQueueSubmit2_cvm_test(cvm_vk_graphics_queue,1,&submit_info,(flags&CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE)?presenting_image->completion_fence:VK_NULL_HANDLE));
    CVM_VK_CHECK(vkQueueSubmit2_cvm_test(cvm_vk_graphics_queue,1,&submit_info,VK_NULL_HANDLE));
//vkDeviceWaitIdle(cvm_vk_device);


    /// present if last payload that uses the swapchain
    if(flags&CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE)
    {
        ///acquire on presenting queue if necessary, reuse submit info
        if(cvm_vk_present_queue_family!=cvm_vk_graphics_queue_family)
        {
            ///acquire
            submit_info.waitSemaphoreInfoCount=1;

            wait_semaphores[0].semaphore=presenting_image->graphics_work_tracking.semaphore;
            wait_semaphores[0].value=presenting_image->graphics_work_tracking.value;
            wait_semaphores[0].stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;///questionable


            submit_info.signalSemaphoreInfoCount=2;

            signal_semaphores[0].semaphore=presenting_image->present_semaphore;
            signal_semaphores[0].value=0;///binary semaphore
            signal_semaphores[0].stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;/// almost certainly isnt necessary

            signal_semaphores[1].semaphore=presenting_image->graphics_work_tracking.semaphore;
            signal_semaphores[1].value=++presenting_image->graphics_work_tracking.value;
            signal_semaphores[1].stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;///is this correct?


            command_buffer.commandBuffer=presenting_image->present_acquire_command_buffer;


            CVM_VK_CHECK(vkQueueSubmit2_cvm_test(cvm_vk_present_queue,1,&submit_info,VK_NULL_HANDLE));
        }




        VkPresentInfoKHR present_info=(VkPresentInfoKHR)
        {
            .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext=NULL,
            .waitSemaphoreCount=1,
            .pWaitSemaphores=&presenting_image->present_semaphore,
            .swapchainCount=1,
            .pSwapchains=&cvm_vk_swapchain,
            .pImageIndices=&cvm_vk_current_acquired_image_index,
            .pResults=NULL
        };


        VkResult r=vkQueuePresentKHR(cvm_vk_present_queue,&present_info);

        //vkDeviceWaitIdle(cvm_vk_device);

        //printf("present %u r=%u\n",acquired_image->image_index,r);

        if(r>=VK_SUCCESS)
        {
            if(r==VK_SUBOPTIMAL_KHR)
            {
                puts("presented swapchain suboptimal");///common error
                cvm_vk_rendering_resources_valid=false;///requires resource rebuild
            }
            else if(r!=VK_SUCCESS)fprintf(stderr,"PRESENTATION SUCCEEDED WITH RESULT : %d\n",r);
            presenting_image->successfully_submitted=true;
        }
        else
        {
            if(r==VK_ERROR_OUT_OF_DATE_KHR)puts("couldn't present (out of date)");///common error
            else fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);
        }
    }
}

void cvm_vk_submit_transfer_work(cvm_vk_module_work_payload * payload)
{
    static VkSemaphoreSubmitInfo wait_semaphores[16];
    static VkSemaphoreSubmitInfo signal_semaphores[2];
    static VkCommandBufferSubmitInfo command_buffer;
    static VkSubmitInfo2 submit_info;
    static bool initialised=false;

    uint32_t i;

    if(cvm_vk_current_acquired_image_index==CVM_VK_INVALID_IMAGE_INDEX)
    {
        fprintf(stderr,"SHOULDN'T BE SUBMITTING WORK WHEN NO VALID SWAPCHAIN IMAGE WAS ACQUIRED THIS FRAME\n");
        exit(-1);
    }

    cvm_vk_swapchain_image_present_data * presenting_image;

    presenting_image=cvm_vk_presenting_images+cvm_vk_current_acquired_image_index;

    if(!initialised)
    {
        initialised=true;
        command_buffer=(VkCommandBufferSubmitInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext=NULL,
            .commandBuffer=VK_NULL_HANDLE,
            .deviceMask=0,
        };
        for(i=0;i<2;i++)signal_semaphores[i]=(VkSemaphoreSubmitInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext=NULL,
            .semaphore=VK_NULL_HANDLE,
            .value=0,
            .stageMask=0,
            .deviceIndex=0,
        };
        for(i=0;i<16;i++)wait_semaphores[i]=(VkSemaphoreSubmitInfo)
        {
            .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext=NULL,
            .semaphore=VK_NULL_HANDLE,
            .value=0,
            .stageMask=0,
            .deviceIndex=0,
        };
        submit_info=(VkSubmitInfo2)
        {
            .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext=NULL,
            .flags=0,
            .waitSemaphoreInfoCount=0,
            .pWaitSemaphoreInfos=wait_semaphores,
            .commandBufferInfoCount=1,
            .pCommandBufferInfos=&command_buffer,
            .signalSemaphoreInfoCount=0,
            .pSignalSemaphoreInfos=signal_semaphores
        };
    }

    submit_info.waitSemaphoreInfoCount=0;
    submit_info.signalSemaphoreInfoCount=0;

    ///signal that transfer work has completed for cpu synchronisation (and potentially gpu in some use cases)
    signal_semaphores[submit_info.signalSemaphoreInfoCount].semaphore=presenting_image->transfer_work_tracking.semaphore;
    signal_semaphores[submit_info.signalSemaphoreInfoCount].value=++presenting_image->transfer_work_tracking.value;
    signal_semaphores[submit_info.signalSemaphoreInfoCount].stageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT;///is this correct?
    submit_info.signalSemaphoreInfoCount++;

    ///load payload data into submit info!
    if(submit_info.waitSemaphoreInfoCount+payload->wait_count>16)
    {
        fprintf(stderr,"TRYING TO USE TOO MANY WAITS\n");
        exit(-1);
    }

    for(i=0;i<payload->wait_count;i++)
    {
        wait_semaphores[submit_info.waitSemaphoreInfoCount].semaphore=payload->waits[i].semaphore;
        wait_semaphores[submit_info.waitSemaphoreInfoCount].value=payload->waits[i].value;
        wait_semaphores[submit_info.waitSemaphoreInfoCount].stageMask=payload->wait_stages[i];
        submit_info.waitSemaphoreInfoCount++;
    }

    if(payload->signal)
    {
        signal_semaphores[submit_info.signalSemaphoreInfoCount].semaphore=payload->signal->semaphore;
        signal_semaphores[submit_info.signalSemaphoreInfoCount].value=++payload->signal->value;
        signal_semaphores[submit_info.signalSemaphoreInfoCount].stageMask=payload->signal_stages;///is this correct?
        submit_info.signalSemaphoreInfoCount++;
    }

    ///probably also want to signal per frame transfer operations

    CVM_VK_CHECK(vkEndCommandBuffer(payload->command_buffer));///must be ended here in case QFOT barrier needed to be injected!
    command_buffer.commandBuffer=payload->command_buffer;

    CVM_VK_CHECK(vkQueueSubmit2_cvm_test(cvm_vk_graphics_queue,1,&submit_info,VK_NULL_HANDLE));
}

bool cvm_vk_recreate_rendering_resources(void) /// this should be made unnecessary id change noted in cvm_vk_transition_frame is implemented
{
//    int i;
//    for(i=0;i<cvm_vk_swapchain_image_count;i++)
//    {
//        printf("img:%u if:%u\n",i,cvm_vk_presenting_images[i].in_flight);
//    }
//    printf("acquired img count:%u\n",cvm_vk_acquired_image_count);
    //return ((cvm_vk_acquired_image_count == 0)&&(!cvm_vk_rendering_resources_valid));
    return !cvm_vk_rendering_resources_valid;
}

bool cvm_vk_check_for_remaining_frames(uint32_t * completed_frame_index)
{
    uint32_t i;

    ///this will leave some images acquired... (and some command buffers in the pending state)

    if(cvm_vk_acquired_image_count)for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        if(cvm_vk_presenting_images[i].successfully_acquired)
        {
            if(cvm_vk_presenting_images[i].successfully_submitted)
            {
                cvm_vk_wait_on_timeline_semaphore(&cvm_vk_presenting_images[i].graphics_work_tracking,1000000000);
                cvm_vk_wait_on_timeline_semaphore(&cvm_vk_presenting_images[i].transfer_work_tracking,1000000000);
//                CVM_VK_CHECK(vkWaitForFences(cvm_vk_device,1,&cvm_vk_presenting_images[i].completion_fence,VK_TRUE,1000000000));
//                CVM_VK_CHECK(vkResetFences(cvm_vk_device,1,&cvm_vk_presenting_images[i].completion_fence));
            }
            cvm_vk_presenting_images[i].successfully_acquired=false;
            cvm_vk_presenting_images[i].successfully_submitted=false;
            cvm_vk_image_acquisition_semaphores[--cvm_vk_acquired_image_count]=cvm_vk_presenting_images[i].acquire_semaphore;
            *completed_frame_index=i;
            return true;
        }
        if(cvm_vk_presenting_images[i].successfully_submitted)
        {
            fprintf(stderr,"SOMEHOW CLEANING UP FRAME THAT WAS SUBMITTED BUT NOT ACQUIRED\n");
            exit(-1);
        }
    }
    #warning do same for submits made to transfer queue
    *completed_frame_index=CVM_VK_INVALID_IMAGE_INDEX;
    return false;
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



void cvm_vk_create_descriptor_set_layout(VkDescriptorSetLayout * descriptor_set_layout,VkDescriptorSetLayoutCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateDescriptorSetLayout(cvm_vk_device,info,NULL,descriptor_set_layout));
}

void cvm_vk_destroy_descriptor_set_layout(VkDescriptorSetLayout descriptor_set_layout)
{
    vkDestroyDescriptorSetLayout(cvm_vk_device,descriptor_set_layout,NULL);
}

void cvm_vk_create_descriptor_pool(VkDescriptorPool * descriptor_pool,VkDescriptorPoolCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateDescriptorPool(cvm_vk_device,info,NULL,descriptor_pool));
}

void cvm_vk_destroy_descriptor_pool(VkDescriptorPool descriptor_pool)
{
    vkDestroyDescriptorPool(cvm_vk_device,descriptor_pool,NULL);
}

void cvm_vk_allocate_descriptor_sets(VkDescriptorSet * descriptor_sets,VkDescriptorSetAllocateInfo * info)
{
    CVM_VK_CHECK(vkAllocateDescriptorSets(cvm_vk_device,info,descriptor_sets));
}

void cvm_vk_write_descriptor_sets(VkWriteDescriptorSet * writes,uint32_t count)
{
    vkUpdateDescriptorSets(cvm_vk_device,count,writes,0,NULL);
}

void cvm_vk_create_image(VkImage * image,VkImageCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateImage(cvm_vk_device,info,NULL,image));
}

void cvm_vk_destroy_image(VkImage image)
{
    vkDestroyImage(cvm_vk_device,image,NULL);
}

void cvm_vk_create_and_bind_memory_for_images(VkDeviceMemory * memory,VkImage * images,uint32_t image_count)
{
    VkDeviceSize offsets[image_count];
    VkDeviceSize current_offset=0;
    VkMemoryRequirements requirements;
    uint32_t i,supported_type_bits=0xFFFFFFFF;

    for(i=0;i<image_count;i++)
    {
        vkGetImageMemoryRequirements(cvm_vk_device,images[i],&requirements);
        offsets[i]=(current_offset+requirements.alignment-1)& ~(requirements.alignment-1);
        current_offset=offsets[i]+requirements.size;
        supported_type_bits&=requirements.memoryTypeBits;
    }

    for(i=0;i<cvm_vk_memory_properties.memoryTypeCount;i++)
    {
        if( supported_type_bits & 1<<i )
        {
            VkMemoryAllocateInfo memory_allocate_info=(VkMemoryAllocateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext=NULL,
                .allocationSize=current_offset,
                .memoryTypeIndex=i
            };

            CVM_VK_CHECK(vkAllocateMemory(cvm_vk_device,&memory_allocate_info,NULL,memory));

            break;
        }
    }

    if(i==cvm_vk_memory_properties.memoryTypeCount)
    {
        fprintf(stderr,"COULD NOT FIND APPROPRIATE MEMORY FOR IMAGE ALLOCATION\n");
        exit(-1);
    }

    for(i=0;i<image_count;i++)
    {
        CVM_VK_CHECK(vkBindImageMemory(cvm_vk_device,images[i],*memory,offsets[i]));
    }
}

void cvm_vk_create_image_view(VkImageView * image_view,VkImageViewCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateImageView(cvm_vk_device,info,NULL,image_view));
}

void cvm_vk_destroy_image_view(VkImageView image_view)
{
    vkDestroyImageView(cvm_vk_device,image_view,NULL);
}

void cvm_vk_create_sampler(VkSampler * sampler,VkSamplerCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateSampler(cvm_vk_device,info,NULL,sampler));
}

void cvm_vk_destroy_sampler(VkSampler sampler)
{
    vkDestroySampler(cvm_vk_device,sampler,NULL);
}

void cvm_vk_free_memory(VkDeviceMemory memory)
{
    vkFreeMemory(cvm_vk_device,memory,NULL);
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
                ///following if can be used to test staging on my UMA system
                if(require_host_visible)///REMOVE
                CVM_VK_CHECK(vkMapMemory(cvm_vk_device,*memory,0,VK_WHOLE_SIZE,0,&mapping));
            }

            return mapping;
        }
    }

    fprintf(stderr,"COULD NOT FIND APPROPRIATE MEMORY FOR BUFFER ALLOCATION\n");
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
    ///is this thread safe??
    CVM_VK_CHECK(vkFlushMappedMemoryRanges(cvm_vk_device,1,flush_range));
}

uint32_t cvm_vk_get_buffer_alignment_requirements(VkBufferUsageFlags usage)
{
    uint32_t alignment=1;

    /// need specialised functions for vertex buffers and index buffers (or leave it up to user)
    /// vertex: alignment = size largest primitive/type used in vertex inputs - need way to handle this as it isnt really specified, perhaps assume 16? perhaps rely on user to ensure this is satisfied
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

    if(alignment<cvm_vk_device_properties.limits.nonCoherentAtomSize)
        alignment=cvm_vk_device_properties.limits.nonCoherentAtomSize;

    return alignment;
}

bool cvm_vk_format_check_optimal_feature_support(VkFormat format,VkFormatFeatureFlags flags)
{
    VkFormatProperties prop;
    vkGetPhysicalDeviceFormatProperties(cvm_vk_physical_device,format,&prop);
    return (prop.optimalTilingFeatures&flags)==flags;
}


void cvm_vk_create_module_data(cvm_vk_module_data * module_data,uint32_t sub_batch_count,uint32_t graphics_scb_count)
{
    module_data->batch_count=0;
    module_data->batches=NULL;

    module_data->sub_batch_count=sub_batch_count;

    module_data->graphics_scb_count=graphics_scb_count;

    if(sub_batch_count && !graphics_scb_count )
    {
        fprintf(stderr,"TRYING TO CREATE MODULE WITH SUB BLOCKS BUT NO SECONDARY COMMAND BUFFERS\n");
        exit(-1);
    }

    module_data->batch_index=0;
}

static void cvm_vk_create_work_sub_batch(cvm_vk_module_data * md,cvm_vk_module_work_sub_batch * wsb)
{
    VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex=cvm_vk_graphics_queue_family
    };

    CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&wsb->graphics_pool));

    VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext=NULL,
        .commandPool=wsb->graphics_pool,
        .level=VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        .commandBufferCount=md->graphics_scb_count
    };

    if(md->graphics_scb_count)
    {
        wsb->graphics_scbs=malloc(sizeof(VkCommandBuffer)*md->graphics_scb_count);
        command_buffer_allocate_info.commandPool=wsb->graphics_pool;
        command_buffer_allocate_info.commandBufferCount=md->graphics_scb_count;
        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,wsb->graphics_scbs));
    }
    else wsb->graphics_scbs=NULL;
}

static void cvm_vk_destroy_work_sub_batch(cvm_vk_module_data * md,cvm_vk_module_work_sub_batch * wsb)
{
    if(md->graphics_scb_count)
    {
        vkFreeCommandBuffers(cvm_vk_device,wsb->graphics_pool,md->graphics_scb_count,wsb->graphics_scbs);
        free(wsb->graphics_scbs);
    }

    vkDestroyCommandPool(cvm_vk_device,wsb->graphics_pool,NULL);
}

void cvm_vk_resize_module_graphics_data(cvm_vk_module_data * module_data)
{
    uint32_t i,j;

    uint32_t new_batch_count=cvm_vk_swapchain_image_count;

    if(new_batch_count==module_data->batch_count)return;///no resizing necessary

    for(i=new_batch_count;i<module_data->batch_count;i++)///clean up unnecessary blocks
    {
        if(module_data->batches[i].in_flight)
        {
            fprintf(stderr,"TRYING TO DESTROY MODULE FRAME THAT IS IN FLIGHT\n");
            exit(-1);
        }

        vkFreeCommandBuffers(cvm_vk_device,module_data->batches[i].main_sub_batch.graphics_pool,1,&module_data->batches[i].graphics_pcb);
        vkFreeCommandBuffers(cvm_vk_device,module_data->batches[i].transfer_pool,1,&module_data->batches[i].transfer_pcb);

        cvm_vk_destroy_work_sub_batch(module_data,&module_data->batches[i].main_sub_batch);
        for(j=0;j<module_data->sub_batch_count;j++)cvm_vk_destroy_work_sub_batch(module_data,module_data->batches[i].sub_batches+j);

        free(module_data->batches[i].sub_batches);
    }


    module_data->batches=realloc(module_data->batches,sizeof(cvm_vk_module_batch)*new_batch_count);


    for(i=module_data->batch_count;i<new_batch_count;i++)///create new necessary frames
    {
        module_data->batches[i].in_flight=false;
        module_data->batches[i].sub_batches=malloc(sizeof(cvm_vk_module_work_sub_batch)*module_data->sub_batch_count);

        cvm_vk_create_work_sub_batch(module_data,&module_data->batches[i].main_sub_batch);
        for(j=0;j<module_data->sub_batch_count;j++)cvm_vk_create_work_sub_batch(module_data,module_data->batches[i].sub_batches+j);


        if(cvm_vk_transfer_queue_family!=cvm_vk_graphics_queue_family)
        {
            VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext=NULL,
                .flags= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queueFamilyIndex=cvm_vk_transfer_queue_family
            };

            CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&module_data->batches[i].transfer_pool));
        }
        else module_data->batches[i].transfer_pool=module_data->batches[i].main_sub_batch.graphics_pool;


        VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext=NULL,
            .commandPool=VK_NULL_HANDLE,///set later
            .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount=1
        };

        command_buffer_allocate_info.commandPool=module_data->batches[i].main_sub_batch.graphics_pool;
        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&module_data->batches[i].graphics_pcb));

        command_buffer_allocate_info.commandPool=module_data->batches[i].transfer_pool;
        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&module_data->batches[i].transfer_pcb));
    }

    module_data->batch_index=0;
    module_data->batch_count=new_batch_count;
}

void cvm_vk_destroy_module_data(cvm_vk_module_data * module_data)
{
    uint32_t i,j;

    for(i=0;i<module_data->batch_count;i++)
    {
        if(module_data->batches[i].in_flight)
        {
            fprintf(stderr,"TRYING TO DESTROY MODULE BLOCK THAT IS IN FLIGHT\n");
            exit(-1);
        }

        vkFreeCommandBuffers(cvm_vk_device,module_data->batches[i].main_sub_batch.graphics_pool,1,&module_data->batches[i].graphics_pcb);
        vkFreeCommandBuffers(cvm_vk_device,module_data->batches[i].transfer_pool,1,&module_data->batches[i].transfer_pcb);

        cvm_vk_destroy_work_sub_batch(module_data,&module_data->batches[i].main_sub_batch);
        for(j=0;j<module_data->sub_batch_count;j++)cvm_vk_destroy_work_sub_batch(module_data,module_data->batches[i].sub_batches+j);

        free(module_data->batches[i].sub_batches);
    }

    free(module_data->batches);
}

cvm_vk_module_batch * cvm_vk_get_module_batch(cvm_vk_module_data * module_data,uint32_t * swapchain_image_index)
{
    uint32_t i;

    *swapchain_image_index = cvm_vk_current_acquired_image_index;///value passed back by rendering engine

    cvm_vk_module_batch * batch=module_data->batches+module_data->batch_index++;
    module_data->batch_index *= module_data->batch_index < module_data->batch_count;

    if(*swapchain_image_index != CVM_VK_INVALID_IMAGE_INDEX)///data passed in is valid and everything is up to date
    {
        if(batch->in_flight)
        {
            fprintf(stderr,"TRYING TO WRITE MODULE FRAME THAT IS STILL IN FLIGHT\n");
            exit(-1);
        }

        vkResetCommandPool(cvm_vk_device,batch->main_sub_batch.graphics_pool,0);
        for(i=0;i<module_data->sub_batch_count;i++)vkResetCommandPool(cvm_vk_device,batch->sub_batches[i].graphics_pool,0);

        if(cvm_vk_transfer_queue_family!=cvm_vk_graphics_queue_family)
        {
            vkResetCommandPool(cvm_vk_device,batch->transfer_pool,0);
        }

        VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext=NULL,
            .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT  necessary? in tutorial bet shouldn't be used?
            .pInheritanceInfo=NULL
        };

        CVM_VK_CHECK(vkBeginCommandBuffer(batch->graphics_pcb,&command_buffer_begin_info));
        CVM_VK_CHECK(vkBeginCommandBuffer(batch->transfer_pcb,&command_buffer_begin_info));

        ///have to begin recording of secondaries individually based on their specific requirements
        /// should acquire command buffers individually from a batch
        return batch;
    }

    return NULL;
}

VkCommandBuffer cvm_vk_module_batch_start_secondary_command_buffer(cvm_vk_module_batch * mb,uint32_t sub_batch_index,uint32_t scb_index,VkFramebuffer framebuffer,VkRenderPass render_pass,uint32_t sub_pass)
{
    VkCommandBuffer scb;

    scb=(sub_batch_index==CVM_VK_MAIN_SUB_BATCH_INDEX)?mb->main_sub_batch.graphics_scbs[scb_index]:mb->sub_batches[sub_batch_index].graphics_scbs[scb_index];

    VkCommandBufferBeginInfo scb_info=(VkCommandBufferBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext=NULL,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT|VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo=(VkCommandBufferInheritanceInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
                .pNext=NULL,
                .renderPass=render_pass,
                .subpass=sub_pass,
                .framebuffer=framebuffer,
                .occlusionQueryEnable=VK_FALSE,
                .queryFlags=0,
                .pipelineStatistics=0
            }
        }
    };

    vkBeginCommandBuffer(scb,&scb_info);

    return scb;
}






uint32_t cvm_vk_get_transfer_queue_family(void)
{
    return cvm_vk_transfer_queue_family;
}

uint32_t cvm_vk_get_graphics_queue_family(void)
{
    return cvm_vk_graphics_queue_family;
}





void vkCmdPipelineBarrier2_cvm_test(VkCommandBuffer commandBuffer,const VkDependencyInfo* pDependencyInfo)
{
    vkCmdPipelineBarrier2(commandBuffer,pDependencyInfo);

//    uint32_t i;
//    if(pDependencyInfo->memoryBarrierCount)
//    {
//        exit(7);
//    }
//
//    for(i=0;i<pDependencyInfo->bufferMemoryBarrierCount;i++)
//    {
//        VkBufferMemoryBarrier bmb=
//        {
//            .sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
//            .pNext=NULL,
//            .srcAccessMask=pDependencyInfo->pBufferMemoryBarriers[i].srcAccessMask,
//            .dstAccessMask=pDependencyInfo->pBufferMemoryBarriers[i].dstAccessMask,
//            .srcQueueFamilyIndex=pDependencyInfo->pBufferMemoryBarriers[i].srcQueueFamilyIndex,
//            .dstQueueFamilyIndex=pDependencyInfo->pBufferMemoryBarriers[i].dstQueueFamilyIndex,
//            .buffer=pDependencyInfo->pBufferMemoryBarriers[i].buffer,
//            .offset=pDependencyInfo->pBufferMemoryBarriers[i].offset,
//            .size=pDependencyInfo->pBufferMemoryBarriers[i].size,
//        };
//
//        vkCmdPipelineBarrier(commandBuffer,
//                             pDependencyInfo->pBufferMemoryBarriers[i].srcStageMask,
//                             pDependencyInfo->pBufferMemoryBarriers[i].dstStageMask,
//                             pDependencyInfo->dependencyFlags,
//                             0,NULL,
//                             1,&bmb,
//                             0,NULL);
//    }
//
//    for(i=0;i<pDependencyInfo->imageMemoryBarrierCount;i++)
//    {
//        VkImageMemoryBarrier imb=
//        {
//            .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//            .pNext=NULL,
//            .srcAccessMask=pDependencyInfo->pImageMemoryBarriers[i].srcAccessMask,
//            .dstAccessMask=pDependencyInfo->pImageMemoryBarriers[i].dstAccessMask,
//            .oldLayout=pDependencyInfo->pImageMemoryBarriers[i].oldLayout,
//            .newLayout=pDependencyInfo->pImageMemoryBarriers[i].newLayout,
//            .srcQueueFamilyIndex=pDependencyInfo->pImageMemoryBarriers[i].srcQueueFamilyIndex,
//            .dstQueueFamilyIndex=pDependencyInfo->pImageMemoryBarriers[i].dstQueueFamilyIndex,
//            .image=pDependencyInfo->pImageMemoryBarriers[i].image,
//            .subresourceRange=pDependencyInfo->pImageMemoryBarriers[i].subresourceRange,
//        };
//
//        vkCmdPipelineBarrier(commandBuffer,
//                             pDependencyInfo->pImageMemoryBarriers[i].srcStageMask,
//                             pDependencyInfo->pImageMemoryBarriers[i].dstStageMask,
//                             pDependencyInfo->dependencyFlags,
//                             0,NULL,
//                             0,NULL,
//                             1,&imb);
//    }
}

VkResult vkQueueSubmit2_cvm_test(VkQueue queue,uint32_t submitCount,const VkSubmitInfo2* pSubmits,VkFence fence)
{
    return vkQueueSubmit2(queue,submitCount,pSubmits,fence);

//    VkSubmitInfo subs[4];
//    VkSemaphore w_sems[4][16];
//    VkPipelineStageFlags w_stgs[4][16];
//    VkSemaphore s_sems[4][16];
//    VkCommandBuffer cbs[4][16];
//    uint32_t i,j;
//
//
//    for(i=0;i<submitCount;i++)
//    {
//        for(j=0;j<pSubmits[i].waitSemaphoreInfoCount;j++)
//        {
//            w_sems[i][j]=pSubmits[i].pWaitSemaphoreInfos[j].semaphore;
//            w_stgs[i][j]=pSubmits[i].pWaitSemaphoreInfos[j].stageMask;
//        }
//        for(j=0;j<pSubmits[i].commandBufferInfoCount;j++)
//        {
//            cbs[i][j]=pSubmits[i].pCommandBufferInfos[j].commandBuffer;
//        }
//        for(j=0;j<pSubmits[i].signalSemaphoreInfoCount;j++)
//        {
//            s_sems[i][j]=pSubmits[i].pSignalSemaphoreInfos[j].semaphore;
//        }
//
//        subs[i]=(VkSubmitInfo)
//        {
//            .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
//            .pNext=NULL,
//            .waitSemaphoreCount=pSubmits[i].waitSemaphoreInfoCount,
//            .pWaitSemaphores=w_sems[i],
//            .pWaitDstStageMask=w_stgs[i],
//            .commandBufferCount=pSubmits[i].commandBufferInfoCount,
//            .pCommandBuffers=cbs[i],
//            .signalSemaphoreCount=pSubmits[i].signalSemaphoreInfoCount,
//            .pSignalSemaphores=s_sems[i],
//        };
//    }
//
//    return vkQueueSubmit(queue,submitCount,subs,fence);
}
