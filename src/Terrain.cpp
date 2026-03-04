#include "pch.h"

#include "MapSettings.h"
#include "Perlin.h"
#include "Terrain.h"



void TerrainGenerator::genTerrainHeightsInto(float* buffer) const {
    perlin.genInto(buffer, details.width, details.height);
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

void TerrainGenerator::genVertexChunksInto(glm::vec2* buffer) const {
    for (uint32_t x = 0; x < details.chunks_x; x++) {
        for (uint32_t y = 0; y < details.chunks_y; y++) {
            const uint32_t index = x * details.chunks_y + y;

            buffer[index].x = float(x) / float(details.chunks_x);
            buffer[index].y = float(y) / float(details.chunks_y);
        }
    }
}

void TerrainGenerator::genTriangleIndicesInto(uint32_t* buffer) const {
    const uint32_t length = calcIndicesLength();

    const uint32_t stride = 2 * details.chunks_x - 1;

    uint32_t index = 0;

    for (uint32_t row = 0; row < length / stride; ++row)
    {
        const bool increasing = (row & 1) == 0;
        const uint32_t rowBase = row * details.chunks_x;

        for (uint32_t col = 0; col < stride; col++, index++)
        {
            uint32_t base = col >> 1;          // col / 2
            uint32_t upper = (col & 1) * details.chunks_x;

            uint32_t n;

            if (increasing)
                n = base + upper;
            else
                n = (details.chunks_x - base - 1) + upper;

            buffer[index] = n + rowBase;
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

    perlin.clear();

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

    perlin.clear();
}

uint32_t TerrainGenerator::calcIndicesLength() const {
    return 1 + (2 * details.chunks_x - 1) * (details.chunks_y - 1);
}