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




static void initialise_gfx_instance(gfx_data * gfx,SDL_Window * window)
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

    CVM_VK_CHECK(vkCreateInstance(&instance_creation_info,NULL,&gfx->instance));

    free(extension_names);
}

static void initialise_gfx_surface(gfx_data * gfx,SDL_Window * window)
{
    if(!SDL_Vulkan_CreateSurface(window,gfx->instance,&gfx->surface))
    {
        fprintf(stderr,"COULD NOT CREATE SDL VULKAN SURFACE\n");
        exit(-1);
    }
}

static bool check_physical_device_appropriate(gfx_data * gfx,bool dedicated_gpu_required)
{
    VkPhysicalDeviceProperties properties;
    uint32_t i,queue_family_count;
    VkQueueFamilyProperties * queue_family_properties=NULL;
    VkBool32 surface_supported;

    //VkPhysicalDeviceFeatures required_features={VK_FALSE};

    ///add more if more back-end queues are desired (add them to back-end family)
    ///also potentially add transfer queue for quicker data transfer?

    vkGetPhysicalDeviceProperties(gfx->physical_device,&properties);
    vkGetPhysicalDeviceFeatures(gfx->physical_device,&gfx->device_features);

    if((dedicated_gpu_required)&&(properties.deviceType!=VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))return false;

    printf("testing GPU : %s\n",properties.deviceName);

    #warning test for required features. need comparitor struct to do this?   enable all by default for time being?

    //printf("geometry shader: %d\n",gfx->device_features.geometryShader);

    vkGetPhysicalDeviceQueueFamilyProperties(gfx->physical_device,&queue_family_count,NULL);
    queue_family_properties=malloc(sizeof(VkQueueFamilyProperties)*queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gfx->physical_device,&queue_family_count,queue_family_properties);

    ///use first queue family to support each desired operation

    gfx->queue_family=queue_family_count;

    for(i=0;i<queue_family_count;i++)
    {
        CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(gfx->physical_device,i,gfx->surface,&surface_supported));

        if((surface_supported==VK_TRUE)&&(queue_family_properties[i].queueFlags&VK_QUEUE_GRAPHICS_BIT))
        {
            gfx->queue_family=i;
            free(queue_family_properties);
            return true;
        }
    }

    free(queue_family_properties);

    return false;
}

static void initialise_gfx_physical_device(gfx_data * gfx)
{
    ///pick best physical device with all required features

    uint32_t i,device_count,format_count,present_mode_count;
    VkPhysicalDevice * physical_devices;
    VkSurfaceFormatKHR * formats=NULL;
    VkPresentModeKHR * present_modes=NULL;

    CVM_VK_CHECK(vkEnumeratePhysicalDevices(gfx->instance,&device_count,NULL));
    physical_devices=malloc(sizeof(VkPhysicalDevice)*device_count);
    CVM_VK_CHECK(vkEnumeratePhysicalDevices(gfx->instance,&device_count,physical_devices));

    for(i=0;i<device_count;i++)///check for dedicated gfx cards first
    {
        gfx->physical_device=physical_devices[i];
        if(check_physical_device_appropriate(gfx,true))break;
    }

    if(i==device_count)for(i=0;i<device_count;i++)///check for dedicated gfx cards first
    {
        gfx->physical_device=physical_devices[i];
        if(check_physical_device_appropriate(gfx,false))break;
    }

    free(physical_devices);

    if(i==device_count)
    {
        fprintf(stderr,"NONE OF %d PHYSICAL DEVICES MEET REQUIREMENTS\n",device_count);
        exit(-1);
    }


    ///select screen image format
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(gfx->physical_device,gfx->surface,&format_count,NULL));
    formats=malloc(sizeof(VkSurfaceFormatKHR)*format_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(gfx->physical_device,gfx->surface,&format_count,formats));

    gfx->surface_format=formats[0];

    free(formats);


    ///select screen present mode
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gfx->physical_device,gfx->surface,&present_mode_count,NULL));
    present_modes=malloc(sizeof(VkPresentModeKHR)*present_mode_count);
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gfx->physical_device,gfx->surface,&present_mode_count,present_modes));

    gfx->surface_present_mode=VK_PRESENT_MODE_FIFO_KHR;
