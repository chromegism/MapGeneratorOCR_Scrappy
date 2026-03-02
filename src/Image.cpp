#include "pch.h"

#include "Image.h"
#include "Tools.h"

void SwapchainImage::genImageView(VkFormat _format) {
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = handle_;

	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = _format;

	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkResult error_code = vkCreateImageView(deviceHandle_, &createInfo, nullptr, &view_);
	handleVkResult(error_code, "Failed to create swapchain image view");
}