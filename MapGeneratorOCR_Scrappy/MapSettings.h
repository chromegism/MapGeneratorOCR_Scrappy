#pragma once
#include <string>
#include <vector>

struct MapSettings {
	int width;
	int height;
	int resolution;

	std::vector<float> perlinOctaves;
	float perlinBase;
};

enum class ValidateSettingsErrorCode {
	OK,
	NegativeWidth,
	NegativeHeight,
	NegativeResolution,
};

ValidateSettingsErrorCode validateSettings(const MapSettings& settings);

std::string validateSettingsErrorCode_toString(ValidateSettingsErrorCode error_code);