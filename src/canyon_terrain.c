// canyon_terrain.c
#include "common.h"
#include "canyon_terrain.h"
//-----------------------
#include "bounds.h"
#include "camera.h"
#include "canyon.h"
#include "canyon_zone.h"
#include "collision.h"
#include "terrain_collision.h"
#include "terrain_generate.h"
#include "terrain_render.h"
#include "worker.h"
#include "actor/actor.h"
#include "base/pair.h"
#include "maths/geometry.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/texture.h"
#include "terrain/cache.h"
#include "terrain/buildCacheTask.h"

// *** Forward Declarations
canyonTerrainBlock* canyonTerrainBlock_create( canyonTerrain* t, absolute u, absolute v );
void				canyonTerrainBlock_delete( canyonTerrainBlock* b );
void				canyonTerrain_updateBlocks( canyon* c, canyonTerrain* t );
void				canyonTerrain_calculateBounds( canyon* c, int bounds[2][2], canyonTerrain* t, vector* sample_point );

// ***
int canyonTerrain_blockIndexFromUV( canyonTerrain* t, int u, int v ) { return u + v * t->u_block_count; }
int canyonTerrain_blockIndexFromAbsoluteUV( canyonTerrain* t, absolute u, absolute v ) { return u.coord - t->bounds[0][0] + (v.coord - t->bounds[0][1]) * t->u_block_count; }

int blockCoord( int coord[2], int bounds[2][2], int uCount ) {
	return (coord[0] - bounds[0][0]) + (coord[1] - bounds[0][1]) * uCount;
}

absolute Absolute( int a ) {
	absolute ab = { a };
	return ab;
}

void canyonTerrain_updateBlocks( canyon* c, canyonTerrain* t ) {
	/* We have a set of current blocks, B, and a set of projected blocks based on the new position, B'
	   Calculate the intersection I = B n B';
	   All blocks ( b | b is in I ) we keep, shifting their pointers to the correct position
	   All other blocks fill up the empty spaces, then are recalculated
	   The block pointer array remains sorted */
	int bounds[2][2];
	int intersection[2][2];
	vmutex_lock( &t->mutex ); {
		canyonTerrain_calculateBounds( c, bounds, t, &t->sample_point );

		const int vTrim = ((float)bounds[0][1] - 0.5f) * ((2 * t->v_radius) / (float)t->v_block_count);
		terrainCache_trim( c->terrainCache, vTrim );

		if (t->firstUpdate || !boundsEqual( bounds, t->bounds )) {
			boundsIntersection( intersection, bounds, t->bounds );
			printf( "New bounds: %d %d -> %d %d.\n", bounds[0][0], bounds[0][1], bounds[1][0], bounds[1][1] );
			canyonTerrainBlock** newBlocks = stackArray( canyonTerrainBlock*, t->total_block_count );
			memset( newBlocks, 0, sizeof( canyonTerrainBlock* ) * t->total_block_count );

			for ( int v = 0; v < t->v_block_count; v++ ) {
				for ( int u = 0; u < t->u_block_count; u++ ) {
					int coord[2];
					coord[0] = t->bounds[0][0] + u;
					coord[1] = t->bounds[0][1] + v;
					int i = canyonTerrain_blockIndexFromUV( t, u, v );
					if ( boundsContains( intersection, coord ) )
						newBlocks[blockCoord(coord, bounds, t->u_block_count)] = t->blocks[i];
					else if ( t->blocks[i] )
						canyonTerrainBlock_delete( t->blocks[i] );
				}
			}

			for ( int v = 0; v < t->v_block_count; v++ ) {
				for ( int u = 0; u < t->u_block_count; u++ ) {
					int coord[2];
					coord[0] = bounds[0][0] + u;
					coord[1] = bounds[0][1] + v;
					if ( !boundsContains( intersection, coord ) || t->firstUpdate ) {
						canyonTerrainBlock* b = canyonTerrainBlock_create( t, Absolute(coord[0]), Absolute(coord[1]) );
						printf( "Queuing block %d %d.\n", coord[0], coord[1] );
						actorMsg( b->actor, generateVertices( b ));
					}
				}
			}

			memcpy( t->bounds, bounds, sizeof( int ) * 2 * 2 );
			memcpy( t->blocks, newBlocks, sizeof( canyonTerrainBlock* ) * t->total_block_count );
			t->firstUpdate = false;
		}
	} vmutex_unlock( &t->mutex );
}

