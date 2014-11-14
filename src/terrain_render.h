// terrain_render.h
#include "canyon_terrain.h"

// *** Static init
void canyonTerrain_renderInit();

// *** Buffers
void	canyonTerrainBlock_fillTrianglesForVertex( canyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert );
void	canyonTerrain_initVertexBuffers( canyonTerrain* t );
vertex* canyonTerrain_nextVertexBuffer( canyonTerrain* t );
void	canyonTerrain_initElementBuffers( canyonTerrain* t );
short unsigned int* canyonTerrain_nextElementBuffer( canyonTerrain* t );
future* terrainBlock_initVBO( canyonTerrainBlock* b );

void canyonTerrainBlock_createBuffers( canyonTerrainBlock* b );
int canyonTerrainBlock_renderVertCount( canyonTerrainBlock* b );

// *** Terrain Renderable
terrainRenderable*	terrainRenderable_create( canyonTerrainBlock* b );
void				terrainRenderable_delete( terrainRenderable* r );
