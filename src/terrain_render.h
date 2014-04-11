// terrain_render.h
#include "canyon_terrain.h"

// Static init
void canyonTerrain_renderInit();

void canyonTerrainBlock_fillTrianglesForVertex( canyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert );
void canyonTerrain_initVertexBuffers( canyonTerrain* t );
void* canyonTerrain_nextVertexBuffer( canyonTerrain* t );
void canyonTerrain_freeVertexBuffer( canyonTerrain* t, unsigned short* buffer );
void canyonTerrain_initElementBuffers( canyonTerrain* t );
void* canyonTerrain_nextElementBuffer( canyonTerrain* t );
void canyonTerrain_freeElementBuffer( canyonTerrain* t, unsigned short* buffer );
void terrainBlock_initVBO( canyonTerrainBlock* b );

void canyonTerrainBlock_createBuffers( canyonTerrainBlock* b );
int canyonTerrainBlock_renderVertCount( canyonTerrainBlock* b );
terrainRenderable* terrainRenderable_create( canyonTerrainBlock* b );
