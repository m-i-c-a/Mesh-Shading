#ifndef RESOURCES_HPP
#define RESOURCES_HPP

#include <vulkan/vulkan.h>

struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
};

void setPhysicalDeviceMemoryProperties(const VkPhysicalDeviceMemoryProperties& _physicalDeviceMemoryProperties);

void createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, Buffer& buffer);
void uploadBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, const Buffer& stagingBuffer, const Buffer& dstBuffer, VkDeviceSize size, void* data);
void destroyBuffer(VkDevice device, Buffer& buffer);

#endif // RESOURCES_HPP