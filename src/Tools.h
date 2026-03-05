#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include <vulkan/vulkan.h>

std::vector<const char*> stringVectorToCStrVector(const std::vector<std::string>& strings);
std::string vkResultToString(VkResult);
void handleVkResult(VkResult error_code, std::string error_msg);
uint32_t findMemoryType(VkPhysicalDevice _physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkFormat findSupportedFormat(VkPhysicalDevice _physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer _commandBuffer);