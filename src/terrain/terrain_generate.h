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

void   terrainBlock_build( CanyonTerrainBlock* b, vertPositions* vertSources );

// Total number of real (not rendered) verts in this block
int vertCount( CanyonTerrainBlock* b );

// Worker Task to generate this block
void canyonTerrainBlock_generate( vertPositions* vs, CanyonTerrainBlock* b );
void vertPositions_delete( vertPositions* vs );

// Turn local u,v pair into a vert-array index
int indexFromUV( CanyonTerrainBlock* b, int u, int v );
