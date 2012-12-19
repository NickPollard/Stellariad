// matrix.c
#include "common.h"
#include "matrix.h"
//----------------------
#include "test.h"
#include "maths/maths.h"
#include "maths/vector.h"
#include "maths/quaternion.h"

matrix matrix_identity =
{
	{ 1.f, 0.f, 0.f, 0.f },
	{ 0.f, 1.f, 0.f, 0.f },
	{ 0.f, 0.f, 1.f, 0.f },
	{ 0.f, 0.f, 0.f, 1.f }
};

// Matrix Vector multiply
vector matrix_vecMul( matrix m, const vector* v) {
	vector out;
	out.coord.x = m[0][0] * v->coord.x + 
					m[1][0] * v->coord.y + 
					m[2][0] * v->coord.z + 
					m[3][0] * v->coord.w;
	out.coord.y = m[0][1] * v->coord.x + 
					m[1][1] * v->coord.y + 
					m[2][1] * v->coord.z + 
					m[3][1] * v->coord.w;
	out.coord.z = m[0][2] * v->coord.x + 
					m[1][2] * v->coord.y + 
					m[2][2] * v->coord.z + 
					m[3][2] * v->coord.w;
	out.coord.w = m[0][3] * v->coord.x + 
					m[1][3] * v->coord.y + 
					m[2][3] * v->coord.z + 
					m[3][3] * v->coord.w;
	return out;
}

// Matrix multiply
/*
matrix matrixMul( const matrix a, const matrix b ) {
	matrix out;
	for (int m = 0; m < 4; m++ ) {
		for (int n = 0; n < 4; n++ ) {
			out.val[n][m] = a[0][m] * b[n][0] + 
							a[1][m] * b[n][1] + 
							a[2][m] * b[n][2] + 
							a[3][m] * b[n][3];
		}
	}
	return out;
}
*/

// Set a row in a matrix to a given vector
void matrix_setRow(matrix m, int row, const vector* v) {
	m[0][row] = v->val[0];
	m[1][row] = v->val[1];
	m[2][row] = v->val[2];
	m[3][row] = v->val[3]; }

// Set a column in a matrix to a given vector
void matrix_setColumn(matrix m, int col, const vector* v) {
	m[col][0] = v->val[0];
	m[col][1] = v->val[1];
	m[col][2] = v->val[2];
	m[col][3] = v->val[3];
}

// Set the translation component of a 4x4 matrix
void matrix_setTranslation(matrix m, const vector* v) {
//	matrix_setColumn(m, 3, v);
	m[3][0] = v->val[0];
	m[3][1] = v->val[1];
	m[3][2] = v->val[2];
}

void matrix_clearTranslation( matrix m ) {
	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
}

void matrix_setRotation( matrix dst, matrix src ) {
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			dst[i][j] = src[i][j];
		}
	}
}

// Get the translation component of a 4x4 matrix
const vector* matrix_getTranslation(matrix m) {
	return (vector*)m[3]; }

const vector* matrix_getCol( matrix m, int i ) {
	return (vector*)m[i]; }

float matrix_getTrace33( matrix m ) {
	return m[0][0] + m[1][1] + m[2][2];
}

