#ifndef RESOURCES_HPP
#define RESOURCES_HPP

#include <vulkan/vulkan.h>

struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
};

struct Attachment
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};

void setPhysicalDeviceMemoryProperties(const VkPhysicalDeviceMemoryProperties& _physicalDeviceMemoryProperties);

void createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, Buffer& buffer);
void uploadToBuffer(VkDevice device, const Buffer& buffer, VkDeviceSize size, VkDeviceSize offset, void* data);
void uploadBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, const Buffer& stagingBuffer, const Buffer& dstBuffer, VkDeviceSize size, void* data);
void destroyBuffer(VkDevice device, Buffer& buffer);

void createAttachment(const VkDevice device, const VkFormat format, const VkExtent3D extent, VkImageUsageFlags usage, VkImageAspectFlags aspectMask, Attachment& attachment);
void destroyAttachment(VkDevice device, Attachment& attachment);

#endif // RESOURCES_HPP