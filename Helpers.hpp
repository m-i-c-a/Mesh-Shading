#include <vulkan/vulkan.h>

VkShaderModule createShaderModule(VkDevice device, const char *filename);
VkSemaphore createSemaphore(VkDevice device);
VkFence createFence(VkDevice device, bool signaled);
VkCommandBuffer createCommandBuffer(VkDevice device, VkCommandPool pool);
VkCommandPool createCommandPool(VkDevice device, uint32_t queueFamilyIdx);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t numMipLevels);