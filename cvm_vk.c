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

//static VkInstance cvm_vk_instance;
//static VkPhysicalDevice cvm_vk_physical_device;
static VkDevice cvm_vk_device;
static VkSurfaceKHR cvm_vk_surface;
//static VkPhysicalDeviceMemoryProperties cvm_vk_memory_properties;
//static VkPhysicalDeviceProperties cvm_vk_device_properties;
//static VkPhysicalDeviceFeatures2 cvm_vk_device_features;
//static VkPhysicalDeviceVulkan11Features cvm_vk_device_features_11;
//static VkPhysicalDeviceVulkan12Features cvm_vk_device_features_12;
//static VkPhysicalDeviceVulkan13Features cvm_vk_device_features_13;


static cvm_vk cvm_vk_;

static VkSwapchainKHR cvm_vk_swapchain=VK_NULL_HANDLE;
static VkSurfaceFormatKHR cvm_vk_surface_format;
static VkPresentModeKHR cvm_vk_surface_present_mode;

///these can be the same
//static uint32_t cvm_vk_transfer_queue_family;
//static uint32_t cvm_vk_graphics_queue_family;
//static uint32_t cvm_vk_present_queue_family;
//static uint32_t cvm_vk_compute_queue_family;///perhaps its actually desirable to support multiple async compute queue families? doesnt seem to be a feature on any gpus
///only support one of each of above (allow these to be the same if above are as well?)
static VkQueue cvm_vk_transfer_queue;
static VkQueue cvm_vk_graphics_queue;///doesn't seem to be any (mainstream) hardware that supports more than one graphics queue
static VkQueue cvm_vk_present_queue;


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
static bool cvm_vk_rendering_resources_valid=false;///can this be determined for next frame during critical section?


///could transfer from graphics being separate from compute to semi-unified grouping of work semaphores (and similar concepts)
static cvm_vk_timeline_semaphore cvm_vk_graphics_timeline;
static cvm_vk_timeline_semaphore cvm_vk_transfer_timeline;///cycled at the same rate as graphics(?), only incremented when there is work to be done, only to be used for low priority data transfers
static cvm_vk_timeline_semaphore cvm_vk_present_timeline;
///high priority should be handled by command buffer that will use it
//static cvm_vk_timeline_semaphore * cvm_vk_asynchronous_compute_timelines;///same as number of compute queues (1 for each), needs to have a way to query the count (which will be associated with compute submission scheduling paradigm
//static cvm_vk_timeline_semaphore * cvm_vk_host_timelines;///used to wait on host events from different threads, needs to have count as input
//static uint32_t cvm_vk_host_timeline_count;

static cvm_vk cvm_vk_base;
static cvm_vk_surface_swapchain cvm_vk_surface_swapchain_base;

#warning make more of this custom
static void cvm_vk_create_instance(VkInstance * instance,SDL_Window * window, const char * application_name)
{
    uint32_t extension_count;
    uint32_t api_version;
    const char ** extension_names;
    const char * layer_names[]={"VK_LAYER_KHRONOS_validation"};///VK_LAYER_LUNARG_standard_validation

    CVM_VK_CHECK(vkEnumerateInstanceVersion(&api_version));

    printf("Vulkan API version: %u.%u.%u - %u\n",(api_version>>22)&0x7F,(api_version>>12)&0x3FF,api_version&0xFFF,api_version>>29);
    SDL_bool r;

    r=SDL_Vulkan_GetInstanceExtensions(window,&extension_count,NULL);
    assert(r);///COULD NOT GET SDL WINDOW EXTENSION COUNT

    extension_names=malloc(sizeof(const char*)*extension_count);
    r=SDL_Vulkan_GetInstanceExtensions(window,&extension_count,extension_names);
    assert(r);///COULD NOT GET SDL WINDOW EXTENSIONS


    VkApplicationInfo application_info=(VkApplicationInfo)
    {
        .sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext=NULL,
        .pApplicationName=application_name,
        .applicationVersion=0,
        .pEngineName=CVM_SHARED_ENGINE_NAME,
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

    CVM_VK_CHECK(vkCreateInstance(&instance_creation_info,NULL,instance));

    free(extension_names);
}

static void cvm_vk_create_surface_from_window(VkSurfaceKHR * surface, cvm_vk * vk, SDL_Window * window)
{
    uint32_t i,format_count,present_mode_count;
    SDL_bool created_surface;
    VkSurfaceFormatKHR * formats=NULL;
    VkPresentModeKHR * present_modes=NULL;
    VkBool32 surface_supported;


    created_surface=SDL_Vulkan_CreateSurface(window,vk->instance,surface);
    assert(created_surface);///COULD NOT CREATE SDL VULKAN SURFACE


    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vk->physical_device,vk->present_queue_family_index,*surface,&surface_supported));
    assert(surface_supported==VK_TRUE);///presenting not supported on provided window


    ///select screen image format
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk->physical_device,*surface,&format_count,NULL));
    formats=malloc(sizeof(VkSurfaceFormatKHR)*format_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk->physical_device,*surface,&format_count,formats));

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


