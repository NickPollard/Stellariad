// terrain_render.h
#include "canyon_terrain.h"

// *** Static init
void canyonTerrain_renderInit();

// *** Buffers
void	canyonTerrainBlock_fillTrianglesForVertex( CanyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert );
void	canyonTerrain_initVertexBuffers( canyonTerrain* t );
vertex* canyonTerrain_nextVertexBuffer( canyonTerrain* t );
void	canyonTerrain_initElementBuffers( canyonTerrain* t );
short unsigned int* canyonTerrain_nextElementBuffer( canyonTerrain* t );
brando::concurrent::Future<bool> terrainBlock_initVBO( CanyonTerrainBlock* b );

void canyonTerrainBlock_createBuffers( CanyonTerrainBlock* b );
int canyonTerrainBlock_renderVertCount( CanyonTerrainBlock* b );

// *** Terrain Renderable
terrainRenderable*	terrainRenderable_create( CanyonTerrainBlock* b );
void				terrainRenderable_delete( terrainRenderable* r );
