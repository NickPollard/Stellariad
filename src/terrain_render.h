// terrain_render.h
#include "canyon_terrain.h"

void canyonTerrainBlock_fillTrianglesForVertex( canyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert );
void canyonTerrain_initVertexBuffers( canyonTerrain* t );
void* canyonTerrain_nextVertexBuffer( canyonTerrain* t );
void canyonTerrain_initElementBuffers( canyonTerrain* t );
void* canyonTerrain_nextElementBuffer( canyonTerrain* t );
void canyonTerrainBlock_initVBO( canyonTerrainBlock* b );
