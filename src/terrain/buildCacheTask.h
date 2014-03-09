// buildCacheTask.h
#include "maths/vector.h"
#include "base/list.h"

#define CacheBlockSize 16

typedef struct cacheBlock_s {
	int uMin;
	int vMin;
	vector positions[CacheBlockSize][CacheBlockSize];
} cacheBlock;

DEF_LIST(cacheBlock)

struct terrainCache_s {
	cacheBlocklist* blocks;	
};

void buildCache_queueWorkerTask( canyon* c, int u, int v );
