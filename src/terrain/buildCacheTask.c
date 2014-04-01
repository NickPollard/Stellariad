// buildCacheTask.c
#include "src/common.h"
#include "src/terrain/buildCacheTask.h"
//---------------------
#include "canyon.h"
#include "canyon_terrain.h"
#include "terrain_generate.h"
#include "terrain/cache.h"
#include "worker.h"
#include "actor/actor.h"
#include "base/pair.h"
#include "mem/allocator.h"
#include "system/thread.h"

void* generateVertices_( void* args ) {
	canyonTerrainBlock* b = args;
	canyonTerrainBlock_calculateExtents( b, b->terrain, b->coord );
	vertPositions* vertSources = generatePositions( b );
	worker_addTask( task( canyonTerrain_workerGenerateBlock, Pair( vertSources, b )));
	return NULL;
}

Msg generateVertices( canyonTerrainBlock* b ) { 
	return task( generateVertices_, b );
}
