// terrain_generate.h
#include "canyon_terrain.h"
#include "terrain/cache.h"

struct vertPositions_s {
	int uMin;
	int uCount;
	int vMin;
	int vCount;
	vector* positions; // (uCount * vCount) in size
};

// Worker Task to generate this block
void canyonTerrainBlock_generate( vertPositions* vs, CanyonTerrainBlock* b );
void vertPositions_delete( vertPositions* vs );
