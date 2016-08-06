// terrain_collision.h
#include "canyon_terrain.h"

void terrainBlock_removeCollision( CanyonTerrainBlock* b );
void terrainBlock_calculateCollision( CanyonTerrainBlock* b );
void terrainBlock_calculateAABB( terrainRenderable* b );
