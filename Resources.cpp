#include <string.h>

#include "Resources.hpp"
#include "Defines.hpp"

static VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties {};

static uint32_t getHeapIdx(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryProperties)
{
	// Iterate over all memory types available for the device used in this example
	for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
	{
		if (memoryTypeBits & (1 << i) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties)
		{
			return i;
		}
	}

    assert( false && "Could not find suitable memory type!");
    return 0;
}

void setPhysicalDeviceMemoryProperties(const VkPhysicalDeviceMemoryProperties& _physicalDeviceMemoryProperties)
{
    physicalDeviceMemoryProperties = _physicalDeviceMemoryProperties;
}

void createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, Buffer& buffer)
{
    const VkBufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VK_CHECK(vkCreateBuffer(device, &createInfo, nullptr, &buffer.buffer));

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memReqs);

    const VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = getHeapIdx(memReqs.memoryTypeBits, memoryProperties)
    };

    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &buffer.memory));

    VK_CHECK(vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0));
}

void uploadToBuffer(VkDevice device, const Buffer& buffer, VkDeviceSize size, VkDeviceSize offset, void* data)
{
    // TODO : store mapped pointer
    void* mappedData = nullptr;
    vkMapMemory(device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &mappedData);
    memcpy(mappedData + offset, data, size);

    VkMappedMemoryRange range{
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = buffer.memory,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };

    vkFlushMappedMemoryRanges(device, 1, &range);
    vkUnmapMemory(device, buffer.memory);
}

void uploadBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, const Buffer& stagingBuffer, const Buffer& dstBuffer, VkDeviceSize size, void* data)
{
    void* stagingData = nullptr;
    vkMapMemory(device, stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &stagingData);
    memcpy(stagingData, data, size);

    VkMappedMemoryRange range{
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = stagingBuffer.memory,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };

    vkFlushMappedMemoryRanges(device, 1, &range);
    vkUnmapMemory(device, stagingBuffer.memory);

    static const VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

    const VkBufferCopy buffer_copy {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, dstBuffer.buffer, 1u, &buffer_copy);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    const VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1u,
        .pCommandBuffers = &commandBuffer
    };

    VK_CHECK(vkQueueSubmit(queue, 1u, &submitInfo, VK_NULL_HANDLE));

    vkQueueWaitIdle(queue);
    vkResetCommandPool(device, commandPool,  0x0);
}

void destroyBuffer(VkDevice device, Buffer& buffer)
{
    vkFreeMemory(device, buffer.memory, nullptr);
    vkDestroyBuffer(device, buffer.buffer, nullptr);

    buffer.memory = VK_NULL_HANDLE;
    buffer.buffer = VK_NULL_HANDLE;
}

void createAttachment(const VkDevice device, const VkFormat format, const VkExtent3D extent, VkImageUsageFlags usage, VkImageAspectFlags aspectMask, Attachment& attachment)
{
    const VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VK_CHECK(vkCreateImage(device, &imageCreateInfo, nullptr, &attachment.image));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, attachment.image, &memReqs);

    const VkMemoryAllocateInfo allocateInfo = { 
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = getHeapIdx(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &attachment.memory));
    VK_CHECK(vkBindImageMemory(device, attachment.image, attachment.memory, 0));

    const VkImageViewCreateInfo imageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = attachment.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &attachment.view));
}

void destroyAttachment(VkDevice device, Attachment& attachment)
{
    vkDestroyImage(device, attachment.image, nullptr);
    vkFreeMemory(device, attachment.memory, nullptr);
    vkDestroyImageView(device, attachment.view, nullptr);

    attachment.image = VK_NULL_HANDLE;
    attachment.memory = VK_NULL_HANDLE;
    attachment.view = VK_NULL_HANDLE;
}