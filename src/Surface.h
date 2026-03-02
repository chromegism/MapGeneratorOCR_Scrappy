#pragma once

#include <iostream>

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Instance.h"

// RAII wrapper for VkSurfaceKHR
// 
// Ownership model:
//  - Surface owns VkSurfaceKHR
//  - Instance is non-owning but must outlive Surface
class Surface {
private:
	VkInstance instanceHandle_ = VK_NULL_HANDLE;  // non-owning

	// handle_ == VK_NULL_HANDLE  <=>  no owned surface
	VkSurfaceKHR handle_ = VK_NULL_HANDLE;

	void setHandles(VkInstance _instanceHandle, VkSurfaceKHR _handle) {
		instanceHandle_ = _instanceHandle;
		handle_ = _handle;
	}

	void clearHandles() {
		instanceHandle_ = VK_NULL_HANDLE;
		handle_ = VK_NULL_HANDLE;
	}

public:
	Surface() = default;
	Surface(VkInstance _instanceHandle, SDL_Window* _windowHandle) :
		instanceHandle_(_instanceHandle)
	{
		generate(_windowHandle);
	}
	Surface(const Surface&) = delete; // move only
	Surface(Surface&& other) noexcept {
		setHandles(other.instanceHandle(), other.handle());
		other.clearHandles();
	}
	// Look at Instance.h for reasoning of this function
	Surface& operator=(Surface&& other) noexcept {
		if (this != &other) {
			if (handle_)
				destroy();

			setHandles(other.instanceHandle(), other.handle());
			other.clearHandles();
		}
		return *this;
	}
	~Surface() { destroy(); }

	void generate(SDL_Window*);

	VkSurfaceKHR handle() const noexcept { return handle_; }
	VkInstance instanceHandle() const noexcept { return instanceHandle_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }

	void destroy() {
		if (isValid() && instanceHandle_ != VK_NULL_HANDLE) {
			vkDestroySurfaceKHR(instanceHandle_, handle_, nullptr);
			clearHandles();
		}
	}
};