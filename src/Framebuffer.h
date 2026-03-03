#pragma once

#include <vulkan/vulkan.h>

class Framebuffer {
	VkDevice device_ = VK_NULL_HANDLE;			// non owning
	VkImageView colorView_ = VK_NULL_HANDLE;		// non owning
	VkImageView depthView_ = VK_NULL_HANDLE;	// non owning
	VkRenderPass renderPass_ = VK_NULL_HANDLE;	// non owning
	VkFramebuffer handle_ = VK_NULL_HANDLE;		// owning

	void exchangeHandles(VkDevice& _device, VkImageView& _colorView, VkImageView& _depthView, VkRenderPass& _renderPass, VkFramebuffer& _handle) noexcept {
		device_ = std::exchange(_device, VK_NULL_HANDLE);
		colorView_ = std::exchange(_colorView, VK_NULL_HANDLE);
		depthView_ = std::exchange(_depthView, VK_NULL_HANDLE);
		renderPass_ = std::exchange(_renderPass, VK_NULL_HANDLE);
		handle_ = std::exchange(_handle, VK_NULL_HANDLE);
	}
	void clearHandles() noexcept {
		device_ = VK_NULL_HANDLE;
		colorView_ = VK_NULL_HANDLE;
		depthView_ = VK_NULL_HANDLE;
		renderPass_ = VK_NULL_HANDLE;
		handle_ = VK_NULL_HANDLE;
	}

public:
	struct FramebufferCreateInfo {
		uint32_t width;
		uint32_t height;
		VkImageView colorView;
		VkImageView depthView;
		VkRenderPass renderPass;
	};

private:
	void genFramebuffer(FramebufferCreateInfo createInfo);

	Framebuffer(VkDevice _device, FramebufferCreateInfo createInfo) :
		device_(_device), colorView_(createInfo.colorView), depthView_(createInfo.depthView), renderPass_(createInfo.renderPass)
	{
		genFramebuffer(createInfo);
	}

public:
	static Framebuffer createDefaultSwap(VkDevice _device, VkImageView _colorView, VkImageView _depthView, VkRenderPass _renderPass, uint32_t width, uint32_t height) {
		return Framebuffer(_device, { width, height, _colorView, _depthView, _renderPass });
	}
	static Framebuffer createDefaultSwap(VkDevice _device, FramebufferCreateInfo createInfo) {
		return Framebuffer(_device, createInfo);
	}

	Framebuffer() noexcept = default;
	Framebuffer(const Framebuffer&) noexcept = delete; // move only
	Framebuffer(Framebuffer&& other) noexcept {
		exchangeHandles(other.device_, other.colorView_, other.depthView_, other.renderPass_, other.handle_);
		other.clearHandles();
	}
	Framebuffer& operator=(Framebuffer&& other) noexcept {
		if (this != &other) {
			destroy();

			exchangeHandles(other.device_, other.colorView_, other.depthView_, other.renderPass_, other.handle_);
			other.clearHandles();
		}
		return *this;
	}

	VkDevice device() const noexcept { return device_; }
	VkImageView colorView() const noexcept { return colorView_; }
	VkImageView depthView() const noexcept { return depthView_; }
	VkRenderPass renderPass() const noexcept { return renderPass_; }
	VkFramebuffer handle() const noexcept { return handle_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void destroy() noexcept {
		if (isValid()) {
			vkDestroyFramebuffer(device_, handle_, nullptr);
		}
		clearHandles();
	}
};