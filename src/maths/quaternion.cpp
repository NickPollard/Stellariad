// quaternion.c
#include "common.h"
#include "quaternion.h"
//---------------------
#include "maths/maths.h"
#include "maths/vector.h"
#include "test.h"

#include <math.h>

// Constructor
quaternion Quaternion( float s, float x, float y, float z ) {
	quaternion q = { s, x, y, z };
	return q;
}

// Are two quaternions equal value?
bool quaternion_equal( quaternion a, quaternion b ) {
	bool equal = true;
	equal &= f_eq( a.s, b.s );
	equal &= f_eq( a.x, b.x );
	equal &= f_eq( a.y, b.y );
	equal &= f_eq( a.z, b.z );
	return equal;
}

quaternion quaternion_fromAxisAngle( vector axis, float angle ) {
	quaternion q;
	q.s = cos( angle/2 );
	float k = sinf( angle/2 );
	q.x = k * axis.coord.x;
	q.y = k * axis.coord.y;
	q.z = k * axis.coord.z;
	return q;
}

void quaternion_getAxisAngle( quaternion q, vector* axis, float* angle ) {
	*angle = 2 * acos( q.s );
	float k = 1 / sinf( (*angle)/2 );
	axis->coord.x = q.x * k;
	axis->coord.y = q.y * k;
	axis->coord.z = q.z * k;
	axis->coord.w = 0.f;
}

quaternion quaternion_conjugation( quaternion a, quaternion b ) {
	quaternion a_ = quaternion_inverse( a );
	quaternion q = quaternion_multiply( a, quaternion_multiply( b, a_ ));
	return q;
}

quaternion quaternion_fromVector( vector v ) {
	quaternion q;
	q.s = 0.f;
	q.x = v.coord.x;
	q.y = v.coord.y;
	q.z = v.coord.z;
	return q;
}

// Do a straight quaternion rotation on vector v
vector	quaternion_rotation( quaternion rotation, vector v ) {
	quaternion q = quaternion_fromVector( v );
	quaternion q_rot = quaternion_conjugation( rotation, q );
	return vector_fromQuaternion( q_rot );
}

quaternion quaternion_inverse( quaternion q ) {
	quaternion q_;
	q_.s = q.s;
	q_.x = -q.x;
	q_.y = -q.y;
	q_.z = -q.z;
	return q_;
}

// Quaternion Multiplication a * b - Hamilton product
quaternion quaternion_multiply( quaternion a, quaternion b ) {
	quaternion q;
	q.s = ( a.s*b.s - a.x*b.x - a.y*b.y - a.z*b.z );
	q.x = ( a.s*b.x + a.x*b.s + a.y*b.z - a.z*b.y );
	q.y = ( a.s*b.y - a.x*b.z + a.y*b.s + a.z*b.x );
	q.z = ( a.s*b.z + a.x*b.y - a.y*b.x + a.z*b.s );
	return q;
}

// Build a rotation quaternion from Euler Angle values
// Vitae Uses YAW-PITCH-ROLL ordering of Euler Angles
//   Yaw is the angle between the x-axis and the line of nodes
//   Pitch is the angle between the z-axis and the Z-axis
//   Roll is the angle between the line of nodes and the X-axis
quaternion quaternion_fromEuler( vector* euler_angles ) {
	//vector y_axis = Vector( 0.f, 1.f, 0.f, 0.f );
	// Compose it as 3 subsequent rotations
	quaternion yaw = quaternion_fromAxisAngle( y_axis, euler_angles->coord.y );
	vector X_axis = quaternion_rotation( yaw, Vector( 1.f, 0.f, 0.f, 0.f ));
	quaternion pitch = quaternion_fromAxisAngle( X_axis, euler_angles->coord.x );
	quaternion yaw_pitch = quaternion_multiply( pitch, yaw );
	vector Z_axis = quaternion_rotation( yaw_pitch, Vector( 0.f, 0.f, 1.f, 0.f ));
	//vector Z_axis = Vector( 0.f, 0.f, 1.f, 0.f );
	quaternion roll = quaternion_fromAxisAngle( Z_axis, euler_angles->coord.z );

	quaternion q = quaternion_multiply( roll, quaternion_multiply( pitch, yaw ));

	return q;
}

