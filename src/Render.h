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

	void init();
	// Must go at the end of the frame's gpu operations
	void drawFrame();
	void waitIdle();

	void updateTerrain(TerrainGenerator& generator);

	void destroy();

private:
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	bool isFirstFrame = false;

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

	// Make these into objects
	std::vector<Buffer> uniformBuffers;
	std::vector<void*> uniformBuffersMapped;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	Buffer vertexBuffer;
	Buffer indexBuffer;

	Buffer erosionImageStager;
	float* erosionImageStagerMapped;
	std::array<Image, 2> erosionImages;
	std::atomic<uint8_t> imageIndex;

	VkSemaphore copyCompleteSemaphore;
	VkSemaphore renderCompleteSemaphore;

	Image renderHeightImage;
	VkSampler renderHeightSampler;
	//Image gradientImage;
	//VkSampler gradientSampler;
	Queue computeQueue;

	std::thread erosionThread;
	std::atomic<bool> erosionRunning = false;

	VkPipelineLayout erosionPipelineLayout;
	VkPipeline erosionPipeline;
	VkDescriptorSetLayout erosionDescriptorSetLayout;
	std::array<VkDescriptorSet, 2> erosionDescriptorSets;
	std::array<VkCommandBuffer, 2> erosionCommandBuffers;
	VkCommandPool erosionCommandPool;
	VkFence erosionFence;

	MapDetailsObject mapDetailsData;

	void createInstance();

	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createErosionDescriptorSetLayout();
	void createErosionPipeline();
	void createCommandPool();
	void createVertexBuffer(TerrainGenerator& generator);
	void createIndexBuffer(TerrainGenerator& generator);
	void createErosionImages(TerrainGenerator& generator);
	void createHeightImage(TerrainGenerator& generator);
	void updateCurrentErosionImage(TerrainGenerator& generator);
	void createHeightSampler();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createErosionDescriptorSets();
	void createCommandBuffers();
	void createErosionFence();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void recordCommandBuffer(VkCommandBuffer _commandBuffer, uint32_t imageIndex);

	void updateUniformBuffers(uint32_t currentImage);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void runErosionPipeline(uint32_t index);
	void erode();
	void setupThread();
	void joinThread();

	void primeSemaphores();
};