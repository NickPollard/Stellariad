// cache.c
#include "src/common.h"
#include "src/terrain/cache.h"
//---------------------
#include "canyon.h"
#include "future.h"
#include "terrain_generate.h"
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"
#include "terrain/canyon_terrain.h"
#include "terrain/terrain.h"

// Brando
#include "unit.h"
#include "concurrent/future.h"
#include "concurrent/task.h"
#include "functional/traverse.h"
#include "immutable/list.h"

DEF_LIST(cacheGrid);
IMPLEMENT_LIST(cacheBlock);
IMPLEMENT_LIST(cacheGrid);

struct TerrainCache {
  TerrainCache() : grids(nullptr), toDelete(nullptr) { }

	cacheGridlist* grids;	
	cacheBlocklist* toDelete;
};

vmutex terrainMutex = kMutexInitialiser;
vmutex blockMutex = kMutexInitialiser;

// Create a new cache block
cacheBlock* terrainCacheBlock( canyon* c, CanyonTerrain* t, int uMin, int vMin, int requiredLOD );

TerrainCache* terrainCache_create() {
	TerrainCache* t = new (mem_alloc(sizeof(TerrainCache))) TerrainCache();
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
cacheGrid* gridFor( TerrainCache* cache, int uMin, int vMin ) {
	const int uGrid = minStride(uMin, GridCapacity);
	const int vGrid = minStride(vMin, GridCapacity);
	for( cacheGridlist* g = cache->grids; g && g->head; g = g->tail )
		if ( g->head->uMin == uGrid && g->head->vMin == vGrid )
			return g->head;
	return NULL;
}

// Find the grid in the list, return it from that, else NULL
cacheBlock* terrainCached( TerrainCache* cache, int u, int v ) {
	cacheBlock* b = NULL;
	vmutex_lock( &terrainMutex ); {
		cacheGrid* g = gridFor( cache, u, v );
		b = g ? gridBlock( g, u, v ) : NULL;
	} vmutex_unlock( &terrainMutex );
	takeRef( b );
	return b;
}

cacheGrid* terrainCacheAddGrid( TerrainCache* t, cacheGrid* g ) {
	t->grids = cacheGridlist_cons( g, t->grids );
	return g;
}

// Idempotent
void terrain_deleteBlock( TerrainCache* cache, cacheBlock* b ) {
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
cacheGrid* gridGetOrAdd( TerrainCache* cache, int u, int v ) {
	cacheGrid* g = gridFor( cache, u, v );
	if (!g)
		g = terrainCacheAddGrid( cache, gridForIndex( u, v ));
	vAssert( g );
	return g;
}

// TerrainMutex-Locked
cacheBlock* terrainCacheAddInternal( TerrainCache* t, cacheBlock* b ) {
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

int cacheGetNeededLod( TerrainCache* cache, int u, int v ) {
	cacheGrid* g = gridGetOrAdd( cache, u, v );
	return g ? gridLod( g, u, v ) : lowestLod;
}

cacheBlock* terrainCacheBuildAndAdd( canyon* c, CanyonTerrain* t, int uMin, int vMin, int lod ) {
	vmutex_lock( &terrainMutex );
		const int highestLodNeeded = min( cacheGetNeededLod( t->cache, uMin, vMin), lod );
	vmutex_unlock( &terrainMutex );

	// Only add it if we need to!
	vmutex_lock( &terrainMutex );
		cacheGrid* g = gridGetOrAdd( t->cache, uMin, vMin );
		cacheBlock* cache = gridBlock( g, uMin, vMin );
	vmutex_unlock( &terrainMutex );
	if (cache && cache->lod <= lod ) {
		//skip
		printf( "Skipping building block, already have one.\n" );
	} else {
		cacheBlock* b = terrainCacheBlock( c, t, uMin, vMin, highestLodNeeded );
		vmutex_lock( &terrainMutex );
			cache = terrainCacheAddInternal( t->cache, b );
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
			b->positions[uOffset][vOffset] = Vector( vec.coord.x, canyon_sampleUV( u, v ), vec.coord.z, 1.f );
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

void terrainCache_trim( TerrainCache* t, int v ) {
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

cacheBlocklist* terrainCache_processDeletes( TerrainCache* t ) {
	for (cacheBlocklist* b = t->toDelete; b; b = b->tail )
		mem_free(b->head);
	cacheBlocklist_delete( t->toDelete );
	return NULL;
}

void terrainCache_tick( TerrainCache* t, float dt, vector sample ) {
	(void)dt; (void)t; (void)sample;

	vmutex_lock( &terrainMutex ); {
		t->toDelete = terrainCache_processDeletes( t );
	} vmutex_unlock( &terrainMutex );
}

// Need to have locked terrainMutex!
void setLodNeeded( TerrainCache* cache, int u, int v, int lodNeeded ) {
	cacheGrid* g = gridGetOrAdd( cache, u, v );
	const int lod = min(gridLod(g, u, v), lodNeeded);
	gridSetLod( g, lod, u, v );
}

bool cacheBlockFuture( TerrainCache* cache, int uMin, int vMin, int lodNeeded, future** f ) {
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

// Calculate the cacheblock indices for a given point coord
void cacheBlockFor( CanyonTerrainBlock* b, int uRelative, int vRelative, int* uCache, int* vCache ) {
	const int uReal = b->uMin + uRelative;
	const int vReal = b->vMin + vRelative;
	*uCache = uReal - offset( uReal, CacheBlockSize );
	*vCache = vReal - offset( vReal, CacheBlockSize );
}

cacheBlock* cachedBlock( TerrainCache* cache, int uMin, int vMin ) { return gridBlock( gridFor( cache, uMin, vMin ), uMin, vMin ); }

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
			cacheBlock* c = cachedBlock( b->terrain->cache, u, v );
			takeRef( c );
			caches = cacheBlocklist_cons( c, caches );
		}
	return caches;
}

bool cacheBlockContains( cacheBlock* b, int u, int v ) { return ( b->uMin == u && b->vMin == v ); }

cacheBlock* terrainCachedFromList( cacheBlocklist* blist, int u, int v ) {
	for ( cacheBlocklist* b = blist; b; b = b->tail )
		if ( b && cacheBlockContains( b->head, u, v ) )
			return b->head;
	return NULL;
}

// Read a position from the terrain Cache
vector terrainPointCached( CanyonTerrainBlock* b, cacheBlocklist* caches, int uRelative, int vRelative ) {
	const int uReal = b->uMin + uRelative;
	const int vReal = b->vMin + vRelative;
	const int uOffset = offset( uReal, CacheBlockSize );
	const int vOffset = offset( vReal, CacheBlockSize );
	const int uMin = uReal - uOffset;
	const int vMin = vReal - vOffset;

	cacheBlock* cache = terrainCachedFromList( caches, uMin, vMin );
	if ( !cache ) printf( "Missing cache block %d %d.\n", uMin, vMin );
	vAssert( cache && cache->lod <= b->lod_level );
	return cache->positions[uOffset][vOffset];
}

using brando::unit;
using brando::Unit;
using brando::concurrent::Future;
using brando::concurrent::Promise;
using brando::concurrent::Task;
using brando::functional::sequenceFutures;
using brando::immutable::List;
using brando::immutable::nil;

void* runBrandoTask( void* task ) {
  Task<Unit>* t = (Task<Unit>*)task;
  t->run();
  delete t;
  return nullptr;
}

worker_task fromBrandoTask(Task<Unit> t) {
  Task<Unit>* t_ = new Task<Unit>(t);
  return task( runBrandoTask, t_ );
}

void* buildCacheBlockTask(CanyonTerrainBlock* b, future* f, int uMin, int vMin) {
	canyon* c = b->terrain->_canyon;
	// ! only if not exist or lower-lod
	cacheBlock* cache = terrainCached( b->terrain->cache, uMin, vMin );
	bool rebuild = cache && cache->lod > b->lod_level;
	if (rebuild)
		cacheBlockFree( cache ); // Release the cache we don't want
	if (!cache || rebuild)
		cache = terrainCacheBuildAndAdd( c, b->terrain, uMin, vMin, b->lod_level );

	future_complete( f, cache ); // TODO - Should this be tryComplete? hit a segfault here
	cacheBlockFree( cache );
	return nullptr;
}

void* completeBrandoPromise(const void* v, void* promise) {
  (void)v;
  Promise<bool>* p = (Promise<bool>*)promise;
  p->complete(true);
  return nullptr;
}

// Request a cacheBlock of at least the required Lod for (U,V)
Future<bool> requestCache( CanyonTerrainBlock* b, int u, int v, Executor& ex ) {
	// If already built, or building, return that future, else start it building
	future* f = nullptr;
  // TODO replace with Brando future
	bool needCreating = cacheBlockFuture( b->terrain->cache, u, v, b->lod_level, &f);
	vAssert( f );
	if (needCreating)
		async( ex, buildCacheBlockTask(b, f, u, v); return unit(); );
	future_onComplete( f, takeCacheRef, nullptr );
  Promise<bool>* p = new Promise<bool>(ex);
  future_onComplete( f, completeBrandoPromise, p );
	return p->future();
}

void releaseAllCaches( cacheBlocklist* caches ) {
	for( cacheBlocklist* b = caches; b; b = b->tail )
		cacheBlockFree( b->head );
}

/* Builds a vertPositions to be passed to a child worker to actually construct the terrain.
   This should normally just pull from cache - if blocks aren't there, build them */
vertPositions* newVertPositions( CanyonTerrainBlock* b ) {
	vertPositions* vertSources = (vertPositions*)mem_alloc( sizeof( vertPositions )); // TODO - don't do a full mem_alloc here
	vertSources->uMin = -1;
	vertSources->vMin = -1;
	vertSources->uCount = b->u_samples + 2;
	vertSources->vCount = b->v_samples + 2;
	vertSources->positions = (vector*)mem_alloc( sizeof( vector ) * vertCount( b ));
  return vertSources;
}

int f(int stride, int strideMax, int samples, int i) {
  return (i == -1) ? -strideMax : (i == samples ? ((samples - 1) * stride + strideMax) : i * stride);
}

/* At this point all the terrain caches should be filled,
   so we can just use them safely */
void generateVerts( CanyonTerrainBlock* b, cacheBlocklist* caches, Executor& ex ) {
  auto vertSources = newVertPositions( b );
	vector* verts = vertSources->positions;
	const int max = vertCount( b );
	for ( int v = -1; v < b->v_samples+1; ++v )
		for ( int u = -1; u < b->u_samples+1; ++u ) {
			const int index = indexFromUV(b, u, v);
			vAssert( index >= 0 && index < max );
			const int strideMax = 4;
			const int stride = b->lodStride();
      int uu = f(stride, strideMax, b->u_samples, u);
      int vv = f(stride, strideMax, b->v_samples, v);
			verts[index] = terrainPointCached( b, caches, uu, vv );
		}

	releaseAllCaches( caches );
  async( ex, {
	  canyonTerrainBlock_generate( vertSources, b );
	  vertPositions_delete( vertSources );
    return unit();
    });
}

Future<bool> generateAllCaches( CanyonTerrainBlock* b, Executor& ex ) {
	int cacheMinU = 0, cacheMinV = 0, cacheMaxU = 0, cacheMaxV = 0;
	getCacheExtents(b, cacheMinU, cacheMinV, cacheMaxU, cacheMaxV );

	auto futures = nil<Future<bool>>();
	for (int u = cacheMinU; u <= cacheMaxU; u += CacheBlockSize )
		for (int v = cacheMinV; v <= cacheMaxV; v += CacheBlockSize ) {
			futures = requestCache( b, u, v, ex ) << futures;
		}
	return sequenceFutures(futures, ex).map(std::function<bool(List<bool>)>([](auto a){ (void)a; return true; }));
}

Msg generateVertices( CanyonTerrainBlock* b, Executor& ex ) { 
  Executor* e = &ex;
	Task<Unit> t = brando::concurrent::task([=]{
      generateAllCaches( b, *e ).foreach( [=](auto ignore){ (void)ignore; generateVerts( b, cachesForBlock( b ), *e ); });
      return unit();
      });
  return fromBrandoTask( t );
}