quaternion matrix_getRotation( matrix m ) {
	quaternion q;

	float trace = matrix_getTrace33( m );

	/*

	// trace + 1 == 4w^2
	q.s = sqrt( trace + 1 ) / 2.f;
	q.x = sqrt( 1 + m[0][0] - m[1][1] - m[2][2] ) / 2.f;
	q.y = sqrt( 1 - m[0][0] + m[1][1] - m[2][2] ) / 2.f;
	q.z = sqrt( 1 - m[0][0] - m[1][1] + m[2][2] ) / 2.f;
	// Get the correct sign out of the off-diagonal parts of the matrix
	q.x *= fsign( m[1][2] - m[2][1] );
	q.y *= fsign( m[2][0] - m[0][2] );
	q.z *= fsign( m[0][1] - m[1][0] );
	*/

	if ( trace >= 0.f ) {
		float r = sqrt( 1 + trace );
		float s = 0.5f / r;
		q.s = 0.5f * r;
		q.x = ( m[1][2] - m[2][1] ) * s;
		q.y = ( m[2][0] - m[0][2] ) * s;
		q.z = ( m[0][1] - m[1][0] ) * s;
	}
	else if (( m[0][0] > m[1][1] ) && ( m[0][0] > m[2][2] )) {
		float r = sqrt( 1 + m[0][0] - m[1][1] - m[2][2] );
		float s = 0.5f / r;
		q.s = ( m[1][2] - m[2][1] ) * s;
		q.x = 0.5 * r;
		q.y = ( m[1][0] + m[0][1] ) * s;
		q.z = ( m[0][2] + m[2][0] ) * s;
	} 
	else if ( m[1][1] > m[2][2] ) {
		float r = sqrt( 1 - m[0][0] + m[1][1] - m[2][2] );
		float s = 0.5f / r;

		q.s = ( m[2][0] - m[0][2] ) * s;
		q.x = ( m[0][1] + m[1][0] ) * s;
		q.y = 0.5 * r;
		q.z = ( m[1][2] + m[2][1] ) * s;
	} 
	else {
		float r = sqrt( 1 - m[0][0] - m[1][1] + m[2][2] );
		float s = 0.5f / r;

		q.s = ( m[0][1] - m[1][0] ) * s;
		q.x = ( m[2][0] + m[0][2] ) * s;
		q.y = ( m[1][2] + m[2][1] ) * s;
		q.z = 0.5 * r;
	}

	return q;
}

// Initialise a matrix to the identity
void matrix_setIdentity(matrix m) {
	memset(m, 0, sizeof(matrix));
	m[0][0] = 1.f;
	m[1][1] = 1.f;
	m[2][2] = 1.f;
	m[3][3] = 1.f; }

bool matrix_equal( matrix a, matrix b ) {
	for (int i = 0; i < 16; i++)
		if ( !f_eq( ((float*)a)[i], ((float*)b)[i] ) )
			return false;
	return true;
}

// Find the determinant of a 4x4 matrix using Laplace Expansion
// This can probably be simplified further if I multiply out the 2x2 and 3x3 factors
float matrix_determinant( matrix src ) {
	// first calculate and cache the determinants of the base 2x2 matrices
	float (*m)[4] = (float(*)[4])src;
	float a = (m[0][2] * m[1][3]) - (m[1][2] * m[0][3]);
	float b = (m[1][2] * m[2][3]) - (m[2][2] * m[1][3]);
	float c = (m[2][2] * m[3][3]) - (m[3][2] * m[2][3]);
	float d = (m[0][2] * m[2][3]) - (m[2][2] * m[0][3]);
	float e = (m[1][2] * m[3][3]) - (m[3][2] * m[1][3]);
	float f = (m[0][2] * m[3][3]) - (m[3][2] * m[0][3]);

	// calculate the determinants of the 4 3x3 sub matrices
	// Each is made from the det of 3 2x2 matrices
	float det00 = ( 				  (m[1][1] * c) - (m[2][1] * e) + (m[3][1] * b) );
	float det01 = ( (m[0][1] * c) - 				  (m[2][1] * f) + (m[3][1] * d) );
	float det02 = ( (m[0][1] * e) - (m[1][1] * f) + 				  (m[3][1] * a) );
	float det03 = ( (m[0][1] * b) - (m[1][1] * d) + (m[2][1] * a)					);

	float det =   ( m[0][0] * det00 )
				- ( m[1][0] * det01 )
				+ ( m[2][0] * det02 )
				- ( m[3][0] * det03 );

	return det;
}

float matrix_determinantFromCofactors( matrix m, matrix cofactors ) {

	float det = ( m[0][0] * cofactors[0][0] ) +
				( m[1][0] * cofactors[1][0] ) +
				( m[2][0] * cofactors[2][0] ) +
				( m[3][0] * cofactors[3][0] );

	return det;
}



void matrix_transpose( matrix dst, matrix src ) {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = i; j < 4; j++ ) {
			dst[i][j] = src[j][i];
			dst[j][i] = src[i][j];
		}
	}
}

void test_create_matrix( matrix m ) {
	float k = 0.f;
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			m[i][j] = k;
			k += 1.f;
		}
	}
}

void test_matrix_transpose( ) {
	matrix dst, src;
	test_create_matrix( src );
	matrix_transpose( dst, src );
//	matrix_print( src );
//	matrix_print( dst );
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			assert( f_eq( dst[i][j], src[j][i] ));
		}
	}
}

