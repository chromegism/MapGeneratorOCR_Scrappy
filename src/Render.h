#pragma once

#include <vector>
#include <optional>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

class Renderer {
public:
	void init(SDL_Window* window);

	~Renderer();

	void drawFrame();
	void waitIdle();

	void kill();

private:
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	uint32_t maxFramesInFlight = 0;

	uint32_t currentFrame = 0;

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	void createInstance();
	void pickPhysicalDevice();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _pDevice);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice _pDevice);
	void createLogicalDevice();
	void createSurface(SDL_Window* window);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities);
	void createSwapChain(SDL_Window* window);
	void createSwapChainViews();

	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createGraphicsPipeline();
	void createRenderPass();

	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();

	void createSyncObjects();

	void recordCommandBuffer(VkCommandBuffer _commandBuffer, uint32_t imageIndex);

	VkPhysicalDevice findBestPhysicalDevice(const std::vector<VkPhysicalDevice>& devices);

	bool isDeviceSuitable(VkPhysicalDevice _pDevice);
	bool checkDeviceExtensionSupport(VkPhysicalDevice _pDevice);
};