canyonTerrainBlock* canyonTerrainBlock_create( canyonTerrain* t, absolute u, absolute v ) {
	canyonTerrainBlock* b = mem_alloc( sizeof( canyonTerrainBlock ));
	memset( b, 0, sizeof( canyonTerrainBlock ));
	b->u_samples = t->uSamplesPerBlock;
	b->v_samples = t->vSamplesPerBlock;
	b->terrain = t;
	b->actor = spawnActor( t->system );
	b->canyon = t->canyon;
	b->u = u;
	b->v = v;
	b->renderable = terrainRenderable_create( b );
	canyonTerrainBlock_calculateExtents( b, b->terrain, u, v );
	return b;
}

void canyonTerrainBlock_delete( canyonTerrainBlock* b ) {
	canyonTerrain_freeElementBuffer( b->terrain, b->renderable->element_buffer );
	canyonTerrain_freeVertexBuffer( b->terrain, (unsigned short*)b->renderable->vertex_buffer );
	terrainBlock_removeCollision( b );
	mem_free( b->renderable );
	stopActor( b->actor );
	mem_free( b ); 
}

void canyonTerrain_blockContaining( canyon* c, int coord[2], canyonTerrain* t, vector* point ) {
	float u, v;
	canyonSpaceFromWorld( c, point->coord.x, point->coord.z, &u, &v );
	float block_width = (2 * t->u_radius) / (float)t->u_block_count;
	float block_height = (2 * t->v_radius) / (float)t->v_block_count;
	coord[0] = fround( u / block_width, 1.f );
	coord[1] = fround( v / block_height, 1.f );
}

int canyonTerrain_lodLevelForBlock( canyon* c, canyonTerrain* t, absolute u, absolute v ) {
	int block[2];
	canyonTerrain_blockContaining( c, block, t, &t->sample_point );
	vAssert( t->lod_interval_u > 0 );
	vAssert( t->lod_interval_v > 0 );
	return min( 2, ( abs( u.coord - block[0] ) / t->lod_interval_u ) + ( abs( v.coord - block[1]) / t->lod_interval_v ));
}

void canyonTerrainBlock_calculateSamplesForLoD( canyon* c, canyonTerrainBlock* b, canyonTerrain* t, absolute u, absolute v ) {
	b->lod_level = canyonTerrain_lodLevelForBlock( c, t, u, v );
	// Set samples based on U offset, We add one so that we always get a centre point
	int ratio = max( 1, 2 * b->lod_level );
	b->u_samples = t->uSamplesPerBlock / ratio + 1;
	b->v_samples = t->vSamplesPerBlock / ratio + 1;
}

void canyonTerrainBlock_calculateExtents( canyonTerrainBlock* b, canyonTerrain* t, absolute u, absolute v ) {
	float uf = (2 * t->u_radius) / (float)t->u_block_count;
	float vf = (2 * t->v_radius) / (float)t->v_block_count;
	b->u_min = ((float)u.coord - 0.5f) * uf;
	b->v_min = ((float)v.coord - 0.5f) * vf;
	b->u_max = b->u_min + uf;
	b->v_max = b->v_min + vf;
	b->uMin = u.coord * t->uSamplesPerBlock - (t->uSamplesPerBlock / 2);
	b->vMin = v.coord * t->vSamplesPerBlock - (t->vSamplesPerBlock / 2);
	
	canyonTerrainBlock_calculateSamplesForLoD( b->canyon, b, t, u, v );
}

// Calculate the block bounds for the terrain, at a given sample point
void canyonTerrain_calculateBounds( canyon* c, int bounds[2][2], canyonTerrain* t, vector* sample_point ) {
	/* Find the block we are in, Then extend by the correct radius
	   The center block [0] is from -half_block_width to half_block_width */
	int block[2];
	canyonTerrain_blockContaining( c, block, t, sample_point );
	int rx = ( t->u_block_count - 1 ) / 2;
	int ry = ( t->v_block_count - 1 ) / 2;
	bounds[0][0] = block[0] - rx;
	bounds[0][1] = block[1] - ry;
	bounds[1][0] = block[0] + rx;
	bounds[1][1] = block[1] + ry;
}