//    gfx->surface_present_mode=VK_PRESENT_MODE_IMMEDIATE_KHR;

    if(gfx->surface_present_mode==VK_PRESENT_MODE_FIFO_KHR)for(i=0;i<present_mode_count;i++)if(present_modes[i]==VK_PRESENT_MODE_MAILBOX_KHR) gfx->surface_present_mode=VK_PRESENT_MODE_MAILBOX_KHR;
    if(gfx->surface_present_mode==VK_PRESENT_MODE_FIFO_KHR)for(i=0;i<present_mode_count;i++)if(present_modes[i]==VK_PRESENT_MODE_FIFO_RELAXED_KHR) gfx->surface_present_mode=VK_PRESENT_MODE_FIFO_RELAXED_KHR;

    free(present_modes);
}

static void initialise_gfx_logical_device(gfx_data * gfx)
{
    const char * device_extensions[]={"VK_KHR_swapchain"};
    const char * layer_names[]={"VK_LAYER_LUNARG_standard_validation"};///VK_LAYER_LUNARG_standard_validation
    float queue_priorities[16]={1.0};

    ///try copying device_features from gfx until appropriate list of required features found
    VkPhysicalDeviceFeatures features=(VkPhysicalDeviceFeatures){};
    features.geometryShader=VK_TRUE;
    features.multiDrawIndirect=VK_TRUE;
    features.sampleRateShading=VK_TRUE;
    features.fillModeNonSolid=VK_TRUE;
    features.fragmentStoresAndAtomics=VK_TRUE;
//    features=gfx->device_features;


    VkDeviceQueueCreateInfo device_queue_creation_info=(VkDeviceQueueCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueFamilyIndex=gfx->queue_family,
        .queueCount=1,
        .pQueuePriorities=queue_priorities
    };

    VkDeviceCreateInfo device_creation_info=(VkDeviceCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .queueCreateInfoCount=1,
        .pQueueCreateInfos= &device_queue_creation_info,
        .enabledLayerCount=1,///make 0 for release version (no validation), test performance diff
        .ppEnabledLayerNames=layer_names,
        .enabledExtensionCount=1,
        .ppEnabledExtensionNames=device_extensions,
        .pEnabledFeatures= &features
    };

    CVM_VK_CHECK(vkCreateDevice(gfx->physical_device,&device_creation_info,NULL,&gfx->device));

    vkGetDeviceQueue(gfx->device,gfx->queue_family,0,&gfx->queue);
}

