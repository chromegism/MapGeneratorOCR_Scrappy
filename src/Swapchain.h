#pragma once

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.h>

#include "Image.h"
#include "Framebuffer.h"
#include "Device.h"

class Swapchain {
	const LogicalDevice* device_ = nullptr;		// non owning
	VkSurfaceKHR surface_ = VK_NULL_HANDLE;		// non owning
	VkSwapchainKHR handle_ = VK_NULL_HANDLE;	// owning
	VkExtent2D extent_;							// owning

	VkRenderPass renderPass_ = VK_NULL_HANDLE;	// owning
	std::vector<SwapchainImage> images_;		// owning
	Image depthImage_;							// owning
	std::vector<Framebuffer> framebuffers_;		// owning

	VkSurfaceFormatKHR surfaceFormat_;			// owning

	SDL_Window* window_ = nullptr;				// non owning

	void exchangeHandles(Swapchain& other) noexcept {
		device_ = std::exchange(other.device_, nullptr);
		surface_ = std::exchange(other.surface_, VK_NULL_HANDLE);
		handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
		extent_ = std::exchange(other.extent_, {});
		renderPass_ = std::exchange(other.renderPass_, VK_NULL_HANDLE);
		images_ = std::move(other.images_);
		depthImage_ = std::exchange(other.depthImage_, {});
		framebuffers_ = std::move(other.framebuffers_);
		surfaceFormat_ = std::exchange(other.surfaceFormat_, {});
		window_ = std::exchange(other.window_, nullptr);
	}
	void clearHandles() noexcept {
		device_ = nullptr;
		surface_ = VK_NULL_HANDLE;
		handle_ = VK_NULL_HANDLE;
		extent_ = {};
		renderPass_ = VK_NULL_HANDLE;
		images_.clear();
		depthImage_ = {};
		framebuffers_.clear();
		surfaceFormat_ = {};
		window_ = nullptr;
	}

	struct SwapchainCreateInfos {
		const LogicalDevice& device;
		VkSurfaceKHR surface;
		SDL_Window* window;
		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;
		VkExtent2D extent;
		VkSurfaceTransformFlagBitsKHR transform;
		uint32_t imageCount;
	};

	Swapchain(const SwapchainCreateInfos&);
	void genImages(uint32_t imageCount);
	void genRenderPass();
	void genFramebuffers();

public:
	static Swapchain create(const LogicalDevice& device, VkSurfaceKHR surface, SDL_Window* window, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform, uint32_t imageCount)
	{
		return Swapchain(
			{ 
				device,
				surface, 
				window, 
				surfaceFormat, 
				presentMode, 
				extent, 
				transform,
				imageCount
			}
		);
	}
	Swapchain() noexcept = default;
	Swapchain(const Swapchain&) noexcept = delete; // move only
	Swapchain(Swapchain&& other) noexcept {
		exchangeHandles(other);
	}
	Swapchain& operator=(Swapchain&& other) noexcept {
		if (this != &other) {
			destroy();

			exchangeHandles(other);
		}
		return *this;
	}

	const LogicalDevice& device() const noexcept {
		assert(device_ != nullptr);
		return *device_;
	}
	VkDevice deviceHandle() const noexcept { return device_->handle(); }
	VkSurfaceKHR surfaceHandle() const noexcept { return surface_; }
	VkSwapchainKHR handle() const noexcept { return handle_; }
	VkRenderPass renderPass() const noexcept { return renderPass_; }
	const std::vector<SwapchainImage>& images() const noexcept { return images_; }
	const Image& depthImage() const noexcept { return depthImage_; }
	const std::vector<Framebuffer>& framebuffers() const noexcept { return framebuffers_; }
	VkSurfaceFormatKHR surfaceFormat() const noexcept { return surfaceFormat_; }
	SDL_Window* window() const noexcept { return window_; }
	VkExtent2D extent() const noexcept { return extent_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }
	void destroy() noexcept {
		if (isValid()) {
			images_.clear();
			depthImage_.destroy();
			framebuffers_.clear();
			vkDestroyRenderPass(deviceHandle(), renderPass_, nullptr);
			vkDestroySwapchainKHR(deviceHandle(), handle_, nullptr);
		}
		clearHandles();
	}

	~Swapchain() { destroy(); }

	static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);
	static VkExtent2D chooseExtent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities);
	static uint32_t chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities);
};