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

void* blocksForVerts( vertPositions* verts, int* num ) {
	(void)verts;(void)num;
	int n = 5;
	pair** data = mem_alloc( sizeof( pair* ) * n );
	for ( int i = 0; i < n; ++i ) {
		data[i] = Pair( NULL, NULL ); // TODO
	}
	*num = n;
	return data;
}

void* testTask( void* args ) {
	(void)args;
	//printf( "Running a test task!\n" );
	return NULL;
}

void cacheBlockFor( canyonTerrainBlock* b, int uRelative, int vRelative, int* uCache, int* vCache ) {
	const int r = 4 / lodRatio(b);
	const int uReal = b->uMin + r*uRelative;
	const int vReal = b->vMin + r*vRelative;
	const int uOffset = uReal > 0 ? uReal % CacheBlockSize : (CacheBlockSize + (uReal % CacheBlockSize)) % CacheBlockSize;
	const int vOffset = vReal > 0 ? vReal % CacheBlockSize : (CacheBlockSize + (vReal % CacheBlockSize)) % CacheBlockSize;
	*uCache = uReal - uOffset;
	*vCache = vReal - vOffset;
}

void* buildCacheBlockTask(void* args) {
	canyonTerrainBlock* b = _1(_1(args));
	future* f = _2(_1(args));
	int uMin = (uintptr_t)_1(_2(args));
	int vMin = (uintptr_t)_2(_2(args));

	canyon* c = b->terrain->canyon;
	// ! only if not exist or lower-lod
	int found_lod = b->lod_level;
	cacheBlock* cache = terrainCached( c->terrainCache, uMin, vMin );
	if (!cache || cache->lod > b->lod_level) {
		terrainCacheAdd( c->terrainCache, terrainCacheBlock( c, b->terrain, uMin, vMin, b->lod_level ));
	} else {
		found_lod = cache->lod;
	}

	//printf( "Finished generating cache block %d %d.\n", uMin, vMin );
	void* lod = (void*)(uintptr_t)min(b->lod_level, found_lod);
	future_complete( f, lod );
	////cacheBlockFree( cache );
	mem_free( _1(args) );
	mem_free( _2(args) );
	mem_free( args );

	return NULL;
}

future* generateCache( canyonTerrainBlock* b, int u, int v ) {
	(void)u;(void)v;
	// If already built, or building, return that future
	future* f = NULL;
	bool pending = cacheBlockFuture( b->terrain->canyon->terrainCache, u, v, &f);
	vAssert( f );
	// else start it building
	if (pending) {
		//f = future_create(); // TODO - SUSPICIOUS!
		//printf( "Creating block future " xPTRf "\n", (uintptr_t)f);
		void* uu = (void*)(uintptr_t)u;
		void* vv = (void*)(uintptr_t)v;
		worker_addTask( task( buildCacheBlockTask, Pair(Pair(b, f), Pair(uu, vv))));
	} else {
		//printf( "Using pending block future " xPTRf "\n", (uintptr_t)f);
		//if (!f->complete)
		//	future_complete_(f);
		//printf( "block already in flight!\n");
	}
	return f;
}

/*
   At this point all the terrain caches should be filled,
   so we can just use them safely
   */
void generateVerts( canyonTerrainBlock* b, vertPositions* vertSources ) {
	vector* verts = vertSources->positions;
	for ( int v = -1; v < b->v_samples + 1; ++v )
		for ( int u = -1; u < b->u_samples + 1; ++u )
			verts[indexFromUV(b, u, v)] = terrainPointCached( b->canyon, b, u, v );

	worker_addTask( task( canyonTerrain_workerGenerateBlock, Pair( vertSources, b )));
}
void* worker_generateVerts( void* args ) {
	(void)args;
	generateVerts( _1(args), _2(args) );
	return NULL;
}

void* echo(const void* args, void* args2) {
	(void)args;(void)args2;
	//printf( "Finished!\n" );
	return NULL;
}

futurelist* generateAllCaches( canyonTerrainBlock* b ) {
	(void)b;
	//future* f = future_create();
	//future* ff = future_create();

	int cacheMinU = 0, cacheMinV = 0, cacheMaxU = 0, cacheMaxV = 0;
	cacheBlockFor( b, -1, -1, &cacheMinU, &cacheMinV );
	cacheBlockFor( b, b->u_samples, b->v_samples, &cacheMaxU, &cacheMaxV );

	futurelist* fs = NULL;
	//printf( "Generating range: %d,%d -> %d,%d\n", cacheMinU,cacheMinV, cacheMaxU,cacheMaxV );
	for (int u = cacheMinU; u <= cacheMaxU; u+=CacheBlockSize ) {
		for (int v = cacheMinV; v <= cacheMaxV; v+=CacheBlockSize ) {
			fs = futurelist_cons( future_onComplete( generateCache( b, u, v ), echo, NULL), fs );
		}
	}
	/*
	futurelist* fs = futurelist_cons( f, NULL );
	fs = futurelist_cons( ff, fs );
	future_complete_( f );
	future_complete_( ff );
	*/
	return fs;
}

void* buildCacheTask( void* args ) {
	future* f = _2(args);
	future_completeWith( f, futures_sequence( generateAllCaches( _1(args) )));
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
	future_onComplete( f, runTask, taskAlloc( worker_generateVerts, Pair( b, vertSources )));
	//future_complete_(future_onComplete( future_create(), runTask, taskAlloc( worker_generateVerts, Pair( b, vertSources ))));
}

void* generateVertices_( void* args ) {
	generatePositions( args );
	return NULL;
}

Msg generateVertices( canyonTerrainBlock* b ) { 
	return task( generateVertices_, b );
}
