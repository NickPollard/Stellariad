// vector.c
#include "common.h"
#include "vector.h"
//----------------------
#include "maths/quaternion.h"
#include "maths/maths.h"

const vector origin = {{ 0.f, 0.f, 0.f, 1.f }};

const vector x_axis = {{ 1.f, 0.f, 0.f, 0.f }};
const vector y_axis = {{ 0.f, 1.f, 0.f, 0.f }};
const vector z_axis = {{ 0.f, 0.f, 1.f, 0.f }};

const vector neg_x_axis = {{ -1.f, 0.f, 0.f, 0.f }};
const vector neg_y_axis = {{ 0.f, -1.f, 0.f, 0.f }};
const vector neg_z_axis = {{ 0.f, 0.f, -1.f, 0.f }};

const vector color_red = {{ 1.f, 0.f, 0.f, 1.f }};
const vector color_green = {{ 0.f, 1.f, 0.f, 1.f }};
const vector color_blue = {{ 0.f, 0.f, 1.f, 1.f }};
const vector color_black = {{ 0.f, 0.f, 0.f, 1.f }};

vector Vector(float x, float y, float z, float w) {
	vector v;
	v.coord.x = x;
	v.coord.y = y;
	v.coord.z = z;
	v.coord.w = w;
	return v;
}

void Set(vector* v, float x, float y, float z, float w) {
	v->coord.x = x;
	v->coord.y = y;
	v->coord.z = z;
	v->coord.w = w;
}

// Vector Addition
void Add(vector* restrict out, const vector* restrict a, const vector* restrict b) {
	out->coord.x = a->coord.x + b->coord.x;
	out->coord.y = a->coord.y + b->coord.y;
	out->coord.z = a->coord.z + b->coord.z;
	out->coord.w = a->coord.w + b->coord.w;
}

// Vector &subtraction
void Sub(vector* restrict out, const vector* restrict a, const vector* restrict b) {
	//for (int i = 0; i < 4; i++)
		//dst->val[i] = srcA->val[i] - srcB->val[i];
	out->coord.x = a->coord.x - b->coord.x;
	out->coord.y = a->coord.y - b->coord.y;
	out->coord.z = a->coord.z - b->coord.z;
	out->coord.w = a->coord.w - b->coord.w;
}

// Vector dot product
float Dot( const vector* A, const vector* B ) {
	return (A->coord.x * B->coord.x + A->coord.y * B->coord.y + A->coord.z * B->coord.z);
}

float Dot4( const vector* A, const vector* B ) {
	return (A->coord.x * B->coord.x + A->coord.y * B->coord.y + A->coord.z * B->coord.z + A->coord.w * B->coord.w );
}

// Vector cross product
void Cross(vector* restrict dst, const vector* restrict srcA, const vector* restrict srcB) {
	vAssert( dst != srcA );
	vAssert( dst != srcB );
	dst->coord.x = (srcA->coord.y * srcB->coord.z) - (srcA->coord.z * srcB->coord.y);
	dst->coord.y = (srcA->coord.z * srcB->coord.x) - (srcA->coord.x * srcB->coord.z);
	dst->coord.z = (srcA->coord.x * srcB->coord.y) - (srcA->coord.y * srcB->coord.x);
	dst->coord.w = 0.f; // If we're getting the cross product, it's a vector not a point
}

vector vector_cross( vector a, vector b ) {
	vector c;
	Cross( &c, &a, &b );
	return c;
}

vector vector_add( vector a, vector b ) {
	vector r = {{ a.coord.x + b.coord.x,
						a.coord.y + b.coord.y,
						a.coord.z + b.coord.z,
						a.coord.w + b.coord.w }};
	return r;
}


vector vector_sub( vector a, vector b ) {
	vector r = {{ a.coord.x - b.coord.x,
						a.coord.y - b.coord.y,
						a.coord.z - b.coord.z,
						a.coord.w - b.coord.w }};
	return r;
}

vector normalized( vector v ) {
	float inv_mag = 1.f / vector_length( &v );
	vector n = {{ v.coord.x * inv_mag,
						v.coord.y * inv_mag,
						v.coord.z * inv_mag,
						v.coord.w }};
	return n;
}
vector vector_scaled( vector v, float scale ) {
	vector scaled = {{ v.coord.x * scale,
					v.coord.y * scale,
					v.coord.z * scale,
					v.coord.w * scale }};
	return scaled;
}

