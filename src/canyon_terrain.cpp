// canyon_terrain.c
#include "common.h"
#include "canyon_terrain.h"
//-----------------------
#include "bounds.h"
#include "camera.h"
#include "canyon.h"
#include "canyon_zone.h"
#include "collision.h"
#include "future.h"
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

#define LowestLod 2

// *** Forward Declarations
canyonTerrainBlock* newBlock( canyonTerrain* t, absolute u, absolute v, engine* e );
void				deleteBlock( canyonTerrainBlock* b );
void				canyonTerrain_calculateBounds( canyon* c, int bounds[2][2], canyonTerrain* t, vector* sample_point );
void				canyonTerrainBlock_calculateExtents( canyonTerrainBlock* b, canyonTerrain* t, absolute u, absolute v );
int					canyonTerrain_lodLevelForBlock( canyon* c, canyonTerrain* t, absolute u, absolute v );

// ***

DECLARE_POOL(canyonTerrainBlock)
IMPLEMENT_POOL(canyonTerrainBlock)

pool_canyonTerrainBlock* static_block_pool = NULL;

void canyonTerrain_staticInit() {
	static_block_pool = pool_canyonTerrainBlock_create( PoolMaxBlocks );

	canyonTerrain_renderInit();
}

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

void canyonTerrain_updateBlocks( canyon* c, canyonTerrain* t, engine* e ) {
	/* We have a set of current blocks, B, and a set of projected blocks based on the new position, B'
	   All blocks ( b | b is in B n B' ) we keep, shifting their pointers to the correct position
	   All other blocks fill up the empty spaces, then are recalculated */
	int bounds[2][2];
	int intersection[2][2];
	vmutex_lock( &t->mutex ); {
		canyonTerrain_calculateBounds( c, bounds, t, &t->sample_point );

		const int vTrim = ((float)bounds[0][1] - 0.5f) * ((2 * t->v_radius) / (float)t->v_block_count);
		//printf( "Trimming up to %d.\n", vTrim );
		terrainCache_trim( c->cache, vTrim );

		if (t->firstUpdate || !boundsEqual( bounds, t->bounds )) {
			boundsIntersection( intersection, bounds, t->bounds );
			canyonTerrainBlock** newBlocks = (canyonTerrainBlock**)stackArray( canyonTerrainBlock*, t->total_block_count );
			memset( newBlocks, 0, sizeof( canyonTerrainBlock* ) * t->total_block_count );

			//for ( int i = 0; i < t->total_block_count; ++i ) {
				//canyonTerrainBlock* b = t->blocks[i];
				//vAssert( (uintptr_t)b < 0x0001000000000000 );
			//}

			for ( int v = 0; v < t->v_block_count; v++ ) {
				for ( int u = 0; u < t->u_block_count; u++ ) {
					int coord[2];
					coord[0] = t->bounds[0][0] + u;
					coord[1] = t->bounds[0][1] + v;
					const int i = canyonTerrain_blockIndexFromUV( t, u, v );
					if ( boundsContains( intersection, coord ) )
						newBlocks[blockCoord(coord, bounds, t->u_block_count)] = t->blocks[i];
					else
						if ( t->blocks[i] ) deleteBlock( t->blocks[i] );
				}
			}

			canyonTerrainBlock** blocks = (canyonTerrainBlock**)stackArray( canyonTerrainBlock*, t->total_block_count );
			int blockCount = 0;
			for ( int v = 0; v < t->v_block_count; v++ ) {
				for ( int u = 0; u < t->u_block_count; u++ ) {
					absolute uu = Absolute(bounds[0][0] + u);
					absolute vv = Absolute(bounds[0][1] + v);
					int coord[2];
					coord[0] = uu.coord;
					coord[1] = vv.coord;
					const int i = canyonTerrain_blockIndexFromUV( t, u, v );
					const int oldLOD = t->blocks[i] ? t->blocks[i]->lod_level : LowestLod;
					const int newLOD = canyonTerrain_lodLevelForBlock( c, t, uu, vv );
					if ( !boundsContains( intersection, coord ) || t->firstUpdate || newLOD < oldLOD ) {
						canyonTerrainBlock* b = newBlock( t, uu, vv, e );
						blocks[blockCount++] = b;
					}
				}
			}
			for ( int i = 0; i < blockCount; ++i ) {
				canyonTerrainBlock* b = blocks[i];
				tell( b->actor, generateVertices( b ));
			}

			memcpy( t->bounds, bounds, sizeof( int ) * 2 * 2 );
			//for ( int i = 0; i < t->total_block_count; ++i ) {
				//canyonTerrainBlock* b = newBlocks[i];
				//vAssert( (uintptr_t)b < 0x0001000000000000 );
			//}
			memcpy( t->blocks, newBlocks, sizeof( canyonTerrainBlock* ) * t->total_block_count );
			bool vl = true;
			for ( int v = 0; v < t->v_block_count; v++ ) {
				for ( int u = 0; u < t->u_block_count; u++ ) {
					const int i = canyonTerrain_blockIndexFromUV( t, u, v );
					vl = vl && (uintptr_t)t->blocks[i] < 0x0001000000000000;
				}
			}
			vAssert( vl );
			t->firstUpdate = false;
		}
	} vmutex_unlock( &t->mutex );
}