static bool cvm_vk_internal_device_feature_validation_function(const VkBaseInStructure* valid_list,
                                                               const VkPhysicalDeviceProperties* device_properties,
                                                               const VkPhysicalDeviceMemoryProperties* memory_properties,
                                                               const VkExtensionProperties* extension_properties,
                                                               uint32_t extension_count)
{
    const VkBaseInStructure* entry;
    const VkPhysicalDeviceVulkan12Features * features_12;
    const VkPhysicalDeviceVulkan13Features * features_13;

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
                if(features_12->timelineSemaphore==VK_FALSE)return false;
                break;

            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES:
                features_13 = (const VkPhysicalDeviceVulkan13Features*)entry;
                if(features_13->synchronization2==VK_FALSE)return false;
                break;

            default: break;
        }
    }

    return true;
}

static void cvm_vk_internal_device_feature_request_function(VkBaseOutStructure* set_list,
                                                            const VkBaseInStructure* valid_list,
                                                            const VkPhysicalDeviceProperties* device_properties,
                                                            const VkPhysicalDeviceMemoryProperties* memory_properties,
                                                            const VkExtensionProperties* extension_properties,
                                                            uint32_t extension_count)
{
    VkBaseOutStructure* set_entry;
    VkPhysicalDeviceVulkan12Features * set_features_12;
    VkPhysicalDeviceVulkan13Features * features_13;

    /// properties unused
    (void)device_properties;
    (void)memory_properties;
    (void)extension_properties;
    (void)extension_count;

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
}


