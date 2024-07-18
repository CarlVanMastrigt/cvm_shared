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

#include "cvm_shared.h"


static cvm_vk_device cvm_vk_;
static cvm_vk_surface_swapchain cvm_vk_surface_swapchain_;


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

VkSurfaceKHR cvm_vk_create_surface_from_SDL_window(VkInstance instance, SDL_Window * window)
{
    SDL_bool created_surface;
    VkSurfaceKHR surface;

    created_surface=SDL_Vulkan_CreateSurface(window,instance,&surface);

    return created_surface ? surface : VK_NULL_HANDLE;
}

void cvm_vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface)
{
    vkDestroySurfaceKHR(instance,surface,NULL);
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

static void cvm_vk_initialise_device_queue(cvm_vk_device * device,cvm_vk_device_queue * queue,uint32_t queue_family_index,uint32_t queue_index)
{
    cvm_vk_timeline_semaphore_initialise(device,&queue->timeline);
    vkGetDeviceQueue(device->device,queue_family_index,queue_index,&queue->queue);
}

static void cvm_vk_terminate_device_queue(cvm_vk_device * device,cvm_vk_device_queue * queue)
{
    cvm_vk_timeline_semaphore_terminate(device,&queue->timeline);
}

static void cvm_vk_initialise_device_queue_family(cvm_vk_device * device,cvm_vk_device_queue_family * queue_family,uint32_t queue_family_index,uint32_t queue_count)
{
    uint32_t i;

    VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=queue_family_index,
    };

    CVM_VK_CHECK(vkCreateCommandPool(device->device,&command_pool_create_info,NULL,&queue_family->internal_command_pool));

    queue_family->queues=malloc(sizeof(cvm_vk_device_queue)*queue_count);
    queue_family->queue_count=queue_count;
    for(i=0;i<queue_count;i++)
    {
        cvm_vk_initialise_device_queue(device,queue_family->queues+i,queue_family_index,i);
    }
}

static void cvm_vk_terminate_device_queue_family(cvm_vk_device * device,cvm_vk_device_queue_family * queue_family)
{
    uint32_t i;

    vkDestroyCommandPool(device->device,queue_family->internal_command_pool,NULL);

    for(i=0;i<queue_family->queue_count;i++)
    {
        cvm_vk_terminate_device_queue(device,queue_family->queues+i);
    }
    free(queue_family->queues);
}

static void cvm_vk_create_logical_device(cvm_vk_device * device, const cvm_vk_device_setup * device_setup)
{
    uint32_t i,j,queue_count,enabled_extension_count,available_extension_count;
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
    uint32_t queue_counts[CVM_VK_MAX_QUEUE_FAMILY_COUNT];

    vkGetPhysicalDeviceProperties(device->physical_device, (VkPhysicalDeviceProperties*)&device->properties);
    vkGetPhysicalDeviceMemoryProperties(device->physical_device, (VkPhysicalDeviceMemoryProperties*)&device->memory_properties);

    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &device->queue_family_count, NULL);
    if(device->queue_family_count>CVM_VK_MAX_QUEUE_FAMILY_COUNT)return -1;
    device->queue_family_properties=malloc(sizeof(VkQueueFamilyProperties)*device->queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &device->queue_family_count, (VkQueueFamilyProperties*)device->queue_family_properties);

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



    for(i=0;i<device->queue_family_count;i++)
    {
        queue_counts[i]=1;
        /// default of 1 per queue family
    }

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
            queue_counts[i] = CVM_MIN(device->queue_family_properties[i].queueCount,device_setup->desired_graphics_queues);
        }

        if(transfer && !graphics && !compute)
        {
            device->transfer_queue_family_index=i;
            queue_counts[i] = CVM_MIN(device->queue_family_properties[i].queueCount,device_setup->desired_transfer_queues);
        }

        if(compute && !graphics)
        {
            device->async_compute_queue_family_index=i;
            queue_counts[i] = CVM_MIN(device->queue_family_properties[i].queueCount,device_setup->desired_async_compute_queues);
        }
    }


    #warning REMOVE THESE
    device->fallback_present_queue_family_index = device->async_compute_queue_family_index;
