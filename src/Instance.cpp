#include <iostream>
#include <algorithm>
#include <iterator>
#include <ranges>

#include "Instance.h"

inline std::vector<const char*> stringVectorToCStrVector(const std::vector<std::string>& strings) {
	std::vector<const char*> cstrings{};
	cstrings.reserve(strings.size());

	std::ranges::copy(
    	std::views::transform(strings, [](const std::string& s) { return s.c_str(); }),
    	std::back_inserter(cstrings)
	);

	return cstrings;
}

void Instance::genExtensionPtrs() {
	extensionPtrs_ = stringVectorToCStrVector(extensions_);
}

void Instance::genLayerPtrs() {
	layerPtrs_ = stringVectorToCStrVector(layers_);
}

VkApplicationInfo Instance::createApplicationInfo() const {
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = name_.c_str();
	appInfo.applicationVersion = version_.makeVersion();
	appInfo.pEngineName = engineName_.c_str();
	appInfo.engineVersion = engineVersion_.makeVersion();
	appInfo.apiVersion = apiVersion_;

	return appInfo;
}

VkInstanceCreateInfo Instance::createInstanceCreateInfo(VkApplicationInfo* pAppInfo) const {
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = pAppInfo;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions_.size());
	createInfo.ppEnabledExtensionNames = extensionPtrs_.data();

	createInfo.enabledLayerCount = static_cast<uint32_t>(layers_.size());
	createInfo.ppEnabledLayerNames = layerPtrs_.data();

	return createInfo;
}

void Instance::generate() {
	VkApplicationInfo appInfo = createApplicationInfo();
	genExtensionPtrs();
	genLayerPtrs();
	VkInstanceCreateInfo createInfo = createInstanceCreateInfo(&appInfo);

	VkResult error_code = vkCreateInstance(&createInfo, nullptr, &handle_);
	if (error_code != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan instance");
	}
}

std::vector<std::string> Instance::requiredExtensions() {
	uint32_t extensionCount = 0;
	// Get platform specific extensions
	const char* const* availableExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
	const std::vector<std::string> extensions(availableExtensions, availableExtensions + extensionCount);
	return extensions;
}