void matrix_scalarMul( matrix dst, matrix src, float scalar ) {
	float* srcf = (float*)src;
	float* dstf = (float*)dst;
	for ( int i = 0; i < 16; i++ )
		dstf[i] = srcf[i] * scalar;
}

void test_matrix_scalarMul( ) {
	matrix dst, src;
	float scale = 0.5f;
	test_create_matrix( src );
	matrix_scalarMul( dst, src, scale );
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			assert( f_eq( dst[i][j], src[i][j]*scale ));
		}
	}
}

// Matrix inverse
// Calculate the matrix of cofactors
// Take the tranpose of this, which is the adjugate
// Calculate the determinant using Laplacian expansion from the cofactors
// the inverse is 1/det * adjugate
void matrix_inverse( matrix inverse, matrix src ) {
	// calulate the matrix of cofactors
	matrix cofactors;

	float bA = src[0][2]*src[1][3] - src[0][3]*src[1][2];
	float bB = src[1][2]*src[2][3] - src[1][3]*src[2][2];
	float bC = src[2][2]*src[3][3] - src[2][3]*src[3][2];
	float bD = src[0][2]*src[2][3] - src[0][3]*src[2][2];
	float bE = src[1][2]*src[3][3] - src[1][3]*src[3][2];
	float bF = src[0][2]*src[3][3] - src[0][3]*src[3][2];

	float tA = src[0][0]*src[1][1] - src[0][1]*src[1][0];
	float tB = src[1][0]*src[2][1] - src[1][1]*src[2][0];
	float tC = src[2][0]*src[3][1] - src[2][1]*src[3][0];
	float tD = src[0][0]*src[2][1] - src[0][1]*src[2][0];
	float tE = src[1][0]*src[3][1] - src[1][1]*src[3][0];
	float tF = src[0][0]*src[3][1] - src[0][1]*src[3][0];

	// Top Row
	cofactors[0][0] = src[1][1]*bC - src[2][1]*bE + src[3][1]*bB;
	cofactors[1][0] = -(src[0][1]*bC - src[2][1]*bF + src[3][1]*bD);
	cofactors[2][0] = src[0][1]*bE - src[1][1]*bF + src[3][1]*bA;
	cofactors[3][0] = -(src[0][1]*bB - src[2][1]*bD + src[2][1]*bA);
	// Second row
	cofactors[0][1] = -(src[1][0]*bC - src[2][0]*bE + src[3][0]*bB);
	cofactors[1][1] = src[0][0]*bC - src[2][0]*bF + src[3][0]*bD;
	cofactors[2][1] = -(src[0][0]*bE - src[1][0]*bF + src[3][0]*bA);
	cofactors[3][1] = src[0][0]*bB - src[2][0]*bD + src[2][0]*bA;

	// Third Row
	cofactors[0][2] = src[1][3]*tC - src[2][3]*tE + src[3][3]*tB;
	cofactors[1][2] = -(src[0][3]*tC - src[2][3]*tF + src[3][3]*tD);
	cofactors[2][2] = src[0][3]*tE - src[1][3]*tF + src[3][3]*tA;
	cofactors[3][2] = -(src[0][3]*tB - src[2][3]*tD + src[2][3]*tA);
	// Fourth row
	cofactors[0][3] = -(src[1][2]*tC - src[2][2]*tE + src[3][2]*tB);
	cofactors[1][3] = src[0][2]*tC - src[2][2]*tF + src[3][2]*tD;
	cofactors[2][3] = -(src[0][2]*tE - src[1][2]*tF + src[3][2]*tA);
	cofactors[3][3] = src[0][2]*tB - src[1][2]*tD + src[2][2]*tA;

	float other_det = + tA * bC
						- tD * bE
						+ tF * bB
						+ tB * bF
						- tE * bD
						+ tC * bA;

	/*
	float det = matrix_determinantFromCofactors( src, cofactors );
	printf( "det: %.6f, other_det: %.6f\n", det, other_det  );
	assert( f_eq( det, other_det ));
	*/

	//float invDet = 1.f / matrix_determinantFromCofactors( src, cofactors );
	float invDet = 1.f / other_det;

	matrix adjugate;
	matrix_transpose( adjugate, cofactors );
	matrix_scalarMul( inverse, adjugate, invDet );

#if 0
	vector a, b, c;
	a = Vector( 0.5f, 1.2f, 3.0f, 1.0 );
	b = matrix_vecMul( src, &a );
	c = matrix_vecMul( inverse, &b );

	printf( "det: %.6f\n", other_det );
	printf( "Inverse:\n" );
	matrix_print( src );
	matrix_print( inverse );

	/*
	printf( "Inverse testing vectors: " );
	vector_print( &a );
	printf( "\n" );
	vector_print( &c );
	printf( "\n" );
	*/
	assert( vector_equal( &a, &c ));
#endif
}

