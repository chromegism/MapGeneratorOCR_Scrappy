#include "pch.h"

#include "Buffer.h"
#include "Tools.h"

void Buffer::genBuffer(VkDeviceSize size) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usageFlags_;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult error_code = vkCreateBuffer(device_->handle(), &bufferInfo, nullptr, &handle_);
	handleVkResult(error_code, "Failed to create buffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device_->handle(), handle_, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(device_->physicalDeviceHandle(), memRequirements.memoryTypeBits, memoryPropertyFlags_);

	error_code = vkAllocateMemory(device_->handle(), &allocInfo, nullptr, &memory_);
	handleVkResult(error_code, "Failed to allocate buffer memory");

	vkBindBufferMemory(device_->handle(), handle_, memory_, 0);
}

void Buffer::copyBuffer(const Buffer& other, VkCommandBuffer commandBuffer) {
	// Check for copy validity
	assert(usageFlags_ & VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	assert(other.usageFlags_ & VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	VkBufferCopy copyRegion{};
	copyRegion.size = size_;
	vkCmdCopyBuffer(commandBuffer, other.handle_, handle_, 1, &copyRegion);
}