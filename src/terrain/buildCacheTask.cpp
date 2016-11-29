// buildCacheTask.c
#include "src/common.h"
#include "src/terrain/buildCacheTask.h"
//---------------------
#include "canyon.h"
#include "terrain/canyon_terrain.h"
#include "future.h"
#include "terrain_generate.h"
#include "terrain/cache.h"
#include "worker.h"
#include "actor/actor.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"
// Brando
#include "unit.h"
#include "concurrent/future.h"
#include "concurrent/task.h"
#include "functional/traverse.h"
#include "immutable/list.h"

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
	cacheBlock* cache = terrainCached( c->cache, uMin, vMin );
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
	bool needCreating = cacheBlockFuture( b->terrain->_canyon->cache, u, v, b->lod_level, &f);
	vAssert( f );
	if (needCreating)
		worker_addTask( fromBrandoTask(brando::concurrent::task([=]{buildCacheBlockTask(b, f, u, v); return unit(); })));
	future_onComplete( f, takeCacheRef, nullptr );
  Promise<bool>* p = new Promise<bool>(ex);
  future_onComplete( f, completeBrandoPromise, p );
	return p->future();
}

void releaseAllCaches( cacheBlocklist* caches ) {
	for( cacheBlocklist* b = caches; b; b = b->tail )
		cacheBlockFree( b->head );
}

/* At this point all the terrain caches should be filled,
   so we can just use them safely */
void generateVerts( CanyonTerrainBlock* b, vertPositions* vertSources, cacheBlocklist* caches ) {
	vector* verts = vertSources->positions;
	const int max = vertCount( b );
	for ( int v = -1; v < b->v_samples+1; ++v )
		for ( int u = -1; u < b->u_samples+1; ++u ) {
			const int index = indexFromUV(b, u, v);
			vAssert( index >= 0 && index < max );
			const int strideMax = 4;
			const int stride = b->lodStride();
			int uu = (u == -1) ? -strideMax : (u == b->u_samples ? ((b->u_samples - 1) * stride + strideMax) : u * stride);
			int vv = (v == -1) ? -strideMax : (v == b->v_samples ? ((b->v_samples - 1) * stride + strideMax) : v * stride);
			//int vv = (v == -1) ? -strideMax : v * b->lodStride();
			verts[index] = terrainPointCached( b->_canyon, b, caches, uu, vv );
		}

	releaseAllCaches( caches );
	worker_addTask( task( canyonTerrain_workerGenerateBlock, Pair( vertSources, b )));
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

Msg generateVertices( CanyonTerrainBlock* b, Executor& ex ) { 
  Executor* e = &ex;
	Task<Unit> t = brando::concurrent::task([=]{
      generateAllCaches( b, *e ).foreach( [=](auto ignore){ (void)ignore; generateVerts( b, newVertPositions(b), cachesForBlock( b )); });
      return unit();
      });
  return fromBrandoTask( t );
}
