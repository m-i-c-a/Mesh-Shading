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