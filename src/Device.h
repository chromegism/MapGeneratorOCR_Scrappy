#pragma once

#include <vector>
#include <optional>
#include <string>

#include <vulkan/vulkan_core.h>

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
		const std::vector<std::string> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};
private:

	std::vector<VkPhysicalDevice> enumerateDevices() const;
	VkPhysicalDevice findBestPhysicalDevice(const std::vector<VkPhysicalDevice>&, const Conditions&);

public:
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

public:
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice) const;
private:
	static std::vector<VkExtensionProperties> enumerateExtensionProperties(VkPhysicalDevice);
	bool checkDeviceExtensionSupport(VkPhysicalDevice, const Conditions&);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);

	bool isDeviceSuitable(VkPhysicalDevice, const Conditions&);

public:
	PhysicalDevice() noexcept = default;
	PhysicalDevice(const PhysicalDevice&) = delete; // move only
	PhysicalDevice(VkInstance _instanceHandle, VkSurfaceKHR _surfaceHandle) :
		instanceHandle_(_instanceHandle), surfaceHandle_(_surfaceHandle)
	{
		pickBest();
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

	void pickBest();
};