quaternion quaternion_negate( quaternion q ) {
	q.s = -q.s;
	q.x = -q.x;
	q.y = -q.y;
	q.z = -q.z;
	return q;
}

quaternion quaternion_slerp( quaternion p0, quaternion p1, float t ) {
	// If the dot product is negative, we negate one of the ends to force the short path ( o < 180deg )
	quaternion r;
	float dot = Dot4( (vector*)&p0, (vector*)&p1 );
	if ( dot < 0.f ) {
		p1 = quaternion_negate( p1 );
		dot = -dot;
	}
	// If angles are really close (or identical), just do a linear interp to stop degeneracy
	if ( dot  > 0.99f ) {
		r.s = (1.f - t) * p0.s + t * p1.s;
		r.x = (1.f - t) * p0.x + t * p1.x;
		r.y = (1.f - t) * p0.y + t * p1.y;
		r.z = (1.f - t) * p0.z + t * p1.z;
	}
	else {
		float o = acosf( dot );		// Omega - total angle between p0 and p1
		float sin_o = sinf( o );	// Cache this
		float kp0 = sinf( (1 - t) * o ) / sin_o;
		float kp1 = sinf( t * o ) / sin_o;
		r.s = kp0 * p0.s + kp1 * p1.s;
		r.x = kp0 * p0.x + kp1 * p1.x;
		r.y = kp0 * p0.y + kp1 * p1.y;
		r.z = kp0 * p0.z + kp1 * p1.z;
	}
	return r;
}

float quaternion_angleBetween( quaternion p0, quaternion p1 ) {
	vector v0 = quaternion_rotation( p0, z_axis );
	vector v1 = quaternion_rotation( p1, z_axis );
	float d = Dot( &v0, &v1 );
	return acosf( d );
}

// Slerp by a given angle
quaternion quaternion_slerpAngle( quaternion p0, quaternion p1, float angle ) {
	float total_angle = quaternion_angleBetween( p0, p1 );
	float t = fclamp( angle / total_angle, 0.f, 1.f );
	return quaternion_slerp( p0, p1, t );
}


/*
quaternion quat_fromMatrix( matrix m ) {
	quaternion q;
	// Need to calculate axis and angle of rotation
	// Quaternion is:
	// v s where v is sin(2t) . axis
	// s is cos(2t)

	vector* z_old = NULL;
	const vector* z_new = matrix_getCol( m, 2 );
	// The axis of rotation must be perpendicular to both old and new angles
	// If both Z axis are identical, the axis of rotation must be the Z axis
	vector axis; 
	Cross( &axis, z_old, z_new );
	float cos_t = Dot( z_old, z_new );
	float t = acos( cos_t );
	(void)t;
	return q;
}
*/

// *** Output
void quaternion_print( const quaternion* q ) {
	printf( "%.4f, %.4f, %.4f, %.4f", q->s, q->x, q->y, q->z );
}

void quaternion_printf( const char* label, const quaternion* q ) {
	printf( "%s", label );
	quaternion_print( q );
	printf( "\n" );
}

