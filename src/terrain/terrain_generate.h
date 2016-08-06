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

void terrainBlock_build( CanyonTerrainBlock* b, vertPositions* vertSources );
vector terrainPointCached( canyon* c, CanyonTerrainBlock* b, cacheBlocklist* caches, int uRelative, int vRelative );

// Total number of real (not rendered) verts in this block
int vertCount( CanyonTerrainBlock* b );

// Worker Task to generate this block
void* canyonTerrain_workerGenerateBlock( void* args );

// Turn local u,v pair into a vert-array index
int indexFromUV( CanyonTerrainBlock* b, int u, int v );
