#include "pch.h"

#include "Device.h"
#include "Tools.h"

std::vector<VkPhysicalDevice> PhysicalDevice::enumerateDevices(VkInstance _instance) {
	uint32_t deviceCount;
	vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	if (deviceCount > 0)
		vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

	return devices;
}

VkPhysicalDevice PhysicalDevice::findBestPhysicalDevice(const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR _surface, const Conditions& conditions) {
	// since gpu heap size can theoretically go up to 64GB (as of writing)
	// which is 69527932928 (a big number)
	// the score representing the type needs to be much bigger than this
	constexpr const unsigned long long int MAX_MEMORY_SIZE_GB = 64;
	constexpr const unsigned long long int MAX_MEMORY_SIZE = MAX_MEMORY_SIZE_GB * 1024ULL * 1024ULL * 1024ULL;

	VkPhysicalDevice best_device = nullptr;
	uint64_t best_score = 0;

	for (const auto _pDevice : devices) {
		if (!isDeviceSuitable(_pDevice, _surface, conditions)) {
			continue;
		}

		uint64_t score = 0;

		auto props = VkPhysicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(_pDevice, &props);

		if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += MAX_MEMORY_SIZE * 1000; // best case so 1000 score
		}
		else if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			score += MAX_MEMORY_SIZE * 100; // not the best but better than cpu so 100 score
		}
		else if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU) {
			score += 0; // worst case
		}
		else {
			continue; // not usable
		}

		auto memoryProps = VkPhysicalDeviceMemoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(_pDevice, &memoryProps);

		auto heapsPointer = memoryProps.memoryHeaps;
		auto heaps = std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + memoryProps.memoryHeapCount);

		for (const auto& heap : heaps) {
			if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				score += static_cast<uint64_t>(heap.size); // the heap size in bytes
			}
		}

		if (score > best_score) {
			best_score = score;
			best_device = _pDevice;
		}
	}

	return best_device;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR _surface) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}
		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			indices.computeFamily = i;
		}

		if (indices.isComplete()) break;

		i++;
	}

	return indices;
}

std::vector<VkExtensionProperties> PhysicalDevice::enumerateExtensionProperties(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	if (extensionCount > 0)
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensionProperties.data());

	return extensionProperties;
}

bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device, const Conditions& conditions) {
	std::vector<VkExtensionProperties> availableExtensions = enumerateExtensionProperties(device);

	std::unordered_set<std::string> requiredExtensions(conditions.requiredExtensions.begin(), conditions.requiredExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

PhysicalDevice::SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR _surface) {
	SwapChainSupportDetails details{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR _surface, const Conditions& conditions) {
	QueueFamilyIndices indices = findQueueFamilies(device, _surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device, conditions);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, _surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

PhysicalDevice PhysicalDevice::pickBest(VkInstance _instance, VkSurfaceKHR _surface) {
	std::vector<VkPhysicalDevice> devices = enumerateDevices(_instance);
	if (devices.size() == 0)
		throw std::runtime_error("No devices supporting Vulkan found!");

	Conditions conditions{};
	VkPhysicalDevice deviceHandle = findBestPhysicalDevice(devices, _surface, conditions);
	if (deviceHandle == nullptr) {
		throw std::runtime_error("No suitable devices");
	}

	PhysicalDevice device(_instance, _surface, deviceHandle);

	return device;
}

PhysicalDevice PhysicalDevice::pickBest(const Instance& _instance, const Surface& _surface) {
	return pickBest(_instance.handle(), _surface.handle());
}


template<typename T>
std::unordered_map<T, uint32_t> respectiveReferencesCounter(std::vector<T> inps) {
	std::unordered_map<T, uint32_t> counts{};
	for (const auto& inp : inps) {
		counts[inp]++;
	}
	return counts;
}


LogicalDevice::LogicalDevice(const PhysicalDevice& physicalDevice, const PhysicalDevice::Conditions& conditions) {
	PhysicalDevice::QueueFamilyIndices indices = physicalDevice.queueFamilyIndices();
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::vector<uint32_t> queueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), 0 };

	std::unordered_map<uint32_t, uint32_t> queueFamilyIndexCounts = respectiveReferencesCounter(queueFamilies);

	queueCreateInfos.reserve(queueFamilyIndexCounts.size());

	std::vector<std::vector<float>> priorities;
	priorities.reserve(queueFamilyIndexCounts.size());

	uint32_t index = 0;
	for (const auto& [queueFamily, count] : queueFamilyIndexCounts) {
		priorities.emplace_back(std::vector<float>(count));
		for (uint32_t i = 0; i < count; i++) {
			priorities[index][i] = static_cast<float>(count - i) / static_cast<float>(count);
		}

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = count;
		queueCreateInfo.pQueuePriorities = priorities[index].data();
		queueCreateInfos.emplace_back(queueCreateInfo);
		index++;
	}


	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(conditions.requiredExtensions.size());
	std::vector<const char*> requiredExtensionPtrs = stringVectorToCStrVector(conditions.requiredExtensions);
	deviceCreateInfo.ppEnabledExtensionNames = requiredExtensionPtrs.data();

	if (vkCreateDevice(physicalDevice.handle(), &deviceCreateInfo, nullptr, &handle_) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device");
	}

	physicalDevice_ = &physicalDevice;
	std::unordered_map<uint32_t, uint32_t> allocatedCounts{};
	graphicsQueue_ = Queue(handle_, indices.graphicsFamily.value(), allocatedCounts[indices.graphicsFamily.value()]);
	allocatedCounts[indices.graphicsFamily.value()]++;
	presentQueue_ = Queue(handle_, indices.presentFamily.value(), allocatedCounts[indices.presentFamily.value()]);
	allocatedCounts[indices.presentFamily.value()]++;
	computeQueue_ = Queue(handle_, indices.computeFamily.value(), allocatedCounts[indices.computeFamily.value()]);
	allocatedCounts[indices.computeFamily.value()]++;
}