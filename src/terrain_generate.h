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
vector lodV( canyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio );
vector lodU( canyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio );
void lodVectors( canyonTerrainBlock* b, vector* vectors);
void vertPositions_delete( vertPositions* vs );
// TODO
vector terrainPointCached( canyon* c, canyonTerrainBlock* b, int uIndex, int vIndex );
