#include <vulkan/vulkan.h>

#include "Defines.hpp"

VkShaderModule createShaderModule(VkDevice device, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL)
    {
        printf("Failed to open file %s!\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    const size_t nbytes_file_size = (size_t)ftell(f);
    rewind(f);

    uint32_t *buffer = (uint32_t *)malloc(nbytes_file_size);
    fread(buffer, nbytes_file_size, 1, f);
    fclose(f);

    const VkShaderModuleCreateInfo ci_shader_module{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .codeSize = nbytes_file_size,
        .pCode = buffer,
    };

    VkShaderModule vk_shader_module;
    VkResult result = vkCreateShaderModule(device, &ci_shader_module, NULL, &vk_shader_module);
    if (result != VK_SUCCESS)
    {
        printf("Failed to create shader module for %s!\n", filename);
        exit(EXIT_FAILURE);
    }

    free(buffer);

    return vk_shader_module;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t queueFamilyIdx)
{
    const VkCommandPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .queueFamilyIndex = queueFamilyIdx};

    VkCommandPool pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateCommandPool(device, &createInfo, nullptr, &pool));
    return pool;
}

VkCommandBuffer createCommandBuffer(VkDevice device, VkCommandPool pool)
{
    const VkCommandBufferAllocateInfo commandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer));
    return commandBuffer;
}

VkFence createFence(VkDevice device, bool signaled)
{
    const VkFenceCreateInfo fenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = (signaled) ? VK_FENCE_CREATE_SIGNALED_BIT : 0u,
    };

    VkFence fence;
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    return fence;
}

VkSemaphore createSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkSemaphore semaphore;
    VK_CHECK(vkCreateSemaphore(device, &createInfo, nullptr, &semaphore));
    return semaphore;
}