#include "MapSettings.h"

ValidateSettingsErrorCode validateSettings(const MapSettings& settings) {
	if (settings.width <= 0) {
		return ValidateSettingsErrorCode::NegativeWidth;
	}
	else if (settings.height <= 0) {
		return ValidateSettingsErrorCode::NegativeHeight;
	}
	else if (settings.resolution <= 0) {
		return ValidateSettingsErrorCode::NegativeResolution;
	}

	return ValidateSettingsErrorCode::OK;
}

std::string validateSettingsErrorCode_toString(ValidateSettingsErrorCode error_code) {
	switch (error_code) {
		case ValidateSettingsErrorCode::NegativeWidth: return "code 1: Negative width";
		case ValidateSettingsErrorCode::NegativeHeight: return "code 2: Negative height";
		case ValidateSettingsErrorCode::NegativeResolution: return "code 3: Negative resolution";
		default: return "OK";
	}
}