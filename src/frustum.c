// frustum.c

#include "common.h"
#include "frustum.h"
//-----------------------
#include "render/debugdraw.h"

#define kFrustumPlanes 4

void aabb_expand( aabb* bb, vector* points ) {
	points[0] = bb->min;
	points[1] = bb->max;
	points[2] = Vector( bb->max.coord.x, bb->min.coord.y, bb->min.coord.z, 1.f );
	points[3] = Vector( bb->min.coord.x, bb->max.coord.y, bb->min.coord.z, 1.f );
	points[4] = Vector( bb->min.coord.x, bb->min.coord.y, bb->max.coord.z, 1.f );
	points[5] = Vector( bb->max.coord.x, bb->max.coord.y, bb->min.coord.z, 1.f );
	points[6] = Vector( bb->min.coord.x, bb->max.coord.y, bb->max.coord.z, 1.f );
	points[7] = Vector( bb->max.coord.x, bb->min.coord.y, bb->max.coord.z, 1.f );
}

bool plane_cull( aabb* bb, vector* plane ) {
	vector points[8];
	aabb_expand( bb, points );
	for ( int i = 0; i < 6; i++ ) {
		if ( Dot( &points[i], plane ) > plane->coord.w )
			return false;
	}
	return true;
}

// Cull the AABB *bb* against the frustum defined as 6 planes in *frustum*
bool frustum_cull( aabb* bb, vector* frustum ) {
	for ( int i = 0; i < kFrustumPlanes; ++i )
		if ( plane_cull( bb, &frustum[i] ))
			return true;
	return false;
}

aabb aabb_calculate( obb bb, matrix m ) {
	vector points[8];
	points[0] = bb.min;
	points[1] = bb.max;
	points[2] = Vector( bb.max.coord.x, bb.min.coord.y, bb.min.coord.z, 1.f );
	points[3] = Vector( bb.min.coord.x, bb.max.coord.y, bb.min.coord.z, 1.f );
	points[4] = Vector( bb.min.coord.x, bb.min.coord.y, bb.max.coord.z, 1.f );
	points[5] = Vector( bb.max.coord.x, bb.max.coord.y, bb.min.coord.z, 1.f );
	points[6] = Vector( bb.min.coord.x, bb.max.coord.y, bb.max.coord.z, 1.f );
	points[7] = Vector( bb.max.coord.x, bb.min.coord.y, bb.max.coord.z, 1.f );
	
	aabb aligned_bb;

	vector vert = points[0];
	if ( m )
		vert = matrix_vecMul( m, &points[0] );
	aligned_bb.min = vert;
	aligned_bb.max = vert;

	for ( int i = 1; i < 8; ++i ) {
		vert = points[i];
		if ( m )
			vert = matrix_vecMul( m, &points[i] );
		aligned_bb.min.coord.x = fminf( aligned_bb.min.coord.x, vert.coord.x );
		aligned_bb.min.coord.y = fminf( aligned_bb.min.coord.y, vert.coord.y );
		aligned_bb.min.coord.z = fminf( aligned_bb.min.coord.z, vert.coord.z );

		aligned_bb.max.coord.x = fmaxf( aligned_bb.max.coord.x, vert.coord.x );
		aligned_bb.max.coord.y = fmaxf( aligned_bb.max.coord.y, vert.coord.y );
		aligned_bb.max.coord.z = fmaxf( aligned_bb.max.coord.z, vert.coord.z );
	}
	return aligned_bb;
}

void debugdraw_aabb( aabb bb ) {
	vector points[8];
	aabb_expand( &bb, points );
	debugdraw_line3d( points[0], points[3], color_green );
	debugdraw_line3d( points[0], points[4], color_green );
	debugdraw_line3d( points[3], points[6], color_green );
	debugdraw_line3d( points[4], points[6], color_green );

	debugdraw_line3d( points[1], points[5], color_green );
	debugdraw_line3d( points[1], points[7], color_green );
	debugdraw_line3d( points[5], points[2], color_green );
	debugdraw_line3d( points[7], points[2], color_green );

	debugdraw_line3d( points[0], points[2], color_green );
	debugdraw_line3d( points[1], points[6], color_green );
	debugdraw_line3d( points[3], points[5], color_green );
	debugdraw_line3d( points[4], points[7], color_green );
}
