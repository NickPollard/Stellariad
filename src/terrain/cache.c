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
	cacheBlocklist* toDelete;
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

void* takeCacheRef( const void* value, void* args ) {
	(void)args;
	takeRef( (cacheBlock*)value ); // URGH const cast?
	return NULL;
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
	memset( g->neededLods, 0, sizeof( int ) * GridSize * GridSize );
	const int lowestLod = 2;
	for ( int x = 0; x < GridSize; ++x )
		for ( int y = 0; y < GridSize; ++y )
			g->neededLods[x][y] = lowestLod;
	g->uMin = u;
	g->vMin = v;
	return g;
}

cacheGrid* terrainCacheAddGrid( terrainCache* t, cacheGrid* g ) {
	t->grids = cacheGridlist_cons( g, t->grids );
	return g;
}

cacheBlock* terrainCacheAddInternal( terrainCache* t, cacheBlock* b ) {
	cacheGrid* g = cachedGrid( t, b->uMin, b->vMin );
	if (!g)
		g = terrainCacheAddGrid( t, cacheGrid_create( minPeriod( b->uMin, GridCapacity ), minPeriod( b->vMin, GridCapacity )));
	const int u = gridIndex( b->uMin, minPeriod( b->uMin, GridCapacity ));
	const int v = gridIndex( b->vMin, minPeriod( b->vMin, GridCapacity ));
	cacheBlock* old = g->blocks[u][v];
	vAssert( u < GridSize && v < GridSize );
	if (old && old->lod <= b->lod)
		mem_free( b );
	else {
		// Is this safe?
		if (old)
			printf( "Freeing old cache block " xPTRf "\n", (uintptr_t)old );
		t->toDelete = cacheBlocklist_cons( old, t->toDelete );
		g->blocks[u][v] = b;
		takeRef( b );
	}
	return b;
}

int cacheGetNeededLod( terrainCache* cache, int uMin, int vMin ) {
	cacheGrid* g = cachedGrid( cache, uMin, vMin );
	if (!g)
		g = terrainCacheAddGrid( cache, cacheGrid_create( minPeriod( uMin, GridCapacity ), minPeriod( vMin, GridCapacity )));
	const int lowestLod = 2;
	const int lod = g ? g->neededLods[gridIndex(uMin, minPeriod(uMin, GridCapacity))][gridIndex(vMin, minPeriod(vMin, GridCapacity))] : lowestLod;
	return lod;
}

cacheBlock* terrainCacheBuildAndAdd( canyon* c, canyonTerrain* t, int uMin, int vMin, int lod ) {
	cacheBlock* b = NULL;
	vmutex_lock( &terrainMutex ); {
		const int highestLodNeeded = min( cacheGetNeededLod( c->terrainCache, uMin, vMin), lod ); // TODO - Get this from the cache, in case we need a higher lod than we thought
		//const int highestLodNeeded = min( lod, lod ); // TODO - Get this from the cache, in case we need a higher lod than we thought
		b =	terrainCacheAddInternal( c->terrainCache, terrainCacheBlock( c, t, uMin, vMin, highestLodNeeded ));
	} vmutex_unlock( &terrainMutex );
	return b;
}

cacheBlock* terrainCacheAdd( terrainCache* t, cacheBlock* b ) {
	vmutex_lock( &terrainMutex ); {
		terrainCacheAddInternal( t, b );
	} vmutex_unlock( &terrainMutex );
	return b;
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

cacheBlock* terrainCacheBlock( canyon* c, canyonTerrain* t, int uMin, int vMin, int requiredLOD ) {
	++numCaches;
	cacheBlock* b = mem_alloc( sizeof( cacheBlock )); // TODO - don't do full mem_alloc here
	b->uMin = uMin;
	b->vMin = vMin;
	b->lod = requiredLOD;
	int lod = max( 1, 2 * requiredLOD );
#define LodVerts (CacheBlockSize / 4 + 1)
	vector worldSpace[LodVerts][LodVerts];
	for ( int vOffset = 0; vOffset < LodVerts; ++vOffset ) {
		for ( int uOffset = 0; uOffset < LodVerts; ++uOffset ) {
			float u, v, x, z;
			terrain_positionsFromUV( t, uMin + uOffset*4, vMin + vOffset*4, &u, &v );
			terrain_worldSpaceFromCanyon( c, u, v, &x, &z );
			worldSpace[uOffset][vOffset] = Vector(x, 0.f, z, 0.f);
		}
	}
	for ( int vOffset = 0; vOffset < CacheBlockSize; vOffset+=lod ) {
		for ( int uOffset = 0; uOffset < CacheBlockSize; uOffset+=lod ) {
			float u, v;
			terrain_positionsFromUV( t, uMin + uOffset, vMin + vOffset, &u, &v );
			vector vec = vecSample( (vector*)worldSpace, LodVerts, LodVerts, ((float)uOffset) / 4.f, ((float)vOffset) / 4.f );
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
				printf( "Deleting cache block " xPTRf "\n", (uintptr_t)b );
				mem_free( b );
			}
		}
	} vmutex_unlock( &blockMutex );
}

