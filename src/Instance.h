#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

class Instance {
private:
	VkInstance handle_ = VK_NULL_HANDLE;

	std::string name_;
	std::string engineName_;

public:
	struct Version {
		uint32_t major = 0;
		uint32_t minor = 0;
		uint32_t patch = 0;

		uint32_t makeVersion() const { return VK_MAKE_VERSION(major, minor, patch); }
	};
private:

	Version version_{};
	Version engineVersion_{};
	uint32_t apiVersion_ = 0;

	std::vector<std::string> extensions_;
	std::vector<std::string> layers_;

	std::vector<const char*> extensionPtrs_;
	std::vector<const char*> layerPtrs_;

	void genExtensionPtrs();
	void genLayerPtrs();

	VkApplicationInfo createApplicationInfo() const;
	VkInstanceCreateInfo createInstanceCreateInfo(VkApplicationInfo* appInfo) const;

public:
	Instance() noexcept = default;
	Instance(const Instance&) = delete; // move only

	Instance(Instance&& other) noexcept : handle_(other.handle_) {
		other.handle_ = VK_NULL_HANDLE;
	}
	// If operator= is done with a temporary, the other temporary will call the destructor, and thus vkDestroyInstance
	// So we need to prevent that, otherwise it will crash
	// This took quite literally 6 hours of debugging to catch
	Instance& operator=(Instance&& other) noexcept {
		if (this != &other) {
			if (handle_)
				destroy();

			handle_ = other.handle();
			other.handle_ = VK_NULL_HANDLE;
		}
		return *this;
 	}

	Instance(
		const std::string& _name, Version _version,
		const std::string& _engineName, Version _engineVersion,
		uint32_t _apiVersion,
		const std::vector<std::string>& _extensions,
		const std::vector<std::string>& _layers) : 
			name_(_name), version_(_version),
			engineName_(_engineName), engineVersion_(_engineVersion),
			apiVersion_(_apiVersion),
			extensions_(_extensions),
			layers_(_layers)
	{
		generate();
	};

	~Instance() { destroy(); }

	const std::string& name() const noexcept { return name_; }
	const std::string& engineName() const noexcept { return engineName_; }
	const Version& version() const noexcept { return version_; }
	const Version& engineVersion() const noexcept { return engineVersion_; }
	const uint32_t& apiVersion() const noexcept { return apiVersion_; }
	const std::vector<std::string>& extensions() const noexcept { return extensions_; }
	const std::vector<std::string>& layers() const noexcept { return layers_; }
	const VkInstance handle() const noexcept { return handle_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void generate();
	void destroy() { 
		if (isValid()) {
			vkDestroyInstance(handle_, nullptr);
			handle_ = VK_NULL_HANDLE;
		}
	}

	static std::vector<std::string> requiredExtensions();
};