static void initialise_gfx_render_pass(gfx_data * gfx)
{
    ///render pass (requires format of surface so must go after swapchain creation)
    VkAttachmentDescription attachment_description=(VkAttachmentDescription)
    {
        .flags=0,
        .format=gfx->surface_format.format,
        .samples=VK_SAMPLE_COUNT_1_BIT,
        .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,///VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,//contents are disregarded anyway but this gets incorrectly detected as a bug by validation layer?
        .finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
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

    VkRenderPassCreateInfo render_pass_creation_info=(VkRenderPassCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .attachmentCount=1,
        .pAttachments= &attachment_description,
        .subpassCount=1,
        .pSubpasses= &subpass_description,
        .dependencyCount=0,
        .pDependencies=NULL
    };

    CVM_VK_CHECK(vkCreateRenderPass(gfx->device,&render_pass_creation_info,NULL,&gfx->render_pass));
}

static void initialise_gfx_command_pool(gfx_data * gfx)
{
//    #warning VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT probably not necessary, shouldnt be reseting command buffers to 0 size?
//    ///or am I wrong on how this works, does changing data require reset, if so what data? instanced render input? draw count data? vertex data?
    VkCommandPoolCreateInfo command_pool_create_info=(VkCommandPoolCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext=NULL,
        .flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,/// necessary?
        .queueFamilyIndex=gfx->queue_family
    };

    CVM_VK_CHECK(vkCreateCommandPool(gfx->device,&command_pool_create_info,NULL,&gfx->command_pool));
}

static void initialise_gfx_synchronisation(gfx_data * gfx)
{
    VkSemaphoreCreateInfo semaphore_create_info=(VkSemaphoreCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext=NULL,
        .flags=0
    };

    CVM_VK_CHECK(vkCreateSemaphore(gfx->device,&semaphore_create_info,NULL,&gfx->acquire_image_semaphore));
    CVM_VK_CHECK(vkCreateSemaphore(gfx->device,&semaphore_create_info,NULL,&gfx->render_finished_semaphore));


    VkFenceCreateInfo fence_create_info=(VkFenceCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext=NULL,
        .flags=VK_FENCE_CREATE_SIGNALED_BIT
    };

    CVM_VK_CHECK(vkCreateFence(gfx->device,&fence_create_info,NULL,&gfx->submission_finished_fence));
}



static void initialise_gfx_presentation_instance(gfx_data * gfx,gfx_presentation_instance * gfx_pi,VkImage swapchain_image,VkExtent2D surface_extent)
{
    VkImageViewCreateInfo image_view_create_info=(VkImageViewCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .image=swapchain_image,
        .viewType=VK_IMAGE_VIEW_TYPE_2D,
        .format=gfx->surface_format.format,
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

    CVM_VK_CHECK(vkCreateImageView(gfx->device,&image_view_create_info,NULL,&gfx_pi->image_view));

    VkFramebufferCreateInfo framebuffer_create_info=(VkFramebufferCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .renderPass=gfx->render_pass,
        .attachmentCount=1,
        .pAttachments= &gfx_pi->image_view,
        .width=surface_extent.width,
        .height=surface_extent.height,
        .layers=1
    };

    CVM_VK_CHECK(vkCreateFramebuffer(gfx->device,&framebuffer_create_info,NULL,&gfx_pi->framebuffer));


    ///command buffer allocation
    VkCommandBufferAllocateInfo command_buffer_allocate_info=(VkCommandBufferAllocateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext=NULL,
        .commandPool=gfx->command_pool,
        .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount=1
    };

    CVM_VK_CHECK(vkAllocateCommandBuffers(gfx->device,&command_buffer_allocate_info,&gfx_pi->command_buffer));


    ///command buffer filling

    VkClearValue clear_value=(VkClearValue)
    {
        .color=(VkClearColorValue){{0.95f,0.5f,0.75f,1.0f}}
    };

    VkCommandBufferBeginInfo command_buffer_begin_info=(VkCommandBufferBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext=NULL,
        .flags=0,/// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT  necessary? in tutorial bet shouldn't be used?
        .pInheritanceInfo=NULL
    };

    VkRenderPassBeginInfo render_pass_begin_info=(VkRenderPassBeginInfo)
    {
        .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext=NULL,
        .renderPass=gfx->render_pass,
        .framebuffer=gfx_pi->framebuffer,
        .renderArea=gfx->screen_rectangle,
        .clearValueCount=1,
        .pClearValues= &clear_value
    };

    CVM_VK_CHECK(vkBeginCommandBuffer(gfx_pi->command_buffer,&command_buffer_begin_info));

    vkCmdBeginRenderPass(gfx_pi->command_buffer,&render_pass_begin_info,VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(gfx_pi->command_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,gfx->test_pipeline);

    vkCmdDraw(gfx_pi->command_buffer,3,1,0,0);

    vkCmdEndRenderPass(gfx_pi->command_buffer);

    CVM_VK_CHECK(vkEndCommandBuffer(gfx_pi->command_buffer));



}

static void free_gfx_presentation_instance(gfx_data * gfx,gfx_presentation_instance * gfx_pi)
{
    vkFreeCommandBuffers(gfx->device,gfx->command_pool,1,&gfx_pi->command_buffer);
    vkDestroyFramebuffer(gfx->device,gfx_pi->framebuffer,NULL);
    vkDestroyImageView(gfx->device,gfx_pi->image_view,NULL);
}

void initialise_gfx_swapchain(gfx_data * gfx)
{
    uint32_t i;
    VkSurfaceCapabilitiesKHR surface_capabilities;

    ///swapchain
    CVM_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gfx->physical_device,gfx->surface,&surface_capabilities));

    gfx->screen_rectangle.extent=surface_capabilities.currentExtent;

    VkSwapchainCreateInfoKHR swapchain_create_info=(VkSwapchainCreateInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext=NULL,
        .flags=0,
        .surface=gfx->surface,
        .minImageCount=surface_capabilities.minImageCount,
        .imageFormat=gfx->surface_format.format,
        .imageColorSpace=gfx->surface_format.colorSpace,
        .imageExtent=surface_capabilities.currentExtent,
        .imageArrayLayers=1,
        .imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode=VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount=0,
        .pQueueFamilyIndices=NULL,
        .preTransform=surface_capabilities.currentTransform,
        .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode=gfx->surface_present_mode,
        .clipped=VK_TRUE,
        .oldSwapchain=gfx->swapchain
    };

    CVM_VK_CHECK(vkCreateSwapchainKHR(gfx->device,&swapchain_create_info,NULL,&gfx->swapchain));


    ///vkDestroyPipeline(gfx->device,gfx->test_pipeline,NULL); only call upon destruction/recreation, not on initial creation
    create_pipeline(gfx,"shaders/test_vert.spv",NULL,"shaders/test_frag.spv");




    ///swapchain present instances (for each swapchain buffer image)
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(gfx->device,gfx->swapchain,&gfx->presentation_instance_count,NULL));
    VkImage * swapchain_images=malloc(sizeof(swapchain_images)*gfx->presentation_instance_count);
    CVM_VK_CHECK(vkGetSwapchainImagesKHR(gfx->device,gfx->swapchain,&gfx->presentation_instance_count,swapchain_images));

    gfx->presentation_instances=malloc(sizeof(gfx_presentation_instance)*gfx->presentation_instance_count);

    for(i=0;i<gfx->presentation_instance_count;i++)initialise_gfx_presentation_instance(gfx,gfx->presentation_instances+i,swapchain_images[i],surface_capabilities.currentExtent);

    free(swapchain_images);
}

