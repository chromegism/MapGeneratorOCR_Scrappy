#pragma once

#include <vulkan/vulkan_core.h>

#include "Device.h"
#include "Tools.h"

class Image {
private:
	VkDevice deviceHandle_ = VK_NULL_HANDLE;	// non owning
	VkImage handle_ = VK_NULL_HANDLE;			// owning
	VkDeviceMemory memory_ = VK_NULL_HANDLE;	// owning
	VkImageView view_ = VK_NULL_HANDLE;			// owning

	void setHandles(VkDevice _deviceHandle, VkImage _handle, VkDeviceMemory _memory, VkImageView _view) {
		deviceHandle_ = _deviceHandle;
		handle_ = _handle;
		memory_ = _memory;
		view_ = _view;
	}
	void clearHandles() {
		deviceHandle_ = VK_NULL_HANDLE;
		handle_ = VK_NULL_HANDLE;
		memory_ = VK_NULL_HANDLE;
		view_ = VK_NULL_HANDLE;
	}
	void exchangeHandles(VkDevice& _deviceHandle, VkImage& _handle, VkDeviceMemory& _memory, VkImageView& _view) {
		deviceHandle_ = std::exchange(_deviceHandle, VK_NULL_HANDLE);
		handle_ = std::exchange(_handle, VK_NULL_HANDLE);
		memory_ = std::exchange(_memory, VK_NULL_HANDLE);
		view_ = std::exchange(_view, VK_NULL_HANDLE);
	}
	
	void genImage(VkPhysicalDevice _physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples);
	void genImageView(VkFormat _format, VkImageAspectFlags _aspectFlags);

	Image(VkPhysicalDevice phys, VkDevice device,
		uint32_t width, uint32_t height,
		VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect,
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT) : deviceHandle_(device)
	{
		genImage(phys, width, height, format, usage, samples);
		genImageView(format, aspect);
	}

public:
	static Image fromHandle(VkDevice _deviceHandle, VkImage _handle, VkDeviceMemory _memory, VkImageView _view) {
		auto im = Image();
		im.setHandles(_deviceHandle, _handle, _memory, _view);
		return im;
	}
	static Image createColor(const LogicalDevice& dev, uint32_t width, uint32_t height, VkFormat format) {
		return Image(dev.physicalDeviceHandle(), dev.handle(), width, height, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	static Image createDepth(const LogicalDevice& dev, uint32_t width, uint32_t height, VkFormat format) {
		return Image(dev.physicalDeviceHandle(), dev.handle(), width, height, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	Image() noexcept = default;
	Image(const Image&) noexcept = delete; // move only
	Image(Image&& other) noexcept {
		exchangeHandles(other.deviceHandle_, other.handle_, other.memory_, other.view_);
	}
	Image& operator=(Image&& other) noexcept {
		if (this != &other) {
			if (isValid())
				destroy();

			// Make sure other object's handles are empty to avoid premature destruction of vulkan objects
			exchangeHandles(other.deviceHandle_, other.handle_, other.memory_, other.view_);
		}
		return *this;
	}

	VkDevice deviceHandle() const noexcept { return deviceHandle_; }
	VkImage handle() const noexcept { return handle_; }
	VkDeviceMemory memory() const noexcept { return memory_; }
	VkImageView view() const noexcept { return view_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void destroy() noexcept {
		if (isValid()) {
			vkDestroyImageView(deviceHandle_, view_, nullptr);
			vkDestroyImage(deviceHandle_, handle_, nullptr);
			vkFreeMemory(deviceHandle_, memory_, nullptr);
		}
		clearHandles();
	}

	static VkFormat findDepthFormat(VkPhysicalDevice _physicalDevice) {
		return findSupportedFormat(_physicalDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
	static VkFormat findDepthFormat(const PhysicalDevice& _physicalDevice) {
		return findSupportedFormat(_physicalDevice.handle(),
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
};





class SwapchainImage {
private:
	VkDevice deviceHandle_ = VK_NULL_HANDLE;	// non owning
	VkImage handle_ = VK_NULL_HANDLE;			// owning
	VkImageView view_ = VK_NULL_HANDLE;			// owning

	void genImageView(VkFormat);

	explicit SwapchainImage(VkDevice _deviceHandle, VkImage _handle, VkImageView _view) :
		deviceHandle_(_deviceHandle), handle_(_handle), view_(_view) {
	}
	explicit SwapchainImage(VkDevice _deviceHandle, VkImage _handle, VkFormat _format) :
		deviceHandle_(_deviceHandle), handle_(_handle) {
		genImageView(_format);
	}

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
	void exchangeHandles(VkDevice& _deviceHandle, VkImage& _handle, VkImageView& _view) noexcept {
		deviceHandle_ = std::exchange(_deviceHandle, VK_NULL_HANDLE);
		handle_ = std::exchange(_handle, VK_NULL_HANDLE);
		view_ = std::exchange(_view, VK_NULL_HANDLE);
	}
public:
	static SwapchainImage fromImageView(VkDevice _deviceHandle, VkImage _handle, VkImageView _view) {
		return SwapchainImage(_deviceHandle, _handle, _view);
	}
	static SwapchainImage fromImage(VkDevice _deviceHandle, VkImage _handle, VkFormat _format) {
		return SwapchainImage(_deviceHandle, _handle, _format);
	}

	SwapchainImage(const SwapchainImage&) noexcept = delete;
	SwapchainImage(SwapchainImage&& other) noexcept {
		exchangeHandles(other.deviceHandle_, other.handle_, other.view_);
	}
	SwapchainImage& operator=(SwapchainImage&& other) noexcept {
		if (this != &other) {
			if (isViewValid())
				destroy();

			exchangeHandles(other.deviceHandle_, other.handle_, other.view_);
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