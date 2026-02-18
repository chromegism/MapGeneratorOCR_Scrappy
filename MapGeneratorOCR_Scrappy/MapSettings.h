#pragma once
#include <string>

struct MapSettings {
	int width;
	int height;
	int resolution;
};

enum class ValidateSettingsErrorCode {
	OK,
	NegativeWidth,
	NegativeHeight,
	NegativeResolution,
};

ValidateSettingsErrorCode validateSettings(const MapSettings& settings);

std::string validateSettingsErrorCode_toString(ValidateSettingsErrorCode error_code);