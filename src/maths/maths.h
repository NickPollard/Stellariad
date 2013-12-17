// Maths.h
#pragma once

#include "maths/mathstypes.h"
#include <float.h>
#include <math.h>

#define PI 3.1415926535897932

// *** Integer and Floating-point Maths
bool f_eq( float a, float b );

float fclamp( float a, float bottom, float top );
float fround( float value, float round );
float fsign( float f );
double fract( double d );
float fractf( float f );

int max( int a, int b );
int min( int a, int b );

int clamp( int a, int bottom, int top );
bool contains( int a, int min, int max );

float lerp( float a, float b, float factor );
float sinerp( float a, float b, float factor );
// map a point to the range from begin-end, returning a value from 0.f to 1.f
float map_range( float point, float begin, float end );

bool isPowerOf2( unsigned int n );

// *** Test
#ifdef UNIT_TEST
void test_maths();
#endif // UNIT_TEST

