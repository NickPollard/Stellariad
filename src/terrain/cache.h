// cache.h
#pragma once
#include "maths/vector.h"
#include "base/list.h"
#include "system/thread.h"
#include "terrain/grid.h"

#define CacheBlockSize 32
#define lowestLod 2;

#include "concurrent/task.h"

using brando::concurrent::Executor;

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
TerrainCache* terrainCache_create();

// Trim cache blocks (and grids) that are too far behind the V coord
void terrainCache_trim( TerrainCache* t, int v );

// Tick the cache
void terrainCache_tick( TerrainCache* t, float dt, vector sample );

// *** Worker Task ***
Msg generateVertices( CanyonTerrainBlock* b, Executor& ex );
