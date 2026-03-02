#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan_core.h>

std::vector<const char*> stringVectorToCStrVector(const std::vector<std::string>& strings);
std::string vkResultToString(VkResult);
void handleVkResult(VkResult error_code, std::string error_msg);