//    device->fallback_present_queue_family_index = device->graphics_queue_family_index;
#warning this is temporary while transfers get back up and running!
    device->transfer_queue_family_index = device->graphics_queue_family_index;
    if(device->transfer_queue_family_index==CVM_INVALID_U32_INDEX)device->transfer_queue_family_index=device->graphics_queue_family_index;



    device_queue_creation_infos = malloc(sizeof(VkDeviceQueueCreateInfo)*device->queue_family_count);
    for(i=0;i<device->queue_family_count;i++)
    {
        queue_count=queue_counts[i];
        priorities=malloc(sizeof(float)*queue_count);

        for(j=0;j<queue_count;j++)
        {
            priorities[j] = 1.0;
        }

        device_queue_creation_infos[i]=(VkDeviceQueueCreateInfo)
        {
            .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext=NULL,
            .flags=0,
            .queueFamilyIndex=i,
            .queueCount=queue_count,
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

    device->queue_families=malloc(sizeof(cvm_vk_device_queue_family)*device->queue_family_count);

    for(i=0;i<device->queue_family_count;i++)
    {
        cvm_vk_initialise_device_queue_family(device,device->queue_families+i,i,queue_counts[i]);
    }



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
    uint32_t i;
    for(i=0;i<device->queue_family_count;i++)
    {
        cvm_vk_terminate_device_queue_family(device,device->queue_families+i);
    }
    free(device->queue_families);
    free(device->queue_family_properties);

    vkDestroyDevice(device->device,NULL);
    cvm_vk_destroy_device_feature_structure_list((void*)device->features);
    free((void*)device->extensions);
}



static void cvm_vk_create_transfer_chain(void)
{
    //
}

static void cvm_vk_destroy_transfer_chain(void)
{
    //
}





VkSemaphoreSubmitInfo cvm_vk_binary_semaphore_submit_info(VkSemaphore semaphore,VkPipelineStageFlags2 stages)
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

VkFormat cvm_vk_get_screen_format(void)
{
    return cvm_vk_surface_swapchain_.surface_format.format;
}

uint32_t cvm_vk_get_swapchain_image_count(void)
{
    return cvm_vk_surface_swapchain_.image_count;
}

VkImageView cvm_vk_get_swapchain_image_view(uint32_t index)
{
    return cvm_vk_surface_swapchain_.presenting_images[index].image_view;
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

    device->instance=device_setup.instance;
    device->physical_device=cvm_vk_create_physical_device(device_setup.instance,&device_setup);
    if(device->physical_device==VK_NULL_HANDLE)return -1;

    cvm_vk_create_logical_device(device, &device_setup);

    cvm_vk_create_transfer_chain();///make conditional on separate transfer queue?

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

    cvm_vk_destroy_transfer_chain();///make conditional on separate transfer queue?

    cvm_vk_destroy_logical_device(device);
}































void cvm_vk_submit_graphics_work(cvm_vk_module_work_payload * payload,cvm_vk_payload_flags flags)
{
    VkSemaphoreSubmitInfo wait_semaphores[CVM_VK_PAYLOAD_MAX_WAITS+1];
    uint32_t wait_count=0;
    VkSemaphoreSubmitInfo signal_semaphores[2];
    uint32_t signal_count=0;
    VkSubmitInfo2 submit_info;
//    VkQueue present_queue,graphics_queue;
    cvm_vk_device_queue * present_queue_;
    cvm_vk_device_queue * graphics_queue_;
    cvm_vk_timeline_semaphore_moment graphics_last_moment;

    graphics_queue_=cvm_vk_.queue_families[cvm_vk_.graphics_queue_family_index].queues+0;
    present_queue_=cvm_vk_.queue_families[cvm_vk_.fallback_present_queue_family_index].queues+0;
    #warning get queue indices from somewhere? (is 0 above) need to make decisions regarding them

    uint32_t i;

    cvm_vk_swapchain_presentable_image * presenting_image;

    ///check if there is actually a frame in flight...

    assert(cvm_vk_surface_swapchain_.acquired_image_index!=CVM_INVALID_U32_INDEX);///SHOULDN'T BE SUBMITTING WORK WHEN NO VALID SWAPCHAIN IMAGE WAS ACQUIRED THIS FRAME

    presenting_image=cvm_vk_surface_swapchain_.presenting_images+cvm_vk_surface_swapchain_.acquired_image_index;


    if(flags&CVM_VK_PAYLOAD_FIRST_QUEUE_USE)
    {
        ///add initial semaphore
        wait_semaphores[wait_count++]=cvm_vk_binary_semaphore_submit_info(presenting_image->acquire_semaphore,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

    if(flags&CVM_VK_PAYLOAD_LAST_QUEUE_USE)
    {
        #warning following check needs to take compatibility mask instead!
        ///add final semaphore and other synchronization
        if(cvm_vk_surface_swapchain_.fallback_present_queue_family!=cvm_vk_.graphics_queue_family_index)///requires QFOT
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
                        .dstQueueFamilyIndex=cvm_vk_surface_swapchain_.fallback_present_queue_family,
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

            signal_semaphores[signal_count++]=cvm_vk_timeline_semaphore_signal_submit_info(&graphics_queue_->timeline,payload->signal_stages | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,&graphics_last_moment);
        }
        else
        {
            /// presenting_image->present_semaphore triggered either here or below when CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE, this path being taken when present queue == graphics queue
            signal_semaphores[signal_count++]=cvm_vk_binary_semaphore_submit_info(presenting_image->present_semaphore,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
            signal_semaphores[signal_count++]=cvm_vk_timeline_semaphore_signal_submit_info(&graphics_queue_->timeline, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, &presenting_image->last_use_moment);
        }

//        signal_semaphores[signal_count++]=cvm_vk_create_timeline_semaphore_signal_submit_info(&cvm_vk_graphics_timeline,payload->signal_stages | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,&presenting_image->last_use_moment);
    }
//    else
//    {
//        signal_semaphores[signal_count++]=cvm_vk_create_timeline_semaphore_signal_submit_info(&cvm_vk_graphics_timeline,payload->signal_stages,&presenting_image->last_use_moment);
//    }

//    for(i=0;i<payload->wait_count;i++)
//    {
//        wait_semaphores[wait_count++]=cvm_vk_create_timeline_semaphore_wait_submit_info(payload->waits+i,payload->wait_stages[i]);
//    }

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



    CVM_VK_CHECK(vkQueueSubmit2(graphics_queue_->queue,1,&submit_info,VK_NULL_HANDLE));

    /// present if last payload that uses the swapchain
    if(flags&CVM_VK_PAYLOAD_LAST_QUEUE_USE)
    {
        ///dstAccessMask member of the VkImageMemoryBarrier should be set to 0, and the dstStageMask parameter should be set to VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.

        ///acquire on presenting queue if necessary, reuse submit info
        if(cvm_vk_surface_swapchain_.fallback_present_queue_family!=cvm_vk_.graphics_queue_family_index)
        {

            #warning replace cvm_vk_.graphics_queue_family_index with source queue family index
            #warning requires synchronization! (uses shared internal command pool)
            if(presenting_image->present_acquire_command_buffers[cvm_vk_.graphics_queue_family_index]==VK_NULL_HANDLE)
            {
                VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
                {
                    .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .pNext=NULL,
                    .commandPool=cvm_vk_.queue_families[cvm_vk_surface_swapchain_.fallback_present_queue_family].internal_command_pool,
                    .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount=1
                };

                CVM_VK_CHECK(vkAllocateCommandBuffers(cvm_vk_.device,&command_buffer_allocate_info,presenting_image->present_acquire_command_buffers+cvm_vk_.graphics_queue_family_index));

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
                            .dstQueueFamilyIndex=cvm_vk_surface_swapchain_.fallback_present_queue_family,
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


                CVM_VK_CHECK(vkBeginCommandBuffer(presenting_image->present_acquire_command_buffers[cvm_vk_.graphics_queue_family_index],&command_buffer_begin_info));

                vkCmdPipelineBarrier2(presenting_image->present_acquire_command_buffers[cvm_vk_.graphics_queue_family_index],&present_acquire_dependencies);

                CVM_VK_CHECK(vkEndCommandBuffer(presenting_image->present_acquire_command_buffers[cvm_vk_.graphics_queue_family_index]));
            }

            ///fixed count and layout of wait and signal semaphores here
            wait_semaphores[0]=cvm_vk_timeline_semaphore_moment_wait_submit_info(&graphics_last_moment,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

            /// presenting_image->present_semaphore triggered either here or above when CVM_VK_PAYLOAD_LAST_SWAPCHAIN_USE, this path being taken when present queue != graphics queue
            signal_semaphores[0]=cvm_vk_binary_semaphore_submit_info(presenting_image->present_semaphore,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
            signal_semaphores[1]=cvm_vk_timeline_semaphore_signal_submit_info(&present_queue_->timeline, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, &presenting_image->last_use_moment);

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
                        .commandBuffer=presenting_image->present_acquire_command_buffers[cvm_vk_.graphics_queue_family_index],
                        .deviceMask=0
                    }
                },
                .signalSemaphoreInfoCount=2,///fixed number, set above
                .pSignalSemaphoreInfos=signal_semaphores
            };

            CVM_VK_CHECK(vkQueueSubmit2(present_queue_->queue,1,&submit_info,VK_NULL_HANDLE));
        }

        VkPresentInfoKHR present_info=(VkPresentInfoKHR)
        {
            .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext=NULL,
            .waitSemaphoreCount=1,
            .pWaitSemaphores=&presenting_image->present_semaphore,
            .swapchainCount=1,
            .pSwapchains=&cvm_vk_surface_swapchain_.swapchain,
            .pImageIndices=&cvm_vk_surface_swapchain_.acquired_image_index,
            .pResults=NULL
        };
        #warning present should be synchronised (check this is true)

        #warning separate present out from modules/work, it should operate on the presentable_image


        VkResult r=vkQueuePresentKHR(present_queue_->queue,&present_info);

        if(r>=VK_SUCCESS)
        {
            if(r==VK_SUBOPTIMAL_KHR)
            {
                puts("presented swapchain suboptimal");///common error
                cvm_vk_surface_swapchain_.rendering_resources_valid=false;///requires resource rebuild
            }
            else if(r!=VK_SUCCESS)fprintf(stderr,"PRESENTATION SUCCEEDED WITH RESULT : %d\n",r);
            presenting_image->presented=true;
        }
        else
        {
            if(r==VK_ERROR_OUT_OF_DATE_KHR)puts("couldn't present (out of date)");///common error
            else fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);
        }
    }
}

VkFence cvm_vk_create_fence(const cvm_vk_device * device,bool initially_signalled)
{
    VkFence fence;
    VkResult r;

    VkFenceCreateInfo create_info=
    {
        .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext=NULL,
        .flags=initially_signalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0,
    };

    r=vkCreateFence(device->device,&create_info,NULL,&fence);
    assert(r==VK_SUCCESS);

    return r==VK_SUCCESS ? fence : VK_NULL_HANDLE;
}

void cvm_vk_destroy_fence(const cvm_vk_device * device,VkFence fence)
{
    vkDestroyFence(device->device,fence,NULL);
}

void cvm_vk_wait_on_fence_and_reset(const cvm_vk_device * device,VkFence fence)
{
    CVM_VK_CHECK(vkWaitForFences(device->device,1,&fence,VK_TRUE,CVM_VK_DEFAULT_TIMEOUT));
    vkResetFences(device->device,1,&fence);
}

VkSemaphore cvm_vk_create_binary_semaphore(const cvm_vk_device * device)
{
    VkSemaphore semaphore;
    VkResult r;

    VkSemaphoreCreateInfo create_info=(VkSemaphoreCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=NULL,
        .flags=0
    };

    r=vkCreateSemaphore(device->device,&create_info,NULL,&semaphore);
    assert(r==VK_SUCCESS);

    return r==VK_SUCCESS ? semaphore : VK_NULL_HANDLE;
}

void cvm_vk_destroy_binary_semaphore(const cvm_vk_device * device,VkSemaphore semaphore)
{
    vkDestroySemaphore(device->device,semaphore,NULL);
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
    #warning remove
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
    #warning remove
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

    assert((alignment & (alignment-1))==0);///alignments must all be a power of 2 according to the vulkan spec

//    printf("ALIGNMENT %u\n",alignment);

    return alignment;
}

VkDeviceSize cvm_vk_buffer_alignment_requirements(const cvm_vk_device * device, VkBufferUsageFlags usage)
{
    VkDeviceSize alignment=device->properties.limits.nonCoherentAtomSize;

    if(usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT))
    {
        alignment = CVM_MAX(alignment,device->properties.limits.optimalBufferCopyOffsetAlignment);
    }

    if(usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT))
    {
        alignment = CVM_MAX(alignment,device->properties.limits.minTexelBufferOffsetAlignment);
    }

    if(usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    {
        alignment = CVM_MAX(alignment,device->properties.limits.minUniformBufferOffsetAlignment);
    }

    if(usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    {
        alignment = CVM_MAX(alignment,device->properties.limits.minStorageBufferOffsetAlignment);
    }

    assert((alignment & (alignment-1))==0);///alignments must all be a power of 2 according to the vulkan spec

    return alignment;
}

static uint32_t cvm_vk_find_appropriate_memory_type_(const cvm_vk_device * device, uint32_t supported_type_bits, VkMemoryPropertyFlags required_properties)
{
    uint32_t i;
    for(i=0;i<device->memory_properties.memoryTypeCount;i++)
    {
        if(( supported_type_bits & 1<<i ) && ((device->memory_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties))
        {
            return i;
        }
    }
    return CVM_INVALID_U32_INDEX;
}

void cvm_vk_buffer_memory_pair_create(const cvm_vk_device * device, cvm_vk_buffer_memory_pair_setup * setup)
{
    VkMemoryRequirements memory_requirements;
    uint32_t memory_type_index;

    VkBufferCreateInfo buffer_create_info=(VkBufferCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .size=setup->buffer_size,
        .usage=setup->usage,
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL
    };

    if(vkCreateBuffer(device->device,&buffer_create_info,NULL,&setup->buffer)!=VK_SUCCESS)
    {
        setup->buffer=VK_NULL_HANDLE;
        setup->memory=VK_NULL_HANDLE;
        return;
    }


    vkGetBufferMemoryRequirements(device->device,setup->buffer,&memory_requirements);

    memory_type_index = cvm_vk_find_appropriate_memory_type_(device, memory_requirements.memoryTypeBits, setup->desired_properties|setup->required_properties);
    if(memory_type_index==CVM_INVALID_U32_INDEX)
    {
        ///try again without desired properties
        memory_type_index = cvm_vk_find_appropriate_memory_type(device, memory_requirements.memoryTypeBits, setup->required_properties);
        if(memory_type_index==CVM_INVALID_U32_INDEX)
        {
            vkDestroyBuffer(device->device,setup->buffer,NULL);
            setup->buffer=VK_NULL_HANDLE;
            setup->memory=VK_NULL_HANDLE;
            return;
        }
    }


    VkMemoryAllocateInfo memory_allocate_info=(VkMemoryAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext=NULL,
        .allocationSize=memory_requirements.size,
        .memoryTypeIndex=memory_type_index
    };

    if(vkAllocateMemory(device->device,&memory_allocate_info,NULL,&setup->memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(device->device,setup->buffer,NULL);
        setup->buffer=VK_NULL_HANDLE;
        setup->memory=VK_NULL_HANDLE;
        return;
    }

    if(vkBindBufferMemory(device->device,setup->buffer,setup->memory,0) != VK_SUCCESS)
    {
        vkDestroyBuffer(device->device,setup->buffer,NULL);
        vkFreeMemory(device->device,setup->memory,NULL);
        setup->buffer=VK_NULL_HANDLE;
        setup->memory=VK_NULL_HANDLE;
        return;
    }

    /// set mapping fallback values
    setup->mapping=NULL;
    setup->mapping_coherent=false;

    if(setup->map_memory && (device->memory_properties.memoryTypes[memory_type_index].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
    {
        if(vkMapMemory(device->device, setup->memory, 0, VK_WHOLE_SIZE, 0, &setup->mapping)==VK_SUCCESS)
        {
            setup->mapping_coherent = cvm_vk_.memory_properties.memoryTypes[memory_type_index].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
//            printf("MAPPING COHERENT? : %d\n",setup->mapping_coherent);
        }
        else
        {
            fprintf(stderr,"CVM VK ERROR - unable to map buffer that should be mappable\n");
        }
    }
}

void cvm_vk_buffer_memory_pair_destroy(const cvm_vk_device * device, VkBuffer buffer, VkDeviceMemory memory, bool memory_was_mapped)
{
    if(memory_was_mapped)
    {
        vkUnmapMemory(device->device,memory);
    }

    vkDestroyBuffer(device->device,buffer,NULL);
    vkFreeMemory(device->device,memory,NULL);
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
//    assert(mb->graphics_pcb_space);///module was destroyed without ever being used
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

    uint32_t new_batch_count=cvm_vk_surface_swapchain_.image_count;
    #warning above is some bullshit

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
        cvm_vk_buffer_barrier_stack_initialise(&module_data->transfer_data[i].acquire_barriers);
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
        cvm_vk_buffer_barrier_stack_terminate(&module_data->transfer_data[i].acquire_barriers);
    }
}

cvm_vk_module_batch * cvm_vk_get_module_batch(cvm_vk_module_data * module_data,uint32_t * swapchain_image_index)
{
    uint32_t i;

    *swapchain_image_index = cvm_vk_surface_swapchain_.acquired_image_index;///value passed back by rendering engine

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
            puts("a");
            vkResetCommandPool(cvm_vk_.device,batch->transfer_pool,0);
            puts("b");
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
            #warning this needs replacing with new paradigm
            .signalSemaphoreInfoCount=0,//1
            .pSignalSemaphoreInfos=(VkSemaphoreSubmitInfo[1])
            {
//                cvm_vk_create_timeline_semaphore_signal_submit_info(&cvm_vk_transfer_timeline,VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,&cvm_vk_surface_swapchain_.presenting_images[cvm_vk_surface_swapchain_.acquired_image_index].transfer_wait) /// note: also increments this timeline semaphore
            }
        };

        ///need to make sure this wait is actually only performed once per QF... (not on EVERY subsequent "frame")
//        for(i=0;i<CVM_VK_MAX_QUEUES;i++)///set copy wait value on appropriate queues
//        {
//            if(batch->transfer_affceted_queues_bitmask & 1<<i)
//            {
//                batch->transfer_data[i].transfer_semaphore_wait_value=cvm_vk_transfer_timeline.value;
//            }
//        }

        #warning get queue index from somewhere? possibly batch, give batches priorities for both tranfer and execution? could pass in priority at execution time? possibly as override to batch priority?
        vkGetDeviceQueue(cvm_vk_.device,cvm_vk_.transfer_queue_family_index,0,&transfer_queue);
        CVM_VK_CHECK(vkQueueSubmit2(transfer_queue,1,&submit_info,VK_NULL_HANDLE));

        assert(cvm_vk_surface_swapchain_.acquired_image_index!=CVM_INVALID_U32_INDEX);///SHOULDN'T BE SUBMITTING WORK WHEN NO VALID SWAPCHAIN IMAGE WAS ACQUIRED THIS FRAME
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
//            payload->waits[0].value=transfer_data->transfer_semaphore_wait_value;
//            payload->waits[0].semaphore=cvm_vk_transfer_timeline.semaphore;
//            payload->wait_stages[0]=transfer_data->wait_stages;
//            payload->wait_count++;

            ///rest data now that we're done with it
            transfer_data->wait_stages=VK_PIPELINE_STAGE_2_NONE;

            cvm_vk_buffer_barrier_stack_reset(&transfer_data->acquire_barriers);
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
