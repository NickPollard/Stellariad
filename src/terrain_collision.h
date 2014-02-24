// terrain_collision.h
#include "canyon_terrain.h"

void terrainBlock_removeCollision( canyonTerrainBlock* b );
void terrainBlock_calculateCollision( canyonTerrainBlock* b );
void terrainBlock_calculateAABB( canyonTerrainBlock* b );