void trimCacheGrid( cacheGrid* g, int v ) {
	const int vMax = clamp( v - g->vMin, 0, GridSize );
	//printf( "Trimming for V %d (this block: %d)\n", v, vMax );
	for ( int u = 0; u < GridSize; ++u )
		for ( int v = 0; v < vMax; ++v )
			if ( g->blocks[u][v] && g->blocks[u][v]->refCount == 1 ) {
				// TODO - Need to store a separate RefCount for the co-ord, NOT the block
				// Allow blocks to be replaced, but not trimmed unless the refcount is 0
				//printf( "Freeing cache %d %d.\n", g->uMin + u, g->vMin + v );
				//cacheBlockFree(g->blocks[u][v]);
				//g->blocks[u][v] = NULL;
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
	vmutex_lock( &terrainMutex ); {
		//cacheGridlist* nw = NULL;
		cacheGridlist* g = t->grids;
		while ( g && g->head ) {
			trimCacheGrid( g->head, v );
			/*
			(void)v;
			if (gridEmpty(g->head)) {
				//printf( "Freeing grid list " xPTRf "\n", (uintptr_t)g );
				mem_free( g->head );
			} else {
				//vAssert( (uintptr_t)g->head < 0x00ffffffffffffff );
				nw = cacheGridlist_cons( g->head, nw );
			}
			*/
			cacheGridlist* next = g->tail;
			//mem_free( g );
			g = next;
		}
		//t->grids = nw;
	} vmutex_unlock( &terrainMutex );
	(void)t;(void)v;
}

cacheBlocklist* terrainCache_processDeletes( terrainCache* t ) {
	for (cacheBlocklist* b = t->toDelete; b; b = b->tail ) {
		mem_free(b->head);
	}
	return NULL;
}

void terrainCache_tick( terrainCache* t, float dt, vector sample ) {
	(void)dt;
	(void)t;
	(void)sample;
	t->toDelete = terrainCache_processDeletes( t );
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

// Need to have locked terrainMutex!
void setLodNeeded( terrainCache* cache, int uMin, int vMin, int lodNeeded ) {
	cacheGrid* g = cachedGrid( cache, uMin, vMin );
	if (!g)
		g = terrainCacheAddGrid( cache, cacheGrid_create( minPeriod( uMin, GridCapacity ), minPeriod( vMin, GridCapacity )));
	vAssert( g );
	const int lod = min(g->neededLods[gridIndex(uMin, minPeriod(uMin, GridCapacity))][gridIndex(vMin, minPeriod(vMin, GridCapacity))], lodNeeded);
	g->neededLods[gridIndex(uMin, minPeriod(uMin, GridCapacity))][gridIndex(vMin, minPeriod(vMin, GridCapacity))] = lod;
}

bool cacheBlockFuture( terrainCache* cache, int uMin, int vMin, int lodNeeded, future** f ) {
	(void)lodNeeded;
	bool empty = false;
	vmutex_lock( &terrainMutex ); {
		const int u = minPeriod( uMin, GridCapacity );
		const int v = minPeriod( vMin, GridCapacity );
		const int gu = gridIndex(uMin, u);
		const int gv = gridIndex(vMin, v);
		cacheGrid* g = cachedGrid( cache, uMin, vMin );
		if (!g)
			g = terrainCacheAddGrid( cache, cacheGrid_create( u, v ));
		vAssert( g ) ;
		future* fut = g->futures[gu][gv];
		empty = !fut || (g->neededLods[gu][gv] > lodNeeded);
		if (empty) {
			fut = future_create();
			g->futures[gu][gv] = fut;
		}
		setLodNeeded( cache, uMin, vMin, lodNeeded );
		*f = fut;
	} vmutex_unlock( &terrainMutex );
	return empty;
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

cacheBlock* cachedBlock( terrainCache* cache, int uMin, int vMin ) {

	const int u = minPeriod( uMin, GridCapacity );
	const int v = minPeriod( vMin, GridCapacity );
	const int gu = gridIndex(uMin, u);
	const int gv = gridIndex(vMin, v);
	//cacheGrid* g = cachedGrid( cache, uMin, vMin );

	cacheGrid* grid = cachedGrid( cache, uMin, vMin );
	//if (grid) { // TODO!
		//const int u = uMin - grid->uMin;
		//const int v = vMin - grid->vMin;
		return grid->blocks[gu][gv];
	//}
	//return NULL;
}

cacheBlocklist* cachesForBlock( canyonTerrainBlock* b ) {
	int cacheMinU = 0, cacheMinV = 0, cacheMaxU = 0, cacheMaxV = 0;
	cacheBlockFor( b, -1, -1, &cacheMinU, &cacheMinV );
	cacheBlockFor( b, b->u_samples, b->v_samples, &cacheMaxU, &cacheMaxV );
	cacheBlocklist* caches = NULL;
	for (int u = cacheMinU; u <= cacheMaxU; u+=CacheBlockSize )
		for (int v = cacheMinV; v <= cacheMaxV; v+=CacheBlockSize ) {
			cacheBlock* c = cachedBlock( b->canyon->terrainCache, u, v );
			takeRef( c );
			caches = cacheBlocklist_cons( c, caches );
		}
	return caches;
}

bool cacheBlockContains( cacheBlock* b, int u, int v ) {
	return ( b->uMin == u && b->vMin == v );
}
