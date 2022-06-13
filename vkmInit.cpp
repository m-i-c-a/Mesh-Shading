
#include "vkmInit.hpp"
#include "Defines.hpp"
#include "vkmDeviceFeatureManager.hpp"

static VkInstance createInstance(const std::vector<const char *> &extensions, const std::vector<const char *> &layers)
{
    constexpr VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_2};

    const VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkInstance instance;
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
    return instance;
}

static VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    return surface;
}

static VkPhysicalDevice selectPhysicalDevice(VkInstance instance)
{
    uint32_t numPhysicalDevice = 0;
    vkEnumeratePhysicalDevices(instance, &numPhysicalDevice, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevice);
    vkEnumeratePhysicalDevices(instance, &numPhysicalDevice, physicalDevices.data());

    LOG("# Physical Devices: %u\n", numPhysicalDevice);
    for (uint32_t i = 0; i < numPhysicalDevice; ++i)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
        LOG("%i : %s\n", i, props.deviceName);
    }

    const uint32_t physicalDeviceIndex = 2u;
    LOG("Using Physical Device %u\n\n", physicalDeviceIndex);

    return physicalDevices[physicalDeviceIndex];
}

static std::vector<uint32_t> selectQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<VkQueueFlagBits> &queueFlags)
{
    uint32_t numQueueFamilyProperties = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilyProperties, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(numQueueFamilyProperties);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilyProperties, queueFamilyProperties.data());

    auto getQueueFamilyIdx = [&](const VkQueueFlagBits queueFlag, const bool present)
    {
        // Dedicated queue for compute
        // Try to find a queue family index that supports compute but not graphics
        if (queueFlag & VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & queueFlag) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // Dedicated queue for transfer
        // Try to find a queue family index that supports transfer but not graphics and compute
        if (queueFlag & VK_QUEUE_TRANSFER_BIT)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & queueFlag) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
        {
            if (queueFamilyProperties[i].queueFlags & queueFlag)
            {
                if (present)
                {
                    VkBool32 q_fam_supports_present = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &q_fam_supports_present);
                    if (q_fam_supports_present)
                        return i;
                }
                else
                {
                    return i;
                }
            }
        }

        EXIT("Could not find a matching queue family index");
    };

    std::vector<uint32_t> queueFamilyIndices(queueFlags.size(), 0x0);

    for (uint32_t i = 0; i < queueFlags.size(); ++i)
    {
        queueFamilyIndices[i] = getQueueFamilyIdx(queueFlags[i], (queueFlags[i] & VK_QUEUE_GRAPHICS_BIT) ? true : false);
    }

    return queueFamilyIndices;
}

static VkDevice createDevice(VkPhysicalDevice physicalDevice, const std::vector<uint32_t> &queueFamilyIndices, const std::vector<const char*> &requestedDeviceExtensions, const std::vector<SupportedDeviceFeature>& requestedDeviceFeatures)
{
    const float q_priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(queueFamilyIndices.size());
    for (const uint32_t idx : queueFamilyIndices)
    {
        const VkDeviceQueueCreateInfo queueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = idx,
            .queueCount = 1u,
            .pQueuePriorities = &q_priority};

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vkmDeviceFeatureManager deviceFeatureManager(requestedDeviceFeatures);

    const VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = deviceFeatureManager.getPNext(),
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = 0u,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(requestedDeviceExtensions.size()),
        .ppEnabledExtensionNames = requestedDeviceExtensions.data(),
        .pEnabledFeatures = nullptr};

    VkDevice device;
    VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
    return device;
}

static std::vector<VkQueue> getQueues(VkDevice device, const std::vector<uint32_t> &queueFamilyIndices)
{
    std::vector<VkQueue> queues(queueFamilyIndices.size(), VK_NULL_HANDLE);

    for (uint32_t i = 0; i < queueFamilyIndices.size(); ++i)
    {
        vkGetDeviceQueue(device, queueFamilyIndices[i], 0, &queues[i]);
    }

    return queues;
}

