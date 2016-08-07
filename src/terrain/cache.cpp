// cache.c
#include "src/common.h"
#include "src/terrain/cache.h"
//---------------------
#include "canyon.h"
#include "terrain/canyon_terrain.h"
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
	cacheBlocklist* toDelete;
};

vmutex terrainMutex = kMutexInitialiser;
vmutex blockMutex = kMutexInitialiser;

terrainCache* terrainCache_create() {
	terrainCache* t = (terrainCache*)mem_alloc(sizeof(terrainCache));
	t->blocks = cacheBlocklist_create();
	t->toDelete = NULL;
	t->grids = NULL;
	return t;
}

void takeRef( cacheBlock* b ) {
	vmutex_lock( &blockMutex ); {
		if (b) ++b->refCount; // TODO - CAS instruction?
	} vmutex_unlock( &blockMutex );
}

void* takeCacheRef( const void* value, void* args ) {
	(void)args;
	takeRef( (cacheBlock*)value ); // URGH const cast?
	return NULL;
}

// Find the grid in the list, else NULL
cacheGrid* gridFor( terrainCache* cache, int uMin, int vMin ) {
	const int uGrid = minStride(uMin, GridCapacity);
	const int vGrid = minStride(vMin, GridCapacity);
	for( cacheGridlist* g = cache->grids; g && g->head; g = g->tail )
		if ( g->head->uMin == uGrid && g->head->vMin == vGrid )
			return g->head;
	return NULL;
}

// Find the grid in the list, return it from that, else NULL
cacheBlock* terrainCached( terrainCache* cache, int u, int v ) {
	cacheBlock* b = NULL;
	vmutex_lock( &terrainMutex ); {
		cacheGrid* g = gridFor( cache, u, v );
		b = g ? gridBlock( g, u, v ) : NULL;
	} vmutex_unlock( &terrainMutex );
	takeRef( b );
	return b;
}

cacheGrid* terrainCacheAddGrid( terrainCache* t, cacheGrid* g ) {
	t->grids = cacheGridlist_cons( g, t->grids );
	return g;
}

// Idempotent
void terrain_deleteBlock( terrainCache* cache, cacheBlock* b ) {
	// Check it's not already in there, it could be!
	for ( cacheBlocklist* bl = cache->toDelete; bl; bl = bl->tail )
		if (bl->head == b)
			return;
	cache->toDelete = cacheBlocklist_cons( b, cache->toDelete );
}

cacheGrid* gridForIndex( int u, int v ) {
	return cacheGrid_create( minStride( u, GridCapacity ), minStride( v, GridCapacity ));
}
cacheGrid* gridForBlock( cacheBlock* b ) {
	return gridForIndex( b->uMin, b->vMin );
}
cacheGrid* gridGetOrAdd( terrainCache* cache, int u, int v ) {
	cacheGrid* g = gridFor( cache, u, v );
	if (!g)
		g = terrainCacheAddGrid( cache, gridForIndex( u, v ));
	vAssert( g );
	return g;
}

// TerrainMutex-Locked
cacheBlock* terrainCacheAddInternal( terrainCache* t, cacheBlock* b ) {
	cacheGrid* g = gridGetOrAdd( t, b->uMin, b->vMin );
	cacheBlock* old = gridBlock( g, b->uMin, b->vMin );
	if (old && old->lod <= b->lod) {
		mem_free( b );
		return old;
	}
	else {
		if (old) terrain_deleteBlock( t, old );
		gridSetBlock( g, b, b->uMin, b->vMin );
		takeRef( b );
		return b;
	}
}

int cacheGetNeededLod( terrainCache* cache, int u, int v ) {
	cacheGrid* g = gridGetOrAdd( cache, u, v );
	return g ? gridLod( g, u, v ) : lowestLod;
}

cacheBlock* terrainCacheBuildAndAdd( canyon* c, CanyonTerrain* t, int uMin, int vMin, int lod ) {
	vmutex_lock( &terrainMutex );
		const int highestLodNeeded = min( cacheGetNeededLod( c->cache, uMin, vMin), lod );
	vmutex_unlock( &terrainMutex );

	// Only add it if we need to!
	vmutex_lock( &terrainMutex );
		cacheGrid* g = gridGetOrAdd( c->cache, uMin, vMin );
		cacheBlock* cache = gridBlock( g, uMin, vMin );
	vmutex_unlock( &terrainMutex );
	if (cache && cache->lod <= lod ) {
		//skip
		printf( "Skipping building block, already have one.\n" );
	} else {
		cacheBlock* b = terrainCacheBlock( c, t, uMin, vMin, highestLodNeeded );
		vmutex_lock( &terrainMutex );
			cache = terrainCacheAddInternal( c->cache, b );
		vmutex_unlock( &terrainMutex );
	}

	return cache;
}

