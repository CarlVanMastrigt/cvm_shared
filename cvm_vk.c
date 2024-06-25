/**
Copyright 2020,2021,2022,2023 Carl van Mastrigt

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


static VkSurfaceKHR cvm_vk_surface;



static cvm_vk_device cvm_vk_;
static cvm_vk_surface_swapchain cvm_vk_surface_swapchain_;

static VkSwapchainKHR cvm_vk_swapchain=VK_NULL_HANDLE;
static VkSurfaceFormatKHR cvm_vk_surface_format;
static VkPresentModeKHR cvm_vk_surface_present_mode;


///these command pools should only ever be used to create command buffers that are created once, then submitted multiple times, and only accessed internally (only ever used within this file)
static VkCommandPool cvm_vk_internal_present_command_pool;
//static VkCommandPool cvm_vk_internal_transfer_command_pool;
//static VkCommandPool cvm_vk_internal_graphics_command_pool;


///may want to rename cvm_vk_frames, cvm_vk_presentation_instance probably isnt that bad tbh...
///realloc these only if number of swapchain image count changes (it wont really)
static VkSemaphore * cvm_vk_image_acquisition_semaphores=NULL;///number of these should match swapchain image count
static cvm_vk_swapchain_image_present_data * cvm_vk_presenting_images=NULL;
static uint32_t cvm_vk_swapchain_image_count=0;/// this is also the number of swapchain images
static uint32_t cvm_vk_current_acquired_image_index=CVM_INVALID_U32_INDEX;
static uint32_t cvm_vk_acquired_image_count=0;///both frames in flight and frames acquired by rendereer

///following used to determine number of swapchain images to allocate
static uint32_t cvm_vk_min_swapchain_images;
#warning remove above, move to swapchain?


static bool cvm_vk_rendering_resources_valid=false;///can this be determined for next frame during critical section?


///could transfer from graphics being separate from compute to semi-unified grouping of work semaphores (and similar concepts)
static cvm_vk_timeline_semaphore cvm_vk_graphics_timeline;
static cvm_vk_timeline_semaphore cvm_vk_transfer_timeline;///cycled at the same rate as graphics(?), only incremented when there is work to be done, only to be used for low priority data transfers
static cvm_vk_timeline_semaphore cvm_vk_present_timeline;
///high priority should be handled by command buffer that will use it
//static cvm_vk_timeline_semaphore * cvm_vk_asynchronous_compute_timelines;///same as number of compute queues (1 for each), needs to have a way to query the count (which will be associated with compute submission scheduling paradigm
//static cvm_vk_timeline_semaphore * cvm_vk_host_timelines;///used to wait on host events from different threads, needs to have count as input
//static uint32_t cvm_vk_host_timeline_count;

cvm_vk_device * cvm_vk_device_get(void)
{
    return &cvm_vk_;
    #warning REMOVE THIS FUNCTION
}

cvm_vk_surface_swapchain * cvm_vk_swapchain_get(void)
{
    return &cvm_vk_surface_swapchain_;
    #warning REMOVE THIS FUNCTION
}

VkInstance cvm_vk_instance_initialise(const cvm_vk_instance_setup * setup)
{
    VkInstance instance;
    uint32_t api_version,major_version,minor_version,patch_version,variant;

    CVM_VK_CHECK(vkEnumerateInstanceVersion(&api_version));

    variant=api_version>>29;
    major_version=(api_version>>22)&0x7F;
    minor_version=(api_version>>12)&0x3FF;
    patch_version=api_version&0xFFF;
    printf("Vulkan API version: %u.%u.%u - %u\n",major_version,minor_version,patch_version,variant);

    if(major_version==1 && minor_version<3) return VK_NULL_HANDLE;
    if(variant) return VK_NULL_HANDLE;

    VkApplicationInfo application_info=(VkApplicationInfo)
    {
        .sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext=NULL,
        .pApplicationName=setup->application_name,
        .applicationVersion=setup->application_version,
        .pEngineName=CVM_SHARED_ENGINE_NAME,
        .engineVersion=CVM_SHARED_ENGINE_VERSION,
        .apiVersion=api_version,
    };

    uint32_t i;
    for(i=0;i<setup->extension_count;i++)puts(setup->extension_names[i]);

    VkInstanceCreateInfo instance_creation_info=(VkInstanceCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo= &application_info,
        .enabledLayerCount=setup->layer_count,
        .ppEnabledLayerNames=setup->layer_names,
        .enabledExtensionCount=setup->extension_count,
        .ppEnabledExtensionNames=setup->extension_names,
    };

    CVM_VK_CHECK(vkCreateInstance(&instance_creation_info,NULL,&instance));

    return instance;
}

VkInstance cvm_vk_instance_initialise_for_SDL(const char * application_name,uint32_t application_version,bool validation_enabled)
{
    VkInstance instance=VK_NULL_HANDLE;
    cvm_vk_instance_setup setup;
    SDL_Window * window;
    const char * validation = "VK_LAYER_KHRONOS_validation";

    if(validation_enabled)
    {
        setup.layer_names=&validation;
        setup.layer_count=1;
    }
    else
    {
        setup.layer_count=0;
    }

    setup.application_name=application_name;
    setup.application_version=application_version;

    window = SDL_CreateWindow("",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,0,0,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);

    if(SDL_Vulkan_GetInstanceExtensions(window,&setup.extension_count,NULL))
    {
        setup.extension_names=malloc(sizeof(const char*)*setup.extension_count);
        if(SDL_Vulkan_GetInstanceExtensions(window,&setup.extension_count,setup.extension_names))
        {
            instance=cvm_vk_instance_initialise(&setup);
        }
        free(setup.extension_names);
    }

    SDL_DestroyWindow(window);

    return instance;
}

void cvm_vk_instance_terminate(VkInstance instance)
{
    vkDestroyInstance(instance,NULL);
}

#warning remove below
static void cvm_vk_create_surface_from_window(VkSurfaceKHR * surface, cvm_vk_device * vk, SDL_Window * window,VkInstance instance)
{
    uint32_t i,format_count,present_mode_count;
    SDL_bool created_surface;
    VkSurfaceFormatKHR * formats=NULL;
    VkPresentModeKHR * present_modes=NULL;
    VkBool32 surface_supported;


    created_surface=SDL_Vulkan_CreateSurface(window,instance,surface);
    assert(created_surface);///COULD NOT CREATE SDL VULKAN SURFACE


    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vk->physical_device,vk->fallback_present_queue_family_index,*surface,&surface_supported));
    assert(surface_supported==VK_TRUE);///presenting not supported on provided window


    ///select screen image format
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk->physical_device,*surface,&format_count,NULL));
    formats=malloc(sizeof(VkSurfaceFormatKHR)*format_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk->physical_device,*surface,&format_count,formats));

    printf("surface formats: %u\n",format_count);
    for(i=0;i<format_count;i++)
    {
        printf("%u %u\n",formats[i].colorSpace,formats[i].format);
    }

    cvm_vk_surface_format=formats[0];///search for preferred/fallback instead of taking first?

    free(formats);


    ///select screen present mode
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk->physical_device,*surface,&present_mode_count,NULL));
    present_modes=malloc(sizeof(VkPresentModeKHR)*present_mode_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk->physical_device,*surface,&present_mode_count,present_modes));

    printf("present modes: %u\n",present_mode_count);
    for(i=0;i<present_mode_count;i++)
    {
        if(present_modes[i]==VK_PRESENT_MODE_FIFO_KHR)break;
    }

    assert(i!=present_mode_count);///VK_PRESENT_MODE_FIFO_KHR NOT AVAILABLE (WTH)

    ///maybe want do do something meaningful here in later versions, below should be fine for now
    cvm_vk_surface_present_mode=VK_PRESENT_MODE_FIFO_KHR;///guaranteed to exist

    free(present_modes);
}


static float cvm_vk_internal_device_feature_validation_function(const VkBaseInStructure* valid_list,
                                                               const VkPhysicalDeviceProperties* device_properties,
                                                               const VkPhysicalDeviceMemoryProperties* memory_properties,
                                                               const VkExtensionProperties* extension_properties,
                                                               uint32_t extension_count,
                                                               const VkQueueFamilyProperties * queue_family_properties,
                                                               uint32_t queue_family_count)
{
    uint32_t i,extensions_found;
    const VkBaseInStructure* entry;
    const VkPhysicalDeviceVulkan12Features * features_12;
    const VkPhysicalDeviceVulkan13Features * features_13;

    #warning SHOULD actually check if presentable with some SDL window (maybe externally? and in default case) because the selected laptop adapter may alter presentability later!

    /// properties unused
    (void)device_properties;
    (void)memory_properties;
    (void)extension_properties;
    (void)extension_count;

    for(entry = valid_list;entry;entry=entry->pNext)
    {
        switch(entry->sType)
        {
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES:
                features_12 = (const VkPhysicalDeviceVulkan12Features*)entry;
                if(features_12->timelineSemaphore==VK_FALSE)return 0.0;
                break;

            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES:
                features_13 = (const VkPhysicalDeviceVulkan13Features*)entry;
                if(features_13->synchronization2==VK_FALSE)return 0.0;
                break;

            default: break;
        }
    }

    extensions_found=0;
    for(i=0;i<extension_count;i++)
    {
        if(strcmp(extension_properties[i].extensionName,"VK_KHR_swapchain")==0)
        {
            extensions_found++;
        }
    }
    if(extensions_found!=1)return 0.0;

    switch(device_properties->deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 1.0;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 0.25;
        default: return 0.0625;
    }
}

static void cvm_vk_internal_device_feature_request_function(VkBaseOutStructure* set_list,
                                                            bool* extension_set_table,
                                                            const VkBaseInStructure* valid_list,
                                                            const VkPhysicalDeviceProperties* device_properties,
                                                            const VkPhysicalDeviceMemoryProperties* memory_properties,
                                                            const VkExtensionProperties* extension_properties,
                                                            uint32_t extension_count,
                                                            const VkQueueFamilyProperties * queue_family_properties,
                                                            uint32_t queue_family_count)
{
    uint32_t i;
    VkBaseOutStructure* set_entry;
    VkPhysicalDeviceVulkan12Features * set_features_12;
    VkPhysicalDeviceVulkan13Features * features_13;

    /// properties unused
    (void)valid_list;
    (void)device_properties;
    (void)memory_properties;

    for(set_entry = set_list;set_entry;set_entry = set_entry->pNext)
    {
        switch(set_entry->sType)
        {
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES:
                set_features_12 = (VkPhysicalDeviceVulkan12Features*)set_entry;
                set_features_12->timelineSemaphore=VK_TRUE;
                break;

            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES:
                features_13 = (VkPhysicalDeviceVulkan13Features*)set_entry;
                features_13->synchronization2=VK_TRUE;
                break;

            default: break;
        }
    }

    for(i=0;i<extension_count;i++)
    {
        #warning (somehow) only do this if a surface is provided
        if(strcmp(extension_properties[i].extensionName,"VK_KHR_swapchain")==0)
        {
            extension_set_table[i]=true;
        }
    }
}


/// also removes duplicates!
static void cvm_vk_internal_device_setup_init(cvm_vk_device_setup * internal, const cvm_vk_device_setup * external)
{
    static const cvm_vk_device_setup empty_device_setup = {0};
    VkStructureType feature_struct_types[3]={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    size_t feature_struct_sizes[3]={sizeof(VkPhysicalDeviceVulkan11Features), sizeof(VkPhysicalDeviceVulkan12Features), sizeof(VkPhysicalDeviceVulkan13Features)};
    uint32_t i,j;

    if(!external)
    {
        external=&empty_device_setup;
    }

    internal->feature_validation=malloc(sizeof(cvm_vk_device_feature_validation_function*)*(external->feature_validation_count+1));
    internal->feature_validation_count=external->feature_validation_count+1;
    internal->feature_validation[0]=&cvm_vk_internal_device_feature_validation_function;
    memcpy(internal->feature_validation+1,external->feature_validation,sizeof(cvm_vk_device_feature_validation_function*)*external->feature_validation_count);

    internal->feature_request=malloc(sizeof(cvm_vk_device_feature_request_function*)*(external->feature_request_count+1));
    internal->feature_request_count=external->feature_request_count+1;
    internal->feature_request[0]=&cvm_vk_internal_device_feature_request_function;
    memcpy(internal->feature_request+1,external->feature_request,sizeof(cvm_vk_device_feature_request_function*)*external->feature_request_count);


    ///following ones shouldnt contain duplicates
    internal->device_feature_struct_types=malloc(sizeof(VkStructureType)*(external->device_feature_struct_count+3));
    internal->device_feature_struct_sizes=malloc(sizeof(size_t)*(external->device_feature_struct_count+3));
    internal->device_feature_struct_count=3;
    memcpy(internal->device_feature_struct_types, feature_struct_types, sizeof(VkStructureType)*3);
    memcpy(internal->device_feature_struct_sizes, feature_struct_sizes, sizeof(size_t)*3);

    for(i=0;i<external->device_feature_struct_count;i++)
    {
        for(j=0;j<internal->device_feature_struct_count;j++)
        {
            if(external->device_feature_struct_types[i]==internal->device_feature_struct_types[j] || external->device_feature_struct_types[i]==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2)
            {
                break;
            }
        }
        if(j==internal->device_feature_struct_count)
        {
            internal->device_feature_struct_types[j]=external->device_feature_struct_types[i];
            internal->device_feature_struct_sizes[j]=external->device_feature_struct_sizes[i];
            internal->device_feature_struct_count++;
        }
    }

    internal->desired_graphics_queues = CVM_CLAMP(external->desired_graphics_queues,1,8);
    internal->desired_transfer_queues = CVM_CLAMP(external->desired_transfer_queues,1,8);
    internal->desired_async_compute_queues = CVM_CLAMP(external->desired_async_compute_queues,1,8);

    internal->instance=external->instance;
}

static void cvm_vk_internal_device_setup_destroy(cvm_vk_device_setup * setup)
{
    free(setup->feature_validation);
    free(setup->feature_request);
    free(setup->device_feature_struct_types);
    free(setup->device_feature_struct_sizes);
}

static VkPhysicalDeviceFeatures2* cvm_vk_create_device_feature_structure_list(const cvm_vk_device_setup * device_setup)
{
    uint32_t i;
    VkBaseInStructure * last_feature_struct;
    VkBaseInStructure * feature_struct;
    VkPhysicalDeviceFeatures2 * features;

    /// set up chain of features, used to ensure
    /// init to zero, which is false for when this will be setting up only required features
    features = calloc(1,sizeof(VkPhysicalDeviceFeatures2));
    features->sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features->pNext=NULL;

    last_feature_struct=(VkBaseInStructure*)features;

    for(i=0;i<device_setup->device_feature_struct_count;i++)
    {
        /// init to zero, which is false for when this will be setting up only required features
        feature_struct = calloc(1,device_setup->device_feature_struct_sizes[i]);
        feature_struct->sType = device_setup->device_feature_struct_types[i];
        feature_struct->pNext = NULL;

        last_feature_struct->pNext = feature_struct;
        last_feature_struct = feature_struct;
    }

    return features;
}

static void cvm_vk_destroy_device_feature_structure_list(VkPhysicalDeviceFeatures2 * features)
{
    VkBaseInStructure * s_next;
    VkBaseInStructure * s;

    for(s=(VkBaseInStructure*)features; s; s=s_next)
    {
        s_next=(VkBaseInStructure*)s->pNext;
        free(s);
    }
}

#warning make device selection process float based (preference based) with 0 representing unusable
static float cvm_vk_test_physical_device_capabilities(VkPhysicalDevice physical_device_to_test, const cvm_vk_device_setup * device_setup)
{
    uint32_t i,queue_family_count,extension_count;
    VkQueueFamilyProperties * queue_family_properties;
    VkBool32 surface_supported;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceFeatures2 * features;
    VkExtensionProperties * extensions;
    float score = 1.0;


    vkGetPhysicalDeviceProperties(physical_device_to_test, &properties);
    vkGetPhysicalDeviceMemoryProperties(physical_device_to_test,&memory_properties);

    features = cvm_vk_create_device_feature_structure_list(device_setup);
    vkGetPhysicalDeviceFeatures2(physical_device_to_test,features);

    vkEnumerateDeviceExtensionProperties(physical_device_to_test,NULL,&extension_count,NULL);
    extensions=malloc(sizeof(VkExtensionProperties)*extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device_to_test,NULL,&extension_count,extensions);

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_to_test,&queue_family_count,NULL);
    queue_family_properties=malloc(sizeof(VkQueueFamilyProperties)*queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_to_test,&queue_family_count,queue_family_properties);

    /// external feature requirements
    for(i=0;i<device_setup->feature_validation_count;i++)
    {
        #warning pass in queue_family_satisfactory_bits and queue families!
        score *= device_setup->feature_validation[i]((VkBaseInStructure*)features, &properties, &memory_properties, extensions, extension_count, queue_family_properties, queue_family_count);
    }

    cvm_vk_destroy_device_feature_structure_list(features);
    free(extensions);
    free(queue_family_properties);

    printf("testing GPU : %s : %f\n",properties.deviceName,score);

    return score;
}


/// returns the available device features and extensions
static VkPhysicalDevice cvm_vk_create_physical_device(VkInstance vk_instance, const cvm_vk_device_setup * device_setup)
{
    ///pick best physical device with all required features

    uint32_t i,device_count;
    VkPhysicalDevice * physical_devices;
    SDL_bool created_surface;
    float score,best_score;
    VkPhysicalDevice best_device;

    CVM_VK_CHECK(vkEnumeratePhysicalDevices(vk_instance,&device_count,NULL));
    physical_devices=malloc(sizeof(VkPhysicalDevice)*device_count);
    CVM_VK_CHECK(vkEnumeratePhysicalDevices(vk_instance,&device_count,physical_devices));

    best_score=0.0;
    for(i=0;i<device_count;i++)///check for dedicated gfx cards first
    {
        score=cvm_vk_test_physical_device_capabilities(physical_devices[i],device_setup);
        if(score > best_score)
        {
            best_device=physical_devices[i];
            best_score = score;
        }
    }

    free(physical_devices);

    /// no applicable devices meet requirements
    if(best_score==0.0) return VK_NULL_HANDLE;

    return best_device;
}

static void cvm_vk_create_logical_device(cvm_vk_device * device, const cvm_vk_device_setup * device_setup)
{
    uint32_t i,j,count,enabled_extension_count,available_extension_count;
    bool * enabled_extensions_table;
    VkExtensionProperties * enabled_extensions;
    VkPhysicalDeviceFeatures2 * enabled_features;
    VkExtensionProperties * available_extensions;
    const char ** enabled_extension_names;
    VkPhysicalDeviceFeatures2 * available_features;
    VkBool32 surface_supported;
    VkDeviceQueueCreateInfo * device_queue_creation_infos;
    VkQueueFlags queue_flags;
    bool graphics, transfer, compute, present;
    float * priorities;

    vkGetPhysicalDeviceProperties(device->physical_device, (VkPhysicalDeviceProperties*)&device->properties);
    vkGetPhysicalDeviceMemoryProperties(device->physical_device, (VkPhysicalDeviceMemoryProperties*)&device->memory_properties);

    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &device->queue_family_count, NULL);
    device->queue_family_properties=malloc(sizeof(VkQueueFamilyProperties)*device->queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &device->queue_family_count, (VkQueueFamilyProperties*)device->queue_family_properties);

    device->queue_family_queue_count=malloc(sizeof(uint32_t)*device->queue_family_count);
    for(i=0;i<device->queue_family_count;i++)
    {
        ///default
        device->queue_family_queue_count[i]=1;
    }

    available_features = cvm_vk_create_device_feature_structure_list(device_setup);
    vkGetPhysicalDeviceFeatures2(device->physical_device,available_features);

    vkEnumerateDeviceExtensionProperties(device->physical_device,NULL,&available_extension_count,NULL);
    available_extensions=malloc(sizeof(VkExtensionProperties)*available_extension_count);
    vkEnumerateDeviceExtensionProperties(device->physical_device,NULL,&available_extension_count,available_extensions);


    enabled_features = cvm_vk_create_device_feature_structure_list(device_setup);
    enabled_extensions_table=calloc(available_extension_count,sizeof(bool));

    for(i=0;i<device_setup->feature_request_count;i++)
    {
        device_setup->feature_request[i]((VkBaseOutStructure*)enabled_features,
                                         enabled_extensions_table,
                                         (VkBaseInStructure*)available_features,
                                         &device->properties,
                                         &device->memory_properties,
                                         available_extensions,
                                         available_extension_count,
                                         device->queue_family_properties,
                                         device->queue_family_count);
    }

    enabled_extension_count=0;
    for(i=0;i<available_extension_count;i++)
    {
        if(enabled_extensions_table[i])
        {
            enabled_extension_count++;
        }
    }

    enabled_extension_names=malloc(sizeof(const char*)*enabled_extension_count);
    enabled_extensions=malloc(sizeof(VkExtensionProperties)*enabled_extension_count);

    enabled_extension_count=0;
    for(i=0;i<available_extension_count;i++)
    {
        if(enabled_extensions_table[i])
        {
            enabled_extensions[enabled_extension_count]=available_extensions[i];
            enabled_extension_names[enabled_extension_count]=available_extensions[i].extensionName;
            enabled_extension_count++;
        }
    }

    device->features = enabled_features;
    device->extensions = enabled_extensions;
    device->extension_count = enabled_extension_count;


    device->graphics_queue_family_index=CVM_INVALID_U32_INDEX;
    device->transfer_queue_family_index=CVM_INVALID_U32_INDEX;
    device->fallback_present_queue_family_index=CVM_INVALID_U32_INDEX;
    device->async_compute_queue_family_index=CVM_INVALID_U32_INDEX;

    i=device->queue_family_count;
    while(i--)///assume lowest is best fit for anything in particular
    {
        queue_flags=device->queue_family_properties[i].queueFlags;
        graphics=queue_flags & VK_QUEUE_GRAPHICS_BIT;
        transfer=queue_flags & VK_QUEUE_TRANSFER_BIT;
        compute =queue_flags & VK_QUEUE_COMPUTE_BIT;

        if(graphics)
        {
            device->graphics_queue_family_index=i;
            device->queue_family_queue_count[i] = CVM_MIN(device->queue_family_properties[i].queueCount,device_setup->desired_graphics_queues);
        }

        if(transfer && !graphics && !compute)
        {
            device->transfer_queue_family_index=i;
            device->queue_family_queue_count[i] = CVM_MIN(device->queue_family_properties[i].queueCount,device_setup->desired_transfer_queues);
        }

        if(compute && !graphics)
        {
            device->async_compute_queue_family_index=i;
            device->queue_family_queue_count[i] = CVM_MIN(device->queue_family_properties[i].queueCount,device_setup->desired_async_compute_queues);
        }
    }


    #warning REMOVE THESE
//    device->fallback_present_queue_family_index = device->async_compute_queue_family_index;
    device->fallback_present_queue_family_index = device->graphics_queue_family_index;
    if(device->transfer_queue_family_index==CVM_INVALID_U32_INDEX)device->transfer_queue_family_index=device->graphics_queue_family_index;



    device_queue_creation_infos = malloc(sizeof(VkDeviceQueueCreateInfo)*device->queue_family_count);
    for(i=0;i<device->queue_family_count;i++)
    {
        count=device->queue_family_queue_count[i];
        priorities=malloc(sizeof(float)*count);

        for(j=0;j<count;j++)
        {
            priorities[j] = 1.0 - (float)j/(float)count;
        }

        device_queue_creation_infos[i]=(VkDeviceQueueCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .queueFamilyIndex=i,
            .queueCount=count,
            .pQueuePriorities=priorities,
        };
    }

    VkDeviceCreateInfo device_creation_info=(VkDeviceCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext=enabled_features,
        .flags=0,
        .queueCreateInfoCount=device->queue_family_count,
        .pQueueCreateInfos= device_queue_creation_infos,
        .enabledExtensionCount=enabled_extension_count,
        .ppEnabledExtensionNames=enabled_extension_names,
        .pEnabledFeatures=NULL,///using features2 in pNext chain instead
    };

    CVM_VK_CHECK(vkCreateDevice(device->physical_device,&device_creation_info,NULL,&device->device));

    for(i=0;i<device->queue_family_count;i++)
    {
        free((void*)device_queue_creation_infos[i].pQueuePriorities);
    }

    cvm_vk_destroy_device_feature_structure_list(available_features);
    free(available_extensions);

    free(enabled_extensions_table);
    free(enabled_extension_names);
    free(device_queue_creation_infos);
}

static void cvm_vk_destroy_logical_device(cvm_vk_device * device)
{
    vkDestroyDevice(device->device,NULL);
    cvm_vk_destroy_device_feature_structure_list((void*)device->features);
    free((void*)device->extensions);
}

static void cvm_vk_create_internal_command_pools(cvm_vk_device * device)
{
    uint32_t i;
    device->internal_command_pools=malloc(sizeof(VkCommandPool)*device->queue_family_count);
    for(i=0;i<device->queue_family_count;i++)
    {
        VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .queueFamilyIndex=i,
        };

        CVM_VK_CHECK(vkCreateCommandPool(device->device,&command_pool_create_info,NULL,device->internal_command_pools+i));
    }
}

static void cvm_vk_destroy_internal_command_pools(cvm_vk_device * device)
{
    uint32_t i;
    for(i=0;i<device->queue_family_count;i++)
    {
        vkDestroyCommandPool(device->device,device->internal_command_pools[i],NULL);
    }
    free(device->internal_command_pools);
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
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(cvm_vk_.physical_device,cvm_vk_surface,&surface_capabilities));

    printf("SD: %u %u - %u %u\n",surface_capabilities.minImageExtent.width,surface_capabilities.minImageExtent.height,surface_capabilities.maxImageExtent.width,surface_capabilities.maxImageExtent.height);
    printf("U: %x %x\n",surface_capabilities.supportedCompositeAlpha,surface_capabilities.supportedUsageFlags);

    assert(surface_capabilities.supportedUsageFlags&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    assert(surface_capabilities.supportedUsageFlags&VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    cvm_vk_swapchain_image_count=CVM_MAX(surface_capabilities.minImageCount,cvm_vk_min_swapchain_images);

    if((surface_capabilities.maxImageCount)&&(surface_capabilities.maxImageCount < cvm_vk_swapchain_image_count))
    {
        fprintf(stderr,"REQUESTED SWAPCHAIN IMAGE COUNT NOT SUPPORTED\n");
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

    CVM_VK_CHECK(vkCreateSwapchainKHR(cvm_vk_.device,&swapchain_create_info,NULL,&cvm_vk_swapchain));

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_.device,cvm_vk_swapchain,&i,NULL));

    assert(i==cvm_vk_swapchain_image_count);///SUPPOSEDLY CORRECT SWAPCHAIN COUNT WAS NOY CREATED PROPERLY

    VkImage * swapchain_images=malloc(sizeof(VkImage)*cvm_vk_swapchain_image_count);

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_.device,cvm_vk_swapchain,&cvm_vk_swapchain_image_count,swapchain_images));

    cvm_vk_current_acquired_image_index=CVM_INVALID_U32_INDEX;///no longer have a valid swapchain image

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
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_.device,&binary_semaphore_create_info,NULL,cvm_vk_image_acquisition_semaphores+i));
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_.device,&binary_semaphore_create_info,NULL,&cvm_vk_presenting_images[i].present_semaphore));

        cvm_vk_presenting_images[i].graphics_wait_value=0;
        cvm_vk_presenting_images[i].present_wait_value=0;
        cvm_vk_presenting_images[i].transfer_wait_value=0;
        cvm_vk_presenting_images[i].acquire_semaphore=VK_NULL_HANDLE;

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

        CVM_VK_CHECK(vkCreateImageView(cvm_vk_.device,&image_view_create_info,NULL,&cvm_vk_presenting_images[i].image_view));

        ///queue ownership transfer stuff (needs testing)
        if(cvm_vk_.fallback_present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
        {
            VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext=NULL,
                .commandPool=cvm_vk_internal_present_command_pool,
                .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount=1
            };

            CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_.device,&command_buffer_allocate_info,&cvm_vk_presenting_images[i].present_acquire_command_buffer));

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
                        .srcQueueFamilyIndex=cvm_vk_.graphics_queue_family_index,
                        .dstQueueFamilyIndex=cvm_vk_.fallback_present_queue_family_index,
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

            vkCmdPipelineBarrier2(cvm_vk_presenting_images[i].present_acquire_command_buffer,&present_acquire_dependencies);

            CVM_VK_CHECK(vkEndCommandBuffer(cvm_vk_presenting_images[i].present_acquire_command_buffer));
        }
    }

    free(swapchain_images);

    if(old_swapchain!=VK_NULL_HANDLE)vkDestroySwapchainKHR(cvm_vk_.device,old_swapchain,NULL);

    cvm_vk_create_swapchain_dependednt_defaults(surface_capabilities.currentExtent.width,surface_capabilities.currentExtent.height);

    cvm_vk_rendering_resources_valid=true;
}

void cvm_vk_destroy_swapchain(void)
{
    vkDeviceWaitIdle(cvm_vk_.device);
    /// probably not the best place to put this, but so it goes, needed to ensure present workload has actually completed (and thus any batches referencing the present semaphore have completed before deleting it)
    /// could probably just call queue wait idle on graphics and present queues?

    uint32_t i;

    cvm_vk_destroy_swapchain_dependednt_defaults();

    assert(!cvm_vk_acquired_image_count);///MUST WAIT TILL ALL IMAGES ARE RETURNED BEFORE TERMINATING SWAPCHAIN

    for(i=0;i<cvm_vk_swapchain_image_count+1;i++)
    {
        vkDestroySemaphore(cvm_vk_.device,cvm_vk_image_acquisition_semaphores[i],NULL);
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        vkDestroySemaphore(cvm_vk_.device,cvm_vk_presenting_images[i].present_semaphore,NULL);

        /// swapchain frames
        vkDestroyImageView(cvm_vk_.device,cvm_vk_presenting_images[i].image_view,NULL);

        ///queue ownership transfer stuff
        if(cvm_vk_.fallback_present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
        {
            vkFreeCommandBuffers(cvm_vk_.device,cvm_vk_internal_present_command_pool,1,&cvm_vk_presenting_images[i].present_acquire_command_buffer);
        }
    }
}



static void cvm_vk_create_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore)
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

    CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_.device,&timeline_semaphore_create_info,NULL,&timeline_semaphore->semaphore));
    timeline_semaphore->value=0;
}

static inline VkSemaphoreSubmitInfo cvm_vk_create_timeline_semaphore_signal_submit_info(cvm_vk_timeline_semaphore * ts,VkPipelineStageFlags2 stages)
{
    return (VkSemaphoreSubmitInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext=NULL,
        .semaphore=ts->semaphore,
        .value= ++ts->value,
        .stageMask=stages,
        .deviceIndex=0
    };
}

static inline VkSemaphoreSubmitInfo cvm_vk_create_timeline_semaphore_wait_submit_info(cvm_vk_timeline_semaphore * ts,VkPipelineStageFlags2 stages)
{
    return (VkSemaphoreSubmitInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext=NULL,
        .semaphore=ts->semaphore,
        .value=ts->value,
        .stageMask=stages,
        .deviceIndex=0
    };
}

static inline VkSemaphoreSubmitInfo cvm_vk_create_binary_semaphore_submit_info(VkSemaphore semaphore,VkPipelineStageFlags2 stages)
{
    return (VkSemaphoreSubmitInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext=NULL,
        .semaphore=semaphore,
        .value=0,///not needed, is binary
        .stageMask=stages,
        .deviceIndex=0
    };
}

void cvm_vk_wait_on_timeline_semaphore(cvm_vk_timeline_semaphore * timeline_semaphore,uint64_t value,uint64_t timeout_ns)
{
    VkSemaphoreWaitInfo wait=
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext=NULL,
        .flags=0,
        .semaphoreCount=1,
        .pSemaphores=&timeline_semaphore->semaphore,
        .pValues=&value
    };
    CVM_VK_CHECK(vkWaitSemaphores(cvm_vk_.device,&wait,timeout_ns));
}

static void cvm_vk_create_timeline_semaphores(void)
{
    cvm_vk_create_timeline_semaphore(&cvm_vk_graphics_timeline);
    cvm_vk_create_timeline_semaphore(&cvm_vk_transfer_timeline);
    if(cvm_vk_.fallback_present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        cvm_vk_create_timeline_semaphore(&cvm_vk_present_timeline);
    }
}

static void cvm_vk_destroy_timeline_semaphores(void)
{
    vkDestroySemaphore(cvm_vk_.device,cvm_vk_graphics_timeline.semaphore,NULL);
    vkDestroySemaphore(cvm_vk_.device,cvm_vk_transfer_timeline.semaphore,NULL);
    if(cvm_vk_.fallback_present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        vkDestroySemaphore(cvm_vk_.device,cvm_vk_present_timeline.semaphore,NULL);
    }
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



int cvm_vk_device_initialise(cvm_vk_device * device, const cvm_vk_device_setup * external_device_setup, SDL_Window * window)
{
    cvm_vk_device_setup device_setup;

    if(external_device_setup)
    {
        cvm_vk_internal_device_setup_init(&device_setup, external_device_setup);
    }
    else
    {
        #warning set using defaults
    }

    cvm_vk_min_swapchain_images=3;

    device->physical_device=cvm_vk_create_physical_device(device_setup.instance,&device_setup);
    if(device->physical_device==VK_NULL_HANDLE)return -1;

    cvm_vk_create_logical_device(device, &device_setup);

    cvm_vk_create_internal_command_pools(device);

    cvm_vk_create_surface_from_window(&cvm_vk_surface,device,window,device_setup.instance);

    device->instance=device_setup.instance;

    cvm_vk_create_transfer_chain();///make conditional on separate transfer queue?
    cvm_vk_create_timeline_semaphores();

    cvm_vk_create_defaults();

    cvm_vk_internal_device_setup_destroy(&device_setup);

    return 0;
}

void cvm_vk_device_terminate(cvm_vk_device * device)
{
    /// is the following necessary???
    /// shouldnt we have waited on all things using this device before we terminate it?
    vkDeviceWaitIdle(device->device);


    cvm_vk_destroy_defaults();

    cvm_vk_destroy_timeline_semaphores();

    cvm_vk_destroy_transfer_chain();///make conditional on separate transfer queue?
    cvm_vk_destroy_internal_command_pools(device);

    free(cvm_vk_image_acquisition_semaphores);
    free(cvm_vk_presenting_images);

    vkDestroySwapchainKHR(device->device,cvm_vk_swapchain,NULL);
    vkDestroySurfaceKHR(device->instance,cvm_vk_surface,NULL);

    cvm_vk_destroy_logical_device(device);
}











static inline int cvm_vk_swapchain_create(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device)
{
    uint32_t i,j,old_swapchain_image_count,fallback_present_queue_family;
    VkBool32 surface_supported;
    VkSwapchainKHR old_swapchain;

    old_swapchain_image_count=swapchain->image_count;
    old_swapchain=swapchain->swapchain;


    swapchain->queue_family_support_mask=0;
    for(i=device->queue_family_count;i--;)
    {
        CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device,i,swapchain->surface,&surface_supported));
        if(surface_supported)
        {
            fallback_present_queue_family=i;
            swapchain->queue_family_support_mask|=1<<i;
        }
    }
    if(swapchain->queue_family_support_mask==0) return -1;///cannot present to this surface!

    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device,swapchain->surface,&swapchain->surface_capabilities));

    if(swapchain->surface_capabilities.supportedUsageFlags&swapchain->usage_flags!=swapchain->usage_flags) return -1;///intened usage not supported
    if((swapchain->surface_capabilities.maxImageCount != 0)&&(swapchain->surface_capabilities.maxImageCount < swapchain->min_image_count)) return -1;///cannot suppirt minimum image count
    if(swapchain->surface_capabilities.supportedCompositeAlpha&VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR==0)return -1;///compositing not supported
    /// would like to support different compositing, need to extend to allow this

    VkSwapchainCreateInfoKHR swapchain_create_info=(VkSwapchainCreateInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext=NULL,
        .flags=0,
        .surface=swapchain->surface,
        .minImageCount=CVM_MAX(swapchain->surface_capabilities.minImageCount,swapchain->min_image_count),
        .imageFormat=swapchain->surface_format.format,
        .imageColorSpace=swapchain->surface_format.colorSpace,
        .imageExtent=swapchain->surface_capabilities.currentExtent,
        .imageArrayLayers=1,
        .imageUsage=swapchain->usage_flags,
        .imageSharingMode=VK_SHARING_MODE_EXCLUSIVE,///means QFOT is necessary, thus following should not be prescribed
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL,
        .preTransform=swapchain->surface_capabilities.currentTransform,
        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode=swapchain->present_mode,
        .clipped=VK_TRUE,
        .oldSwapchain=swapchain->swapchain,
    };

    CVM_VK_CHECK(vkCreateSwapchainKHR(device->device,&swapchain_create_info,NULL,&swapchain->swapchain));

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(device->device,swapchain->swapchain,&swapchain->image_count,NULL));
    VkImage * swapchain_images=malloc(sizeof(VkImage)*swapchain->image_count);
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(device->device,swapchain->swapchain,&swapchain->image_count,swapchain_images));

    swapchain->acquired_image_index=CVM_INVALID_U32_INDEX;///no longer have a valid swapchain image

    if(old_swapchain_image_count!=swapchain->image_count)
    {
        swapchain->image_acquisition_semaphores=realloc(swapchain->image_acquisition_semaphores,(swapchain->image_count+1)*sizeof(VkSemaphore));
        swapchain->presenting_images=realloc(swapchain->presenting_images,swapchain->image_count*sizeof(cvm_vk_swapchain_image_present_data));
    }

    VkSemaphoreCreateInfo binary_semaphore_create_info=(VkSemaphoreCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=NULL,
        .flags=0
    };

    for(i=0;i<swapchain->image_count+1;i++)
    {
        ///require 1 extra semaphore on acquire b/c we request the next image before the prior one has completed/been made available (and as such all images may have their acquire semaphore in use)
        CVM_VK_CHECK(vkCreateSemaphore(device->device,&binary_semaphore_create_info,NULL,swapchain->image_acquisition_semaphores+i));
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        swapchain->presenting_images[i]=(cvm_vk_swapchain_image_present_data)
        {
            .image=swapchain_images[i],
            ///.image_view,//set later
            .acquire_semaphore=VK_NULL_HANDLE,///set using one of the available image_acquisition_semaphores
            .successfully_acquired=false,
            .successfully_submitted=false,
            ///.present_semaphore,//set later
            .fallback_present_queue_family=fallback_present_queue_family,
            ///.present_acquire_command_buffers,///set later

            #warning being reliant upon graphics is a red flag, this needs to be generalised! make them per QFOT? per queue?? something else entirely?
            #warning also: why TF is transfer here ? it should be added as a dependency on the ops that use it!
            .graphics_wait_value=0,
            .present_wait_value=0,
            .transfer_wait_value=0,
        };

        /// acquired frames
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_.device,&binary_semaphore_create_info,NULL,&swapchain->presenting_images[i].present_semaphore));

        ///image view
        VkImageViewCreateInfo image_view_create_info=(VkImageViewCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .image=swapchain_images[i],
            .viewType=VK_IMAGE_VIEW_TYPE_2D,
            .format=swapchain->surface_format.format,
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
        CVM_VK_CHECK(vkCreateImageView(device->device,&image_view_create_info,NULL,&swapchain->presenting_images[i].image_view));

        swapchain->presenting_images[i].present_acquire_command_buffers=malloc(sizeof(VkCommandBuffer)*device->queue_family_count);
        for(j=0;j<device->queue_family_count;j++)
        {
            swapchain->presenting_images[i].present_acquire_command_buffers[j]=VK_NULL_HANDLE;
        }
    }

    free(swapchain_images);

    if(old_swapchain!=VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device->device,old_swapchain,NULL);
    }

    cvm_vk_create_swapchain_dependednt_defaults(swapchain->surface_capabilities.currentExtent.width,swapchain->surface_capabilities.currentExtent.height);
    #warning make these defaults part of the swapchain!?

    cvm_vk_rendering_resources_valid=true;
}

static inline void cvm_vk_swapchain_destroy(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device)
{
    #warning waiting might not be viable! (it isnt if other swapchain is in use) need to wait on all present completion semaphores (and maybe set them up?)
    vkDeviceWaitIdle(device->device);
    /// probably not the best place to put this, but so it goes, needed to ensure present workload has actually completed (and thus any batches referencing the present semaphore have completed before deleting it)
    /// could probably just call queue wait idle on graphics and present queues?
    #warning wait on present semaphores to ensure device swapchain isnt in use instead of waiting for device to be idle

    uint32_t i,j;

    cvm_vk_destroy_swapchain_dependednt_defaults();
    #warning make above defaults part of the swapchain


    assert(!cvm_vk_acquired_image_count);///MUST WAIT TILL ALL IMAGES ARE RETURNED BEFORE TERMINATING SWAPCHAIN

    for(i=0;i<cvm_vk_swapchain_image_count+1;i++)
    {
        vkDestroySemaphore(device->device,swapchain->image_acquisition_semaphores[i],NULL);
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        vkDestroySemaphore(device->device,swapchain->presenting_images[i].present_semaphore,NULL);

        /// swapchain frames
        vkDestroyImageView(device->device,swapchain->presenting_images[i].image_view,NULL);

        ///queue ownership transfer stuff
        for(j=0;j<device->queue_family_count;i++)
        {
            if(swapchain->presenting_images[i].present_acquire_command_buffers[j]!=VK_NULL_HANDLE)
            {
                vkFreeCommandBuffers(device->device,device->internal_command_pools[j],1,cvm_vk_presenting_images[i].present_acquire_command_buffers+j);
            }
        }
        free(swapchain->presenting_images[i].present_acquire_command_buffers);
    }
}




int cvm_vk_swapchain_initialse(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device, const cvm_vk_swapchain_setup * setup)
{
    uint32_t i,format_count,present_mode_count;
    SDL_bool created_surface;
    VkSurfaceFormatKHR * formats=NULL;
    VkPresentModeKHR * present_modes=NULL;
    VkSurfaceFormatKHR preferred_surface_format;

    swapchain->surface=setup->surface;
    swapchain->swapchain=VK_NULL_HANDLE;

    swapchain->min_image_count=setup->min_image_count;
    swapchain->usage_flags=setup->usage_flags;

    swapchain->image_acquisition_semaphores=NULL;
    swapchain->presenting_images=NULL;

    assert(device->queue_family_count<=64);/// cannot hold bitmask

    #warning what amongst these needs to be called AGAIN after the surface gets recreated??

    /// select screen image format
    if(setup->preferred_surface_format.format) preferred_surface_format=setup->preferred_surface_format;
    else preferred_surface_format=(VkSurfaceFormatKHR){.format=VK_FORMAT_B8G8R8A8_SRGB,.colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device,swapchain->surface,&format_count,NULL));
    formats=malloc(sizeof(VkSurfaceFormatKHR)*format_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device,swapchain->surface,&format_count,formats));

    swapchain->surface_format=formats[0];///fallback in case preferred isnt found, required to have at least 1 at this point
    for(i=0;i<format_count;i++)
    {
        if(preferred_surface_format.colorSpace==formats[i].colorSpace && preferred_surface_format.format==formats[i].format)
        {
            ///preferred format exists
            swapchain->surface_format=preferred_surface_format;
            break;
        }
    }
    free(formats);


    ///select screen present mode
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device,swapchain->surface,&present_mode_count,NULL));
    present_modes=malloc(sizeof(VkPresentModeKHR)*present_mode_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device,swapchain->surface,&present_mode_count,present_modes));

    swapchain->present_mode=VK_PRESENT_MODE_FIFO_KHR;/// this is required to be supported
    for(i=0;i<present_mode_count;i++)
    {
        if(present_modes[i]==setup->preferred_present_mode)
        {
            swapchain->present_mode=setup->preferred_present_mode;
            break;
        }
    }
    free(present_modes);

    return cvm_vk_swapchain_create(swapchain,device);
}

void cvm_vk_swapchain_terminate(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device)
{
    cvm_vk_swapchain_destroy(swapchain,device);


    vkDestroySwapchainKHR(device->device,swapchain->swapchain,NULL);
    vkDestroySurfaceKHR(device->instance,swapchain->surface,NULL);
}

int cvm_vk_swapchain_regenerate(cvm_vk_surface_swapchain * swapchain, cvm_vk_device * device)
{
    cvm_vk_swapchain_destroy(swapchain,device);
    return cvm_vk_swapchain_create(swapchain,device);
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

    uint32_t old_acquired_image_index=CVM_INVALID_U32_INDEX;
    cvm_vk_swapchain_image_present_data * presenting_image;

    if(cvm_vk_rendering_resources_valid)
    {
        VkSemaphore acquire_semaphore=cvm_vk_image_acquisition_semaphores[cvm_vk_acquired_image_count++];

        VkResult r=vkAcquireNextImageKHR(cvm_vk_.device,cvm_vk_swapchain,1000000000,acquire_semaphore,VK_NULL_HANDLE,&cvm_vk_current_acquired_image_index);

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
                assert(presenting_image->successfully_acquired);///SOMEHOW PREPARING/CLEANING UP FRAME THAT WAS SUBMITTED BUT NOT ACQUIRED
                cvm_vk_wait_on_timeline_semaphore(&cvm_vk_graphics_timeline,presenting_image->graphics_wait_value,1000000000);
                cvm_vk_wait_on_timeline_semaphore(&cvm_vk_transfer_timeline,presenting_image->transfer_wait_value,1000000000);
                if(cvm_vk_.fallback_present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
                {
                    cvm_vk_wait_on_timeline_semaphore(&cvm_vk_present_timeline,presenting_image->present_wait_value,1000000000);
                }
            }
            if(presenting_image->successfully_acquired)///if it was acquired, cleanup is in order
            {
                cvm_vk_image_acquisition_semaphores[--cvm_vk_acquired_image_count]=presenting_image->acquire_semaphore;
                old_acquired_image_index=cvm_vk_current_acquired_image_index;
            }

            presenting_image->successfully_acquired=true;
            presenting_image->successfully_submitted=false;
            presenting_image->acquire_semaphore=acquire_semaphore;

            presenting_image->graphics_wait_value=cvm_vk_graphics_timeline.value;
            presenting_image->transfer_wait_value=cvm_vk_transfer_timeline.value;
        }
        else
        {
            if(r==VK_ERROR_OUT_OF_DATE_KHR) fprintf(stderr,"acquired swapchain image out of date\n");
            else fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",r);
            cvm_vk_current_acquired_image_index=CVM_INVALID_U32_INDEX;
            cvm_vk_rendering_resources_valid=false;
            cvm_vk_image_acquisition_semaphores[--cvm_vk_acquired_image_count]=acquire_semaphore;
        }
    }
    else
    {
        cvm_vk_current_acquired_image_index=CVM_INVALID_U32_INDEX;
    }

    return old_acquired_image_index;
}

cvm_vk_timeline_semaphore cvm_vk_submit_graphics_work(cvm_vk_module_work_payload * payload,cvm_vk_payload_flags flags)
{
    VkSemaphoreSubmitInfo wait_semaphores[CVM_VK_PAYLOAD_MAX_WAITS+1];
    uint32_t wait_count=0;
    VkSemaphoreSubmitInfo signal_semaphores[2];
    uint32_t signal_count=0;
    VkSubmitInfo2 submit_info;
    VkQueue present_queue,graphics_queue;

    uint32_t i;

    cvm_vk_swapchain_image_present_data * presenting_image;

    ///check if there is actually a frame in flight...

    assert(cvm_vk_current_acquired_image_index!=CVM_INVALID_U32_INDEX);///SHOULDN'T BE SUBMITTING WORK WHEN NO VALID SWAPCHAIN IMAGE WAS ACQUIRED THIS FRAME

    presenting_image=cvm_vk_presenting_images+cvm_vk_current_acquired_image_index;


    if(flags&CVM_VK_PAYLOAD_FIRST_QUEUE_USE)
    {
        ///add initial semaphore
        wait_semaphores[wait_count++]=cvm_vk_create_binary_semaphore_submit_info(presenting_image->acquire_semaphore,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    if(flags&CVM_VK_PAYLOAD_LAST_QUEUE_USE)
    {
        ///add final semaphore and other synchronization
        if(cvm_vk_.fallback_present_queue_family_index!=cvm_vk_.graphics_queue_family_index)///requires QFOT
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
                        /// ignored by QFOT??
                        .dstStageMask=0,///no relevant stage representing present... (afaik), maybe VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT ??
                        .dstAccessMask=0,///should be 0 by spec
                        .oldLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,///colour attachment optimal? modify  renderpasses as necessary to accommodate this (must match present acquire)
                        .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        .srcQueueFamilyIndex=cvm_vk_.graphics_queue_family_index,
                        .dstQueueFamilyIndex=cvm_vk_.fallback_present_queue_family_index,
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

            vkCmdPipelineBarrier2(payload->command_buffer,&graphics_relinquish_dependencies);
        }
        else
        {
            /// presenting_image->present_semaphore triggered either here or below when CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE, this path being taken when present queue == graphics queue
            signal_semaphores[signal_count++]=cvm_vk_create_binary_semaphore_submit_info(presenting_image->present_semaphore,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        }

        signal_semaphores[signal_count++]=cvm_vk_create_timeline_semaphore_signal_submit_info(&cvm_vk_graphics_timeline,payload->signal_stages | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        presenting_image->graphics_wait_value=cvm_vk_graphics_timeline.value;
    }
    else
    {
        signal_semaphores[signal_count++]=cvm_vk_create_timeline_semaphore_signal_submit_info(&cvm_vk_graphics_timeline,payload->signal_stages);
        presenting_image->graphics_wait_value=cvm_vk_graphics_timeline.value;
    }

    for(i=0;i<payload->wait_count;i++)
    {
        wait_semaphores[wait_count++]=cvm_vk_create_timeline_semaphore_wait_submit_info(payload->waits+i,payload->wait_stages[i]);
    }

    CVM_VK_CHECK(vkEndCommandBuffer(payload->command_buffer));///must be ended here in case QFOT barrier needed to be injected!

    submit_info=(VkSubmitInfo2)
    {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext=NULL,
        .flags=0,
        .waitSemaphoreInfoCount=wait_count,
        .pWaitSemaphoreInfos=wait_semaphores,
        .commandBufferInfoCount=1,
        .pCommandBufferInfos=(VkCommandBufferSubmitInfo[1])
        {
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext=NULL,
                .commandBuffer=payload->command_buffer,
                .deviceMask=0
            }
        },
        .signalSemaphoreInfoCount=signal_count,
        .pSignalSemaphoreInfos=signal_semaphores
    };


    #warning get queue index from somewhere?
    vkGetDeviceQueue(cvm_vk_.device,cvm_vk_.graphics_queue_family_index,0,&graphics_queue);
    CVM_VK_CHECK(vkQueueSubmit2(graphics_queue,1,&submit_info,VK_NULL_HANDLE));

    /// present if last payload that uses the swapchain
    if(flags&CVM_VK_PAYLOAD_LAST_QUEUE_USE)
    {
        ///dstAccessMask member of the VkImageMemoryBarrier should be set to 0, and the dstStageMask parameter should be set to VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.

        ///acquire on presenting queue if necessary, reuse submit info
        if(cvm_vk_.fallback_present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
        {
            ///fixed count and layout of wait and signal semaphores here
            wait_semaphores[0]=cvm_vk_create_timeline_semaphore_wait_submit_info(&cvm_vk_graphics_timeline,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

            /// presenting_image->present_semaphore triggered either here or above when CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE, this path being taken when present queue != graphics queue
            signal_semaphores[0]=cvm_vk_create_binary_semaphore_submit_info(presenting_image->present_semaphore,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
            signal_semaphores[1]=cvm_vk_create_timeline_semaphore_signal_submit_info(&cvm_vk_present_timeline,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

            submit_info=(VkSubmitInfo2)
            {
                .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext=NULL,
                .flags=0,
                .waitSemaphoreInfoCount=1,///fixed number, set above
                .pWaitSemaphoreInfos=wait_semaphores,
                .commandBufferInfoCount=1,
                .pCommandBufferInfos=(VkCommandBufferSubmitInfo[1])
                {
                    {
                        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                        .pNext=NULL,
                        .commandBuffer=presenting_image->present_acquire_command_buffer,
                        .deviceMask=0
                    }
                },
                .signalSemaphoreInfoCount=2,///fixed number, set above
                .pSignalSemaphoreInfos=signal_semaphores
            };

            #warning get queue index from somewhere?
            vkGetDeviceQueue(cvm_vk_.device,cvm_vk_.fallback_present_queue_family_index,0,&present_queue);
            CVM_VK_CHECK(vkQueueSubmit2(present_queue,1,&submit_info,VK_NULL_HANDLE));

            presenting_image->present_wait_value=cvm_vk_present_timeline.value;///have independent present queue semaphore
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

        #warning get queue index from somewhere?
        vkGetDeviceQueue(cvm_vk_.device,cvm_vk_.fallback_present_queue_family_index,0,&present_queue);
        VkResult r=vkQueuePresentKHR(present_queue,&present_info);

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

    return cvm_vk_graphics_timeline;
}

bool cvm_vk_recreate_rendering_resources(void)
{
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
                cvm_vk_wait_on_timeline_semaphore(&cvm_vk_graphics_timeline,cvm_vk_presenting_images[i].graphics_wait_value,1000000000);
                cvm_vk_wait_on_timeline_semaphore(&cvm_vk_transfer_timeline,cvm_vk_presenting_images[i].transfer_wait_value,1000000000);
                if(cvm_vk_.fallback_present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
                {
                    cvm_vk_wait_on_timeline_semaphore(&cvm_vk_present_timeline,cvm_vk_presenting_images[i].present_wait_value,1000000000);
                }
            }
            cvm_vk_presenting_images[i].successfully_acquired=false;
            cvm_vk_presenting_images[i].successfully_submitted=false;
            cvm_vk_image_acquisition_semaphores[--cvm_vk_acquired_image_count]=cvm_vk_presenting_images[i].acquire_semaphore;
            *completed_frame_index=i;
            return true;
        }
        else assert(!cvm_vk_presenting_images[i].successfully_submitted);///SOMEHOW CLEANING UP FRAME THAT WAS SUBMITTED BUT NOT ACQUIRED
    }

    *completed_frame_index=CVM_INVALID_U32_INDEX;
    return false;
}


void cvm_vk_create_render_pass(VkRenderPass * render_pass,VkRenderPassCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateRenderPass(cvm_vk_.device,info,NULL,render_pass));
}

void cvm_vk_destroy_render_pass(VkRenderPass render_pass)
{
    vkDestroyRenderPass(cvm_vk_.device,render_pass,NULL);
}

void cvm_vk_create_framebuffer(VkFramebuffer * framebuffer,VkFramebufferCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateFramebuffer(cvm_vk_.device,info,NULL,framebuffer));
}

void cvm_vk_destroy_framebuffer(VkFramebuffer framebuffer)
{
    vkDestroyFramebuffer(cvm_vk_.device,framebuffer,NULL);
}


void cvm_vk_create_pipeline_layout(VkPipelineLayout * pipeline_layout,VkPipelineLayoutCreateInfo * info)
{
    CVM_VK_CHECK(vkCreatePipelineLayout(cvm_vk_.device,info,NULL,pipeline_layout));
}

void cvm_vk_destroy_pipeline_layout(VkPipelineLayout pipeline_layout)
{
    vkDestroyPipelineLayout(cvm_vk_.device,pipeline_layout,NULL);
}


void cvm_vk_create_graphics_pipeline(VkPipeline * pipeline,VkGraphicsPipelineCreateInfo * info)
{
    #warning use pipeline cache!?
    CVM_VK_CHECK(vkCreateGraphicsPipelines(cvm_vk_.device,VK_NULL_HANDLE,1,info,NULL,pipeline));
}

void cvm_vk_destroy_pipeline(VkPipeline pipeline)
{
    vkDestroyPipeline(cvm_vk_.device,pipeline,NULL);
}


///return VkPipelineShaderStageCreateInfo, but hold on to VkShaderModule (passed by ptr) for deletion at program cleanup ( that module can be kept inside the creation info! )
void cvm_vk_create_shader_stage_info(VkPipelineShaderStageCreateInfo * stage_info,const char * filename,VkShaderStageFlagBits stage)
{
    static char * entrypoint="main";

    FILE * f;
    size_t length;
    char * data_buffer;

    VkShaderModule shader_module;

    f=fopen(filename,"rb");

    assert(f || !fprintf(stderr,"COULD NOT LOAD SHADER: %s\n",filename));

    if(f)
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

        VkResult r= vkCreateShaderModule(cvm_vk_.device,&shader_module_create_info,NULL,&shader_module);
        assert(r==VK_SUCCESS || !fprintf(stderr,"ERROR CREATING SHADER MODULE FROM FILE: %s\n",filename));

        free(data_buffer);
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
    vkDestroyShaderModule(cvm_vk_.device,stage_info->module,NULL);
}



void cvm_vk_create_descriptor_set_layout(VkDescriptorSetLayout * descriptor_set_layout,VkDescriptorSetLayoutCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateDescriptorSetLayout(cvm_vk_.device,info,NULL,descriptor_set_layout));
}

void cvm_vk_destroy_descriptor_set_layout(VkDescriptorSetLayout descriptor_set_layout)
{
    vkDestroyDescriptorSetLayout(cvm_vk_.device,descriptor_set_layout,NULL);
}

void cvm_vk_create_descriptor_pool(VkDescriptorPool * descriptor_pool,VkDescriptorPoolCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateDescriptorPool(cvm_vk_.device,info,NULL,descriptor_pool));
}

void cvm_vk_destroy_descriptor_pool(VkDescriptorPool descriptor_pool)
{
    vkDestroyDescriptorPool(cvm_vk_.device,descriptor_pool,NULL);
}

void cvm_vk_allocate_descriptor_sets(VkDescriptorSet * descriptor_sets,VkDescriptorSetAllocateInfo * info)
{
    CVM_VK_CHECK(vkAllocateDescriptorSets(cvm_vk_.device,info,descriptor_sets));
}

void cvm_vk_write_descriptor_sets(VkWriteDescriptorSet * writes,uint32_t count)
{
    vkUpdateDescriptorSets(cvm_vk_.device,count,writes,0,NULL);
}

void cvm_vk_create_image(VkImage * image,VkImageCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateImage(cvm_vk_.device,info,NULL,image));
}

void cvm_vk_destroy_image(VkImage image)
{
    vkDestroyImage(cvm_vk_.device,image,NULL);
}

static bool cvm_vk_find_appropriate_memory_type(uint32_t supported_type_bits,VkMemoryPropertyFlags required_properties,uint32_t * index)
{
    uint32_t i;
    for(i=0;i<cvm_vk_.memory_properties.memoryTypeCount;i++)
    {
        if(( supported_type_bits & 1<<i ) && ((cvm_vk_.memory_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties))
        {
            *index=i;
            return true;
        }
    }
    return false;
}

void cvm_vk_create_and_bind_memory_for_images(VkDeviceMemory * memory,VkImage * images,uint32_t image_count,VkMemoryPropertyFlags required_properties,VkMemoryPropertyFlags desired_properties)
{
    VkDeviceSize offsets[image_count];
    VkDeviceSize current_offset=0;
    VkMemoryRequirements requirements;
    uint32_t i,memory_type_index,supported_type_bits=0xFFFFFFFF;
    bool type_found;

    for(i=0;i<image_count;i++)
    {
        vkGetImageMemoryRequirements(cvm_vk_.device,images[i],&requirements);
        offsets[i]=(current_offset+requirements.alignment-1)& ~(requirements.alignment-1);
        current_offset=offsets[i]+requirements.size;
        supported_type_bits&=requirements.memoryTypeBits;
    }

    assert(supported_type_bits);

    type_found=cvm_vk_find_appropriate_memory_type(supported_type_bits,desired_properties|required_properties,&memory_type_index);

    if(!type_found)
    {
        type_found=cvm_vk_find_appropriate_memory_type(supported_type_bits,required_properties,&memory_type_index);
    }

    assert(type_found);

    VkMemoryAllocateInfo memory_allocate_info=(VkMemoryAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext=NULL,
        .allocationSize=current_offset,
        .memoryTypeIndex=memory_type_index
    };

    CVM_VK_CHECK(vkAllocateMemory(cvm_vk_.device,&memory_allocate_info,NULL,memory));

    for(i=0;i<image_count;i++)
    {
        CVM_VK_CHECK(vkBindImageMemory(cvm_vk_.device,images[i],*memory,offsets[i]));
    }
}

void cvm_vk_create_image_view(VkImageView * image_view,VkImageViewCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateImageView(cvm_vk_.device,info,NULL,image_view));
}

void cvm_vk_destroy_image_view(VkImageView image_view)
{
    vkDestroyImageView(cvm_vk_.device,image_view,NULL);
}

void cvm_vk_create_sampler(VkSampler * sampler,VkSamplerCreateInfo * info)
{
    CVM_VK_CHECK(vkCreateSampler(cvm_vk_.device,info,NULL,sampler));
}

void cvm_vk_destroy_sampler(VkSampler sampler)
{
    vkDestroySampler(cvm_vk_.device,sampler,NULL);
}

void cvm_vk_free_memory(VkDeviceMemory memory)
{
    vkFreeMemory(cvm_vk_.device,memory,NULL);
}

///unlike other functions, this one takes abstract/resultant data rather than just generic creation info
void cvm_vk_create_buffer(VkBuffer * buffer,VkDeviceMemory * memory,VkBufferUsageFlags usage,VkDeviceSize size,void ** mapping,bool * mapping_coherent,VkMemoryPropertyFlags required_properties,VkMemoryPropertyFlags desired_properties)
{
    uint32_t memory_type_index,supported_type_bits;
    bool type_found;

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

    CVM_VK_CHECK(vkCreateBuffer(cvm_vk_.device,&buffer_create_info,NULL,buffer));

    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(cvm_vk_.device,*buffer,&buffer_memory_requirements);

    type_found=cvm_vk_find_appropriate_memory_type(buffer_memory_requirements.memoryTypeBits,desired_properties|required_properties,&memory_type_index);

    if(!type_found)
    {
        type_found=cvm_vk_find_appropriate_memory_type(buffer_memory_requirements.memoryTypeBits,required_properties,&memory_type_index);
    }

    assert(type_found);

    VkMemoryAllocateInfo memory_allocate_info=(VkMemoryAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext=NULL,
        .allocationSize=buffer_memory_requirements.size,
        .memoryTypeIndex=memory_type_index
    };

    CVM_VK_CHECK(vkAllocateMemory(cvm_vk_.device,&memory_allocate_info,NULL,memory));

    CVM_VK_CHECK(vkBindBufferMemory(cvm_vk_.device,*buffer,*memory,0));///offset/alignment kind of irrelevant because of 1 buffer per allocation paradigm

    if(cvm_vk_.memory_properties.memoryTypes[memory_type_index].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        CVM_VK_CHECK(vkMapMemory(cvm_vk_.device,*memory,0,VK_WHOLE_SIZE,0,mapping));

        *mapping_coherent=!!(cvm_vk_.memory_properties.memoryTypes[memory_type_index].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    else
    {
        *mapping=NULL;
        *mapping_coherent=false;
    }
}

void cvm_vk_destroy_buffer(VkBuffer buffer,VkDeviceMemory memory,void * mapping)
{
    if(mapping)
    {
        vkUnmapMemory(cvm_vk_.device,memory);
    }

    vkDestroyBuffer(cvm_vk_.device,buffer,NULL);
    vkFreeMemory(cvm_vk_.device,memory,NULL);
}

void cvm_vk_flush_buffer_memory_range(VkMappedMemoryRange * flush_range)
{
    ///is this thread safe??
    CVM_VK_CHECK(vkFlushMappedMemoryRanges(cvm_vk_.device,1,flush_range));
}

uint32_t cvm_vk_get_buffer_alignment_requirements(VkBufferUsageFlags usage)
{
    uint32_t alignment=1;

    /// need specialised functions for vertex buffers and index buffers (or leave it up to user)
    /// vertex: alignment = size largest primitive/type used in vertex inputs - need way to handle this as it isnt really specified, perhaps assume 16? perhaps rely on user to ensure this is satisfied
    /// index: size of index type used
    /// indirect: 4

    if(usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT) && alignment < cvm_vk_.properties.limits.optimalBufferCopyOffsetAlignment)
        alignment=cvm_vk_.properties.limits.optimalBufferCopyOffsetAlignment;

    if(usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) && alignment < cvm_vk_.properties.limits.minTexelBufferOffsetAlignment)
        alignment = cvm_vk_.properties.limits.minTexelBufferOffsetAlignment;

    if(usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT && alignment < cvm_vk_.properties.limits.minUniformBufferOffsetAlignment)
        alignment = cvm_vk_.properties.limits.minUniformBufferOffsetAlignment;

    if(usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT && alignment < cvm_vk_.properties.limits.minStorageBufferOffsetAlignment)
        alignment = cvm_vk_.properties.limits.minStorageBufferOffsetAlignment;

    if(alignment<cvm_vk_.properties.limits.nonCoherentAtomSize)
        alignment=cvm_vk_.properties.limits.nonCoherentAtomSize;

    return alignment;
}

bool cvm_vk_format_check_optimal_feature_support(VkFormat format,VkFormatFeatureFlags flags)
{
    VkFormatProperties prop;
    vkGetPhysicalDeviceFormatProperties(cvm_vk_.physical_device,format,&prop);
    return (prop.optimalTilingFeatures&flags)==flags;
}







static void cvm_vk_create_module_sub_batch(cvm_vk_module_sub_batch * msb)
{
    VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex=cvm_vk_.graphics_queue_family_index
    };

    CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_.device,&command_pool_create_info,NULL,&msb->graphics_pool));

    msb->graphics_scbs=NULL;
    msb->graphics_scb_space=0;
    msb->graphics_scb_count=0;
}

static void cvm_vk_destroy_module_sub_batch(cvm_vk_module_sub_batch * msb)
{
    if(msb->graphics_scb_space)
    {
        vkFreeCommandBuffers(cvm_vk_.device,msb->graphics_pool,msb->graphics_scb_space,msb->graphics_scbs);
    }

    free(msb->graphics_scbs);

    vkDestroyCommandPool(cvm_vk_.device,msb->graphics_pool,NULL);
}

static void cvm_vk_create_module_batch(cvm_vk_module_batch * mb,uint32_t sub_batch_count)
{
    uint32_t i;
    mb->sub_batches=malloc(sizeof(cvm_vk_module_sub_batch)*sub_batch_count);

    ///required to go first as 0th serves at main threads pool
    for(i=0;i<sub_batch_count;i++)
    {
        cvm_vk_create_module_sub_batch(mb->sub_batches+i);
    }

    mb->graphics_pcbs=NULL;
    mb->graphics_pcb_space=0;
    mb->graphics_pcb_count=0;

    if(cvm_vk_.transfer_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext=NULL,
            .flags= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex=cvm_vk_.transfer_queue_family_index,
        };

        CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_.device,&command_pool_create_info,NULL,&mb->transfer_pool));
    }
    else
    {
        mb->transfer_pool=mb->sub_batches[0].graphics_pool;
    }

    VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext=NULL,
        .commandPool=mb->transfer_pool,
        .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount=1
    };

    CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_.device,&command_buffer_allocate_info,&mb->transfer_cb));///only need 1
}

static void cvm_vk_destroy_module_batch(cvm_vk_module_batch * mb,uint32_t sub_batch_count)
{
    uint32_t i;
    assert(mb->graphics_pcb_space);///module was destroyed without ever being used
    if(mb->graphics_pcb_space)
    {
        ///hijack first sub batch's command pool for primary command buffers
        vkFreeCommandBuffers(cvm_vk_.device,mb->sub_batches[0].graphics_pool,mb->graphics_pcb_space,mb->graphics_pcbs);
    }

    free(mb->graphics_pcbs);

    vkFreeCommandBuffers(cvm_vk_.device,mb->transfer_pool,1,&mb->transfer_cb);

    if(cvm_vk_.transfer_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        vkDestroyCommandPool(cvm_vk_.device,mb->transfer_pool,NULL);
    }

    for(i=0;i<sub_batch_count;i++)
    {
        cvm_vk_destroy_module_sub_batch(mb->sub_batches+i);
    }

    free(mb->sub_batches);
}

void cvm_vk_resize_module_graphics_data(cvm_vk_module_data * module_data)
{
    uint32_t i;

    uint32_t new_batch_count=cvm_vk_swapchain_image_count;

    if(new_batch_count==module_data->batch_count)return;///no resizing necessary

    for(i=new_batch_count;i<module_data->batch_count;i++)///clean up unnecessary blocks
    {
        cvm_vk_destroy_module_batch(module_data->batches+i,module_data->sub_batch_count);
    }

    module_data->batches=realloc(module_data->batches,sizeof(cvm_vk_module_batch)*new_batch_count);

    for(i=module_data->batch_count;i<new_batch_count;i++)///create new necessary frames
    {
        cvm_vk_create_module_batch(module_data->batches+i,module_data->sub_batch_count);
    }

    module_data->batch_index=0;
    module_data->batch_count=new_batch_count;
}

void cvm_vk_create_module_data(cvm_vk_module_data * module_data,uint32_t sub_batch_count)
{
    uint32_t i;
    module_data->batch_count=0;
    module_data->batches=NULL;
    assert(sub_batch_count>0);///must have at least 1 for "primary" batch

    module_data->sub_batch_count=sub_batch_count;
    module_data->batch_index=0;

    for(i=0;i<CVM_VK_MAX_QUEUES;i++)
    {
        cvm_atomic_lock_create(&module_data->transfer_data[i].spinlock);
        cvm_vk_buffer_barrier_stack_ini(&module_data->transfer_data[i].acquire_barriers);
        module_data->transfer_data[i].transfer_semaphore_wait_value=0;
        module_data->transfer_data[i].wait_stages=VK_PIPELINE_STAGE_2_NONE;
        module_data->transfer_data[i].associated_queue_family_index = (i==CVM_VK_GRAPHICS_QUEUE_INDEX)?cvm_vk_.graphics_queue_family_index:cvm_vk_.graphics_queue_family_index;
        #warning NEED TO SET ABOVE COMPUTE VARIANT APPROPRIATELY! USING cvm_vk_graphics_queue_family WILL NOT DO!
    }
}

void cvm_vk_destroy_module_data(cvm_vk_module_data * module_data)
{
    uint32_t i;

    for(i=0;i<module_data->batch_count;i++)
    {
        cvm_vk_destroy_module_batch(module_data->batches+i,module_data->sub_batch_count);
    }

    free(module_data->batches);

    for(i=0;i<CVM_VK_MAX_QUEUES;i++)
    {
        cvm_vk_buffer_barrier_stack_del(&module_data->transfer_data[i].acquire_barriers);
    }
}

cvm_vk_module_batch * cvm_vk_get_module_batch(cvm_vk_module_data * module_data,uint32_t * swapchain_image_index)
{
    uint32_t i;

    *swapchain_image_index = cvm_vk_current_acquired_image_index;///value passed back by rendering engine

    cvm_vk_module_batch * batch=module_data->batches+module_data->batch_index++;
    module_data->batch_index *= module_data->batch_index < module_data->batch_count;

    if(*swapchain_image_index != CVM_INVALID_U32_INDEX)///data passed in is valid and everything is up to date
    {
        for(i=0;i<module_data->sub_batch_count;i++)
        {
            vkResetCommandPool(cvm_vk_.device,batch->sub_batches[i].graphics_pool,0);
            batch->sub_batches[i].graphics_scb_count=0;
        }

        batch->graphics_pcb_count=0;

        if(cvm_vk_.transfer_queue_family_index!=cvm_vk_.graphics_queue_family_index)
        {
            vkResetCommandPool(cvm_vk_.device,batch->transfer_pool,0);
        }

        ///reset compute pools


        ///responsible for managing QFOT acquisitions of ranges to make them available
        batch->transfer_data=module_data->transfer_data;///used by 1 batch at a time
        batch->queue_submissions_this_batch=0;

        ///for managing transfers performed this frame
        batch->has_begun_transfer=false;
        batch->has_ended_transfer=false;
        batch->transfer_affceted_queues_bitmask=0;

        return batch;
    }

    return NULL;
}

void cvm_vk_end_module_batch(cvm_vk_module_batch * batch)
{
    VkSubmitInfo2 submit_info;
    VkQueue transfer_queue;
    uint32_t i;

    if(batch && batch->has_begun_transfer)
    {
        assert(batch->transfer_affceted_queues_bitmask);
        assert(!batch->has_ended_transfer);
        batch->has_ended_transfer=true;

        CVM_VK_CHECK(vkEndCommandBuffer(batch->transfer_cb));///must be ended here in case QFOT barrier needed to be injected!

        submit_info=(VkSubmitInfo2)
        {
            .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext=NULL,
            .flags=0,
            .waitSemaphoreInfoCount=0,
            .pWaitSemaphoreInfos=NULL,
            .commandBufferInfoCount=1,
            .pCommandBufferInfos=(VkCommandBufferSubmitInfo[1])
            {
                {
                    .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                    .pNext=NULL,
                    .commandBuffer=batch->transfer_cb,
                    .deviceMask=0
                }
            },
            .signalSemaphoreInfoCount=1,
            .pSignalSemaphoreInfos=(VkSemaphoreSubmitInfo[1])
            {
                cvm_vk_create_timeline_semaphore_signal_submit_info(&cvm_vk_transfer_timeline,VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT) /// note: also increments this timeline semaphore
            }
        };

        ///need to make sure this wait is actually only performed once per QF... (not on EVERY subsequent "frame")
        for(i=0;i<CVM_VK_MAX_QUEUES;i++)///set copy wait value on appropriate queues
        {
            if(batch->transfer_affceted_queues_bitmask & 1<<i)
            {
                batch->transfer_data[i].transfer_semaphore_wait_value=cvm_vk_transfer_timeline.value;
            }
        }

        #warning get queue index from somewhere? possibly batch, give batches priorities for both tranfer and execution? could pass in priority at execution time? possibly as override to batch priority?
        vkGetDeviceQueue(cvm_vk_.device,cvm_vk_.transfer_queue_family_index,0,&transfer_queue);
        CVM_VK_CHECK(vkQueueSubmit2(transfer_queue,1,&submit_info,VK_NULL_HANDLE));

        assert(cvm_vk_current_acquired_image_index!=CVM_INVALID_U32_INDEX);///SHOULDN'T BE SUBMITTING WORK WHEN NO VALID SWAPCHAIN IMAGE WAS ACQUIRED THIS FRAME

        cvm_vk_presenting_images[cvm_vk_current_acquired_image_index].transfer_wait_value=cvm_vk_transfer_timeline.value;
    }
}

VkCommandBuffer cvm_vk_access_batch_transfer_command_buffer(cvm_vk_module_batch * batch,uint32_t affected_queue_bitbask)
{
    if(!batch->has_begun_transfer)
    {
        batch->has_begun_transfer=true;

        VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext=NULL,
            .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT necessary?
            .pInheritanceInfo=NULL
        };

        CVM_VK_CHECK(vkBeginCommandBuffer(batch->transfer_cb,&command_buffer_begin_info));
    }

    assert(!batch->has_ended_transfer);

    batch->transfer_affceted_queues_bitmask|=affected_queue_bitbask;

    return batch->transfer_cb;
}

void cvm_vk_setup_new_graphics_payload_from_batch(cvm_vk_module_work_payload * payload,cvm_vk_module_batch * batch)
{
    static const uint32_t pcb_allocation_count=4;
    uint32_t cb_index,queue_index;
    queue_transfer_synchronization_data * transfer_data;

    queue_index=CVM_VK_GRAPHICS_QUEUE_INDEX;

    cb_index=batch->graphics_pcb_count++;

    if(cb_index==batch->graphics_pcb_space)///out of command buffers (or not yet init)
    {
        batch->graphics_pcbs=realloc(batch->graphics_pcbs,sizeof(VkCommandBuffer)*(batch->graphics_pcb_space+pcb_allocation_count));

        VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext=NULL,
            .commandPool=batch->sub_batches[0].graphics_pool,
            .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount=pcb_allocation_count
        };

        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_.device,&command_buffer_allocate_info,batch->graphics_pcbs+batch->graphics_pcb_space));

        batch->graphics_pcb_space+=pcb_allocation_count;
    }

    transfer_data=batch->transfer_data+queue_index;

    payload->command_buffer=batch->graphics_pcbs[cb_index];
    payload->wait_count=0;
    payload->destination_queue=queue_index;
    payload->transfer_data=transfer_data;

    payload->signal_stages=VK_PIPELINE_STAGE_2_NONE;

    VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext=NULL,
        .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT necessary?
        .pInheritanceInfo=NULL
    };

    CVM_VK_CHECK(vkBeginCommandBuffer(payload->command_buffer,&command_buffer_begin_info));

    #warning probably want to enforce that acquisition order is the same as submission order, is easient way to ensure following condition is met
    if(cb_index==0) ///if first (for this queue/queue-family) then inject barriers (must be first SUBMITTED not just first acquired)
    {
        assert( !(batch->queue_submissions_this_batch&1u<<queue_index));

        ///shouldnt need spinlock as this should be appropriately threaded
        if(transfer_data->acquire_barriers.count)///should always be zero if no QFOT is required, (see assert below)
        {
            assert(cvm_vk_.graphics_queue_family_index!=cvm_vk_.transfer_queue_family_index);

            ///add resource acquisition side of QFOT
            VkDependencyInfo copy_aquire_dependencies=
            {
                .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext=NULL,
                .dependencyFlags=0,
                .memoryBarrierCount=0,
                .pMemoryBarriers=NULL,
                .bufferMemoryBarrierCount=transfer_data->acquire_barriers.count,
                .pBufferMemoryBarriers=transfer_data->acquire_barriers.stack,
                .imageMemoryBarrierCount=0,
                .pImageMemoryBarriers=NULL
            };
            vkCmdPipelineBarrier2(payload->command_buffer,&copy_aquire_dependencies);

            //#warning assert queue families are actually different! before performing barriers (perhaps if instead of asserting, not sure yet)

            //assert(transfer_data->associated_queue_family_index==cvm_vk_transfer_queue_family);

            ///add transfer waits as appropriate
            #warning make wait addition a function
            payload->waits[0].value=transfer_data->transfer_semaphore_wait_value;
            payload->waits[0].semaphore=cvm_vk_transfer_timeline.semaphore;
            payload->wait_stages[0]=transfer_data->wait_stages;
            payload->wait_count++;

            ///rest data now that we're done with it
            transfer_data->wait_stages=VK_PIPELINE_STAGE_2_NONE;

            cvm_vk_buffer_barrier_stack_clr(&transfer_data->acquire_barriers);
        }
    }
    else
    {
        assert(batch->queue_submissions_this_batch&1u<<queue_index);
    }
}

uint32_t cvm_vk_get_transfer_queue_family(void)
{
    return cvm_vk_.transfer_queue_family_index;
}

uint32_t cvm_vk_get_graphics_queue_family(void)
{
    return cvm_vk_.graphics_queue_family_index;
}
