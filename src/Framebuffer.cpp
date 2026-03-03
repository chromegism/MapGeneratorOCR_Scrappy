#include "pch.h"

#include "Framebuffer.h"
#include "Tools.h"

void Framebuffer::genFramebuffer(uint32_t width, uint32_t height) {
	std::array<VkImageView, 2> attachments = {
			swapView_,
			depthView_
	};

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass_;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	VkResult error_code = vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &handle_);
	handleVkResult(error_code, "Failed to create framebuffer");
}