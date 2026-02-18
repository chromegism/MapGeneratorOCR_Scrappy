#pragma once

#include <vector>

#include "MapSettings.h"

struct Terrain {
	std::vector<float> heights;
	int width = 0;
	int height = 0;
};


void genTerrain(const MapSettings&, Terrain&);