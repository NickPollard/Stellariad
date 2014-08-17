// grid.c
#include "src/common.h"
#include "src/terrain/grid.h"
//---------------------
#include "future.h"
#include "maths/maths.h"
#include "mem/allocator.h"
#include "terrain/cache.h"

int gridIndex( int i, int grid ) { return (i - grid) / CacheBlockSize; }

cacheBlock* gridBlock( cacheGrid* g, int u, int v ) {
	const int uMin = gridIndex(u, minPeriod(u, GridCapacity));
	const int vMin = gridIndex(v, minPeriod(v, GridCapacity));
	vAssert( uMin >= 0 && uMin < GridSize );
	vAssert( vMin >= 0 && vMin < GridSize );
	return g->blocks[uMin][vMin];
}
void gridSetBlock( cacheGrid* g, cacheBlock* b, int u, int v ) {
	const int uMin = gridIndex(u, minPeriod(u, GridCapacity));
	const int vMin = gridIndex(v, minPeriod(v, GridCapacity));
	vAssert( uMin >= 0 && uMin < GridSize );
	vAssert( vMin >= 0 && vMin < GridSize );
	g->blocks[uMin][vMin] = b;
}
void gridSetLod( cacheGrid* g, int lod, int u, int v ) {
	const int uMin = gridIndex(u, minPeriod(u, GridCapacity));
	const int vMin = gridIndex(v, minPeriod(v, GridCapacity));
	vAssert( uMin >= 0 && uMin < GridSize );
	vAssert( vMin >= 0 && vMin < GridSize );
	g->neededLods[uMin][vMin] = lod;
}
int gridLod( cacheGrid* g, int u, int v ) {
	const int uMin = gridIndex(u, minPeriod(u, GridCapacity));
	const int vMin = gridIndex(v, minPeriod(v, GridCapacity));
	vAssert( uMin >= 0 && uMin < GridSize );
	vAssert( vMin >= 0 && vMin < GridSize );
	return g->neededLods[uMin][vMin];
}
void gridSetFuture( cacheGrid* g, future* f, int u, int v ) {
	const int uMin = gridIndex(u, minPeriod(u, GridCapacity));
	const int vMin = gridIndex(v, minPeriod(v, GridCapacity));
	vAssert( uMin >= 0 && uMin < GridSize );
	vAssert( vMin >= 0 && vMin < GridSize );
	g->futures[uMin][vMin] = f;
}
future* gridFuture( cacheGrid* g, int u, int v ) {
	const int uMin = gridIndex(u, minPeriod(u, GridCapacity));
	const int vMin = gridIndex(v, minPeriod(v, GridCapacity));
	vAssert( uMin >= 0 && uMin < GridSize );
	vAssert( vMin >= 0 && vMin < GridSize );
	return g->futures[uMin][vMin];
}

cacheGrid* cacheGrid_create( int u, int v ) {
	cacheGrid* g = mem_alloc( sizeof( cacheGrid ));
	memset( g->blocks, 0, sizeof( cacheBlock* ) * GridSize * GridSize );
	memset( g->futures, 0, sizeof( future* ) * GridSize * GridSize );
	memset( g->neededLods, 0, sizeof( int ) * GridSize * GridSize );
	for ( int x = 0; x < GridSize; ++x )
		for ( int y = 0; y < GridSize; ++y )
			g->neededLods[x][y] = lowestLod;
	g->uMin = u;
	g->vMin = v;
	return g;
}
