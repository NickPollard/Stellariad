// cache.c
#include "src/common.h"
#include "src/terrain/cache.h"
//---------------------
#include "canyon.h"
#include "canyon_terrain.h"
#include "future.h"
#include "terrain_generate.h"
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"

DEF_LIST(cacheGrid);
IMPLEMENT_LIST(cacheBlock);
IMPLEMENT_LIST(cacheGrid);

struct terrainCache_s {
	cacheGridlist* grids;	
	cacheBlocklist* blocks;	
};

vmutex terrainMutex = kMutexInitialiser;
vmutex blockMutex = kMutexInitialiser;

terrainCache* terrainCache_create() {
	terrainCache* t = mem_alloc(sizeof(terrainCache));
	t->blocks = cacheBlocklist_create();
	return t;
}

void takeRef( cacheBlock* b ) {
	vmutex_lock( &blockMutex ); {
		if (b)
			++b->refCount; // TODO - CAS instruction?
	} vmutex_unlock( &blockMutex );
}

int gridIndex( int i, int grid ) { return (i - grid) / CacheBlockSize; }

// Find the grid in the list, else NULL
cacheGrid* cachedGrid( terrainCache* cache, int uMin, int vMin ) {
	const int uGrid = minPeriod(uMin, GridCapacity);
	const int vGrid = minPeriod(vMin, GridCapacity);
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
	cacheBlock* b = NULL;
	vmutex_lock( &terrainMutex ); {
		cacheGrid* g = cachedGrid( cache, uMin, vMin );
		b = g ? g->blocks[gridIndex(uMin, minPeriod(uMin, GridCapacity))][gridIndex(vMin, minPeriod(vMin, GridCapacity))] : NULL;
	} vmutex_unlock( &terrainMutex );
	takeRef( b );
	return b;
}

cacheGrid* cacheGrid_create( int u, int v ) {
	cacheGrid* g = mem_alloc( sizeof( cacheGrid ));
	memset( g->blocks, 0, sizeof( cacheBlock* ) * GridSize * GridSize );
	memset( g->futures, 0, sizeof( future* ) * GridSize * GridSize );
	g->uMin = u;
	g->vMin = v;
	return g;
}

cacheGrid* terrainCacheAddGrid( terrainCache* t, cacheGrid* g ) {
	t->grids = cacheGridlist_cons( g, t->grids );
	return g;
}

cacheBlock* terrainCacheAdd( terrainCache* t, cacheBlock* b ) {
	vmutex_lock( &terrainMutex ); {
		const int uMin = b->uMin;
		const int vMin = b->vMin;
		cacheGrid* g = cachedGrid( t, uMin, vMin );
		if (!g)
			g = terrainCacheAddGrid( t, cacheGrid_create( minPeriod( uMin, GridCapacity ), minPeriod( vMin, GridCapacity )));
		const int u = gridIndex( uMin, minPeriod( uMin, GridCapacity ));
		const int v = gridIndex( vMin, minPeriod( vMin, GridCapacity ));
		cacheBlock* old = g->blocks[u][v];
		vAssert( u < GridSize );
		vAssert( v < GridSize );
		// TODO check LoD
		if (old && old->lod < b->lod) {
			mem_free( b );
		} else {
			mem_free( old );
			g->blocks[u][v] = b;
			takeRef( b );
		}
	} vmutex_unlock( &terrainMutex );
	return b;
}

static int numCaches = 0;

cacheBlock* terrainCacheBlock( canyon* c, canyonTerrain* t, int uMin, int vMin, int requiredLOD ) {
	//printf( "Generating cache %d %d.\n", uMin, vMin );
	requiredLOD = 0; // TODO - Don't do this
	++numCaches;
	//printf( "Creating cacheBlock #%d. (%d %d) (lod: %d)\n", numCaches, uMin, vMin, requiredLOD );
	cacheBlock* b = mem_alloc( sizeof( cacheBlock )); // TODO - don't do full mem_alloc here
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
	b->refCount = 1;
	return b;
}

