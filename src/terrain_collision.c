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
	terrainRenderable* r = b->renderable;
	for ( int i = 0; i < b->u_samples; i++ ) {
		for ( int j = 0; j < b->v_samples; j++ ) {
			int index = canyonTerrainBlock_renderIndexFromUV( b, i, j );
			h->verts[i + j * b->u_samples] = r->vertex_buffer[index].position;
		}
	}
	heightField_calculateAABB( h );
	shape* s = shape_heightField_create( h );
	b->collision = body_create( s, transform_create());
	b->collision->layers |= kCollisionLayerTerrain;

	//b->collision->collide_with |= kCollisionLayerPlayer;
	collision_addBody( b->collision );
}

void terrainBlock_calculateAABB( terrainRenderable* r ) {
	r->bb.min = r->vertex_buffer[0].position;
	r->bb.max = r->vertex_buffer[0].position;
	int vertex_count = canyonTerrainBlock_renderVertCount( r->block );
	for ( int i = 1; i < vertex_count; ++i ) {
		vector vert = r->vertex_buffer[i].position;
		r->bb.min.coord.x = fminf( r->bb.min.coord.x, vert.coord.x );
		r->bb.min.coord.y = fminf( r->bb.min.coord.y, vert.coord.y );
		r->bb.min.coord.z = fminf( r->bb.min.coord.z, vert.coord.z );

		r->bb.max.coord.x = fmaxf( r->bb.max.coord.x, vert.coord.x );
		r->bb.max.coord.y = fmaxf( r->bb.max.coord.y, vert.coord.y );
		r->bb.max.coord.z = fmaxf( r->bb.max.coord.z, vert.coord.z );
	}
}