static void free_gfx_swapchain_present_data(gfx_data * gfx)
{
    uint32_t i;

    for(i=0;i<gfx->presentation_instance_count;i++)free_gfx_presentation_instance(gfx,gfx->presentation_instances+i);

    free(gfx->presentation_instances);
}

void recreate_gfx_swapchain(gfx_data * gfx)
{
    CVM_VK_CHECK(vkDeviceWaitIdle(gfx->device));

    VkSwapchainKHR old_swapchain=gfx->swapchain;
    free_gfx_swapchain_present_data(gfx);///need free/recreate in case gfx->presentation_data_count changes

    vkDestroyPipeline(gfx->device,gfx->test_pipeline,NULL);

    initialise_gfx_swapchain(gfx);
    vkDestroySwapchainKHR(gfx->device,old_swapchain,NULL);
}





gfx_data * create_gfx_data(SDL_Window * window)
{
    gfx_data * gfx=malloc(sizeof(gfx_data));

    gfx->screen_rectangle.offset.x=0;
    gfx->screen_rectangle.offset.y=0;

    gfx->update_count=0;

    initialise_gfx_instance(gfx,window);
    initialise_gfx_surface(gfx,window);
    initialise_gfx_physical_device(gfx);
    initialise_gfx_logical_device(gfx);

    initialise_gfx_render_pass(gfx);///move to separate function w/ per element (engine,backend) subpass provided extenally? (may not be needed if this is only rendering to screen, (only 1 attachment and operation type) )
    initialise_gfx_command_pool(gfx);
    initialise_gfx_synchronisation(gfx);



    gfx->swapchain=VK_NULL_HANDLE;///so that initialise_gfx_swapchain has correct value

    return gfx;
}

