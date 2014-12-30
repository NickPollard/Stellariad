// buildCacheTask.c
#include "src/common.h"
#include "src/terrain/buildCacheTask.h"
//---------------------
#include "canyon.h"
#include "canyon_terrain.h"
#include "future.h"
#include "terrain_generate.h"
#include "terrain/cache.h"
#include "worker.h"
#include "actor/actor.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"

void* buildCacheBlockTask(void* args) {
	canyonTerrainBlock* b = (canyonTerrainBlock*)_1(args);
	future* f = (future*)_2(args);
	int uMin = (uintptr_t)_3(args);
	int vMin = (uintptr_t)_4(args);

	canyon* c = b->terrain->_canyon;
	// ! only if not exist or lower-lod
	cacheBlock* cache = terrainCached( c->cache, uMin, vMin );
	bool rebuild = cache && cache->lod > b->lod_level;
	if (rebuild)
		cacheBlockFree( cache ); // Release the cache we don't want
	if (!cache || rebuild)
		cache = terrainCacheBuildAndAdd( c, b->terrain, uMin, vMin, b->lod_level );

	future_complete( f, cache ); // TODO - Should this be tryComplete? hit a segfault here
	cacheBlockFree( cache );
	mem_free( args );

	return NULL;
}

// Request a cacheBlock of at least the required Lod for (U,V)
future* requestCache( canyonTerrainBlock* b, int u, int v ) {
	// If already built, or building, return that future, else start it building
	future* f = NULL;
	bool needCreating = cacheBlockFuture( b->terrain->_canyon->cache, u, v, b->lod_level, &f);
	vAssert( f );
	if (needCreating) {
		void* uu = (void*)(uintptr_t)u;
		void* vv = (void*)(uintptr_t)v;
		worker_addTask( task( buildCacheBlockTask, Quad(b, f, uu, vv)));
	}
	future_onComplete( f, takeCacheRef, NULL );
	return f;
}

void releaseAllCaches( cacheBlocklist* caches ) {
	for( cacheBlocklist* b = caches; b; b = b->tail )
		cacheBlockFree( b->head );
}

/* At this point all the terrain caches should be filled,
   so we can just use them safely */
void generateVerts( canyonTerrainBlock* b, vertPositions* vertSources, cacheBlocklist* caches ) {
	vector* verts = vertSources->positions;
	const int max = vertCount( b );
	for ( int v = -1; v < b->v_samples+1; ++v )
		for ( int u = -1; u < b->u_samples+1; ++u ) {
			const int index = indexFromUV(b, u, v);
			vAssert( index >= 0 && index < max );
			const int strideMax = 4;
			const int stride = lodStride(b);
			int uu = (u == -1) ? -strideMax : (u == b->u_samples ? ((b->u_samples - 1) * stride + strideMax) : u * stride);
			int vv = (v == -1) ? -strideMax : (v == b->v_samples ? ((b->v_samples - 1) * stride + strideMax) : v * stride);
			//int vv = (v == -1) ? -strideMax : v * lodStride(b);
			verts[index] = terrainPointCached( b->_canyon, b, caches, uu, vv );
		}

	releaseAllCaches( caches );
	worker_addTask( task( canyonTerrain_workerGenerateBlock, Pair( vertSources, b )));
}

void* worker_generateVerts( void* args ) {
	cacheBlocklist* caches = cachesForBlock( (canyonTerrainBlock*)_1(args) );
	generateVerts( (canyonTerrainBlock*)_1(args), (vertPositions*)_2(args), caches );
	mem_free(args);
	return NULL;
}

futurelist* generateAllCaches( canyonTerrainBlock* b ) {
	int cacheMinU = 0, cacheMinV = 0, cacheMaxU = 0, cacheMaxV = 0;
	getCacheExtents(b, cacheMinU, cacheMinV, cacheMaxU, cacheMaxV );

	futurelist* fs = NULL;
	for (int u = cacheMinU; u <= cacheMaxU; u += CacheBlockSize )
		for (int v = cacheMinV; v <= cacheMaxV; v += CacheBlockSize ) {
			future* f = requestCache( b, u, v );
			fs = f->complete ? fs : futurelist_cons( f, fs );
		}
	return fs;
}

void* buildCacheTask( void* args ) {
	futurelist* fs = generateAllCaches( (canyonTerrainBlock*)_1( args ));
	future_completeWith( (future*)_2(args), futures_sequence( fs ));
	futurelist_delete( fs );
	mem_free(args);
	return NULL;
}

/* Builds a vertPositions to be passed to a child worker to actually construct the terrain.
   This should normally just pull from cache - if blocks aren't there, build them */
future* buildCache(canyonTerrainBlock* b) {
	future* f = future_create();
	worker_addTask( task(buildCacheTask, Pair( b, f )));
	return f;
}

void generatePositions( canyonTerrainBlock* b) {
	vertPositions* vertSources = (vertPositions*)mem_alloc( sizeof( vertPositions )); // TODO - don't do a full mem_alloc here
	vertSources->uMin = -1;
	vertSources->vMin = -1;
	vertSources->uCount = b->u_samples + 2;
	vertSources->vCount = b->v_samples + 2;
	vertSources->positions = (vector*)mem_alloc( sizeof( vector ) * vertCount( b ));

	future* f = buildCache( b );
	future_onComplete( f, runTask, taskAlloc( worker_generateVerts, Pair( b, vertSources )));
}

void* generateVertices_( void* args ) {
	generatePositions( (canyonTerrainBlock*)args );
	return NULL;
}

Msg generateVertices( canyonTerrainBlock* b ) { 
	return task( generateVertices_, b );
}
