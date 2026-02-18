#include "MapGen.h"
#include "Map.h"
#include "Terrain.h"

Map genMap(const MapSettings& settings) {
	Map map;
	genTerrain(settings, map.terrain);

	return map;
}