/// also removes duplicates!
static void cvm_vk_internal_device_setup_create(cvm_vk_device_setup * internal, const cvm_vk_device_setup * external)
{
    static const char* const swapchain_extension="VK_KHR_swapchain";
    VkStructureType feature_struct_types[3]={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    size_t feature_struct_sizes[3]={sizeof(VkPhysicalDeviceVulkan11Features), sizeof(VkPhysicalDeviceVulkan12Features), sizeof(VkPhysicalDeviceVulkan13Features)};
    uint32_t i,j;

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

    internal->extensions=malloc(sizeof(const char*)*(external->extension_count+1));
    internal->extension_count=1;
    internal->extensions[0]=swapchain_extension;
    for(i=0;i<external->extension_count;i++)
    {
        for(j=0;j<internal->extension_count;j++)
        {
            if(strcmp(external->extensions[i],internal->extensions[j])==0)
            {
                break;
            }
        }
        if(j==internal->extension_count)
        {
            internal->extensions[j]=external->extensions[i];
            internal->extension_count++;
        }
    }
}

static void cvm_vk_internal_device_setup_destroy(cvm_vk_device_setup * setup)
{
    free(setup->feature_validation);
    free(setup->feature_request);
    free(setup->device_feature_struct_types);
    free(setup->device_feature_struct_sizes);
    free(setup->extensions);
}



static bool check_physical_device_appropriate_and_get_properties(cvm_vk * vk, VkPhysicalDevice physical_device_to_test, VkPhysicalDeviceFeatures2 * features, VkSurfaceKHR test_surface, bool dedicated_gpu_required, bool sync_compute_required, const cvm_vk_device_setup * device_setup)
{
    uint32_t i,queue_family_count,redundant_compute_queue_family_index,extension_count;
    VkQueueFamilyProperties * queue_family_properties=NULL;
    VkBool32 surface_supported;
    VkQueueFlags flags;
    VkExtensionProperties * extensions;

    vk->physical_device=physical_device_to_test;

    vkGetPhysicalDeviceProperties(vk->physical_device, &vk->physical_device_properties);
    vkGetPhysicalDeviceMemoryProperties(vk->physical_device,&vk->physical_device_memory_properties);

    vkGetPhysicalDeviceFeatures2(vk->physical_device,features);

    vkEnumerateDeviceExtensionProperties(cvm_vk_.physical_device,NULL,&extension_count,NULL);
    extensions=malloc(sizeof(VkExtensionProperties)*extension_count);
    vkEnumerateDeviceExtensionProperties(cvm_vk_.physical_device,NULL,&extension_count,extensions);



    if((dedicated_gpu_required)&&(vk->physical_device_properties.deviceType!=VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))goto check_physical_device_appropriate_and_get_properties_FAIL;

    if(strstr(vk->physical_device_properties.deviceName,"LLVM"))goto check_physical_device_appropriate_and_get_properties_FAIL;
    /// ^ temp fix to exclude buggy LLVM implementation, need a better/more general way to avoid such "software" devices

    printf("testing GPU : %s\n",vk->physical_device_properties.deviceName);

    /// external feature requirements
    for(i=0;i<device_setup->feature_validation_count;i++)
    {
        if(!device_setup->feature_validation[i]((VkBaseInStructure*)features,&vk->physical_device_properties,&vk->physical_device_memory_properties,extensions,extension_count))
        {
            goto check_physical_device_appropriate_and_get_properties_FAIL;
        }
    }

    vkGetPhysicalDeviceQueueFamilyProperties(vk->physical_device,&queue_family_count,NULL);
    queue_family_properties=malloc(sizeof(VkQueueFamilyProperties)*queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vk->physical_device,&queue_family_count,queue_family_properties);

    vk->graphics_queue_family_index=queue_family_count;
    vk->transfer_queue_family_index=queue_family_count;
    vk->present_queue_family_index=queue_family_count;
    vk->compute_queue_family_index=queue_family_count;
    redundant_compute_queue_family_index=queue_family_count;

    for(i=0;i<queue_family_count;i++)
    {
        printf("queue family %u\n available queue count: %u\nqueue family properties %x\n\n",i,queue_family_properties[i].queueCount,queue_family_properties[i].queueFlags);

        flags=queue_family_properties[i].queueFlags;


        /// If an implementation exposes any queue family that supports graphics operations,
        /// at least one queue family of at least one physical device exposed by the implementation must support both graphics and compute operations.

        if((vk->graphics_queue_family_index==queue_family_count)&&(flags&VK_QUEUE_GRAPHICS_BIT)&&(flags&VK_QUEUE_COMPUTE_BIT || !sync_compute_required))
        {
            vk->graphics_queue_family_index=i;
        }

        ///if an explicit transfer queue family separate from selected graphics and compute queue families exists use it independently
        if((vk->transfer_queue_family_index==queue_family_count) && (flags&VK_QUEUE_TRANSFER_BIT) && !(flags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)))
        {
            vk->transfer_queue_family_index=i;
        }

        ///prefer async if possible (non-graphics)
        if((vk->compute_queue_family_index==queue_family_count) && (flags&VK_QUEUE_COMPUTE_BIT) && !(flags&VK_QUEUE_GRAPHICS_BIT))
        {
            vk->compute_queue_family_index=i;
        }

        ///set up redundant though otherwise
        if((redundant_compute_queue_family_index==queue_family_count) && (flags&VK_QUEUE_COMPUTE_BIT))
        {
            redundant_compute_queue_family_index=i;
        }

        if(vk->present_queue_family_index==queue_family_count)
        {
            CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vk->physical_device,i,test_surface,&surface_supported));
            if(surface_supported)vk->present_queue_family_index=i;
        }
    }

    if(vk->transfer_queue_family_index==queue_family_count)
    {
        ///graphics must support transfer, even if not specified
        vk->transfer_queue_family_index=vk->graphics_queue_family_index;
    }

    if(vk->compute_queue_family_index==queue_family_count)
    {
        assert(redundant_compute_queue_family_index!=queue_family_count);
        vk->compute_queue_family_index=redundant_compute_queue_family_index;
    }

    free(queue_family_properties);

    printf("Queue Families count:%u g:%u t:%u p:%u c:%u\n",queue_family_count,vk->graphics_queue_family_index,vk->transfer_queue_family_index,vk->present_queue_family_index,vk->compute_queue_family_index);

    if ((vk->graphics_queue_family_index == queue_family_count)||
        (vk->transfer_queue_family_index == queue_family_count)||
        (vk->present_queue_family_index  == queue_family_count)||
        (vk->compute_queue_family_index  == queue_family_count))
    {
        goto check_physical_device_appropriate_and_get_properties_FAIL;
    }

    vk->available_physical_device_extensions=extensions;
    vk->available_device_extension_count=extension_count;
    return true;

    check_physical_device_appropriate_and_get_properties_FAIL:
    free(extensions);
    return false;
    ///return true only if all queue families needed can be satisfied
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

