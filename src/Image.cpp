#include "pch.h"

#include "Image.h"
#include "Tools.h"

VkImageView generateImageView(VkDevice _device, VkImage _image, VkFormat _format, VkImageAspectFlags _aspectFlags) {
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = _image;

	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = _format;

	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	createInfo.subresourceRange.aspectMask = _aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView view;
	VkResult error_code = vkCreateImageView(_device, &createInfo, nullptr, &view);
	handleVkResult(error_code, "Failed to create swapchain image view");

	return view;
}

void SwapchainImage::genImageView(VkFormat _format) {
	view_ = generateImageView(deviceHandle_, handle_, _format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Image::genImage(VkPhysicalDevice _physicalDevice, VkImageUsageFlags usage, VkSampleCountFlagBits samples) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = extent_.width;
	imageInfo.extent.height = extent_.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format_;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = samples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult error_code = vkCreateImage(device_->handle(), &imageInfo, nullptr, &handle_);
	handleVkResult(error_code, "Failed to create image");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device_->handle(), handle_, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(_physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	error_code = vkAllocateMemory(device_->handle(), &allocInfo, nullptr, &memory_);
	handleVkResult(error_code, "Failed to allocate image memory");

	vkBindImageMemory(device_->handle(), handle_, memory_, 0);
}

void Image::genImageView(VkFormat _format, VkImageAspectFlags _aspectFlags) {
	view_ = generateImageView(device_->handle(), handle_, _format, _aspectFlags);
}

void Image::copyBuffer(const Buffer& other, VkCommandPool commandPool) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device_->handle(), commandPool);

	VkBufferImageCopy copyRegion{};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;

	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = { extent_.width, extent_.height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, other.handle(), handle_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	endSingleTimeCommands(device_->handle(), commandPool, device_->graphicsQueue().handle(), commandBuffer);
}