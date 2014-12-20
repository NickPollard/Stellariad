// vector.h
#pragma once

#include "maths/mathstypes.h"

extern const vector origin;

extern const vector x_axis;
extern const vector y_axis;
extern const vector z_axis;

extern const vector neg_x_axis;
extern const vector neg_y_axis;
extern const vector neg_z_axis;

extern const vector color_red;
extern const vector color_green;
extern const vector color_blue;
extern const vector color_black;

vector Vector(float x, float y, float z, float w);

void Set(vector* v, float x, float y, float z, float w);

void Add(vector* dst, const vector* srcA, const vector* srcB);
void Sub(vector *dst, const vector* srcA, const vector* srcB);
float Dot( const vector* A, const vector* B );
float Dot4( const vector* A, const vector* B );
void Cross(vector* dst, const vector* srcA, const vector* srcB);

vector vector_cross( vector a, vector b );
vector vector_add( vector a, vector b );
vector vector_sub( vector a, vector b );
vector vector_scaled( vector a, float f );
vector normalized( vector a );
float vector_lengthI( vector v );

// Normalise a vector
// No use of restrict; dst *can* alias src
void Normalize( vector* dst, const vector* src );
bool isNormalized( const vector* v );

void vector_scale( vector* dst, vector* src, float scale );
vector vector_lerp( const vector* from, const vector* to, float amount );
vector veclerp( const vector from, const vector to, float amount );

vector vector_mul( vector* a, vector* b );
vector vector_max( vector* a, vector* b );
vector vector_min( vector* a, vector* b );

float vector_length( const vector* v );
float vector_lengthSq( const vector* v );
float vector_distance( const vector* a, const vector* b );

bool vector_equal( const vector* a, const vector* b );

vector vector_fromQuaternion( quaternion q );
vector vector_rotateAxisAngle( vector v, vector axis, float angle );

// *** Output
void vector_print( const vector* v );
void vector_printf( const char* label, const vector* v );

#ifdef UNIT_TEST
void test_vector();
#endif // UNIT_TEST

vec2 Vec2( float x, float y );
uint32_t intFromVector( vector v );