static void cvm_vk_create_physical_device(cvm_vk * vk, bool sync_compute_required, const cvm_vk_device_setup * device_setup)
{
    ///pick best physical device with all required features

    uint32_t i,device_count;
    VkPhysicalDevice * physical_devices;
    SDL_Window * test_window;
    VkSurfaceKHR test_surface;
    SDL_bool created_surface;
    VkPhysicalDeviceFeatures2 * features;


    test_window = SDL_CreateWindow("",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,0,0,SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN);
    assert(test_window);/// test window could not be created
    created_surface=SDL_Vulkan_CreateSurface(test_window,vk->instance,&test_surface);
    assert(created_surface);/// test surface could not be created


    features = cvm_vk_create_device_feature_structure_list(device_setup);


    CVM_VK_CHECK(vkEnumeratePhysicalDevices(vk->instance,&device_count,NULL));
    physical_devices=malloc(sizeof(VkPhysicalDevice)*device_count);
    CVM_VK_CHECK(vkEnumeratePhysicalDevices(vk->instance,&device_count,physical_devices));

    for(i=0;i<device_count;i++)///check for dedicated gfx cards first
    {
        if(check_physical_device_appropriate_and_get_properties(vk,physical_devices[i],features,test_surface,true,sync_compute_required,device_setup))
        {
            break;
        }
    }

    if(i==device_count) for(i=0;i<device_count;i++)///check for non-dedicated (integrated) gfx cards next
    {

        if(check_physical_device_appropriate_and_get_properties(vk,physical_devices[i],features,test_surface,false,sync_compute_required,device_setup))
        {
            break;
        }
    }

    free(physical_devices);

    cvm_vk_destroy_device_feature_structure_list(features);///feature list used for validation no longer required

    vkDestroySurfaceKHR(vk->instance,test_surface,NULL);
    SDL_DestroyWindow(test_window);

    assert(i!=device_count || fprintf(stderr,"NONE OF %d PHYSICAL DEVICES MEET REQUIREMENTS\n",device_count));

//    printf("memory types: %u\nheap count: %u\n",vk->physical_device_memory_properties.memoryTypeCount,vk->physical_device_memory_properties.memoryHeapCount);
//    for(i=0;i<vk->physical_device_memory_properties.memoryTypeCount;i++)
//    {
//        if(vk->physical_device_memory_properties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
//        {
//            printf("device local memory type: %u using heap: %u\n",i,vk->physical_device_memory_properties.memoryTypes[i].heapIndex);
//        }
//        printf("memory type %u has properties %x\n",i,vk->physical_device_memory_properties.memoryTypes[i].propertyFlags);
//    }
}

