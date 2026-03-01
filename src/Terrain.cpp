#include <iostream>
#include <thread>
#include <chrono>

#include "MapSettings.h"
#include "Perlin.h"
#include "Terrain.h"


void TerrainGenerator::genTerrainHeightsInto(float* buffer) const {
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

std::vector<float> TerrainGenerator::genTerrainHeights() const {
    std::vector<float> buffer(details.width * details.height);
    genTerrainHeightsInto(buffer.data());
    return buffer;
}

std::vector<uint32_t> TerrainGenerator::genTriangleIndices() const {
    const uint32_t length = calcIndicesLength();

    std::vector<uint32_t> indices;
    indices.resize(length);

    genTriangleIndicesInto(indices.data());

    return indices;
}

void TerrainGenerator::genVertexModelInto(glm::vec2* buffer) const {
    for (uint32_t x = 0; x < details.model_x; x++) {
        for (uint32_t y = 0; y < details.model_y; y++) {
            const uint32_t index = x * details.model_y + y;

            buffer[index].x = float(x) / float(details.model_x);
            buffer[index].y = float(y) / float(details.model_y);
        }
    }
}

void TerrainGenerator::genTriangleIndicesInto(uint32_t* buffer) const {
    const uint32_t length = calcIndicesLength();

    const uint32_t stride = 2 * details.model_x - 1;

    uint32_t i = 0;

    for (uint32_t row = 0; row < length / stride; ++row)
    {
        const bool increasing = (row & 1) == 0;
        const uint32_t rowBase = row * details.model_x;

        for (uint32_t col = 0; col < stride; ++col, ++i)
        {
            uint32_t base = col >> 1;          // col / 2
            uint32_t upper = (col & 1) * details.model_x;

            uint32_t n;

            if (increasing)
                n = base + upper;
            else
                n = (details.model_x - base - 1) + upper;

            buffer[i] = n + rowBase;
        }
    }
}

TerrainData TerrainGenerator::genTerrain() {
    TerrainData terrain{};

    if (!initialisedPerlin) {
        perlin = PerlinMap(
            float(details.width) / float(details.resolution),
            float(details.height) / float(details.resolution),
            details.perlinOctaves, details.base);
        initialisedPerlin = true;
    }

    perlin.regenerate();

	terrain.width = details.width;
	terrain.height = details.height;
	terrain.heights = genTerrainHeights();
    terrain.triangleIndices = genTriangleIndices();

    return terrain;
}

void TerrainGenerator::genTerrainInto(float* buffer) {
    if (!initialisedPerlin) {
        perlin = PerlinMap(
            float(details.width) / float(details.resolution),
            float(details.height) / float(details.resolution),
            details.perlinOctaves, details.base);
        initialisedPerlin = true;
    }

    perlin.regenerate();

    genTerrainHeightsInto(buffer);
}

uint32_t TerrainGenerator::calcIndicesLength() const {
    return 1 + (2 * details.model_x - 1) * (details.model_y - 1);
}