static int numCaches = 0;

vector vecSample( vector* array, int w, int h, float u, float v ) {
	(void)w;
	const int iu = (int)floorf(u);
	const int iv = (int)floorf(v);
	vAssert( iu >= 0 && iu + 1 < w );
	vAssert( iv >= 0 && iv + 1 < h );
	const vector A = array[iu * h + iv];
	const vector B = array[iu * h + (iv+1)];
	const vector C = array[(iu+1) * h + iv];
	const vector D = array[(iu+1) * h + (iv+1)];
	const float uLerp = fractf(u);
	const float vLerp = fractf(v);
	return veclerp(veclerp( A, B, vLerp), veclerp( C, D, vLerp), uLerp);
}

cacheBlock* terrainCacheBlock( canyon* c, CanyonTerrain* t, int uMin, int vMin, int requiredLOD ) {
	++numCaches;
	cacheBlock* b = (cacheBlock*)mem_alloc( sizeof( cacheBlock )); // TODO - don't do full mem_alloc here
	b->uMin = uMin;
	b->vMin = vMin;
	b->lod = requiredLOD;
	int lod = max( 1, 2 * requiredLOD );
#define canyonSampleInterval 4
#define LodVerts ((CacheBlockSize / canyonSampleInterval) + 1)
	vector worldSpace[LodVerts][LodVerts];
	for ( int vOffset = 0; vOffset < LodVerts; ++vOffset ) {
		for ( int uOffset = 0; uOffset < LodVerts; ++uOffset ) {
			float u, v, x, z;
			t->positionsFromUV( uMin + uOffset*canyonSampleInterval, vMin + vOffset*canyonSampleInterval, &u, &v );
			terrain_worldSpaceFromCanyon( c, u, v, &x, &z );
			worldSpace[uOffset][vOffset] = Vector(x, 0.f, z, 0.f);
		}
	}
	float f = (float)canyonSampleInterval;
	for ( int vOffset = 0; vOffset < CacheBlockSize; vOffset+=lod ) {
		for ( int uOffset = 0; uOffset < CacheBlockSize; uOffset+=lod ) {
			float u, v;
			t->positionsFromUV( uMin + uOffset, vMin + vOffset, &u, &v );
			vector vec = vecSample( (vector*)worldSpace, LodVerts, LodVerts, ((float)uOffset) / f, ((float)vOffset) / f );
			b->positions[uOffset][vOffset] = Vector( vec.coord.x, canyonTerrain_sampleUV( u, v ), vec.coord.z, 1.f );
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
			if (b->refCount == 0) {
//				printf( "Deleting cache block %d %d (" xPTRf ")\n", b->uMin, b->vMin, (uintptr_t)b );
				//mem_free( b );
				// TODO - we need to have access to the terrain cache here!
				//terrain_deleteBlock( cache, b );
			}
		}
	} vmutex_unlock( &blockMutex );
}

/*
void trimCacheGrid( cacheGrid* g, int v ) {
	const int vMax = clamp( v - g->vMin, 0, GridSize );
	//printf( "Trimming for V %d (this block: %d)\n", v, vMax );
	for ( int u = 0; u < GridSize; ++u )
		for ( int v = 0; v < vMax; ++v ) {
			vAssert( !(g->blocks[u][v] && g->blocks[u][v]->refCount < 1))
			if ( g->blocks[u][v] && g->blocks[u][v]->refCount == 1 ) {
				// TODO - Need to store a separate RefCount for the co-ord, NOT the block
				// Allow blocks to be replaced, but not trimmed unless the refcount is 0
				//printf( "Freeing cache %d %d.\n", g->uMin + u, g->vMin + v );
				cacheBlockFree(g->blocks[u][v]);
				g->blocks[u][v] = NULL;
			}
		}
}
*/

bool gridEmpty( cacheGrid* g ) {
	bool empty = true;
	for ( int u = 0; empty && u < GridSize; ++u )
		for ( int v = 0; empty && v < GridSize; ++v )
			empty = empty && g->blocks[u][v] == NULL;
	return empty;
}

