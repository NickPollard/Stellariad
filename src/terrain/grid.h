// grid.h
#pragma once

#define GridSize 16
#define GridCapacity (CacheBlockSize * GridSize)

typedef struct cacheGrid_s {
	int uMin;
	int vMin;
	cacheBlock* blocks[GridSize][GridSize];
	future* futures[GridSize][GridSize];
	int neededLods[GridSize][GridSize];
} cacheGrid;



cacheBlock* gridBlock( cacheGrid* g, int u, int v );
void		gridSetBlock( cacheGrid* g, cacheBlock* b, int u, int v );

void	gridSetLod( cacheGrid* g, int lod, int u, int v );
int		gridLod( cacheGrid* g, int u, int v );

void	gridSetFuture( cacheGrid* g, future* f, int u, int v );
future* gridFuture( cacheGrid* g, int u, int v );

cacheGrid* cacheGrid_create( int u, int v );
