// canyon_terrain.h
#pragma once
#include "frustum.h"
#include "actor/actor.h"
#include "render/render.h"
#include "system/thread.h"

#define PoolMaxBlocks 512

#define CANYON_TERRAIN_INDEXED 1

#define kMaxTerrainBlockWidth 80
#define kMaxTerrainBlockElements (kMaxTerrainBlockWidth * kMaxTerrainBlockWidth * 6)

typedef struct absolute_s {
	int coord;
} absolute;

struct canyonTerrainBlock_s {
	int u_samples;
	int v_samples;
	absolute u;
	absolute v;

	float u_min;
	float u_max;
	float v_min;
	float v_max;

	// Ints for uv-coord in terrain grid units
	int uMin; // TODO are these now dupes of absolute_u,v?
	int vMin;

	int	lod_level;	// Current lod-level

	// *** Collision
	body*			collision;

	canyonTerrain* terrain;
	canyon* canyon;

	ActorRef actor;

	terrainRenderable* renderable;
};

struct terrainRenderable_s {
	canyonTerrainBlock* block;

	int element_count;
	int element_count_render; // The one currently used to render with; for smooth LoD switching
	unsigned short* element_buffer;
	vertex* vertex_buffer;

	aabb	bb;

	// *** GPU We double-buffer the terrain blocks for LOD purposes, so we can switch instantaneously
	GLuint*			vertex_VBO;
	GLuint*			element_VBO;
	GLuint*			vertex_VBO_alt;
	GLuint*			element_VBO_alt;
} ;

struct canyonTerrain_s {
	transform* trans;
	actorSystem* system;

	float	u_radius;
	float	v_radius;
	int u_block_count;
	int v_block_count;
	int total_block_count;
	canyonTerrainBlock** blocks;
	
	int uSamplesPerBlock;
	int vSamplesPerBlock;

	int lod_interval_u;
	int lod_interval_v;
	
	int				bounds[2][2];
	vector			sample_point;

	canyon*				canyon;
	unsigned short**	vertex_buffers;
	int					vertex_buffer_count;
	unsigned short**	element_buffers;
	int					element_buffer_count;

	bool firstUpdate;
	vmutex mutex;
};

extern texture* terrain_texture;
extern texture* terrain_texture_cliff;

// *** Functions 
void canyonTerrain_staticInit();

canyonTerrain* canyonTerrain_create( canyon* c, int u_blocks, int v_blocks, int u_samples, int v_samples, float u_radius, float v_radius );
void canyonTerrain_setLodIntervals( canyonTerrain* t, int u, int v );
void canyonTerrain_render( void* data, scene* s );
void canyonTerrain_tick( void* data, float dt, engine* eng );

float canyonTerrain_sample( canyon* c, float u, float v );
float canyonTerrain_sampleUV( float u, float v );

int lodRatio( canyonTerrainBlock* b );
int canyonTerrainBlock_renderIndexFromUV( canyonTerrainBlock* b, int u, int v );

// To move to terrain_generate
void canyonTerrainBlock_generateVerts( canyon* c, canyonTerrainBlock* b, vector* verts );
void canyonTerrainBlock_generateVertices( canyonTerrainBlock* b, vector* verts, vector* normals );
void terrain_positionsFromUV( canyonTerrain* t, int u_index, int v_index, float* u, float* v );

void terrain_setBlock( canyonTerrain* t, absolute u, absolute v, canyonTerrainBlock* b );
