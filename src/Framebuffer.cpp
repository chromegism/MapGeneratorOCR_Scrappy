#include "pch.h"

#include "Framebuffer.h"
#include "Tools.h"

void Framebuffer::genFramebuffer(FramebufferCreateInfo createInfo) {
	std::array<VkImageView, 2> attachments = {
			createInfo.colorView,
			createInfo.depthView
	};

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = createInfo.renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = createInfo.width;
	framebufferInfo.height = createInfo.height;
	framebufferInfo.layers = 1;

	VkResult error_code = vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &handle_);
	handleVkResult(error_code, "Failed to create framebuffer");
}