static Swapchain createSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t imageCount, VkFormat format, VkExtent2D extent, VkPresentModeKHR presentMode)
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surface_capabilities));
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.surface = surface;

    //** Image Count
    {
        assert(imageCount > 0 && "Invalid requested image count for swapchain!");

        // If the minImageCount is 0, then there is not a limit on the number of images the swapchain
        // can support (ignoring memory constraints). See the Vulkan Spec for more information.
        if (surface_capabilities.maxImageCount == 0)
        {
            if (imageCount >= surface_capabilities.minImageCount)
            {
                swapchainCreateInfo.minImageCount = imageCount;
            }
            else
            {
                LOG("Failed to create Swapchain. The requested number of images %u does not meet the minimum requirement of %u\n", imageCount, surface_capabilities.minImageCount);
                exit(EXIT_FAILURE);
            }
        }
        else if (imageCount >= surface_capabilities.minImageCount &&
                 imageCount <= surface_capabilities.maxImageCount)
        {
            swapchainCreateInfo.minImageCount = imageCount;
        }
        else
        {
            LOG("The number of requested Swapchain images %u is not supported. Min: %u Max: %u\n", imageCount, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
            exit(EXIT_FAILURE);
        }
    }

    //** Image Format
    {
        uint32_t num_supported_surface_formats = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &num_supported_surface_formats, nullptr));
        std::vector<VkSurfaceFormatKHR> supported_surface_formats(num_supported_surface_formats);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &num_supported_surface_formats, supported_surface_formats.data()));

        bool requested_format_found = false;
        for (uint32_t i = 0; i < num_supported_surface_formats; ++i)
        {
            if (supported_surface_formats[i].format == format)
            {
                swapchainCreateInfo.imageFormat = supported_surface_formats[i].format;
                swapchainCreateInfo.imageColorSpace = supported_surface_formats[i].colorSpace;
                requested_format_found = true;
                break;
            }
        }

        if (!requested_format_found)
        {
            LOG("Requested swapchain format not found! Using first one avaliable!\n\n");
            swapchainCreateInfo.imageFormat = supported_surface_formats[0].format;
            swapchainCreateInfo.imageColorSpace = supported_surface_formats[0].colorSpace;
        }
    }

    //** Extent
    {
        // The Vulkan Spec states that if the current width/height is 0xFFFFFFFF, then the surface size
        // will be deteremined by the extent specified in the VkSwapchainCreateInfoKHR.
        if (surface_capabilities.currentExtent.width != (uint32_t)-1)
        {
            swapchainCreateInfo.imageExtent = extent;
        }
        else
        {
            swapchainCreateInfo.imageExtent = surface_capabilities.currentExtent;
        }
    }

    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;

    //** Pre Transform
    {
        if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            swapchainCreateInfo.preTransform = surface_capabilities.currentTransform;
            LOG("WARNING - Swapchain pretransform is not IDENTITIY_BIT_KHR!\n");
        }
    }

    //** Composite Alpha
    {
        // Determine the composite alpha format the application needs.
        // Find a supported composite alpha format (not all devices support alpha opaque),
        // but we prefer it.
        // Simply select the first composite alpha format available
        // Used for blending with other windows in the system
        VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (size_t i = 0; i < 4; ++i)
        {
            if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[i])
            {
                swapchainCreateInfo.compositeAlpha = composite_alpha_flags[i];
                break;
            };
        }
    }

    //** Present Mode
    {
        uint32_t num_supported_present_modes = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &num_supported_present_modes, nullptr));
        std::vector<VkPresentModeKHR> supported_present_modes(num_supported_present_modes);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &num_supported_present_modes, supported_present_modes.data()));

        // Determine the present mode the application needs.
        // Try to use mailbox, it is the lowest latency non-tearing present mode
        // All devices support FIFO (this mode waits for the vertical blank or v-sync)
        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (uint32_t i = 0; i < num_supported_present_modes; ++i)
        {
            if (supported_present_modes[i] == presentMode)
            {
                swapchainCreateInfo.presentMode = presentMode;
                break;
            }
        }
    }

    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create Swapchain
    Swapchain swapchain {};
    swapchain.extent = swapchainCreateInfo.imageExtent;
    swapchain.format = swapchainCreateInfo.imageFormat;
    VK_CHECK(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain.swapchain));

    // Get Swapchain Images
    uint32_t numSwapchainImages = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain.swapchain, &numSwapchainImages, nullptr));
    swapchain.images.resize(numSwapchainImages);
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain.swapchain, &numSwapchainImages, swapchain.images.data()));
    LOG("Swapchain Image Count: %u\n", numSwapchainImages);


    // Create Swapchain ImageViews
    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchain.format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};

    swapchain.imageViews.resize(swapchain.images.size());

    for (size_t i = 0; i < swapchain.images.size(); ++i)
    {
        imageViewCreateInfo.image = swapchain.images[i];
        VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchain.imageViews[i]));
    }

    return swapchain;
}

