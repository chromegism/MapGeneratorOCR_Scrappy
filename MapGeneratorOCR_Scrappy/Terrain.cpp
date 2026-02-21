#include <iostream>
#include <thread>

#include "MapSettings.h"
#include "Perlin.h"
#include "Terrain.h"

std::vector<float> genTerrainHeights(const MapSettings& settings, const PerlinMap& perlin) {
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

void genTerrain(const MapSettings& settings, Terrain& terrain) {
	std::cout << "Generating terrain" << std::endl;

	PerlinMap perlin_map(
		float(settings.width) / float(settings.resolution),
		float(settings.height) / float(settings.resolution),
		settings.perlinOctaves,
		settings.perlinBase
	);

	terrain.width = settings.width;
	terrain.height = settings.height;
	terrain.heights = genTerrainHeights(settings, perlin_map);
}