// Blocks are ref-counted for thread-safety
void cacheBlockFree( cacheBlock* b ) {
	vmutex_lock( &blockMutex ); {
		if ( b ) {
			--b->refCount;
			vAssert( b->refCount >= 0 );
			if (b->refCount == 0)
				mem_free( b );
		}
	} vmutex_unlock( &blockMutex );
}

void trimCacheGrid( cacheGrid* g, int v ) {
	const int vMax = clamp( v - g->vMin, 0, GridSize );
	for ( int u = 0; u < GridSize; ++u )
		for ( int v = 0; v < vMax; ++v ) {
			cacheBlockFree(g->blocks[u][v]);
			g->blocks[u][v] = NULL;
		}
}

bool gridEmpty( cacheGrid* g ) {
	bool b = true;
	for ( int u = 0; b && u < GridSize; ++u )
		for ( int v = 0; b && v < GridSize; ++v )
			b = b && g->blocks[u][v] == NULL;
	return b;
}

void terrainCache_trim( terrainCache* t, int v ) {
	/*
	vmutex_lock( &terrainMutex ); {
		cacheGridlist* nw = NULL;
		cacheGridlist* g = t->grids;
		while ( g && g->head ) {
			trimCacheGrid( g->head, v );
			(void)v;
			if (gridEmpty(g->head)) {
				//printf( "Freeing grid list " xPTRf "\n", (uintptr_t)g );
				mem_free( g->head );
			} else {
				//vAssert( (uintptr_t)g->head < 0x00ffffffffffffff );
				nw = cacheGridlist_cons( g->head, nw );
			}
			cacheGridlist* next = g->tail;
			mem_free( g );
			g = next;
		}
		t->grids = nw;
	} vmutex_unlock( &terrainMutex );
	*/
	(void)t;(void)v;
}

void terrainCache_tick( terrainCache* t, float dt, vector sample ) {
	(void)dt;
	(void)t;
	(void)sample;
	/*
	float v = from(sample);
	for ( int u = uMin; u < uMax; ++u ) {
		if (closeEnough(u, v)) {
			// generate
		}
	}
	*/
	/*
	//printf( "getting cached point for block %d %d.\n", b->uMin, b->vMin );
	const int r = 4 / lodRatio(b);
	const int uReal = b->uMin + r*uRelative;
	const int vReal = b->vMin + r*vRelative;
	const int uOffset = uReal > 0 ? uReal % CacheBlockSize : (CacheBlockSize + (uReal % CacheBlockSize)) % CacheBlockSize;
	const int vOffset = vReal > 0 ? vReal % CacheBlockSize : (CacheBlockSize + (vReal % CacheBlockSize)) % CacheBlockSize;
	const int uMin = uReal - uOffset;
	const int vMin = vReal - vOffset;

	cacheBlock* cache = terrainCached( c->terrainCache, uMin, vMin );
	if (!cache || cache->lod > b->lod_level)
		cache = terrainCacheAdd( c->terrainCache, terrainCacheBlock( c, b->terrain, uMin, vMin, b->lod_level ));
	vector p = cache->positions[uOffset][vOffset];
	cacheBlockFree( cache );
	*/
}

bool cacheBlockFuture( terrainCache* cache, int uMin, int vMin, future** f ) {
	bool empty = false;
	vmutex_lock( &terrainMutex ); {
		cacheGrid* g = cachedGrid( cache, uMin, vMin );
		if (!g)
			g = terrainCacheAddGrid( cache, cacheGrid_create( minPeriod( uMin, GridCapacity ), minPeriod( vMin, GridCapacity )));
		future* fut = g ? g->futures[gridIndex(uMin, minPeriod(uMin, GridCapacity))][gridIndex(vMin, minPeriod(vMin, GridCapacity))] : NULL;
		empty = !fut;
		if (empty) {
			fut = future_create();
			g->futures[gridIndex(uMin, minPeriod(uMin, GridCapacity))][gridIndex(vMin, minPeriod(vMin, GridCapacity))] = fut;
		}
		*f = fut;
	} vmutex_unlock( &terrainMutex );
	return empty;
}