void destroy_gfx_data(gfx_data * gfx)
{
    vkDeviceWaitIdle(gfx->device);

    free_gfx_swapchain_present_data(gfx);
    vkDestroySwapchainKHR(gfx->device,gfx->swapchain,NULL);

    vkDestroyPipeline(gfx->device,gfx->test_pipeline,NULL);
    vkDestroyRenderPass(gfx->device,gfx->render_pass,NULL);
    vkDestroyFence(gfx->device,gfx->submission_finished_fence,NULL);
    vkDestroySemaphore(gfx->device,gfx->acquire_image_semaphore,NULL);
    vkDestroySemaphore(gfx->device,gfx->render_finished_semaphore,NULL);
    vkDestroyCommandPool(gfx->device,gfx->command_pool,NULL);
    vkDestroyDevice(gfx->device,NULL);
    vkDestroyInstance(gfx->instance,NULL);

    free(gfx);
}


bool present_gfx_data(gfx_data * gfx)
{
    uint32_t image_index;
    VkResult r;

    VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

    r=vkAcquireNextImageKHR(gfx->device,gfx->swapchain,1000000000,gfx->acquire_image_semaphore,VK_NULL_HANDLE,&image_index);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't use swapchain");//recreate_gfx_swapchain(gfx);///rebuild swapchain dependent elements for this buffer
        return false;
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"ACQUIRE NEXT IMAGE FAILED WITH ERROR : %d\n",r);



    ///seperate command bufer for each render element (game/overlay) ?

    ///fence to prevent re-submission of same command buffer ( command buffer wasn't created with VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT )


    ///before  submit/present  ensure update_count associated with incoming command buffer matches the current value (so as to ensure that pipelines &c. have not been deleted)
    ///otherwise just have primary command buffer clear screen

    ///alter primary command buffer

    ///

    ///only delete pipelines & display images between mutexes in main thread (assuming multi-threading)


//    VkImageMemoryBarrier barrier_from_present_to_draw = {
//      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     // VkStructureType                sType
//      nullptr,                                    // const void                    *pNext
//      VK_ACCESS_MEMORY_READ_BIT,                  // VkAccessFlags                  srcAccessMask
//      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,       // VkAccessFlags                  dstAccessMask
//      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // VkImageLayout                  oldLayout
//      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // VkImageLayout                  newLayout
//      GetPresentQueue().FamilyIndex,              // uint32_t                       srcQueueFamilyIndex
//      GetGraphicsQueue().FamilyIndex,             // uint32_t                       dstQueueFamilyIndex
//      swap_chain_images[i],                       // VkImage                        image
//      image_subresource_range                     // VkImageSubresourceRange        subresourceRange
//    };
//    vkCmdPipelineBarrier( Vulkan.GraphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_present_to_draw );

