#ifndef VKM_INIT_HPP
#define VKM_INIT_HPP

#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "vkmEnums.h"
#include "Resources.hpp"

struct Meshlet
{
    uint32_t vertices[64];
    uint8_t indices[126];
    uint8_t triangleCount;
    uint8_t vertexCount;
};

enum class SupportedDeviceFeature
{
    eSynchronization2   = 0,
    eDescriptorIndexing = 1,
    eMeshShadingNV      = 2,
    eInvalidFeature     
};

struct vkmInitParams
{
    GLFWwindow* window;
    uint32_t windowWidth;
    uint32_t windowHeight;

    std::vector<const char *> requestedInstanceExtensions;
    std::vector<const char *> requestedInstanceLayers;

    std::vector<const char *>           requestedDeviceExtensions;
    std::vector<SupportedDeviceFeature> requestedDeviceFeatures;

    std::vector<VkQueueFlagBits> requestedQueueTypes;
    std::vector<float> requestedQueuePriorities;

    uint32_t requestedSwapchainImageCount;
    VkFormat requestedSwapchainFormat;
    VkPresentModeKHR requestedSwapchainPresentMode;
};

struct Swapchain
{
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
};

struct VulkanResources
{
    // Vulkan Core Specific
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    VkDevice device;
    std::vector<uint32_t> queueFamilyIndices;
    std::vector<VkQueue> queues;
    Swapchain swapchain;

    // App Specific
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkCommandPool commandPools[COMMAND_POOL_COUNT];
    VkCommandBuffer commandBuffers[COMMAND_BUFFER_COUNT];
    VkDescriptorPool descriptorPools[DESCRIPTOR_POOL_COUNT];
    VkDescriptorSetLayout descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_COUNT];
    VkDescriptorSet descriptorSets[DESCRIPTOR_SET_COUNT];
    VkSemaphore semaphores[SEMAPHORE_COUNT];
    VkFence fences[FENCE_COUNT];
    Buffer buffers[BUFFER_COUNT];
    uint32_t currentSwapchainImageIdx = 0;
    Attachment attachments[ATTACHMENT_COUNT];
    VkImageView imageViews[IMAGE_VIEW_COUNT];
    std::vector<Meshlet> meshlets[BUFFER_COUNT];
};


VulkanResources vkmInit(const vkmInitParams& params);
void vkmDestroy(VulkanResources& resources);

#endif // VKM_INIT_HPP