// cache.h
#pragma once
#include "maths/vector.h"
#include "base/list.h"

#define CacheBlockSize 64
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
} cacheGrid;

// *** Terrain Cache
terrainCache* terrainCache_create();

// Retrieve a currently cached block (if it exists; may return NULL)
cacheBlock* terrainCached( terrainCache* cache, int uMin, int vMin );

// Add a cacheBlock to the cache, and return the block
cacheBlock* terrainCacheAdd( terrainCache* t, cacheBlock* b );

// Create a new cache block
cacheBlock* terrainCacheBlock( canyon* c, canyonTerrain* t, int uMin, int vMin, int requiredLOD );

// Trim cache blocks (and grids) that are too far behind the V coord
void terrainCache_trim( terrainCache* t, int v );

// Free a cacheblock once no longer referenced
void cacheBlockFree( cacheBlock* b );