#if UNIT_TEST
void test_quaternion() {
	quaternion i = Quaternion( 0.f, 1.f, 0.f, 0.f );
	quaternion j = Quaternion( 0.f, 0.f, 1.f, 0.f );
	quaternion k = Quaternion( 0.f, 0.f, 0.f, 1.f );
	quaternion neg_k = Quaternion( 0.f, 0.f, 0.f, -1.f );

	test( quaternion_equal( i, i ), "Quaternion equals success: i = i", "Quaternion equals failed: i != i" );
	test( !quaternion_equal( i, j ), "Quaternion equals success: i != j", "Quaternion equals failed: i = j" );

	quaternion q = quaternion_multiply( i, j );
	test( quaternion_equal( q, k ), "Quaternion Multiply success: i*j = k", "Quaternion multiply failed: i*j != k" );

	q = quaternion_multiply( j, i );	
	test( quaternion_equal( q, neg_k ), "Quaternion Multiply success: j*i = -k", "Quaternion multiply failed: j*i != -k" );

	// Test axis/angle encoding
	{
		vector axis = Vector( 1.f, 2.f, 3.f, 0.f );
		Normalize( &axis, &axis );
		float angle = 0.3f;
		q = quaternion_fromAxisAngle( axis, angle );
		float angle_;
		vector axis_;
		quaternion_getAxisAngle( q, &axis_, &angle_ );
		test( f_eq( angle, angle_ ) && vector_equal( &axis, &axis_ ), "Quaternion axis/angle extracted same as created", "Quaternion axis/angle extracted different from when created." );
	}			{
		vector axis = Vector( 1.f, 0.f, 0.f, 0.f );
		Normalize( &axis, &axis );
		float angle = PI / 2.f;
		q = quaternion_fromAxisAngle( axis, angle );
		float angle_;
		vector axis_;
		quaternion_getAxisAngle( q, &axis_, &angle_ );
		test( f_eq( angle, angle_ ) && vector_equal( &axis, &axis_ ), "Quaternion axis/angle extracted same as created", "Quaternion axis/angle extracted different from when created." );
	}{
		vector axis = Vector( 0.f, 1.f, 0.f, 0.f );
		Normalize( &axis, &axis );
		float angle = PI / 2.f;
		q = quaternion_fromAxisAngle( axis, angle );
		float angle_;
		vector axis_;
		quaternion_getAxisAngle( q, &axis_, &angle_ );
		test( f_eq( angle, angle_ ) && vector_equal( &axis, &axis_ ), "Quaternion axis/angle extracted same as created", "Quaternion axis/angle extracted different from when created." );
	}{
		vector axis = Vector( 0.f, 0.f, 1.f, 0.f );
		Normalize( &axis, &axis );
		float angle = PI / 2.f;
		q = quaternion_fromAxisAngle( axis, angle );
		float angle_;
		vector axis_;
		quaternion_getAxisAngle( q, &axis_, &angle_ );
		test( f_eq( angle, angle_ ) && vector_equal( &axis, &axis_ ), "Quaternion axis/angle extracted same as created", "Quaternion axis/angle extracted different from when created." );
	}


	// Test rotation

	// Make a pi/2 (90deg) rotation around the x axis
	float angle = PI / 2;
	vector axis = Vector( 1.f, 0.f, 0.f, 0.f );
	quaternion rotation = quaternion_fromAxisAngle( axis, angle );

	// A point 1.f units down the Z axis
	vector point = Vector( 0.f, 0.f, 1.f, 0.f );

	vector point_ = quaternion_rotation( rotation, point );
	vector target_point = Vector( 0.f, -1.f, 0.f, 0.f );
	test( vector_equal( &point_, &target_point ), "Quaternion rotation success", "Quaternion rotation failed" );
	//vector_printf( "point_: ", &point_ );

	// More complex rotation
	angle = PI / 4;
	rotation = quaternion_fromAxisAngle( axis, angle );
	point_ = quaternion_rotation( rotation, point );
	//vector_printf( "point_: ", &point_ );

	// Test slerp
	q = quaternion_slerp( i, j, 0.f );
	test( quaternion_equal( q, i ), "Quaternion slerp with t = 0.f", "Quaternion slerp with t = 0.f" );

	q = quaternion_slerp( i, j, 1.f );
	test( quaternion_equal( q, j ), "Quaternion slerp with t = 1.f", "Quaternion slerp with t = 1.f" );

	q = quaternion_slerp( i, j, 0.5f );
	quaternion expected = Quaternion( 0.f, 0.7071f, 0.7071f, 0.f );
	test( quaternion_equal( q, expected ), "Quaternion slerp with t = 0.5f", "Quaternion slerp with t = 0.5f" );

	// Test quaternion_fromEuler
	//vAssert( 0 );
}
#endif // UNIT_TEST