// Convert a V matrix to an OGL matrix
const GLfloat* matrix_getGlMatrix( matrix m ) {
	return (const GLfloat*)m; }

// Multiply two matrices together
void matrix_mul( matrix dst, matrix a, matrix b ) {
	matrix m;
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			m[i][j] = a[0][j] * b[i][0]
						+ a[1][j] * b[i][1]
						+ a[2][j] * b[i][2]
						+ a[3][j] * b[i][3]; }}
	matrix_cpy( dst, m );
}

// Copy one matrix to another
void matrix_cpy( matrix dst, matrix src ) { 
	/*
	float* a = (float*)dst;
	float* b = (float*)src;
	for (int i = 0; i < 16; i++) {
		*a++ = *b++; } }
		*/
	memcpy( dst, src, sizeof( matrix ));
}

// Normalize the axes to be unit axes
void matrix_normalize( matrix m ) {
	vector v;
	for ( int i = 0; i < 4; i++ ) {
	   	Normalize( &v, matrix_getCol( m, i ));
		matrix_setColumn( m, i, &v );
	}
}

// Create a rotation matrix representing a rotation of ANGLE radians about the X-axis (Pitch)
// Axes are aligned as if +Z is into the screen, +Y is up, +X is right
// A Positive pitch rotation means to pitch up, ie. as if to climb
void matrix_rotX( matrix dst, float angle ) {
	float sinTheta = sinf( angle );
	float cosTheta = cos( angle );
	matrix_setIdentity( dst );
	dst[1][1] = cosTheta;
	dst[1][2] = sinTheta;
	dst[2][1] = -sinTheta;
	dst[2][2] = cosTheta;
}

// Create a rotation matrix representing a rotation of ANGLE radians about the Y-axis (Yaw)
// Axes are aligned as if +Z is into the screen, +Y is up, +X is right
// A Positive Yaw rotation means to yaw left, ie. as if turning a corner to the left
void matrix_rotY( matrix dst, float angle ) {
	float sinTheta = sinf( angle );
	float cosTheta = cos( angle );
	matrix_setIdentity( dst );
	dst[0][0] = cosTheta;
	dst[0][2] = -sinTheta;
	dst[2][0] = sinTheta;
	dst[2][2] = cosTheta;
}

// Create a rotation matrix representing a rotation of ANGLE radians about the Z-axis (Roll)
// Axes are aligned as if +Z is into the screen, +Y is up, +X is right
// A Positive Roll rotation means to roll left, ie. as if banking to the left
void matrix_rotZ( matrix dst, float angle ) {
	float sinTheta = sinf( angle );
	float cosTheta = cos( angle );
	matrix_setIdentity( dst );
	dst[0][0] = cosTheta;
	dst[0][1] = sinTheta;
	dst[1][0] = -sinTheta;
	dst[1][1] = cosTheta;
}

void matrix_print( matrix src ) {
	printf( "{ %.6f, %.6f, %.6f, %.6f\n",	src[0][0], src[1][0], src[2][0], src[3][0] );
	printf( "  %.6f, %.6f, %.6f, %.6f\n",	src[0][1], src[1][1], src[2][1], src[3][1] );
	printf( "  %.6f, %.6f, %.6f, %.6f\n",	src[0][2], src[1][2], src[2][2], src[3][2] );
	printf( "  %.6f, %.6f, %.6f, %.6f }\n", src[0][3], src[1][3], src[2][3], src[3][3] );
}

void matrix_compose3( matrix m, matrix a, matrix b, matrix c ) {
	matrix_mul( m, b, c );
	matrix_mul( m, a, m );
}

