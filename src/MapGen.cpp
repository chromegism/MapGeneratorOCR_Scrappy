#include "MapGen.h"
#include "Map.h"
#include "Terrain.h"

Map genMap(const MapSettings& settings) {
	Map map;
	TerrainGenerator terrainGenerator(settings);
	map.terrain = terrainGenerator.genTerrain();

	return map;
}