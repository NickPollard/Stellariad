// terrain_render.h
#include "canyon_terrain.h"

// *** Static init
void canyonTerrain_renderInit();

// *** Buffers
void	canyonTerrainBlock_fillTrianglesForVertex( CanyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert );
brando::concurrent::Future<bool> terrainBlock_initVBO( CanyonTerrainBlock* b );

void canyonTerrainBlock_createBuffers( CanyonTerrainBlock* b );
void canyonTerrainBlock_generateVertices( CanyonTerrainBlock* b, vector* verts, vector* normals );
int canyonTerrainBlock_renderVertCount( CanyonTerrainBlock* b );
int canyonTerrainBlock_renderIndexFromUV( CanyonTerrainBlock* b, int u, int v );

void canyonTerrain_render( void* data, scene* s );

// *** Terrain Renderable
terrainRenderable* terrainRenderable_create( CanyonTerrainBlock* b );
void terrainRenderable_delete( terrainRenderable* r );

