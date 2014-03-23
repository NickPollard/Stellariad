// buildCacheTask.c
#include "src/common.h"
#include "src/terrain/buildCacheTask.h"
//---------------------
#include "canyon.h"
#include "canyon_terrain.h"
#include "terrain_generate.h"
#include "terrain/cache.h"
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"

// Generate a Cache block, if it's not already built from terrain params
void* buildCache( void* args ) {
	canyon* c = _1(args);
	terrainCache* t = c->terrainCache;
	int u = (intptr_t)_1(_2(args));
	int v = (intptr_t)_2(_2(args));
	if (terrainCached( t, u, v ) == NULL) {
		cacheBlock* b = terrainCacheAdd( t, terrainCacheBlock( c, NULL, u, v, 1 )); // TODO
		cacheBlockFree( b );
	}

	mem_free( _2(args) ); // Free nested Pair
	mem_free( args );
	return NULL;
}

void worker_queueBuildCache( canyon* c, int u, int v ) {
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
