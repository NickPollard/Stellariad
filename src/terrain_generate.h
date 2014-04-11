// terrain_generate.h
#include "canyon_terrain.h"

struct vertPositions_s {
	int uMin;
	int uCount;
	int vMin;
	int vCount;
	vector* positions; // (uCount * vCount) in size
};

void terrainBlock_build( canyonTerrainBlock* b, vertPositions* vertSources );
// TODO
vector terrainPointCached( canyon* c, canyonTerrainBlock* b, int uIndex, int vIndex );
vector terrainPoint( canyon* c, canyonTerrainBlock* b, int uIndex, int vIndex );

// Total number of real (not rendered) verts in this block
int vertCount( canyonTerrainBlock* b );

// Worker Task to generate this block
void* canyonTerrain_workerGenerateBlock( void* args );

// Turn local u,v pair into a vert-array index
int indexFromUV( canyonTerrainBlock* b, int u, int v );
