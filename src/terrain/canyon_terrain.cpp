// canyon_terrain.c
#include "common.h"
#include "terrain/canyon_terrain.h"
//-----------------------
#include "bounds.h"
#include "camera.h"
#include "canyon.h"
#include "canyon_zone.h"
#include "collision/collision.h"
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
//#include "terrain/buildCacheTask.h"

#define LowestLod 2

// *** Forward Declarations
CanyonTerrainBlock* newBlock( CanyonTerrain* t, absolute u, absolute v, engine* e );
void				deleteBlock( CanyonTerrainBlock* b );
void				canyonTerrain_calculateBounds( canyon* c, int bounds[2][2], CanyonTerrain* t, vector* sample_point );
void				canyonTerrainBlock_calculateExtents( CanyonTerrainBlock* b, CanyonTerrain* t, absolute u, absolute v );
int					canyonTerrain_lodLevelForBlock( canyon* c, CanyonTerrain* t, absolute u, absolute v );

// ***

DECLARE_POOL(CanyonTerrainBlock)
IMPLEMENT_POOL(CanyonTerrainBlock)

pool_CanyonTerrainBlock* static_block_pool = NULL;

void canyonTerrain_staticInit() {
	static_block_pool = pool_CanyonTerrainBlock_create( PoolMaxBlocks );

	canyonTerrain_renderInit();
}

// ***
int canyonTerrain_blockIndexFromUV( CanyonTerrain* t, int u, int v ) { return u + v * t->u_block_count; }
int canyonTerrain_blockIndexFromAbsoluteUV( CanyonTerrain* t, absolute u, absolute v ) { return u.coord - t->bounds[0][0] + (v.coord - t->bounds[0][1]) * t->u_block_count; }


int blockCoord( int coord[2], int bounds[2][2], int uCount ) {
	return (coord[0] - bounds[0][0]) + (coord[1] - bounds[0][1]) * uCount;
}

absolute Absolute( int a ) {
	absolute ab = { a };
	return ab;
}

void canyonTerrain_updateBlocks( canyon* c, CanyonTerrain* t, engine* e ) {
	/* We have a set of current blocks, B, and a set of projected blocks based on the new position, B'
	   All blocks ( b | b is in B n B' ) we keep, shifting their pointers to the correct position
	   All other blocks fill up the empty spaces, then are recalculated */
	int bounds[2][2];
	int intersection[2][2];
	vmutex_lock( &t->mutex ); {
		canyonTerrain_calculateBounds( c, bounds, t, &t->sample_point );

		const int vTrim = ((float)bounds[0][1] - 0.5f) * ((2 * t->v_radius) / (float)t->v_block_count);
		terrainCache_trim( t->cache, vTrim );

		if (t->firstUpdate || !boundsEqual( bounds, t->bounds )) {
			boundsIntersection( intersection, bounds, t->bounds );
			CanyonTerrainBlock** newBlocks = (CanyonTerrainBlock**)stackArray( CanyonTerrainBlock*, t->total_block_count );
			memset( newBlocks, 0, sizeof( CanyonTerrainBlock* ) * t->total_block_count );

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

			CanyonTerrainBlock** blocks = (CanyonTerrainBlock**)stackArray( CanyonTerrainBlock*, t->total_block_count );
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
					if ( !boundsContains( intersection, coord ) || t->firstUpdate || newLOD < oldLOD )
						blocks[blockCount++] = newBlock( t, uu, vv, e );
				}
			}
			for ( int i = 0; i < blockCount; ++i ) {
				CanyonTerrainBlock* b = blocks[i];
        Executor* exec = &e->ex();
        assert( exec != nullptr );
				tell( b->actor, generateVertices( b, e->ex() ));
			}

			memcpy( t->bounds, bounds, sizeof( int ) * 2 * 2 );
			memcpy( t->blocks, newBlocks, sizeof( CanyonTerrainBlock* ) * t->total_block_count );
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
	CanyonTerrainBlock* b = (CanyonTerrainBlock*)arg;
	terrainRenderable* r = b->renderable;
	if (( r->vertex_VBO_alt && *r->vertex_VBO_alt ) && ( r->element_VBO_alt && *r->element_VBO_alt ))
		b->readyP.complete(true);
}

CanyonTerrainBlock* newBlock( CanyonTerrain* t, absolute u, absolute v, engine* e ) {
	return new (pool_CanyonTerrainBlock_allocate( static_block_pool )) CanyonTerrainBlock( t, u, v, e); 
}

CanyonTerrainBlock::CanyonTerrainBlock( CanyonTerrain* t, absolute _u, absolute _v, engine* e ) :
	u_samples( t->uSamplesPerBlock ),
	v_samples( t->vSamplesPerBlock ),
	u( _u ),
	v( _v ),
	terrain( t ),
	_canyon( t->_canyon ),
	_engine( e ),
  readyP( e->ex() ) {
	actor = spawnActor( t->system );
	renderable = terrainRenderable_create( this );
	canyonTerrainBlock_calculateExtents( this, terrain, u, v );
	startTick( _engine, this, canyonTerrainBlock_tick );
}

void deleteBlock( CanyonTerrainBlock* b ) {
	vAssert( b );
	stopTick( b->_engine, b, canyonTerrainBlock_tick );
	terrainBlock_removeCollision( b );
	stopActor( b->actor );
	terrainRenderable_delete( b->renderable );
	pool_CanyonTerrainBlock_free( static_block_pool, b );
}

