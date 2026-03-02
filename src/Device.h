#pragma once

#include <vector>
#include <optional>
#include <string>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Instance.h"
#include "Surface.h"

// RAII wrapper for VkPhysicalDevice
//
// Ownership model:
//  - PhysicalDevice owns VkPhysicalDevice
//  - No native destructor => Lifetimes irrelevant
class PhysicalDevice {
private:
	VkInstance instanceHandle_ = VK_NULL_HANDLE;
	VkSurfaceKHR surfaceHandle_ = VK_NULL_HANDLE;
	VkPhysicalDevice handle_ = VK_NULL_HANDLE;

	void setHandles(VkInstance _instanceHandle, VkSurfaceKHR _surfaceHandle, VkPhysicalDevice _handle) {
		instanceHandle_ = _instanceHandle;
		surfaceHandle_ = _surfaceHandle;
		handle_ = _handle;
	}

	void clearHandles() {
		instanceHandle_ = VK_NULL_HANDLE;
		surfaceHandle_ = VK_NULL_HANDLE;
		handle_ = VK_NULL_HANDLE;
	}

public:
	struct Conditions {
		std::vector<std::string> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
	} queueFamilyIndices_;

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	} swapChainSupportDetails_;

private:
	static std::vector<VkPhysicalDevice> enumerateDevices(VkInstance _instance);
	static VkPhysicalDevice findBestPhysicalDevice(const std::vector<VkPhysicalDevice>&, VkSurfaceKHR, const Conditions&);

	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice, VkSurfaceKHR);
	static std::vector<VkExtensionProperties> enumerateExtensionProperties(VkPhysicalDevice);
	static bool checkDeviceExtensionSupport(VkPhysicalDevice, const Conditions&);
	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice, VkSurfaceKHR);

	static bool isDeviceSuitable(VkPhysicalDevice, VkSurfaceKHR, const Conditions&);


	void updateQueueFamily() { queueFamilyIndices_ = findQueueFamilies(handle_, surfaceHandle_); }
	void updateSwapChainSupport() { swapChainSupportDetails_ = querySwapChainSupport(handle_, surfaceHandle_); }

public:
	PhysicalDevice() noexcept = default;
	PhysicalDevice(const PhysicalDevice&) = delete; // move only
	PhysicalDevice(VkInstance _instanceHandle, VkSurfaceKHR _surfaceHandle, VkPhysicalDevice _handle) :
		instanceHandle_(_instanceHandle), surfaceHandle_(_surfaceHandle), handle_(_handle) 
	{ 
		updateQueueFamily();
		updateSwapChainSupport();
	}

	PhysicalDevice(PhysicalDevice&& other) noexcept {
		setHandles(other.instanceHandle(), other.surfaceHandle(), other.handle());
		other.clearHandles();
	}

	// Look at Instance.h for reasoning of this function
	PhysicalDevice& operator=(PhysicalDevice&& other) noexcept {
		if (this != &other) {
			queueFamilyIndices_ = other.queueFamilyIndices();
			swapChainSupportDetails_ = other.swapChainSupportDetails();

			setHandles(other.instanceHandle(), other.surfaceHandle(), other.handle());
			other.clearHandles();
		}
		return *this;
	}

	VkInstance instanceHandle() const noexcept { return instanceHandle_; }
	VkSurfaceKHR surfaceHandle() const noexcept { return surfaceHandle_; }
	VkPhysicalDevice handle() const noexcept { return handle_; }
	const QueueFamilyIndices& queueFamilyIndices() const noexcept { return queueFamilyIndices_; }
	const SwapChainSupportDetails& swapChainSupportDetails() const noexcept { return swapChainSupportDetails_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	static PhysicalDevice pickBest(VkInstance, VkSurfaceKHR);
};

class Queue {
	VkDevice deviceHandle_ = VK_NULL_HANDLE;
	VkQueue handle_ = VK_NULL_HANDLE;
	uint32_t familyIndex_ = 0;

public:
	void setHandles(VkDevice _deviceHandle, VkQueue _handle, uint32_t _familyIndex) noexcept {
		deviceHandle_ = _deviceHandle;
		handle_ = _handle;
		familyIndex_ = _familyIndex;
	}

	void clearHandles() noexcept {
		deviceHandle_ = VK_NULL_HANDLE;
		handle_ = VK_NULL_HANDLE;
		uint32_t familyIndex = 0;
	}

	Queue() noexcept = default;
	Queue(VkDevice _deviceHandle, uint32_t _familyIndex) : deviceHandle_(_deviceHandle), familyIndex_(_familyIndex) { 
		vkGetDeviceQueue(_deviceHandle, _familyIndex, 0, &handle_);
	}
	Queue(const Queue& other) noexcept {
		setHandles(other.deviceHandle(), other.handle(), other.familyIndex());
	}

	VkDevice deviceHandle() const noexcept { return deviceHandle_; }
	VkQueue handle() const noexcept { return handle_; }
	uint32_t familyIndex() const noexcept { return familyIndex_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }
};

class LogicalDevice {
	VkDevice handle_;

	Queue graphicsQueue_;
	Queue presentQueue_;

	void setHandles(VkDevice _handle, const Queue& _graphicsQueue, const Queue& _presentQueue) noexcept {
		handle_ = _handle;
		graphicsQueue_ = _graphicsQueue;
		presentQueue_ = _presentQueue;
	}
	void clearHandles() noexcept {
		handle_ = VK_NULL_HANDLE;
		graphicsQueue_.clearHandles();
		presentQueue_.clearHandles();
	}

public:
	LogicalDevice() noexcept = default;
	LogicalDevice(const PhysicalDevice&, const PhysicalDevice::Conditions&);
	LogicalDevice(const LogicalDevice&) noexcept = delete;

	LogicalDevice(LogicalDevice&& other) noexcept {
		setHandles(other.handle(), other.graphicsQueue(), other.presentQueue());
		other.clearHandles();
	}
	LogicalDevice& operator=(LogicalDevice&& other) noexcept {
		if (this != &other) {
			if (isValid())
				destroy();

			setHandles(other.handle(), other.graphicsQueue(), other.presentQueue());
			other.clearHandles();
		}
		return *this;
	}

	VkDevice handle() const noexcept { return handle_; }
	const Queue& graphicsQueue() const noexcept { return graphicsQueue_; }
	const Queue& presentQueue() const noexcept { return presentQueue_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void destroy() {
		if (isValid()) {
			vkDestroyDevice(handle_, nullptr);
			clearHandles();
		}
	}
};