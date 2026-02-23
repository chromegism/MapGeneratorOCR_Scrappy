#include <iostream>
#include <thread>
#include <chrono>

#include "MapSettings.h"
#include "Perlin.h"
#include "Terrain.h"

void TerrainGenerator::genTerrainHeightsInto(const PerlinMap& perlin, float* buffer) const {
    const float invW = 1.0f / details.width;
    const float invH = 1.0f / details.height;

    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 4;

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    unsigned int rowsPerThread = details.height / threadCount;

    for (unsigned int t = 0; t < threadCount; t++) {
        unsigned int startRow = t * rowsPerThread;
        unsigned int endRow = (t == threadCount - 1)
            ? details.height
            : startRow + rowsPerThread;

        threads.emplace_back([&, startRow, endRow]() {

            for (unsigned int y = startRow; y < endRow; y++) {
                float fy = y * invH;

                for (unsigned x = 0; x < details.width; x++) {
                    buffer[x + y * details.width] =
                        perlin.index(x * invW, fy);
                }
            }

            });
    }

    for (auto& th : threads)
        th.join();
}

std::vector<float> TerrainGenerator::genTerrainHeights(const PerlinMap& perlin) const {
    std::vector<float> buffer(details.width * details.height);
    genTerrainHeightsInto(perlin, buffer.data());
    return buffer;
}

std::vector<uint32_t> TerrainGenerator::genTriangleIndices() const {
    const uint32_t length = calcIndicesLength();

    std::vector<uint32_t> indices;
    indices.resize(length);

    genTriangleIndicesInto(indices.data());

    return indices;
}

void TerrainGenerator::genTriangleIndicesInto(uint32_t* buffer) const {
    const uint32_t length = calcIndicesLength();

    const uint32_t stride = 2 * details.width - 1;

    uint32_t i = 0;

    for (uint32_t row = 0; row < length / stride; ++row)
    {
        const bool increasing = (row & 1) == 0;
        const uint32_t rowBase = row * details.width;

        for (uint32_t col = 0; col < stride; ++col, ++i)
        {
            uint32_t base = col >> 1;          // col / 2
            uint32_t upper = (col & 1) * details.width;

            uint32_t n;

            if (increasing)
                n = base + upper;
            else
                n = (details.width - base - 1) + upper;

            buffer[i] = n + rowBase;
        }
    }
}

TerrainData TerrainGenerator::genTerrain() const {
	std::cout << "Generating terrain" << std::endl;

	PerlinMap perlin_map(
		float(details.width) / float(details.resolution),
		float(details.height) / float(details.resolution),
        details.perlinOctaves,
        details.base
	);

    TerrainData terrain{};

	terrain.width = details.width;
	terrain.height = details.height;
	terrain.heights = genTerrainHeights(perlin_map);
    terrain.triangleIndices = genTriangleIndices();

    return terrain;
}

void TerrainGenerator::genTerrainInto(float* buffer) const {
    std::cout << "Generating terrain" << std::endl;

    PerlinMap perlin_map(
        float(details.width) / float(details.resolution),
        float(details.height) / float(details.resolution),
        details.perlinOctaves,
        details.base
    );

    genTerrainHeightsInto(perlin_map, buffer);
}

inline uint32_t TerrainGenerator::calcIndicesLength() const {
    return 1 + (2 * details.width - 1) * (details.height - 1);
}