//    VkImageMemoryBarrier barrier_from_draw_to_present = {
//      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,       // VkStructureType              sType
//      nullptr,                                      // const void                  *pNext
//      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,         // VkAccessFlags                srcAccessMask
//      VK_ACCESS_MEMORY_READ_BIT,                    // VkAccessFlags                dstAccessMask
//      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,              // VkImageLayout                oldLayout
//      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,              // VkImageLayout                newLayout
//      GetGraphicsQueue().FamilyIndex,               // uint32_t                     srcQueueFamilyIndex
//      GetPresentQueue( ).FamilyIndex,               // uint32_t                     dstQueueFamilyIndex
//      swap_chain_images[i],                         // VkImage                      image
//      image_subresource_range                       // VkImageSubresourceRange      subresourceRange
//    };
//    vkCmdPipelineBarrier( Vulkan.GraphicsCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_draw_to_present );



    VkSubmitInfo submit_info=(VkSubmitInfo)
    {
        .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext=NULL,
        .waitSemaphoreCount=1,
        .pWaitSemaphores= &gfx->acquire_image_semaphore,
        .pWaitDstStageMask= &wait_dst_stage_mask,
        .commandBufferCount=1,
        .pCommandBuffers= &gfx->presentation_instances[image_index].command_buffer,
        .signalSemaphoreCount=1,
        .pSignalSemaphores= &gfx->render_finished_semaphore
    };





    CVM_VK_CHECK(vkWaitForFences(gfx->device,1,&gfx->submission_finished_fence,VK_TRUE,1000000000));
    CVM_VK_CHECK(vkResetFences(gfx->device,1,&gfx->submission_finished_fence));
    CVM_VK_CHECK(vkQueueSubmit(gfx->queue,1,&submit_info,gfx->submission_finished_fence));



    ///if separate queue/queue_families for present & gfx then only this needs be on present queue?



    VkPresentInfoKHR present_info=(VkPresentInfoKHR)
    {
        .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext=NULL,
        .waitSemaphoreCount=1,
        .pWaitSemaphores= &gfx->render_finished_semaphore,
        .swapchainCount=1,
        .pSwapchains= &gfx->swapchain,
        .pImageIndices= &image_index,
        .pResults=NULL
    };

    r=vkQueuePresentKHR(gfx->queue,&present_info);
    if(r==VK_ERROR_OUT_OF_DATE_KHR)/// if VK_SUBOPTIMAL_KHR then image WAS acquired and so must be displayed
    {
        puts("couldn't use swapchain");//recreate_gfx_swapchain(gfx);///rebuild swapchain dependent elements for this buffer
        return false;
    }
    else if(r!=VK_SUCCESS) fprintf(stderr,"PRESENTATION FAILED WITH ERROR : %d\n",r);



//    CVM_VK_CHECK(vkDeviceWaitIdle(gfx->device));

    return true;
}





VkShaderModule load_shader_data(gfx_data * gfx,const char * filename)
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

        if(vkCreateShaderModule(gfx->device,&shader_module_create_info,NULL,&shader_module)!=VK_SUCCESS)
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


void create_pipeline(gfx_data * gfx,const char * vert_file,const char * geom_file,const char * frag_file)
{
    ///load test shaders

    /// "shaders/test_vert.spv" "shaders/test_frag.spv"

    VkShaderModule stage_modules[16];
    VkPipelineShaderStageCreateInfo stage_creation_info[16];
    VkPipelineLayout layout;
    uint32_t i,stage_count=0;

    if(vert_file)
    {
        stage_modules[stage_count]=load_shader_data(gfx,vert_file);
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

    if(frag_file)
    {
        stage_modules[stage_count]=load_shader_data(gfx,frag_file);
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

    VkPipelineVertexInputStateCreateInfo vert_input_info=(VkPipelineVertexInputStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .vertexBindingDescriptionCount=0,
        .pVertexBindingDescriptions=NULL,
        .vertexAttributeDescriptionCount=0,
        .pVertexAttributeDescriptions=NULL
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info=(VkPipelineInputAssemblyStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable=VK_FALSE
    };


    VkViewport viewport=(VkViewport)
    {
        .x=0.0,
        .y=0.0,
        .width=(float)gfx->screen_rectangle.extent.width,
        .height=(float)gfx->screen_rectangle.extent.height,
        .minDepth=0.0,
        .maxDepth=1.0
    };

    VkPipelineViewportStateCreateInfo viewport_state_info=(VkPipelineViewportStateCreateInfo)
    {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext=NULL,
        .flags=0,
        .viewportCount=1,
        .pViewports= &viewport,
        .scissorCount=1,
        .pScissors= &gfx->screen_rectangle
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

    CVM_VK_CHECK(vkCreatePipelineLayout(gfx->device,&layout_creation_info,NULL,&layout));

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
        .renderPass=gfx->render_pass,
        .subpass=0,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=-1
    };

    CVM_VK_CHECK(vkCreateGraphicsPipelines(gfx->device,VK_NULL_HANDLE,1,&pipeline_creation_info,NULL,&gfx->test_pipeline));

    vkDestroyPipelineLayout(gfx->device,layout,NULL);

    for(i=0;i<stage_count;i++)vkDestroyShaderModule(gfx->device,stage_modules[i],NULL);
}











