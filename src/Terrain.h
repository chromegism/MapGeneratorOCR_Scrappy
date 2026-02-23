#pragma once

#include <vector>

#include "MapSettings.h"
#include "Perlin.h"

struct TerrainData {
	std::vector<float> heights;
	std::vector<uint32_t> triangleIndices;
	int width = 0;
	int height = 0;
};

class TerrainGenerator {
public:
	TerrainData genTerrain(const MapSettings&);
private:
	std::vector<float> genTerrainHeights(const MapSettings& settings, const PerlinMap& perlin);
	std::vector<uint32_t> genTriangleIndices(const MapSettings& settings);
};

