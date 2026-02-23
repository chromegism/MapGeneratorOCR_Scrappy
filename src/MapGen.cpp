#include "MapGen.h"
#include "Map.h"
#include "Terrain.h"

Map genMap(const MapSettings& settings) {
	Map map;
	TerrainGenerator terrainGenerator;
	map.terrain = terrainGenerator.genTerrain(settings);

	return map;
}