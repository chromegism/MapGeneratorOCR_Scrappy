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
	std::array<VkCommandBuffer, 3> commandBuffers;

	commandBuffers[0] = transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandPool);
	commandBuffers[1] = beginCommand(device_->handle(), commandPool);

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

	vkCmdCopyBufferToImage(commandBuffers[1], other.handle(), handle_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	endCommand(commandBuffers[1]);

	commandBuffers[2] = transitionLayout(layout_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandPool);
	submitCommands(deviceHandle(), commandPool, device().graphicsQueue().handle(), commandBuffers);
}

VkCommandBuffer Image::transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool) {
	VkCommandBuffer commandBuffer = beginCommand(device_->handle(), commandPool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = handle_;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endCommand(commandBuffer);

	layout_ = newLayout;

	return commandBuffer;
}