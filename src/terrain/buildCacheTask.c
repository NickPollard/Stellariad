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
	canyonTerrainBlock* b = _1(args);
	future* f = _2(args);
	int uMin = (uintptr_t)_3(args);
	int vMin = (uintptr_t)_4(args);

	canyon* c = b->terrain->canyon;
	// ! only if not exist or lower-lod
	cacheBlock* cache = terrainCached( c->terrainCache, uMin, vMin );
	if (!cache)
		terrainCacheBuildAndAdd( c, b->terrain, uMin, vMin, b->lod_level );
	else if (cache->lod > b->lod_level) {
		cacheBlockFree( cache );
		terrainCacheBuildAndAdd( c, b->terrain, uMin, vMin, b->lod_level );
	}

	future_complete( f, cache ); // TODO - Should this be tryComplete? hit a segfault here
	cacheBlockFree( cache );
	mem_free( args );

	return NULL;
}

future* requestCache( canyonTerrainBlock* b, int u, int v ) {
	// If already built, or building, return that future
	// else start it building
	future* f = NULL;
	bool empty = cacheBlockFuture( b->terrain->canyon->terrainCache, u, v, b->lod_level, &f);
	vAssert( f );
	if (empty) {
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
	for ( int v = -1; v < b->v_samples + 1; ++v )
		for ( int u = -1; u < b->u_samples + 1; ++u )
			verts[indexFromUV(b, u, v)] = terrainPointCached( b->canyon, b, caches, u, v );

	releaseAllCaches( caches );
	worker_addTask( task( canyonTerrain_workerGenerateBlock, Pair( vertSources, b )));
}

void* worker_generateVerts( void* args ) {
	cacheBlocklist* caches = cachesForBlock( _1(args) );
	generateVerts( _1(args), _2(args), caches/*_3(args)*/ );
	mem_free(args);
	return NULL;
}


futurelist* generateAllCaches( canyonTerrainBlock* b ) {
	int cacheMinU = 0, cacheMinV = 0, cacheMaxU = 0, cacheMaxV = 0;
	cacheBlockFor( b, -1, -1, &cacheMinU, &cacheMinV );
	cacheBlockFor( b, b->u_samples, b->v_samples, &cacheMaxU, &cacheMaxV );

	futurelist* fs = NULL;
	for (int u = cacheMinU; u <= cacheMaxU; u+=CacheBlockSize )
		for (int v = cacheMinV; v <= cacheMaxV; v+=CacheBlockSize ) {
			future* f = requestCache( b, u, v );
			if ( !f->complete )
				fs = futurelist_cons( f, fs );
		}
	return fs;
}

void* buildCacheTask( void* args ) {
	futurelist* fs = generateAllCaches( _1( args ));
	future_completeWith( _2(args), futures_sequence( fs ));
	futurelist_delete( fs );
	//future_completeWith( _2(args), futures_sequence( generateAllCaches( _1(args) )));
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
	vertPositions* vertSources = mem_alloc( sizeof( vertPositions )); // TODO - don't do a full mem_alloc here
	vertSources->uMin = -1;
	vertSources->vMin = -1;
	vertSources->uCount = b->u_samples + 2;
	vertSources->vCount = b->v_samples + 2;
	vertSources->positions = mem_alloc( sizeof( vector ) * vertCount( b ));

	future* f = buildCache( b );
	cacheBlocklist* caches = NULL;//cachesForBlock( b );
	future_onComplete( f, runTask, taskAlloc( worker_generateVerts, Triple( b, vertSources, caches )));
}

void* generateVertices_( void* args ) {
	generatePositions( args );
	return NULL;
}

Msg generateVertices( canyonTerrainBlock* b ) { 
	return task( generateVertices_, b );
}