// Build a matrix to convert to axis space - i.e. the axis becomes
// the new Z axis, and X and Y axes are perpendicular to axis (and
// each other, of course)
void matrix_toAxisSpace( matrix m, vector axis ) {
	// Check that axis is not equal to what we're crossing it with, as
	// this could give degenerate results
	matrix_setIdentity( m );
	vector x, y;
	if ( vector_equal( &x_axis, &axis )) {
#if 0
		 Cross( &x, &axis, &y_axis );
		 Cross( &y, &x, &axis );
#else
		 Cross( &x, &y_axis, &axis );
		 Cross( &y, &axis, &x );
#endif
	}
	else {
#if 0
		Cross( &y, &x_axis, &axis );
		Cross( &x, &axis, &y );
#else
		Cross( &y, &axis, &x_axis );
		Cross( &x, &y, &axis );
#endif
	}
	matrix_setRow( m, 0, &x );	
	matrix_setRow( m, 1, &y );	
	matrix_setRow( m, 2, &axis );	
}

// Create a rotation matrix M for a rotation ANGLE around an arbitrary AXIS
void matrix_fromAxisAngle( matrix m, vector axis, float angle ) {
	matrix_setIdentity( m );

	matrix to_axis_space, to_original_space, rotation;

	matrix_toAxisSpace( to_axis_space, axis );
	
	matrix_rotZ( rotation, angle );
	matrix_inverse( to_original_space, to_axis_space );

/*
	printf( "\nTo Normal Space:\n" );
	matrix_print( to_axis_space );
	printf( "\n" );
	printf( "Rotation matrix:\n" );
	matrix_print( rotation );
	printf( "\n" );
	printf( "Back to original space:\n" );
	matrix_print( to_original_space );
	printf( "\n" );
	*/

	// We compose the matrices into one operation that:
	//   Converts to axis space
	//	 Rotates around the Z-axis (which is the axis of rotation now)
	//	 Converts back to original space
	matrix_compose3( m, to_original_space, rotation, to_axis_space );
}

// Build a rotation matrix from given Euler Angles
void matrix_fromEuler( matrix m, vector* euler_angles ) {
	quaternion q = quaternion_fromEuler( euler_angles );
	matrix_fromQuaternion( m, q );
}

vector matrix_toEuler( matrix m ) {
	(void)m;
	vector line_of_nodes = vector_cross( *matrix_getCol( m, 2 ), *matrix_getCol( m, 0 ));
	float yaw = atan2( line_of_nodes.coord.x, line_of_nodes.coord.z );
	return Vector( yaw, 0.f, 0.f, 0.f );
}

// Quaternion->Matrix conversion, inspired by http://en.wikipedia.org/wiki/Rotation_matrix#Quaternion
// Can easily be derived through matrix representations of quaternion pre- and post- multiplication
// Works for any Quaternion, even non-unit or zero
void matrix_fromQuaternion( matrix m, quaternion q ) {
	float magnitude = q.s*q.s + q.x*q.x + q.y*q.y + q.z*q.z;
	float s = ( magnitude > 0.f ) ? 2.f/magnitude : 0.f;
	float X = q.x * s, Y = q.y * s, Z = q.z * s;
	// All possible combinations, precalc here for efficiency taking into account s;
	float wX = q.s*X, wY = q.s*Y, wZ = q.s*Z;
	float xX = q.x*X, xY = q.x*Y, xZ = q.x*Z;
	float yY = q.y*Y, yZ = q.y*Z, zZ = q.z*Z;
	
	matrix_setIdentity( m );

	m[0][0] = 1.f - (yY + zZ);
	m[0][1] = xY + wZ;
	m[0][2] = xZ - wY;
	
	m[1][0] = xY - wZ;
	m[1][1] = 1.f - (xX + zZ);
	m[1][2] = yZ + wX;

	m[2][0] = xZ + wY;
	m[2][1] = yZ - wX;	
	m[2][2] = 1.f - (xX + yY);
}

void matrix_fromRotationTranslation( matrix m, quaternion rotation, vector translation ) {
	matrix_fromQuaternion( m, rotation );
	matrix_setTranslation( m, &translation );
}

// Assume up is +y
void matrix_look( matrix m, vector forward ) {
	vector up = y_axis;
	vector right;
	Cross( &right, &up, &forward );
	matrix_setColumn( m, 0, &right );
	matrix_setColumn( m, 1, &up );
	matrix_setColumn( m, 2, &forward );
}

