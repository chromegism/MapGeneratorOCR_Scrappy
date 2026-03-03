#pragma once

#include <vulkan/vulkan.h>

class Framebuffer {
	VkDevice device_ = VK_NULL_HANDLE;
	VkImageView swapView_ = VK_NULL_HANDLE;
	VkImageView depthView_ = VK_NULL_HANDLE;
	VkRenderPass renderPass_ = VK_NULL_HANDLE;
	VkFramebuffer handle_ = VK_NULL_HANDLE;

	void exchangeHandles(VkDevice& _device, VkImageView& _swapView, VkImageView& _depthView, VkRenderPass& _renderPass, VkFramebuffer& _handle) noexcept {
		device_ = std::exchange(_device, VK_NULL_HANDLE);
		swapView_ = std::exchange(_swapView, VK_NULL_HANDLE);
		depthView_ = std::exchange(_depthView, VK_NULL_HANDLE);
		renderPass_ = std::exchange(_renderPass, VK_NULL_HANDLE);
		handle_ = std::exchange(_handle, VK_NULL_HANDLE);
	}
	void clearHandles() noexcept {
		device_ = VK_NULL_HANDLE;
		swapView_ = VK_NULL_HANDLE;
		depthView_ = VK_NULL_HANDLE;
		renderPass_ = VK_NULL_HANDLE;
		handle_ = VK_NULL_HANDLE;
	}

	void genFramebuffer(uint32_t width, uint32_t height);

	Framebuffer(VkDevice _device, VkImageView _swapView, VkImageView _depthView, VkRenderPass _renderPass, uint32_t width, uint32_t height) :
		device_(_device), swapView_(_swapView), depthView_(_depthView), renderPass_(_renderPass) 
	{
		genFramebuffer(width, height);
	}

public:
	static Framebuffer create(VkDevice _device, VkImageView _swapView, VkImageView _depthView, VkRenderPass _renderPass, uint32_t width, uint32_t height) {
		return Framebuffer(_device, _swapView, _depthView, _renderPass, width, height);
	}

	Framebuffer() noexcept = default;
	Framebuffer(const Framebuffer&) noexcept = delete; // move only
	Framebuffer(Framebuffer&& other) {
		exchangeHandles(other.device_, other.swapView_, other.depthView_, other.renderPass_, other.handle_);
		other.clearHandles();
	}
	Framebuffer& operator=(Framebuffer&& other) {
		if (this != &other) {
			destroy();

			exchangeHandles(other.device_, other.swapView_, other.depthView_, other.renderPass_, other.handle_);
			other.clearHandles();
		}
		return *this;
	}

	VkDevice device() const noexcept { return device_; }
	VkImageView swapView() const noexcept { return swapView_; }
	VkImageView depthView() const noexcept { return depthView_; }
	VkRenderPass renderPass() const noexcept { return renderPass_; }
	VkFramebuffer handle() const noexcept { return handle_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void destroy() noexcept {
		if (isValid()) {
			vkDestroyFramebuffer(device_, handle_, nullptr);
			clearHandles();
		}
	}
};