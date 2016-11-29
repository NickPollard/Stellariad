// terrain_render.h
#include "canyon_terrain.h"

// *** Static init
void canyonTerrain_renderInit();

// *** Buffers
void	canyonTerrainBlock_fillTrianglesForVertex( CanyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert );
void	canyonTerrain_initVertexBuffers( CanyonTerrain* t );
vertex* canyonTerrain_nextVertexBuffer( CanyonTerrain* t );
void	canyonTerrain_initElementBuffers( CanyonTerrain* t );
short unsigned int* canyonTerrain_nextElementBuffer( CanyonTerrain* t );
brando::concurrent::Future<bool> terrainBlock_initVBO( CanyonTerrainBlock* b );

void canyonTerrainBlock_createBuffers( CanyonTerrainBlock* b );
int canyonTerrainBlock_renderVertCount( CanyonTerrainBlock* b );

void canyonTerrain_render( void* data, scene* s );

// *** Terrain Renderable
terrainRenderable*	terrainRenderable_create( CanyonTerrainBlock* b );
void				terrainRenderable_delete( terrainRenderable* r );
