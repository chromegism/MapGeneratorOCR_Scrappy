#pragma once
#include <string>
#include <vector>

#include <cstdint>

struct MapSettings {
	uint32_t width;
	uint32_t height;
	uint32_t resolution;

	std::vector<float> perlinOctaves;
	float perlinBase;

	uint32_t model_x, model_y;
};

enum class ValidateSettingsErrorCode {
	OK,
	NegativeWidth,
	NegativeHeight,
	NegativeResolution,
};

ValidateSettingsErrorCode validateSettings(const MapSettings& settings);

std::string validateSettingsErrorCode_toString(ValidateSettingsErrorCode error_code);