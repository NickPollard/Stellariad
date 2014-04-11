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

void* blocksForVerts( vertPositions* verts, int* num ) {
	(void)verts;(void)num;
	int n = 5;
	pair** data = mem_alloc( sizeof( pair* ) * n );
	for ( int i = 0; i < n; ++i ) {
		data[i] = Pair( NULL, NULL ); // TODO
	}
	*num = n;
	return data;
}

void* testTask( void* args ) {
	(void)args;
	//printf( "Running a test task!\n" );
	return NULL;
}

Msg generateCache( pair* block ) {
	(void)block;
	return task( testTask, NULL );
}

/*
   At this point all the terrain caches should be filled,
   so we can just use them safely
   */
void generateVerts( canyonTerrainBlock* b, vertPositions* vertSources ) {
	vector* verts = vertSources->positions;
	for ( int v = -1; v < b->v_samples + 1; ++v )
		for ( int u = -1; u < b->u_samples + 1; ++u )
			verts[indexFromUV(b, u, v)] = terrainPointCached( b->canyon, b, u, v );

	worker_addTask( task( canyonTerrain_workerGenerateBlock, Pair( vertSources, b )));
}
void* worker_generateVerts( void* args ) {
	generateVerts( _1(args), _2(args) );
	return NULL;
}

/*
   Builds a vertPositions to be passed to a child worker to actually construct the terrain.
   This should normally just pull from cache - if blocks aren't there, build them
   */
vertPositions* generatePositions( canyonTerrainBlock* b) {
	vertPositions* vertSources = mem_alloc( sizeof( vertPositions )); // TODO - don't do a full mem_alloc here
	vertSources->uMin = -1;
	vertSources->vMin = -1;
	vertSources->uCount = b->u_samples + 2;
	vertSources->vCount = b->v_samples + 2;
	vertSources->positions = mem_alloc( sizeof( vector ) * vertCount( b ));

	int num;
	pair** cacheBlocks = blocksForVerts( vertSources, &num );
	Msg* messages = stackArray( Msg, num );
	messages[0] = onComplete( generateCache( cacheBlocks[0] ), task( worker_generateVerts, Pair( b, vertSources )) );
	for ( int i = 1; i < num; ++i )
		messages[i] = onComplete( generateCache( cacheBlocks[i] ), messages[i-1]);
	worker_addTask( messages[num-1] );

	return vertSources;
}

void* generateVertices_( void* args ) {
	generatePositions( args );
	return NULL;
}

Msg generateVertices( canyonTerrainBlock* b ) { 
	return task( generateVertices_, b );
}
