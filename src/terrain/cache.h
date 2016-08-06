// cache.h
#pragma once
#include "maths/vector.h"
#include "base/list.h"
#include "system/thread.h"
#include "terrain/grid.h"

#define CacheBlockSize 32
#define lowestLod 2;

// *** Types

struct cacheBlock_s {
	int uMin;
	int vMin;
	vector positions[CacheBlockSize][CacheBlockSize];
	int lod;
	int refCount;
};

DEF_LIST(cacheBlock)

extern vmutex terrainMutex;

// *** Terrain Cache
terrainCache* terrainCache_create();

// Retrieve a currently cached block (if it exists; may return NULL)
cacheBlock* terrainCached( terrainCache* cache, int uMin, int vMin );

// Create and add a cacheBlock to the cache, and return the block
cacheBlock* terrainCacheBuildAndAdd( canyon* c, canyonTerrain* t, int uMin, int vMin, int lod );

// Create a new cache block
cacheBlock* terrainCacheBlock( canyon* c, canyonTerrain* t, int uMin, int vMin, int requiredLOD );

// Trim cache blocks (and grids) that are too far behind the V coord
void terrainCache_trim( terrainCache* t, int v );

// Free a cacheblock once no longer referenced
void cacheBlockFree( cacheBlock* b );

// Tick the cache
void terrainCache_tick( terrainCache* t, float dt, vector sample );

bool cacheBlockFuture( terrainCache* cache, int uMin, int vMin, int lodNeeded, future** f );

// Take a ref to a cache (from a future)
void* takeCacheRef( const void* value, void* args );

// Return a list of cacheblocks needed for building a given block
cacheBlocklist* cachesForBlock( CanyonTerrainBlock* b );

// Calculate the cacheblock indices for a given point coord
void cacheBlockFor( CanyonTerrainBlock* b, int uRelative, int vRelative, int* uCache, int* vCache );

// Get cache extents needed to build a given block
void getCacheExtents( CanyonTerrainBlock* b, int& cacheMinU, int& cacheMinV, int& cacheMaxU, int& cacheMaxV );

bool cacheBlockContains( cacheBlock* b, int u, int v );
