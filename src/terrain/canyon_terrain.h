// canyon_terrain.h
#pragma once
#include "frustum.h"
#include "actor/actor.h"
#include "render/render.h"
#include "system/thread.h"

#include "concurrent/future.h"

#define PoolMaxBlocks 768

#define CANYON_TERRAIN_INDEXED 1

#define kMaxTerrainBlockWidth 80
#define kMaxTerrainBlockElements (kMaxTerrainBlockWidth * kMaxTerrainBlockWidth * 6)

struct absolute {
	int coord;
};

struct CanyonTerrainBlock {

  CanyonTerrainBlock( CanyonTerrain* t, absolute _u, absolute _v, engine* e );

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

	CanyonTerrain* terrain;
	canyon* _canyon;

	ActorRef actor;

	terrainRenderable* renderable;
	engine* _engine;
	brando::concurrent::Promise<bool> readyP;

	int lodRatio() const;
	int lodStride() const;
	auto ready() -> brando::concurrent::Future<bool> { return readyP.future(); }
};

// Move to terrainRender
struct terrainRenderable_s {
	CanyonTerrainBlock* block;

	shader** terrainShader;
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
};

struct CanyonTerrain {
	transform* trans;
	actorSystem* system;

	float	u_radius;
	float	v_radius;
	int u_block_count;
	int v_block_count;
	int total_block_count;
	CanyonTerrainBlock** blocks;
	
	int uSamplesPerBlock;
	int vSamplesPerBlock;

	int lod_interval_u;
	int lod_interval_v;
	
	int				bounds[2][2];
	vector			sample_point;

  TerrainCache* cache;
	canyon*				_canyon;
	vertex**			vertex_buffers;
	int					vertex_buffer_count;
	unsigned short**	element_buffers;
	int					element_buffer_count;

	bool firstUpdate;
	vmutex mutex;

	void setLodIntervals( int u, int v ) {
		lod_interval_u = u;
		lod_interval_v = v;
	}

	void setBlock( absolute u, absolute v, CanyonTerrainBlock* b );
	void positionsFromUV( int u_index, int v_index, float* u, float* v );
};

extern texture* terrain_texture;
extern texture* terrain_texture_cliff;

// *** Functions 
void canyonTerrain_staticInit();

CanyonTerrain* canyonTerrain_create( canyon* c, int u_blocks, int v_blocks, int u_samples, int v_samples, float u_radius, float v_radius );

void canyonTerrain_tick( void* data, float dt, engine* eng );

float canyonTerrain_sample( canyon* c, float u, float v );
float canyonTerrain_sampleUV( float u, float v );

int canyonTerrainBlock_renderIndexFromUV( CanyonTerrainBlock* b, int u, int v );