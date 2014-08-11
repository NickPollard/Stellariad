// cache.h
#pragma once
#include "maths/vector.h"
#include "base/list.h"
#include "system/thread.h"

#define CacheBlockSize 32
#define GridSize 16
#define GridCapacity (CacheBlockSize * GridSize)

// *** Types

typedef struct cacheBlock_s {
	int uMin;
	int vMin;
	vector positions[CacheBlockSize][CacheBlockSize];
	int lod;
	int refCount;
} cacheBlock;

DEF_LIST(cacheBlock)

typedef struct cacheGrid_s {
	int uMin;
	int vMin;
	cacheBlock* blocks[GridSize][GridSize];
	future* futures[GridSize][GridSize];
	int neededLods[GridSize][GridSize];
} cacheGrid;

extern vmutex terrainMutex;

// *** Terrain Cache
terrainCache* terrainCache_create();

// Retrieve a currently cached block (if it exists; may return NULL)
cacheBlock* terrainCached( terrainCache* cache, int uMin, int vMin );

// Create and add a cacheBlock to the cache, and return the block
cacheBlock* terrainCacheBuildAndAdd( canyon* c, canyonTerrain* t, int uMin, int vMin, int lod );

// Add a cacheBlock to the cache, and return the block
cacheBlock* terrainCacheAdd( terrainCache* t, cacheBlock* b );

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
cacheBlocklist* cachesForBlock( canyonTerrainBlock* b );

// Calculate the cacheblock indices for a given point coord
void cacheBlockFor( canyonTerrainBlock* b, int uRelative, int vRelative, int* uCache, int* vCache );

bool cacheBlockContains( cacheBlock* b, int u, int v );