static void cvm_vk_create_logical_device(cvm_vk * vk, const cvm_vk_device_setup * device_setup)
{
    const char * layer_names[]={"VK_LAYER_KHRONOS_validation"};///VK_LAYER_LUNARG_standard_validation
    float queue_priority=1.0;
    uint32_t i,j,found_extensions;
    ///if implementing separate transfer queue use a lower priority?
    /// should also probably try putting transfers on separate queue (when possible) even when they require the same queue family

    VkDeviceQueueCreateInfo device_queue_creation_infos[3];///3 is max queues
    uint32_t queue_family_count=0;

    ///graphics queue family is "base" from which transfer and present can deviate (or be the same as)
    device_queue_creation_infos[queue_family_count++]=(VkDeviceQueueCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=cvm_vk_.graphics_queue_family_index,
        .queueCount=1,
        .pQueuePriorities= &queue_priority
    };

    if(cvm_vk_.transfer_queue_family_index != cvm_vk_.graphics_queue_family_index)
    device_queue_creation_infos[queue_family_count++]=(VkDeviceQueueCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=cvm_vk_.transfer_queue_family_index,
        .queueCount=1,
        .pQueuePriorities= &queue_priority
    };

    if(cvm_vk_.present_queue_family_index != cvm_vk_.graphics_queue_family_index)
    device_queue_creation_infos[queue_family_count++]=(VkDeviceQueueCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=cvm_vk_.present_queue_family_index,
        .queueCount=1,
        .pQueuePriorities= &queue_priority
    };

    found_extensions=0;
    for(i=0;i<vk->available_device_extension_count;i++)
    {
//        puts(vk->available_physical_device_extensions[i].extensionName);
        for(j=0;j<device_setup->extension_count;j++)
        {
            found_extensions+=(0==strcmp(vk->available_physical_device_extensions[i].extensionName,device_setup->extensions[j]));
        }
    }


    assert(found_extensions==device_setup->extension_count);///REQUESTED EXTENSION NOT FOUND
    #warning could/should implement this to instead print WHICH ext wasnt found

    vk->enabled_physical_device_features = cvm_vk_create_device_feature_structure_list(device_setup);

    for(i=0;i<device_setup->feature_request_count;i++)
    {
        device_setup->feature_request[i]((VkBaseOutStructure*)vk->enabled_physical_device_features,
                                         (VkBaseInStructure*)vk->available_physical_device_features,
                                         &vk->physical_device_properties,
                                         &vk->physical_device_memory_properties,
                                         vk->available_physical_device_extensions,
                                         vk->available_device_extension_count);
    }

    VkDeviceCreateInfo device_creation_info=(VkDeviceCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext=vk->enabled_physical_device_features,
        .flags=0,
        .queueCreateInfoCount=queue_family_count,
        .pQueueCreateInfos= device_queue_creation_infos,
        .enabledLayerCount=1,///make 0 for release version (no validation), test performance diff
        .ppEnabledLayerNames=layer_names,
        .enabledExtensionCount=device_setup->extension_count,
        .ppEnabledExtensionNames=device_setup->extensions,
        .pEnabledFeatures=NULL,///using features2 in pNext chain instead
    };

    CVM_VK_CHECK(vkCreateDevice(cvm_vk_.physical_device,&device_creation_info,NULL,&cvm_vk_device));
    vkGetDeviceQueue(cvm_vk_device,cvm_vk_.graphics_queue_family_index,0,&cvm_vk_graphics_queue);
    vkGetDeviceQueue(cvm_vk_device,cvm_vk_.transfer_queue_family_index,0,&cvm_vk_transfer_queue);
    vkGetDeviceQueue(cvm_vk_device,cvm_vk_.present_queue_family_index,0,&cvm_vk_present_queue);
}

static void cvm_vk_create_internal_command_pools(void)
{
    if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .queueFamilyIndex=cvm_vk_.present_queue_family_index,
        };

        CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&cvm_vk_internal_present_command_pool));
    }
}