VulkanResources vkmInit(const vkmInitParams& params)
{
    VulkanResources resources {};
    resources.instance = createInstance(params.requestedInstanceExtensions, params.requestedInstanceLayers);
    resources.surface = createSurface(resources.instance, params.window);
    resources.physicalDevice = selectPhysicalDevice(resources.instance);
    resources.queueFamilyIndices = selectQueueFamilyIndices(resources.physicalDevice, resources.surface, params.requestedQueueTypes);
    resources.device = createDevice(resources.physicalDevice, resources.queueFamilyIndices, params.requestedDeviceExtensions, params.requestedDeviceFeatures);
    resources.queues = getQueues(resources.device, resources.queueFamilyIndices);
    resources.swapchain = createSwapchain(resources.device, resources.physicalDevice, resources.surface, params.requestedSwapchainImageCount, params.requestedSwapchainFormat, { params.windowWidth, params.windowHeight }, params.requestedSwapchainPresentMode);

    vkGetPhysicalDeviceMemoryProperties(resources.physicalDevice, &resources.physicalDeviceMemoryProperties);

    return resources;
}

void vkmDestroy(VulkanResources& resources)
{
    for (Buffer& buffer : resources.buffers)
        destroyBuffer(resources.device, buffer);

    for (size_t i = 0; i < DESCRIPTOR_SET_LAYOUT_COUNT; ++i)
        vkDestroyDescriptorSetLayout(resources.device, resources.descriptorSetLayouts[i], nullptr);

    for (size_t i = 0; i < DESCRIPTOR_POOL_COUNT; ++i)
        vkDestroyDescriptorPool(resources.device, resources.descriptorPools[i], nullptr);

    for (size_t i = 0; i < COMMAND_POOL_COUNT; ++i)
        vkDestroyCommandPool(resources.device, resources.commandPools[i], nullptr);

    for (size_t i = 0; i < SEMAPHORE_COUNT; ++i)
        vkDestroySemaphore(resources.device, resources.semaphores[i], nullptr);

    for (size_t i = 0; i < FENCE_COUNT; ++i)
        vkDestroyFence(resources.device, resources.fences[i], nullptr);

    vkDestroyPipelineLayout(resources.device, resources.pipelineLayout, nullptr);
    vkDestroyPipeline(resources.device, resources.pipeline, nullptr);

    for (size_t i = 0; i < resources.framebuffers.size(); ++i)
        vkDestroyFramebuffer(resources.device, resources.framebuffers[i], nullptr);

    vkDestroyRenderPass(resources.device, resources.renderPass, nullptr);

    for (uint32_t i = 0; i < resources.swapchain.images.size(); ++i)
        vkDestroyImageView(resources.device, resources.swapchain.imageViews[i], nullptr);

    vkDestroySwapchainKHR(resources.device, resources.swapchain.swapchain, nullptr);
    vkDestroyDevice(resources.device, nullptr);
    vkDestroySurfaceKHR(resources.instance, resources.surface, nullptr);
    vkDestroyInstance(resources.instance, nullptr);
}