// terrain_generate.h
#include "canyon_terrain.h"

void terrainBlock_build( canyon* c, canyonTerrainBlock* b, vector* vs );
vector lodV( canyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio );
vector lodU( canyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio );
void lodVectors( canyonTerrainBlock* b, vector* vectors);
