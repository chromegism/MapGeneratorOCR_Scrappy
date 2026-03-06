#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <array>

#include <vulkan/vulkan.h>

template <typename T>
concept NonPointer = !std::is_pointer_v<T>;

std::vector<const char*> stringVectorToCStrVector(const std::vector<std::string>& strings);
std::string vkResultToString(VkResult);
void handleVkResult(VkResult error_code, std::string error_msg);

uint32_t findMemoryType(VkPhysicalDevice _physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkFormat findSupportedFormat(VkPhysicalDevice _physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkCommandBuffer beginCommand(VkDevice device, VkCommandPool commandPool);

void endCommand(VkCommandBuffer);
void submitSingleCommand(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);
void endAndSubmitCommand(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);
void submitCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, size_t count, const VkCommandBuffer* commandBuffers);

inline void submitCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, const std::vector<VkCommandBuffer>& commandBuffers) {
	submitCommands(device, commandPool, queue, commandBuffers.size(), commandBuffers.data());
}
template <size_t N>
inline void submitCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, const std::array<VkCommandBuffer, N>& commandBuffers) {
	submitCommands(device, commandPool, queue, N, commandBuffers.data());
}