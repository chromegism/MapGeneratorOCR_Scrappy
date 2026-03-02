#pragma once

#include <vulkan/vulkan_core.h>

class SwapchainImage {
private:
	VkDevice deviceHandle_ = VK_NULL_HANDLE;
	VkImage handle_ = VK_NULL_HANDLE;
	VkImageView view_ = VK_NULL_HANDLE;

	void genImageView(VkFormat);

	SwapchainImage(VkDevice _deviceHandle, VkImage _handle, VkImageView _view) :
		deviceHandle_(_deviceHandle), handle_(_handle), view_(_view) {
	}
	SwapchainImage(VkDevice _deviceHandle, VkImage _handle, VkFormat _format) :
		deviceHandle_(_deviceHandle), handle_(_handle) {
		genImageView(_format);
	}

protected:
	void setHandles(VkDevice _deviceHandle, VkImage _handle, VkImageView _view) noexcept {
		deviceHandle_ = _deviceHandle;
		handle_ = _handle;
		view_ = _view;
	}
	void clearHandles() noexcept {
		deviceHandle_ = VK_NULL_HANDLE;
		handle_ = VK_NULL_HANDLE;
		view_ = VK_NULL_HANDLE;
	}

public:
	static inline SwapchainImage fromImageView(VkDevice _deviceHandle, VkImage _handle, VkImageView _view) {
		return SwapchainImage(_deviceHandle, _handle, _view);
	}
	static inline SwapchainImage fromImage(VkDevice _deviceHandle, VkImage _handle, VkFormat _format) {
		return SwapchainImage(_deviceHandle, _handle, _format);
	}

	SwapchainImage(const SwapchainImage&) noexcept = delete;
	SwapchainImage(SwapchainImage&& other) noexcept {
		setHandles(other.deviceHandle(), other.handle(), other.view());
		other.clearHandles();
	}
	SwapchainImage& operator=(SwapchainImage&& other) noexcept {
		if (this != &other) {
			if (isViewValid())
				destroy();

			setHandles(other.deviceHandle(), other.handle(), other.view());
			other.clearHandles();
		}
		return *this;
	}

	~SwapchainImage() { destroy(); }

	VkDevice deviceHandle() const noexcept { return deviceHandle_; }
	VkImage handle() const noexcept { return handle_; }
	VkImageView view() const noexcept { return view_; }
	bool isViewValid() const noexcept { return view_ != VK_NULL_HANDLE; }

	void destroy() {
		if (isViewValid()) {
			vkDestroyImageView(deviceHandle_, view_, nullptr);
			clearHandles();
		}
	}
};