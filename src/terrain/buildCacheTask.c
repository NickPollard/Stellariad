// buildCacheTask.c
#include "src/common.h"
#include "src/terrain/buildCacheTask.h"
//---------------------
#include "canyon.h"
#include "canyon_terrain.h"
#include "terrain_generate.h"
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"

IMPLEMENT_LIST(cacheBlock);
IMPLEMENT_LIST(cacheGrid);

vmutex terrainMutex = kMutexInitialiser;

terrainCache* terrainCache_create() {
	terrainCache* t = mem_alloc(sizeof(terrainCache));
	t->blocks = cacheBlocklist_create();
	return t;
}

// Find the element in the list and return it, orElse none
cacheBlock* terrainCached_( terrainCache* cache, int uMin, int vMin ) {
	cacheBlock* block = NULL;
	vmutex_lock( &terrainMutex ); {
		cacheBlocklist* b = cache->blocks;
		while ( b && b->head ) {
			if ( b->head->uMin == uMin && b->head->vMin == vMin ) {
				block = b->head;
				break;
			}
			b = b->tail;
		}
	} vmutex_unlock( &terrainMutex );
	return block;
}

int gridIndex( int i, int grid ) { return (i - grid) / CacheBlockSize; }

// Find the grid in the list, else NULL
cacheGrid* cachedGrid( terrainCache* cache, int uMin, int vMin ) {
	const int uGrid = minPeriod(uMin, GridCapacity);
	const int vGrid = minPeriod(vMin, GridCapacity);
	//printf( "Looking for grid %d %d.\n", uGrid, vGrid );
	// find the grid that would contain it; If there, find the block
	cacheGrid* grid = NULL;
	for( cacheGridlist* g = cache->grids; g && g->head; g = g->tail ) {
		if ( g->head->uMin == uGrid && g->head->vMin == vGrid ) {
			grid = g->head;
			break;
		}
	}
	return grid;
}

// Find the grid in the list, return it from that, else NULL
cacheBlock* terrainCached( terrainCache* cache, int uMin, int vMin ) {
	cacheGrid* g = cachedGrid( cache, uMin, vMin );
	cacheBlock* b = g ? g->blocks[gridIndex(uMin, minPeriod(uMin, GridCapacity))][gridIndex(vMin, minPeriod(vMin, GridCapacity))] : NULL;
	vAssert( (uintptr_t)b != 0xdeadbeef00000000 );
	return b;
}

cacheGrid* cacheGrid_create( int u, int v ) {
	//printf( "Creating grid for %d %d.\n", u, v );
	cacheGrid* g = mem_alloc( sizeof( cacheGrid ));
	memset( g->blocks, 0, sizeof( cacheBlock* ) * GridSize * GridSize );
	g->uMin = u;
	g->vMin = v;
	return g;
}

cacheGrid* terrainCacheAddGrid( terrainCache* t, cacheGrid* g ) {
	t->grids = cacheGridlist_cons( g, t->grids );
	printf( "%d grids!\n", cacheGridlist_length( t->grids ));
	return g;
}

cacheBlock* terrainCacheAdd( terrainCache* t, cacheBlock* b ) {
	vmutex_lock( &terrainMutex ); {
		t->blocks = cacheBlocklist_cons(b, t->blocks);
		const int uMin = b->uMin;
		const int vMin = b->vMin;
		cacheGrid* g = cachedGrid( t, uMin, vMin );
		if (!g) {
			printf( "Creating grid %d %d (for block %d %d)\n", minPeriod(uMin, GridCapacity), minPeriod(vMin, GridCapacity), uMin, vMin ) ;
			g = terrainCacheAddGrid( t, cacheGrid_create( minPeriod( uMin, GridCapacity ), minPeriod( vMin, GridCapacity )));
		}
		g->blocks[gridIndex(uMin, minPeriod(uMin, GridCapacity))][gridIndex(vMin, minPeriod(vMin, GridCapacity))] = b;
	} vmutex_unlock( &terrainMutex );
	return b;
}

cacheBlock* terrainCacheBlock( canyon* c, canyonTerrain* t, int uMin, int vMin, int requiredLOD ) {
	//printf( "Generating block %d %d (lod %d).\n", uMin, vMin, requiredLOD );
	cacheBlock* b = mem_alloc( sizeof( cacheBlock ));
	b->uMin = uMin;
	b->vMin = vMin;
	b->lod = requiredLOD;
	int lod = max( 1, 2 * requiredLOD );
	for ( int vOffset = 0; vOffset < CacheBlockSize; vOffset+=lod ) {
		for ( int uOffset = 0; uOffset < CacheBlockSize; uOffset+=lod ) {
			float u, v, x, z;
			terrain_positionsFromUV( t, uMin + uOffset, vMin + vOffset, &u, &v );
			terrain_worldSpaceFromCanyon( c, u, v, &x, &z );
			b->positions[uOffset][vOffset] = Vector( x, canyonTerrain_sampleUV( u, v ), z, 1.f );
		}
	}
	return b;
}

// Generate a Cache block, if it's not already built from terrain params
void* buildCache( void* args ) {
	canyon* c = _1(args);
	terrainCache* t = c->terrainCache;
	int u = (intptr_t)_1(_2(args));
	int v = (intptr_t)_2(_2(args));
	if (terrainCached( t, u, v ) == NULL)
		terrainCacheAdd( t, terrainCacheBlock( c, NULL, u, v, 1 )); // TODO

	mem_free( _2(args) ); // Free nested Pair
	mem_free( args );
	return NULL;
}

void buildCache_queueWorkerTask( canyon* c, int u, int v ) {
	// make the task and queue it
	worker_task build;
	build.func = buildCache;
	build.args = Pair( c, Pair( (void*)(intptr_t)u, (void*)(intptr_t)v ));
	worker_addTask( build );
}

//////////////

void* worker_generateVertices( void* args ) {
	canyonTerrainBlock* b = args;
	canyonTerrainBlock_calculateExtents( b, b->terrain, b->coord );
	printf( "WORKER: generating verticies for block %d %d.\n", b->uMin, b->vMin );
	vertPositions* vertSources = generatePositions( b );
	canyonTerrain_queueWorkerTaskGenerateBlock( b, vertSources );
	return NULL;
}

void worker_queueGenerateVertices( canyonTerrainBlock* b ) {
	worker_task generateTask;
	generateTask.func = worker_generateVertices;
	generateTask.args = b;
	worker_addTask( generateTask );
}
