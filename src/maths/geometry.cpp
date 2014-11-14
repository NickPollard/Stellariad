// Geometry.c
#include "common.h"
#include "geometry.h"
//----------------------
#include "maths/maths.h"
#include "maths/vector.h"

// Calculate the normal and distance of a plane containing 3 points
// ax + by + cz - d = 0
void plane( vector a, vector b, vector c, vector* normal, float* d ) {
	// Calculate the plane of the 3 points
	vector ab, ac;
	Sub( &ab, &b, &a );
	Sub( &ac, &c, &a );
	Cross( normal, &ab, &ac );
	Normalize( normal, normal );
	*d = Dot( &a, normal );
}

// Find the point closest to POINT on the line segment A->B
// Returns how far (0.f->1.f) along the segment, and sets the position in CLOSEST if not null
float segment_closestPoint( vector a, vector b, vector point, vector* closest ) {
	vector line = vector_sub( b, a );
	vector normal = normalized( line );
	float a_d = Dot( &normal, &a );
	float b_d = Dot( &normal, &b );
	vAssert( a_d <= b_d );
	vector offset = vector_sub( a, vector_scaled( normal, a_d ));

	float point_d = Dot( &normal, &point );
	float d = fclamp( point_d, a_d, b_d);
	vector result = vector_add( vector_scaled( normal, d ), offset );
	if ( closest )
		*closest = result;

	return ( d - a_d ) / ( b_d - a_d );
}

vector eulerAngles( float yaw, float pitch, float roll ) {
	return Vector( pitch, yaw, roll, 0.f );
}

// Take the normal of a line in 2d (x-z) space
vector normal2d( vector line ) {
	return Vector( -line.coord.z, 0.f, line.coord.x, 0.f );
}

