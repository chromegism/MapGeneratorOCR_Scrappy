#pragma once

#include <vulkan/vulkan_core.h>

#include "Device.h"
#include "Tools.h"
#include "Buffer.h"

class Image {
private:
	const LogicalDevice* device_ = nullptr;				// non owning
	VkImage handle_ = VK_NULL_HANDLE;					// owning
	VkDeviceMemory memory_ = VK_NULL_HANDLE;			// owning
	VkImageView view_ = VK_NULL_HANDLE;					// owning
	VkFormat format_ = VK_FORMAT_UNDEFINED;				// trivial
	VkExtent2D extent_ = {0,0};							// trivial
	VkImageLayout layout_ = VK_IMAGE_LAYOUT_UNDEFINED;	// trivial

	void setHandles(const Image& other) {
		device_ = other.device_;
		handle_ = other.handle_;
		memory_ = other.memory_;
		view_ = other.view_;
		format_ = other.format_;
		extent_ = other.extent_;
		layout_ = other.layout_;
	}
	void setHandles(const LogicalDevice* _device, VkImage _handle, VkDeviceMemory _memory, VkImageView _view, VkFormat _format, VkExtent2D _extent, VkImageLayout _layout) {
		device_ = _device;
		handle_ = _handle;
		memory_ = _memory;
		view_ = _view;
		format_ = _format;
		extent_ = _extent;
		layout_ = _layout;
	}
	void clearHandles() {
		device_ = nullptr;
		handle_ = VK_NULL_HANDLE;
		memory_ = VK_NULL_HANDLE;
		view_ = VK_NULL_HANDLE;
		format_ = VK_FORMAT_UNDEFINED;
		extent_ = {};
		layout_ = {};
	}
	void exchangeHandles(Image& other) {
		device_ = std::exchange(other.device_, VK_NULL_HANDLE);
		handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
		memory_ = std::exchange(other.memory_, VK_NULL_HANDLE);
		view_ = std::exchange(other.view_, VK_NULL_HANDLE);
		format_ = other.format_;
		extent_ = other.extent_;
		layout_ = other.layout_;
	}
	
	void genImage(VkPhysicalDevice _physicalDevice, VkImageUsageFlags usage, VkSampleCountFlagBits samples);
	void genImageView(VkFormat _format, VkImageAspectFlags _aspectFlags);

	Image(const LogicalDevice* device,
		uint32_t width, uint32_t height,
		VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect,
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT) : device_(device), format_(format)
	{
		extent_.width = width; extent_.height = height;
		genImage(device->physicalDeviceHandle(), usage, samples);
		genImageView(format, aspect);
	}

public:
	static Image wrapExisting(const LogicalDevice& _device, VkImage _handle, VkDeviceMemory _memory, VkImageView _view, VkFormat _format, VkExtent2D _extent, VkImageLayout _layout) {
		auto im = Image();
		im.setHandles(&_device, _handle, _memory, _view, _format, _extent, _layout);
		return im;
	}
	static Image createColor(const LogicalDevice& dev, uint32_t width, uint32_t height, VkFormat format) {
		return Image(&dev, width, height, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	static Image createDepth(const LogicalDevice& dev, uint32_t width, uint32_t height, VkFormat format) {
		return Image(&dev, width, height, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	static Image createStorage(const LogicalDevice& dev, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags extraUsage = 0) {
		return Image(&dev, width, height, format, VK_IMAGE_USAGE_STORAGE_BIT | extraUsage, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	Image() noexcept = default;
	Image(const Image&) noexcept = delete; // move only
	Image(Image&& other) noexcept {
		exchangeHandles(other);
	}
	Image& operator=(Image&& other) noexcept {
		if (this != &other) {
			destroy();

			// Make sure other object's handles are empty to avoid premature destruction of vulkan objects
			exchangeHandles(other);
		}
		return *this;
	}

	const LogicalDevice& device() const noexcept { return *device_; }
	VkDevice deviceHandle() const noexcept { return device_->handle(); }
	VkImage handle() const noexcept { return handle_; }
	VkDeviceMemory memory() const noexcept { return memory_; }
	VkImageView view() const noexcept { return view_; }
	VkFormat format() const noexcept { return format_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void destroy() noexcept {
		if (isValid()) {
			vkDestroyImageView(device_->handle(), view_, nullptr);
			vkDestroyImage(device_->handle(), handle_, nullptr);
			vkFreeMemory(device_->handle(), memory_, nullptr);
		}
		clearHandles();
	}

	void copyBuffer(const Buffer& other, VkCommandPool commandPool);
	void transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool);

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