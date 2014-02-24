// terrain_collision.c
#include "common.h"
#include "terrain_collision.h"
//-----------------------
#include "canyon_terrain.h"
#include "collision.h"

void terrainBlock_removeCollision( canyonTerrainBlock* b ) {
	transform_delete( b->collision->trans );
	collision_removeBody( b->collision );
	b->collision = NULL;
}

void terrainBlock_calculateCollision( canyonTerrainBlock* b ) {
	if ( b->collision)
		terrainBlock_removeCollision( b );

	heightField* h = heightField_create( b->u_max - b->u_min, 
											b->v_max - b->v_min, 
											b->u_samples, 
											b->v_samples );
	for ( int i = 0; i < b->u_samples; i++ ) {
		for ( int j = 0; j < b->v_samples; j++ ) {
			int index = canyonTerrainBlock_renderIndexFromUV( b, i, j );
			h->verts[i + j * b->u_samples] = b->vertex_buffer[index].position;
		}
	}
	heightField_calculateAABB( h );
	shape* s = shape_heightField_create( h );
	b->collision = body_create( s, transform_create());
	b->collision->layers |= kCollisionLayerTerrain;

	//b->collision->collide_with |= kCollisionLayerPlayer;
	collision_addBody( b->collision );
}

void terrainBlock_calculateAABB( canyonTerrainBlock* b ) {
	b->bb.min = b->vertex_buffer[0].position;
	b->bb.max = b->vertex_buffer[0].position;
	int vertex_count = canyonTerrainBlock_renderVertCount( b );
	for ( int i = 1; i < vertex_count; ++i ) {
		vector vert = b->vertex_buffer[i].position;
		b->bb.min.coord.x = fminf( b->bb.min.coord.x, vert.coord.x );
		b->bb.min.coord.y = fminf( b->bb.min.coord.y, vert.coord.y );
		b->bb.min.coord.z = fminf( b->bb.min.coord.z, vert.coord.z );

		b->bb.max.coord.x = fmaxf( b->bb.max.coord.x, vert.coord.x );
		b->bb.max.coord.y = fmaxf( b->bb.max.coord.y, vert.coord.y );
		b->bb.max.coord.z = fmaxf( b->bb.max.coord.z, vert.coord.z );
	}
}