void canyonTerrainBlock_tick( void* arg, float dt, engine* e ) {
	(void)dt;(void)e;
	canyonTerrainBlock* b = (canyonTerrainBlock*)arg;
	terrainRenderable* r = b->renderable;
	if (( r->vertex_VBO_alt && *r->vertex_VBO_alt ) && ( r->element_VBO_alt && *r->element_VBO_alt )) {
			if (!b->ready->complete)
				future_complete_( b->ready );
	}
}

canyonTerrainBlock* newBlock( canyonTerrain* t, absolute u, absolute v, engine* e ) {
	canyonTerrainBlock* b = pool_canyonTerrainBlock_allocate( static_block_pool ); 
	memset( b, 0, sizeof( canyonTerrainBlock ));
	b->u_samples = t->uSamplesPerBlock;
	b->v_samples = t->vSamplesPerBlock;
	b->terrain = t;
	b->actor = spawnActor( t->system );
	b->_canyon = t->_canyon;
	b->u = u;
	b->v = v;
	b->renderable = terrainRenderable_create( b );
	canyonTerrainBlock_calculateExtents( b, b->terrain, u, v );
	//printf( "Generating new block 0x" xPTRf " at lod %d.\n", (uintptr_t)b, b->lod_level );
	b->_engine = e;
	startTick( b->_engine, b, canyonTerrainBlock_tick );
	b->ready = future_create();
	return b;
}

void deleteBlock( canyonTerrainBlock* b ) {
	vAssert( b );
	stopTick( b->_engine, b, canyonTerrainBlock_tick );
	terrainBlock_removeCollision( b );
	stopActor( b->actor );
	terrainRenderable_delete( b->renderable );
	pool_canyonTerrainBlock_free( static_block_pool, b );
}

void canyonTerrain_blockContaining( canyon* c, int coord[2], canyonTerrain* t, vector* point ) {
	float u, v;
	canyonSpaceFromWorld( c, point->coord.x, point->coord.z, &u, &v );
	const float block_width = (2 * t->u_radius) / (float)t->u_block_count;
	const float block_height = (2 * t->v_radius) / (float)t->v_block_count;
	coord[0] = fround( u / block_width, 1.f );
	coord[1] = fround( v / block_height, 1.f );
}

int canyonTerrain_lodLevelForBlock( canyon* c, canyonTerrain* t, absolute u, absolute v ) {
	int block[2];
	canyonTerrain_blockContaining( c, block, t, &t->sample_point );
	vAssert( t->lod_interval_u > 0 && t->lod_interval_v > 0 );
#if 1
	return min( LowestLod, ( abs( u.coord - block[0] ) / t->lod_interval_u ) + ( abs( v.coord - block[1]) / t->lod_interval_v ));
#else
	(void)u;(void)v;
	return 2;
#endif
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
	
	canyonTerrainBlock_calculateSamplesForLoD( b->_canyon, b, t, u, v );
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
	vAssert( (uintptr_t)b < 0x0001000000000000 );
	vmutex_lock( &t->mutex ); {
		int i = canyonTerrain_blockIndexFromAbsoluteUV( t, u, v );
		if ( i >= 0 && i < t->total_block_count ) { // We might not still need this block when it finishes
			if ( t->blocks[i] )
				deleteBlock( t->blocks[i] );
			t->blocks[i] = b;
		} else
			deleteBlock( b );
	} vmutex_unlock( &t->mutex );
}


void canyonTerrain_createBlocks( canyon* c, canyonTerrain* t ) {
	vAssert( !t->blocks );
	vAssert( t->u_block_count > 0 );
	vAssert( t->v_block_count > 0 );
	t->total_block_count = t->u_block_count * t->v_block_count;
	t->blocks = (canyonTerrainBlock**)mem_alloc( sizeof( canyonTerrainBlock* ) * t->total_block_count );
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
	canyonTerrain* t = (canyonTerrain*)mem_alloc( sizeof( canyonTerrain ));
	memset( t, 0, sizeof( canyonTerrain ));
	t->_canyon = c;
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

	t->trans = transform_create();
	return t;
}

int lodRatio( canyonTerrainBlock* b ) { return b->u_samples / ( b->terrain->uSamplesPerBlock / 4 ); }
int lodStride( canyonTerrainBlock* b ) { return 4 / lodRatio(b); }

void canyonTerrain_tick( void* data, float dt, engine* eng ) {
	(void)dt; (void)eng;
	canyonTerrain* t = (canyonTerrain*)data;

	vector v = Vector( 0.0, 0.0, 30.0, 1.0 );
	t->sample_point = matrix_vecMul( theScene->cam->trans->world, &v );
	canyon_seekForWorldPosition( t->_canyon, t->sample_point );
	zone_sample_point = t->sample_point;

	terrainCache_tick( t->_canyon->cache, dt, t->sample_point );
	canyonTerrain_updateBlocks( t->_canyon, t, eng );
}

//// External utilities

void terrain_positionsFromUV( canyonTerrain* t, int u_index, int v_index, float* u, float* v ) {
	float uPerBlock = (2 * t->u_radius) / (float)t->u_block_count;
	float vPerBlock = (2 * t->v_radius) / (float)t->v_block_count;
	*u = (float)u_index * uPerBlock / (float)(t->uSamplesPerBlock);
	*v = (float)v_index * vPerBlock / (float)(t->vSamplesPerBlock);
}
