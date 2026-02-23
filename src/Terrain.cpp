#include <iostream>
#include <thread>
#include <chrono>

#include "MapSettings.h"
#include "Perlin.h"
#include "Terrain.h"

std::vector<float> TerrainGenerator::genTerrainHeights(const MapSettings& settings, const PerlinMap& perlin) {
    std::vector<float> heights(settings.width * settings.height);

    const float invW = 1.0f / settings.width;
    const float invH = 1.0f / settings.height;

    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 4;

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    unsigned int rowsPerThread = settings.height / threadCount;

    for (unsigned int t = 0; t < threadCount; t++) {
        unsigned int startRow = t * rowsPerThread;
        unsigned int endRow = (t == threadCount - 1)
            ? settings.height
            : startRow + rowsPerThread;

        threads.emplace_back([&, startRow, endRow]() {

            for (unsigned int y = startRow; y < endRow; y++) {
                float fy = y * invH;

                for (int x = 0; x < settings.width; x++) {
                    heights[x + y * settings.width] =
                        perlin.index(x * invW, fy);
                }
            }

            });
    }

    for (auto& th : threads)
        th.join();

    return heights;
}

std::vector<uint32_t> TerrainGenerator::genTriangleIndices(const MapSettings& settings) {
    const uint32_t width = settings.width, height = settings.height;
    const uint32_t length = 1 + (2 * width - 1) * (height - 1);

    std::vector<uint32_t> indices;
    indices.resize(length);

    const uint32_t stride = 2 * width - 1;

    uint32_t i = 0;

    for (uint32_t row = 0; row < length / stride; ++row)
    {
        const bool increasing = (row & 1) == 0;
        const uint32_t rowBase = row * width;

        for (uint32_t col = 0; col < stride; ++col, ++i)
        {
            uint32_t base = col >> 1;          // col / 2
            uint32_t upper = (col & 1) * width;

            uint32_t n;

            if (increasing)
                n = base + upper;
            else
                n = (width - base - 1) + upper;

            indices.at(i) = n + rowBase;
        }
    }
    
    return indices;
}

TerrainData TerrainGenerator::genTerrain(const MapSettings& settings) {
	std::cout << "Generating terrain" << std::endl;

	PerlinMap perlin_map(
		float(settings.width) / float(settings.resolution),
		float(settings.height) / float(settings.resolution),
		settings.perlinOctaves,
		settings.perlinBase
	);

    TerrainData terrain{};

	terrain.width = settings.width;
	terrain.height = settings.height;
	terrain.heights = genTerrainHeights(settings, perlin_map);
    terrain.triangleIndices = genTriangleIndices(settings);

    return terrain;
}