#pragma once

#include <vector>
#include <optional>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "Terrain.h"
#include "Instance.h"
#include "Surface.h"
#include "Device.h"
#include "Image.h"
#include "Swapchain.h"
#include "Buffer.h"

struct MVPBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct MapDetailsObject {
	glm::ivec2 bufferSize;
	glm::vec2 displaySize;
};

struct Vertex {
	glm::float32 height;

	static std::array<VkVertexInputBindingDescription, 1> getBindingDescriptions() {
		std::array<VkVertexInputBindingDescription, 1> bindingDescription{};
		bindingDescription[0].binding = 0;
		bindingDescription[0].stride = sizeof(glm::vec2);
		bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		return attributeDescriptions;
	}
};

class Renderer {
public:
	void init(SDL_Window* window, TerrainGenerator& generator);

	~Renderer();

	void drawFrame();
	void waitIdle();

	void updateTerrain(TerrainGenerator& generator);

	void destroy();

private:
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	Instance instance;
	Surface surface;
	PhysicalDevice physicalDevice;
	LogicalDevice device;
	Swapchain swapchain;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	Buffer vertexBuffer;
	Buffer indexBuffer;

	Image heightImage;
	VkSampler heightSampler;

	uint32_t terrainIndicesLength = 0;

	MapDetailsObject mapDetailsData;

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

	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createCommandPool();
	void createVertexBuffer(TerrainGenerator& generator);
	void createIndexBuffer(TerrainGenerator& generator);
	void createHeightImage(TerrainGenerator& generator);
	void updateHeightImage(TerrainGenerator& generator);
	void createHeightSampler();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);

	void recordCommandBuffer(VkCommandBuffer _commandBuffer, uint32_t imageIndex);

	void updateUniformBuffers(uint32_t currentImage);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};