void canyonTerrain_blockContaining( canyon* c, int coord[2], CanyonTerrain* t, vector* point ) {
	float u, v;
	canyonSpaceFromWorld( c, point->coord.x, point->coord.z, &u, &v );
	const float block_width = (2 * t->u_radius) / (float)t->u_block_count;
	const float block_height = (2 * t->v_radius) / (float)t->v_block_count;
	coord[0] = fround( u / block_width, 1.f );
	coord[1] = fround( v / block_height, 1.f );
}

int canyonTerrain_lodLevelForBlock( canyon* c, CanyonTerrain* t, absolute u, absolute v ) {
	int block[2];
	canyonTerrain_blockContaining( c, block, t, &t->sample_point );
	vAssert( t->lod_interval_u > 0 && t->lod_interval_v > 0 );
	return min( LowestLod, ( abs( u.coord - block[0] ) / t->lod_interval_u ) + ( abs( v.coord - block[1]) / t->lod_interval_v ));
}

void canyonTerrainBlock_calculateSamplesForLoD( canyon* c, CanyonTerrainBlock* b, CanyonTerrain* t, absolute u, absolute v ) {
	b->lod_level = canyonTerrain_lodLevelForBlock( c, t, u, v );
	// Set samples based on U offset, We add one so that we always get a centre point
	int ratio = max( 1, 2 * b->lod_level );
	b->u_samples = t->uSamplesPerBlock / ratio + 1;
	b->v_samples = t->vSamplesPerBlock / ratio + 1;
}

void canyonTerrainBlock_calculateExtents( CanyonTerrainBlock* b, CanyonTerrain* t, absolute u, absolute v ) {
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
void canyonTerrain_calculateBounds( canyon* c, int bounds[2][2], CanyonTerrain* t, vector* sample_point ) {
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

void CanyonTerrain::setBlock( absolute u, absolute v, CanyonTerrainBlock* b ) {
	vmutex_lock( &mutex ); {
		int i = canyonTerrain_blockIndexFromAbsoluteUV( this, u, v );
		if ( i >= 0 && i < total_block_count ) { // We might not still need this block when it finishes
			if ( blocks[i] )
				deleteBlock( blocks[i] );
			blocks[i] = b;
		} else
			deleteBlock( b );
	} vmutex_unlock( &mutex );
}

void canyonTerrain_createBlocks( canyon* c, CanyonTerrain* t ) {
	vAssert( !t->blocks );
	vAssert( t->u_block_count > 0 );
	vAssert( t->v_block_count > 0 );
	t->total_block_count = t->u_block_count * t->v_block_count;
	t->blocks = (CanyonTerrainBlock**)mem_alloc( sizeof( CanyonTerrainBlock* ) * t->total_block_count );
	memset( t->blocks, 0, sizeof( CanyonTerrainBlock* ) * t->total_block_count );

	canyonTerrain_calculateBounds( c, t->bounds, t, &t->sample_point );
}

CanyonTerrain* canyonTerrain_create( canyon* c, int u_blocks, int v_blocks, int u_samples, int v_samples, float u_radius, float v_radius ) {
	bool b = u_blocks % 2 == 1 && v_blocks % 2 == 1;
	vAssert( b );
	CanyonTerrain* t = (CanyonTerrain*)mem_alloc( sizeof( CanyonTerrain ));
	memset( t, 0, sizeof( CanyonTerrain ));
	t->_canyon = c;
	t->u_block_count = u_blocks;
	t->v_block_count = v_blocks;
	vAssert( u_samples <= kMaxTerrainBlockWidth && v_samples <= kMaxTerrainBlockWidth );
	t->uSamplesPerBlock = u_samples;
	t->vSamplesPerBlock = v_samples;
	t->u_radius = u_radius;
	t->v_radius = v_radius;
	t->firstUpdate = true;
	t->setLodIntervals( 3, 2 );
	t->system = actorSystemCreate();
	vmutex_init( &t->mutex );
	t->cache = terrainCache_create();

	canyonTerrain_initVertexBuffers( t );
	if ( CANYON_TERRAIN_INDEXED ) canyonTerrain_initElementBuffers( t );
	canyonTerrain_createBlocks( c, t );

	t->trans = transform_create();
	return t;
}

int CanyonTerrainBlock::lodRatio() const { return u_samples / ( terrain->uSamplesPerBlock / 4 ); }
int CanyonTerrainBlock::lodStride() const { return 4 / lodRatio(); }

void canyonTerrain_tick( void* data, float dt, engine* eng ) {
	(void)dt; (void)eng;
	CanyonTerrain* t = (CanyonTerrain*)data;

	vector v = Vector( 0.0, 0.0, 30.0, 1.0 );
	t->sample_point = matrix_vecMul( theScene->cam->trans->world, &v );
	canyon_seekForWorldPosition( t->_canyon, t->sample_point );
	zone_sample_point = t->sample_point;

	terrainCache_tick( t->cache, dt, t->sample_point );
	canyonTerrain_updateBlocks( t->_canyon, t, eng );
}

void CanyonTerrain::positionsFromUV( int u_index, int v_index, float* u, float* v ) {
	float uPerBlock = (2 * u_radius) / (float)u_block_count;
	float vPerBlock = (2 * v_radius) / (float)v_block_count;
	*u = (float)u_index * uPerBlock / (float)(uSamplesPerBlock);
	*v = (float)v_index * vPerBlock / (float)(vSamplesPerBlock);
}
