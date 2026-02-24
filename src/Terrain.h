#pragma once

#include <vector>
#include <thread>

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
	TerrainGenerator(const MapSettings& settings): perlin(
		float(settings.width) / float(settings.resolution),
		float(settings.height) / float(settings.resolution),
		settings.perlinOctaves, settings.perlinBase)
	{ 
		initialisedPerlin = true;
		updateDetails(settings); 
	}
	TerrainGenerator() : perlin() { updateDetails({}); }

	TerrainData genTerrain();
	void genTerrainInto(float* buffer);

	std::vector<uint32_t> genTriangleIndices() const;
	void genTriangleIndicesInto(uint32_t* buffer) const;

	uint32_t calcIndicesLength() const;

	void updateDetails(const MapSettings& settings) {
		details.width = settings.width; details.height = settings.height; details.resolution = settings.resolution;
		details.perlinOctaves = settings.perlinOctaves, details.base = settings.perlinBase;
	}

	struct {
		uint32_t width = 0, height = 0, resolution = 0;
		std::vector<float> perlinOctaves = {};
		float base = 0.f;
	} details;
private:
	bool initialisedPerlin = false;
	PerlinMap perlin; 
	void genTerrainHeightsInto(float* buffer) const;
	std::vector<float> genTerrainHeights() const;
};