float vector_lengthI( vector v ) {
	float length = sqrt( v.coord.x * v.coord.x
		  			 	  + v.coord.y * v.coord.y + 
							v.coord.z * v.coord.z );
	return length;
}

void vector_scale( vector* dst, vector* src, float scale ) {
	dst->coord.x = src->coord.x * scale;
	dst->coord.y = src->coord.y * scale;
	dst->coord.z = src->coord.z * scale;
	dst->coord.w = src->coord.w;
}

float vector_lengthSq( const vector* v ) {
	float length_sq = v->coord.x * v->coord.x + v->coord.y * v->coord.y + v->coord.z * v->coord.z;
	return length_sq;
}

float vector_length( const vector* v ) {
	float length = sqrt( v->coord.x * v->coord.x + v->coord.y * v->coord.y + v->coord.z * v->coord.z );
	return length;
}

// Normalise a vector
// No use of restrict; dst *can* alias src
void Normalize( vector* dst, const vector* src ) {
	float length = vector_length( src );
	float inv_length = 1.f / length;
	dst->coord.x = src->coord.x * inv_length;
	dst->coord.y = src->coord.y * inv_length;
	dst->coord.z = src->coord.z * inv_length;
	dst->coord.w = src->coord.w; // Preserve the W coord? This seems right to me
}

bool isNormalized( const vector* v ) {
	return f_eq( 1.f, vector_length( v ));
}

vector vector_lerp( vector* from, vector* to, float amount ) {
	vector v;
	float inv = 1.f - amount;
	v.coord.x = from->coord.x * inv + to->coord.x * amount;
	v.coord.y = from->coord.y * inv + to->coord.y * amount;
	v.coord.z = from->coord.z * inv + to->coord.z * amount;
	v.coord.w = from->coord.w * inv + to->coord.w * amount;
	return v;
}

vector vector_mul( vector* a, vector* b ) {
	vector v;
	v.coord.x = a->coord.x * b->coord.x;
	v.coord.y = a->coord.y * b->coord.y;
	v.coord.z = a->coord.z * b->coord.z;
	v.coord.w = a->coord.w * b->coord.w;
	return v;
}


vector vector_max( vector* a, vector* b ) {
	vector m;
	m.coord.x = fmaxf( a->coord.x, b->coord.x );
	m.coord.y = fmaxf( a->coord.y, b->coord.y );
	m.coord.z = fmaxf( a->coord.z, b->coord.z );
	m.coord.w = fmaxf( a->coord.w, b->coord.w );
	return m;
}

vector vector_min( vector* a, vector* b ) {
	vector m;
	m.coord.x = fminf( a->coord.x, b->coord.x );
	m.coord.y = fminf( a->coord.y, b->coord.y );
	m.coord.z = fminf( a->coord.z, b->coord.z );
	m.coord.w = fminf( a->coord.w, b->coord.w );
	return m;
}

float vector_distance( const vector* a, const vector* b ) {
	vector displacement;
	Sub( &displacement, a, b );
	return vector_length( &displacement );
}

bool vector_equal( const vector* a, const vector* b ) {
	// TODO: could this be optimized by just calculating all four in parallel
	// and then doing an &&?
	for (int i = 0; i < 4; i++)
		if ( !f_eq( ((float*)a)[i], ((float*)b)[i] ) ) {
			return false;
		}
	return true;
}

vector vector_fromQuaternion( quaternion q ) {
	vector v;
	v.coord.x = q.x;
	v.coord.y = q.y;
	v.coord.z = q.z;
	v.coord.w = 0.f;
	return v;
}

vector vector_rotateAxisAngle( vector v, vector axis, float angle ) {
	quaternion q = quaternion_fromAxisAngle( axis, angle );
	return quaternion_rotation( q, v );
}

// *** Output
void vector_print( const vector* v ) {
	printf( "%.4f, %.4f, %.4f, %.4f", v->val[0], v->val[1], v->val[2], v->val[3] );
}

void vector_printf( const char* label, const vector* v ) {
	printf( "%s", label );
	vector_print( v );
	printf( "\n" );
}

#ifdef UNIT_TEST
void test_vector() {
	// Vector tests
}
#endif // UNIT_TEST
