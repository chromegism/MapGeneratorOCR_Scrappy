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

void Buffer::copyBuffer(const Buffer& other, VkCommandPool commandPool) {
	// Check for copy validity
	assert(usageFlags_ & VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	assert(other.usageFlags_ & VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device_->handle(), commandPool);

	VkBufferCopy copyRegion{};
	copyRegion.size = size_;
	vkCmdCopyBuffer(commandBuffer, other.handle_, handle_, 1, &copyRegion);

	endSingleTimeCommands(device_->handle(), commandPool, device_->graphicsQueue().handle(), commandBuffer);
}

void* Buffer::mapMemory(VkDeviceSize offset, VkDeviceSize size) {
	void* data;
	vkMapMemory(device_->handle(), memory_, 0, size, 0, &data);
	return data;
}
void* Buffer::mapMemory(VkDeviceSize offset) {
	return mapMemory(offset, size_);
}
void* Buffer::mapMemory() {
	return mapMemory(0, size_);
}
void Buffer::unmapMemory() {
	vkUnmapMemory(device_->handle(), memory_);
}