void matrix_copyRotation( matrix dst, matrix src ) {
	matrix_setColumn( dst, 0, matrix_getCol( src, 0 ));
	matrix_setColumn( dst, 1, matrix_getCol( src, 1 ));
	matrix_setColumn( dst, 2, matrix_getCol( src, 2 ));
}

#if UNIT_TEST
void test_matrix() {
	matrix a, b, c;
	matrix_setIdentity( a );
	matrix_setIdentity( b );
	vector v = Vector( 3.f, 4.f, 5.f, 1.f );
	matrix_setTranslation( a, &v );
	matrix_mul( c, a, b );
	assert( matrix_equal( c, a ));
	assert( !matrix_equal( c, b ));

	matrix_setIdentity( a );
	float det = matrix_determinant( a );
	//printf("TEST: Maths.c: identity det = %.2f\n", det );
	assert( f_eq( det, 1.f ));
	det = matrix_determinant( c );
	//printf("TEST: Maths.c: det = %.2f\n", det );
	assert( f_eq( det, 1.f ));

	test_matrix_transpose();
	test_matrix_scalarMul();

	// Test Inverse
	// Inverse of Identity should be identity 
	matrix_setIdentity( a );
	matrix_setIdentity( b );
	matrix_inverse( c, b );
	assert( matrix_equal( c, a ) );

	a[0][0] = 0.f;
	a[1][0] = 0.5f;
	a[0][1] = -2.f;
	a[1][1] = 0.f;
	matrix_inverse( c, a );
	//matrix_print( a );
	//matrix_print( c );
	assert( f_eq( c[1][0], -0.5f ) );

	v = Vector( 0.4f, 0.2f, -3.f, 1.f );
	vector v2 = matrix_vecMul( a, &v );
	v2 = matrix_vecMul( c, &v2 );
	assert( vector_equal( &v, &v2 ));

	vector vdot = Vector( 1.f, 0.f, 0.f, 0.f );
	vAssert( f_eq( Dot( &vdot, &vdot ), 1.f ) );
	vector vdot_2 = Vector( 0.f, 1.f, 1.f, 0.f );
	vAssert( f_eq( Dot( &vdot, &vdot_2 ), 0.f ) );

	vAssert( isPowerOf2( 4 ) );
	vAssert( isPowerOf2( 1024 ) );
	vAssert( isPowerOf2( 4096 ) );
	vAssert( !isPowerOf2( 5 ) );
	vAssert( !isPowerOf2( 1536 ) );

	vAssert( f_eq( fround( -0.6f, 1.f ), -1.f ));
	vAssert( f_eq( fround( -0.5f, 1.f ), 0.f ));
	vAssert( f_eq( fround( -1.3f, 1.f ),  -1.f ));

	// Test matrix_toAxisSpace
	{
		matrix x,y,z;
		matrix_toAxisSpace( x, x_axis );
		matrix_toAxisSpace( y, y_axis );
		matrix_toAxisSpace( z, z_axis );
		matrix expected_x = {{ 0.f, 0.f, 1.f, 0.f }, { 0.f, 1.f, 0.f, 0.f, }, { -1.f, 0.f, 0.f, 0.f, }, { 0.f, 0.f, 0.f, 1.f }};
		test( matrix_equal( x, expected_x ), "matrix_toAxisSpace produced correct matrix for X axis", "matrix_toAxisSpace produced incorrect matrix for X axis" );
		matrix expected_y = {{ 1.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f, }, { 0.f, -1.f, 0.f, 0.f, }, { 0.f, 0.f, 0.f, 1.f }};
		test( matrix_equal( y, expected_y ), "matrix_toAxisSpace produced correct matrix for Y axis", "matrix_toAxisSpace produced incorrect matrix for Y axis" );
		test( matrix_equal( z, matrix_identity ), "matrix_toAxisSpace produced correct matrix for Z axis", "matrix_toAxisSpace produced incorrect matrix for X axis" );
	}

	{
		matrix m;
		float angle = 1.f;
		vector axis = Vector( 1.f, 2.f, 3.f, 0.f );
		Normalize( &axis, &axis );
		quaternion q = quaternion_fromAxisAngle( axis, angle );
		matrix_fromQuaternion( m, q );
		quaternion q_ = matrix_getRotation( m );
		test( quaternion_equal( q, q_ ), "Quaternion rotation extracted correctly from matrix.", "Quaternion rotation extracted from matrix different from when created." );
	}

	{
		matrix m, m_;
		float angle = 1.f;
		vector axis = Vector( 1.f, 2.f, 3.f, 0.f );
		Normalize( &axis, &axis );
		quaternion q = quaternion_fromAxisAngle( axis, angle );
		matrix_fromQuaternion( m, q );
		matrix_fromAxisAngle( m_, axis, angle );
		test( matrix_equal( m, m_ ), "Constructing matrix from axis angle same as from quaternion made with axis angle", "Constructing matrix from axis angle different than when going via quaternion." );
	}

	{
		// Test matrix_fromAxisAngle()
		// Ensure that doing matrix_fromAxisAngle rotations for the cardinal axes
		// gives the same results as our matrix_rotN functions
		matrix rotationAxisAngle, rotationX, rotationY, rotationZ;
		matrix_fromAxisAngle( rotationAxisAngle, x_axis, PI/2 );
		matrix_rotX( rotationX, PI/2 );
		vector result_a = matrix_vecMul( rotationAxisAngle, &z_axis );
		vector result_b = matrix_vecMul ( rotationX, &z_axis );
		test( vector_equal( &result_a, &result_b ), "Matrix X axis rotation", "Matrix X axis rotation." );

		matrix_fromAxisAngle( rotationAxisAngle, y_axis, PI/2 );
		matrix_rotY( rotationY, PI/2 );
		result_a = matrix_vecMul( rotationAxisAngle, &x_axis );
		result_b = matrix_vecMul ( rotationY, &x_axis );
		test( vector_equal( &result_a, &result_b ), "Matrix Y axis rotation", "Matrix Y axis rotation." );

		matrix_fromAxisAngle( rotationAxisAngle, z_axis, PI/2 );
		matrix_rotZ( rotationZ, PI/2 );
		result_a = matrix_vecMul( rotationAxisAngle, &y_axis );
		result_b = matrix_vecMul ( rotationZ, &y_axis );
		test( vector_equal( &result_a, &result_b ), "Matrix Z axis rotation", "Matrix Z axis rotation." );
	}

	{
		float angle = PI / 2.f;
		quaternion q = quaternion_fromAxisAngle( x_axis, angle );
		vector v = Vector( 0.f, 1.f, 0.f, 0.f );
		vector v_ = quaternion_rotation( q, v );
		matrix m;
		matrix_fromQuaternion( m, q );
		vector v__ = matrix_vecMul( m, &v );
		test( vector_equal( &v_, &v__ ), "quaternion rotation = quaternion->matrix", "quaternion rotation != quaternion->matrix" );
	}
	{
		float angle = PI / 2.f;
		quaternion q = quaternion_fromAxisAngle( y_axis, angle );
		vector v = Vector( 0.f, 0.f, 1.f, 0.f );
		vector v_ = quaternion_rotation( q, v );
		matrix m;
		matrix_fromQuaternion( m, q );
		vector v__ = matrix_vecMul( m, &v );
		test( vector_equal( &v_, &v__ ), "quaternion rotation = quaternion->matrix", "quaternion rotation != quaternion->matrix" );
	}
	{
		float angle = PI / 2.f;
		quaternion q = quaternion_fromAxisAngle( z_axis, angle );
		vector v = Vector( 1.f, 0.f, 0.f, 0.f );
		vector v_ = quaternion_rotation( q, v );
		matrix m;
		matrix_fromQuaternion( m, q );
		vector v__ = matrix_vecMul( m, &v );
		test( vector_equal( &v_, &v__ ), "quaternion rotation = quaternion->matrix", "quaternion rotation != quaternion->matrix" );
	}

	// matrix_look
	{
		matrix m;
		matrix_look( m, z_axis );
		const vector* up = matrix_getCol( m, 1 );
		const vector* right = matrix_getCol( m, 0 );
		const vector* forward = matrix_getCol( m, 2 );
		test( vector_equal( up, &y_axis ), "matrix_look success", "matrix_look fail");
		test( vector_equal( right, &x_axis ), "matrix_look success", "matrix_look fail");
		test( vector_equal( forward, &z_axis ), "matrix_look success", "matrix_look fail");
	}

	// Test matrix_fromEuler();
	//vAssert( 0 );
}
#endif // UNIT_TEST
