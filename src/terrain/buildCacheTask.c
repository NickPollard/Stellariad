// buildCacheTask.c
#include "src/common.h"
#include "src/terrain/buildCacheTask.h"
//---------------------
#include "canyon.h"
#include "canyon_terrain.h"
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"

IMPLEMENT_LIST(cacheBlock);

vmutex terrainMutex = kMutexInitialiser;

terrainCache* terrainCache_create() {
	terrainCache* t = mem_alloc(sizeof(terrainCache));
	t->blocks = cacheBlocklist_create();
	return t;
}

// Find the element in the list and return it, orElse none
cacheBlock* terrainCached( terrainCache* cache, int uMin, int vMin ) {
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

cacheBlock* terrainCacheAdd( terrainCache* t, cacheBlock* b ) {
	vmutex_lock( &terrainMutex ); {
		t->blocks = cacheBlocklist_cons(b, t->blocks);
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