static void cvm_vk_destroy_internal_command_pools(void)
{
    if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        vkDestroyCommandPool(cvm_vk_device,cvm_vk_internal_present_command_pool,NULL);
    }
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

    cvm_vk_swapchain_image_count=((surface_capabilities.minImageCount > cvm_vk_min_swapchain_images) ? surface_capabilities.minImageCount : cvm_vk_min_swapchain_images);

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

    CVM_VK_CHECK(vkCreateSwapchainKHR(cvm_vk_device,&swapchain_create_info,NULL,&cvm_vk_swapchain));

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&i,NULL));

    assert(i==cvm_vk_swapchain_image_count);///SUPPOSEDLY CORRECT SWAPCHAIN COUNT WAS NOY CREATED PROPERLY

    VkImage * swapchain_images=malloc(sizeof(VkImage)*cvm_vk_swapchain_image_count);

    CVM_VK_CHECK(vkGetSwapchainImagesKHR(cvm_vk_device,cvm_vk_swapchain,&cvm_vk_swapchain_image_count,swapchain_images));

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
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&binary_semaphore_create_info,NULL,cvm_vk_image_acquisition_semaphores+i));
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&binary_semaphore_create_info,NULL,&cvm_vk_presenting_images[i].present_semaphore));

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

        CVM_VK_CHECK(vkCreateImageView(cvm_vk_device,&image_view_create_info,NULL,&cvm_vk_presenting_images[i].image_view));




        ///queue ownership transfer stuff (needs testing)
        if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
        {
            VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
            {
                .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext=NULL,
                .commandPool=cvm_vk_internal_present_command_pool,
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
                        .srcQueueFamilyIndex=cvm_vk_.graphics_queue_family_index,
                        .dstQueueFamilyIndex=cvm_vk_.present_queue_family_index,
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

    if(old_swapchain!=VK_NULL_HANDLE)vkDestroySwapchainKHR(cvm_vk_device,old_swapchain,NULL);

    cvm_vk_create_swapchain_dependednt_defaults(surface_capabilities.currentExtent.width,surface_capabilities.currentExtent.height);

    cvm_vk_rendering_resources_valid=true;
}

void cvm_vk_destroy_swapchain(void)
{
    vkDeviceWaitIdle(cvm_vk_device);
    /// probably not the best place to put this, but so it goes, needed to ensure present workload has actually completed (and thus any batches referencing the present semaphore have completed before deleting it)
    /// could probably just call queue wait idle on graphics and present queues?

    uint32_t i;

    cvm_vk_destroy_swapchain_dependednt_defaults();

    assert(!cvm_vk_acquired_image_count);///MUST WAIT TILL ALL IMAGES ARE RETURNED BEFORE TERMINATING SWAPCHAIN

    for(i=0;i<cvm_vk_swapchain_image_count+1;i++)
    {
        vkDestroySemaphore(cvm_vk_device,cvm_vk_image_acquisition_semaphores[i],NULL);
    }

    for(i=0;i<cvm_vk_swapchain_image_count;i++)
    {
        /// acquired frames
        vkDestroySemaphore(cvm_vk_device,cvm_vk_presenting_images[i].present_semaphore,NULL);

        /// swapchain frames
        vkDestroyImageView(cvm_vk_device,cvm_vk_presenting_images[i].image_view,NULL);

        ///queue ownership transfer stuff
        if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
        {
            vkFreeCommandBuffers(cvm_vk_device,cvm_vk_internal_present_command_pool,1,&cvm_vk_presenting_images[i].present_acquire_command_buffer);
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

    CVM_VK_CHECK(vkCreateSemaphore(cvm_vk_device,&timeline_semaphore_create_info,NULL,&timeline_semaphore->semaphore));
    timeline_semaphore->value=0;
}

static inline VkSemaphoreSubmitInfo cvm_vk_create_timeline_semaphore_signal_submit_info(cvm_vk_timeline_semaphore * ts,VkPipelineStageFlags2 stages)
{
    return (VkSemaphoreSubmitInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext=NULL,
        .semaphore=ts->semaphore,
        .value=++ts->value,
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
    CVM_VK_CHECK(vkWaitSemaphores(cvm_vk_device,&wait,timeout_ns));
}

static void cvm_vk_create_timeline_semaphores(void)
{
    cvm_vk_create_timeline_semaphore(&cvm_vk_graphics_timeline);
    cvm_vk_create_timeline_semaphore(&cvm_vk_transfer_timeline);
    if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        cvm_vk_create_timeline_semaphore(&cvm_vk_present_timeline);
    }
}

static void cvm_vk_destroy_timeline_semaphores(void)
{
    vkDestroySemaphore(cvm_vk_device,cvm_vk_graphics_timeline.semaphore,NULL);
    vkDestroySemaphore(cvm_vk_device,cvm_vk_transfer_timeline.semaphore,NULL);
    if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        vkDestroySemaphore(cvm_vk_device,cvm_vk_present_timeline.semaphore,NULL);
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



void cvm_vk_initialise(SDL_Window * window,uint32_t min_swapchain_images,bool sync_compute_required, const cvm_vk_device_setup * external_device_setup)
{
    cvm_vk_device_setup device_setup;

    cvm_vk_internal_device_setup_create(&device_setup, external_device_setup);

    cvm_vk_min_swapchain_images=min_swapchain_images;

    cvm_vk_create_instance(&cvm_vk_.instance,window," ");
    cvm_vk_create_physical_device(&cvm_vk_,sync_compute_required,&device_setup);
    cvm_vk_create_surface_from_window(&cvm_vk_surface,&cvm_vk_,window);
    cvm_vk_create_logical_device(&cvm_vk_, &device_setup);
    cvm_vk_create_internal_command_pools();
    cvm_vk_create_transfer_chain();
    cvm_vk_create_timeline_semaphores();

    cvm_vk_create_defaults();

    cvm_vk_internal_device_setup_destroy(&device_setup);
}

void cvm_vk_terminate(void)
{
    cvm_vk_destroy_defaults();

    cvm_vk_destroy_timeline_semaphores();

    cvm_vk_destroy_transfer_chain();
    cvm_vk_destroy_internal_command_pools();

    free(cvm_vk_image_acquisition_semaphores);
    free(cvm_vk_presenting_images);

    vkDestroySwapchainKHR(cvm_vk_device,cvm_vk_swapchain,NULL);
    vkDestroyDevice(cvm_vk_device,NULL);
    vkDestroySurfaceKHR(cvm_vk_.instance,cvm_vk_surface,NULL);
    vkDestroyInstance(cvm_vk_.instance,NULL);
}






































#warning window will be removed after SDL3 integration
void cvm_vk_initialse_2(cvm_vk * vk, const char * application_name, SDL_Window * window)
{
    cvm_vk_create_instance(&vk->instance, window, application_name);
}

void cvm_vk_terminate_2(cvm_vk * vk)
{
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
                assert(presenting_image->successfully_acquired);///SOMEHOW PREPARING/CLEANING UP FRAME THAT WAS SUBMITTED BUT NOT ACQUIRED
                cvm_vk_wait_on_timeline_semaphore(&cvm_vk_graphics_timeline,presenting_image->graphics_wait_value,1000000000);
                cvm_vk_wait_on_timeline_semaphore(&cvm_vk_transfer_timeline,presenting_image->transfer_wait_value,1000000000);
                if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
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
        if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)///requires QFOT
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
                        .srcQueueFamilyIndex=cvm_vk_.graphics_queue_family_index,
                        .dstQueueFamilyIndex=cvm_vk_.present_queue_family_index,
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

    CVM_VK_CHECK(vkQueueSubmit2(cvm_vk_graphics_queue,1,&submit_info,VK_NULL_HANDLE));

    /// present if last payload that uses the swapchain
    if(flags&CVM_VK_PAYLOAD_LAST_QUEUE_USE)
    {
        ///acquire on presenting queue if necessary, reuse submit info
        if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
        {
            ///fixed count and layout of wait and signal semaphores here
            wait_semaphores[0]=cvm_vk_create_timeline_semaphore_wait_submit_info(&cvm_vk_graphics_timeline,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);///stages is likely wrong/unnecessary...

            /// presenting_image->present_semaphore triggered either here or above when CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE, this path being taken when present queue != graphics queue
            signal_semaphores[0]=cvm_vk_create_binary_semaphore_submit_info(presenting_image->present_semaphore,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);///stages is likely wrong/unnecessary...
            signal_semaphores[1]=cvm_vk_create_timeline_semaphore_signal_submit_info(&cvm_vk_present_timeline,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);///stages is likely wrong/unnecessary...

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
                        .commandBuffer=payload->command_buffer,
                        .deviceMask=0
                    }
                },
                .signalSemaphoreInfoCount=2,///fixed number, set above
                .pSignalSemaphoreInfos=signal_semaphores
            };

            CVM_VK_CHECK(vkQueueSubmit2(cvm_vk_present_queue,1,&submit_info,VK_NULL_HANDLE));

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


        VkResult r=vkQueuePresentKHR(cvm_vk_present_queue,&present_info);

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
                if(cvm_vk_.present_queue_family_index!=cvm_vk_.graphics_queue_family_index)
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

        VkResult r= vkCreateShaderModule(cvm_vk_device,&shader_module_create_info,NULL,&shader_module);
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

static bool cvm_vk_find_appropriate_memory_type(uint32_t supported_type_bits,VkMemoryPropertyFlags required_properties,uint32_t * index)
{
    uint32_t i;
    for(i=0;i<cvm_vk_.physical_device_memory_properties.memoryTypeCount;i++)
    {
        if(( supported_type_bits & 1<<i ) && ((cvm_vk_.physical_device_memory_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties))
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
        vkGetImageMemoryRequirements(cvm_vk_device,images[i],&requirements);
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

    CVM_VK_CHECK(vkAllocateMemory(cvm_vk_device,&memory_allocate_info,NULL,memory));

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

    CVM_VK_CHECK(vkCreateBuffer(cvm_vk_device,&buffer_create_info,NULL,buffer));

    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(cvm_vk_device,*buffer,&buffer_memory_requirements);

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

    CVM_VK_CHECK(vkAllocateMemory(cvm_vk_device,&memory_allocate_info,NULL,memory));

    CVM_VK_CHECK(vkBindBufferMemory(cvm_vk_device,*buffer,*memory,0));///offset/alignment kind of irrelevant because of 1 buffer per allocation paradigm

    if(cvm_vk_.physical_device_memory_properties.memoryTypes[memory_type_index].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        CVM_VK_CHECK(vkMapMemory(cvm_vk_device,*memory,0,VK_WHOLE_SIZE,0,mapping));

        *mapping_coherent=!!(cvm_vk_.physical_device_memory_properties.memoryTypes[memory_type_index].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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

    if(usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT) && alignment < cvm_vk_.physical_device_properties.limits.optimalBufferCopyOffsetAlignment)
        alignment=cvm_vk_.physical_device_properties.limits.optimalBufferCopyOffsetAlignment;

    if(usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) && alignment < cvm_vk_.physical_device_properties.limits.minTexelBufferOffsetAlignment)
        alignment = cvm_vk_.physical_device_properties.limits.minTexelBufferOffsetAlignment;

    if(usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT && alignment < cvm_vk_.physical_device_properties.limits.minUniformBufferOffsetAlignment)
        alignment = cvm_vk_.physical_device_properties.limits.minUniformBufferOffsetAlignment;

    if(usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT && alignment < cvm_vk_.physical_device_properties.limits.minStorageBufferOffsetAlignment)
        alignment = cvm_vk_.physical_device_properties.limits.minStorageBufferOffsetAlignment;

    if(alignment<cvm_vk_.physical_device_properties.limits.nonCoherentAtomSize)
        alignment=cvm_vk_.physical_device_properties.limits.nonCoherentAtomSize;

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

    CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&msb->graphics_pool));

    msb->graphics_scbs=NULL;
    msb->graphics_scb_space=0;
    msb->graphics_scb_count=0;
}

static void cvm_vk_destroy_module_sub_batch(cvm_vk_module_sub_batch * msb)
{
    if(msb->graphics_scb_space)
    {
        vkFreeCommandBuffers(cvm_vk_device,msb->graphics_pool,msb->graphics_scb_space,msb->graphics_scbs);
    }

    free(msb->graphics_scbs);

    vkDestroyCommandPool(cvm_vk_device,msb->graphics_pool,NULL);
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

        CVM_VK_CHECK(vkCreateCommandPool(cvm_vk_device,&command_pool_create_info,NULL,&mb->transfer_pool));
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

    CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,&mb->transfer_cb));///only need 1
}

static void cvm_vk_destroy_module_batch(cvm_vk_module_batch * mb,uint32_t sub_batch_count)
{
    uint32_t i;
    assert(mb->graphics_pcb_space);///module was destroyed without ever being used
    if(mb->graphics_pcb_space)
    {
        ///hijack first sub batch's command pool for primary command buffers
        vkFreeCommandBuffers(cvm_vk_device,mb->sub_batches[0].graphics_pool,mb->graphics_pcb_space,mb->graphics_pcbs);
    }

    free(mb->graphics_pcbs);

    vkFreeCommandBuffers(cvm_vk_device,mb->transfer_pool,1,&mb->transfer_cb);

    if(cvm_vk_.transfer_queue_family_index!=cvm_vk_.graphics_queue_family_index)
    {
        vkDestroyCommandPool(cvm_vk_device,mb->transfer_pool,NULL);
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
            vkResetCommandPool(cvm_vk_device,batch->sub_batches[i].graphics_pool,0);
            batch->sub_batches[i].graphics_scb_count=0;
        }

        batch->graphics_pcb_count=0;

        if(cvm_vk_.transfer_queue_family_index!=cvm_vk_.graphics_queue_family_index)
        {
            vkResetCommandPool(cvm_vk_device,batch->transfer_pool,0);
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

        CVM_VK_CHECK(vkQueueSubmit2(cvm_vk_transfer_queue,1,&submit_info,VK_NULL_HANDLE));

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

        CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_device,&command_buffer_allocate_info,batch->graphics_pcbs+batch->graphics_pcb_space));

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