void terrainCache_trim( terrainCache* t, int v ) {
	/*
	vmutex_lock( &terrainMutex ); {
		//cacheGridlist* nw = NULL;
		cacheGridlist* g = t->grids;
		while ( g && g->head ) {
			//trimCacheGrid( g->head, v );
			(void)v;
			if (gridEmpty(g->head)) {
				//printf( "Freeing grid list " xPTRf "\n", (uintptr_t)g );
				mem_free( g->head );
			} else {
				//vAssert( (uintptr_t)g->head < 0x00ffffffffffffff );
				nw = cacheGridlist_cons( g->head, nw );
			}
			cacheGridlist* next = g->tail;
			//mem_free( g );
			g = next;
		}
		//t->grids = nw;
	} vmutex_unlock( &terrainMutex );
*/
	(void)t;(void)v;
}

cacheBlocklist* terrainCache_processDeletes( terrainCache* t ) {
	for (cacheBlocklist* b = t->toDelete; b; b = b->tail )
		mem_free(b->head);
	cacheBlocklist_delete( t->toDelete );
	return NULL;
}

void terrainCache_tick( terrainCache* t, float dt, vector sample ) {
	(void)dt; (void)t; (void)sample;

	vmutex_lock( &terrainMutex ); {
		t->toDelete = terrainCache_processDeletes( t );
	} vmutex_unlock( &terrainMutex );
}

// Need to have locked terrainMutex!
void setLodNeeded( terrainCache* cache, int u, int v, int lodNeeded ) {
	cacheGrid* g = gridGetOrAdd( cache, u, v );
	const int lod = min(gridLod(g, u, v), lodNeeded);
	gridSetLod( g, lod, u, v );
}

bool cacheBlockFuture( terrainCache* cache, int uMin, int vMin, int lodNeeded, future** f ) {
	(void)lodNeeded;
	bool empty = false;
	vmutex_lock( &terrainMutex ); {
		cacheGrid* g = gridGetOrAdd( cache, uMin, vMin );
		future* fut = gridFuture(g, uMin, vMin);
		empty = !fut || (gridLod(g, uMin, vMin) > lodNeeded);
		if (empty) {
			fut = future_create();
			gridSetFuture(g, fut, uMin, vMin);
		}
		setLodNeeded( cache, uMin, vMin, lodNeeded );
		*f = fut;
	} vmutex_unlock( &terrainMutex );
	return empty;
}

void cacheBlockFor( CanyonTerrainBlock* b, int uRelative, int vRelative, int* uCache, int* vCache ) {
	const int uReal = b->uMin + uRelative;
	const int vReal = b->vMin + vRelative;
	*uCache = uReal - offset( uReal, CacheBlockSize );
	*vCache = vReal - offset( vReal, CacheBlockSize );
}

cacheBlock* cachedBlock( terrainCache* cache, int uMin, int vMin ) { return gridBlock( gridFor( cache, uMin, vMin ), uMin, vMin ); }

void getCacheExtents( CanyonTerrainBlock* b, int& cacheMinU, int& cacheMinV, int& cacheMaxU, int& cacheMaxV ) {
	const int maxStride = 4;
	const int stride = b->lodStride();
	cacheBlockFor( b, -maxStride, -maxStride, &cacheMinU, &cacheMinV );
	cacheBlockFor( b, b->u_samples * stride + maxStride, b->v_samples * stride + maxStride, &cacheMaxU, &cacheMaxV );
}

cacheBlocklist* cachesForBlock( CanyonTerrainBlock* b ) {
	int cacheMinU = 0, cacheMinV = 0, cacheMaxU = 0, cacheMaxV = 0;
	getCacheExtents(b, cacheMinU, cacheMinV, cacheMaxU, cacheMaxV );

	cacheBlocklist* caches = NULL;
	for (int u = cacheMinU; u <= cacheMaxU; u+=CacheBlockSize )
		for (int v = cacheMinV; v <= cacheMaxV; v+=CacheBlockSize ) {
			cacheBlock* c = cachedBlock( b->_canyon->cache, u, v );
			takeRef( c );
			caches = cacheBlocklist_cons( c, caches );
		}
	return caches;
}

bool cacheBlockContains( cacheBlock* b, int u, int v ) { return ( b->uMin == u && b->vMin == v ); }