void terrain_setBlock( canyonTerrain* t, absolute u, absolute v, canyonTerrainBlock* b ) {
	vmutex_lock( &t->mutex ); {
		int i = canyonTerrain_blockIndexFromAbsoluteUV( t, u, v );
		int local_u = u.coord - t->bounds[0][0];
		int local_v = v.coord - t->bounds[0][1];
		if ( i >= 0 && i <= t->total_block_count ) { // We might not still need this block when it finishes
			printf( "Setting local block %d %d as %d %d.\n", local_u, local_v, u.coord, v.coord );
			if ( t->blocks[i] )
				canyonTerrainBlock_delete( t->blocks[i] );
			t->blocks[i] = b;
		} else {
			printf( "Not setting block %d %d.\n", u.coord, v.coord );
			canyonTerrainBlock_delete( b );
		}
	} vmutex_unlock( &t->mutex );
}


void canyonTerrain_createBlocks( canyon* c, canyonTerrain* t ) {
	vAssert( !t->blocks );
	vAssert( t->u_block_count > 0 );
	vAssert( t->v_block_count > 0 );
	t->total_block_count = t->u_block_count * t->v_block_count;
	t->blocks = mem_alloc( sizeof( canyonTerrainBlock* ) * t->total_block_count );
	memset( t->blocks, 0, sizeof( canyonTerrainBlock* ) * t->total_block_count );

	canyonTerrain_calculateBounds( c, t->bounds, t, &t->sample_point );
}

void canyonTerrain_setLodIntervals( canyonTerrain* t, int u, int v ) {
	t->lod_interval_u = u;
	t->lod_interval_v = v;
}

canyonTerrain* canyonTerrain_create( canyon* c, int u_blocks, int v_blocks, int u_samples, int v_samples, float u_radius, float v_radius ) {
	bool b = u_blocks % 2 == 1 && v_blocks % 2 == 1;
	vAssert( b );
	canyonTerrain* t = mem_alloc( sizeof( canyonTerrain ));
	memset( t, 0, sizeof( canyonTerrain ));
	t->canyon = c;
	t->u_block_count = u_blocks;
	t->v_block_count = v_blocks;
	vAssert( u_samples <= kMaxTerrainBlockWidth && v_samples <= kMaxTerrainBlockWidth );
	t->uSamplesPerBlock = u_samples;
	t->vSamplesPerBlock = v_samples;
	t->u_radius = u_radius;
	t->v_radius = v_radius;
	t->firstUpdate = true;
	canyonTerrain_setLodIntervals( t, 3, 2 );
	t->system = actorSystemCreate();
	vmutex_init( &t->mutex );

	canyonTerrain_initVertexBuffers( t );
	if ( CANYON_TERRAIN_INDEXED ) canyonTerrain_initElementBuffers( t );
	canyonTerrain_createBlocks( c, t );
	canyonTerrain_updateBlocks( c, t );

	t->trans = transform_create();
	return t;
}

int lodRatio( canyonTerrainBlock* b ) { return b->u_samples / ( b->terrain->uSamplesPerBlock / 4 ); }

void canyonTerrain_tick( void* data, float dt, engine* eng ) {
	(void)dt; (void)eng;
	canyonTerrain* t = data;

	vector v = Vector( 0.0, 0.0, 30.0, 1.0 );
	t->sample_point = matrix_vecMul( theScene->cam->trans->world, &v );
	canyon_seekForWorldPosition( t->canyon, t->sample_point );
	zone_sample_point = t->sample_point;

	canyonTerrain_updateBlocks( t->canyon, t );
}

//// External utilities

void terrain_positionsFromUV( canyonTerrain* t, int u_index, int v_index, float* u, float* v ) {
	float uPerBlock = (2 * t->u_radius) / (float)t->u_block_count;
	float uScale = uPerBlock / (float)(t->uSamplesPerBlock);
	float vPerBlock = (2 * t->v_radius) / (float)t->v_block_count;
	float vScale = vPerBlock / (float)(t->vSamplesPerBlock);
	*u = (float)u_index * uScale;
	*v = (float)v_index * vScale;
}
