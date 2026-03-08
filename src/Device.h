#pragma once

#include <vector>
#include <optional>
#include <string>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Instance.h"
#include "Surface.h"
#include "Tools.h"

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
		std::optional<uint32_t> computeFamily;

		bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value(); }
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

private:
	QueueFamilyIndices queueFamilyIndices_;
	SwapChainSupportDetails swapChainSupportDetails_;

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
	static PhysicalDevice pickBest(const Instance&, const Surface&);
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
		familyIndex_ = 0;
	}

	Queue() noexcept = default;
	Queue(VkDevice deviceHandle, uint32_t familyIndex, uint32_t queueIndex = 0) : deviceHandle_(deviceHandle), familyIndex_(familyIndex) { 
		vkGetDeviceQueue(deviceHandle, familyIndex, queueIndex, &handle_);
	}
	Queue(const Queue& other) noexcept {
		setHandles(other.deviceHandle(), other.handle(), other.familyIndex());
	}

	VkDevice deviceHandle() const noexcept { return deviceHandle_; }
	VkQueue handle() const noexcept { return handle_; }
	uint32_t familyIndex() const noexcept { return familyIndex_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void submit(const std::vector<VkSubmitInfo>& infos, VkFence fence) const {
		VkResult error_code = vkQueueSubmit(handle_, static_cast<uint32_t>(infos.size()), infos.data(), fence);
		handleVkResult(error_code, "Failed to call vkQueueSubmit");
	}
	void submit(uint32_t infoCount, VkSubmitInfo* infos, VkFence fence) const {
		VkResult error_code = vkQueueSubmit(handle_, infoCount, infos, fence);
		handleVkResult(error_code, "Failed to call vkQueueSubmit");
	}
	void submitCommand(VkCommandPool commandPool, VkCommandBuffer commandBuffer) const {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(handle_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(handle_);

		vkFreeCommandBuffers(deviceHandle_, commandPool, 1, &commandBuffer);
	}
	void waitIdle() const noexcept { vkQueueWaitIdle(handle_); }
};

class LogicalDevice {
	const PhysicalDevice* physicalDevice_ = nullptr;
	VkDevice handle_ = VK_NULL_HANDLE;

	Queue graphicsQueue_;
	Queue presentQueue_;
	Queue computeQueue_;
	std::mutex graphicsMutex_;
	std::mutex presentMutex_;
	std::mutex computeMutex_;

	void clearHandles() noexcept {
		physicalDevice_ = nullptr;
		handle_ = VK_NULL_HANDLE;
		graphicsQueue_.clearHandles();
		presentQueue_.clearHandles();
		computeQueue_.clearHandles();
	}
	void exchangeHandles(LogicalDevice& other) noexcept {
		physicalDevice_ = std::exchange(other.physicalDevice_, nullptr);
		handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
		graphicsQueue_ = std::move(other.graphicsQueue_);
		presentQueue_ = std::move(other.presentQueue_);
		computeQueue_ = std::move(other.computeQueue_);
	}

public:
	LogicalDevice() noexcept = default;
	LogicalDevice(const PhysicalDevice&, const PhysicalDevice::Conditions & = {});
	LogicalDevice(const LogicalDevice&) noexcept = delete;

	LogicalDevice(LogicalDevice&& other) noexcept {
		exchangeHandles(other);
	}
	LogicalDevice& operator=(LogicalDevice&& other) noexcept {
		if (this != &other) {
			destroy();

			exchangeHandles(other);
		}
		return *this;
	}
	~LogicalDevice() { destroy(); }

	const PhysicalDevice& physicalDevice() const noexcept {
		assert(physicalDevice_ != nullptr);
		return *physicalDevice_;
	}
	VkPhysicalDevice physicalDeviceHandle() const noexcept { return physicalDevice_->handle(); }
	VkDevice handle() const noexcept { return handle_; }
	uint32_t graphicsQueueIndex() const noexcept { return graphicsQueue_.familyIndex(); }
	uint32_t presentQueueIndex() const noexcept { return presentQueue_.familyIndex(); }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void destroy() {
		if (isValid()) {
			vkDestroyDevice(handle_, nullptr);
			clearHandles();
		}
	}

	void waitIdle() const noexcept { vkDeviceWaitIdle(handle_); }
	void submitGraphics(const std::vector<VkSubmitInfo>& infos, VkFence fence) noexcept {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		graphicsQueue_.submit(infos, fence);
	}
	void submitCompute(const std::vector<VkSubmitInfo>& infos, VkFence fence) noexcept {
		std::lock_guard<std::mutex> lock(computeMutex_);
		computeQueue_.submit(infos, fence);
	}
	void present(VkPresentInfoKHR* pPresentInfo) {
		std::lock_guard<std::mutex> lock(presentMutex_);
		VkResult error_code = vkQueuePresentKHR(presentQueue_.handle(), pPresentInfo);
		handleVkResult(error_code, "Failed to call vkQueuePresentKHR");
	}
	void graphicsSubmitCommand(VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
		std::lock_guard<std::mutex> lock(graphicsMutex_);
		graphicsQueue_.submitCommand(commandPool, commandBuffer);
	}
	void computeSubmitCommand(VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
		std::lock_guard<std::mutex> lock(computeMutex_);
		computeQueue_.submitCommand(commandPool, commandBuffer);
	}
	void graphicsWaitIdle() const noexcept { graphicsQueue_.waitIdle(); }
	void presentWaitIdle() const noexcept { presentQueue_.waitIdle(); }
	void computeWaitIdle() const noexcept { computeQueue_.waitIdle(); }
};