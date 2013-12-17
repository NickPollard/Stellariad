// noise.c
#include "common.h"
#include "noise.h"
//-----------------------
#include "maths/maths.h"

double msin( double d ) {
	return (0.5 + 0.5 * sin(d));
}

float noise( float f ) {
	double d = f;
	return fractf((float)sin(d * 43276.1432));
}

float noise2D( float u, float v ) {
	(void)v;
	return noise( u + 1000.f * v );
}

float perlin_octave( float u, float v ) {
	float iu = floorf( u );
	float iv = floorf( v );
	float fu = fractf( u );
	float fv = fractf( v );
	
	float a = noise2D(iu,iv);
	float b = noise2D(iu + 1.f,iv);
	float c = noise2D(iu,iv + 1.f);
	float d = noise2D(iu + 1.f,iv + 1.f);

	return sinerp( sinerp( a, b, fu ),
					sinerp( c, d, fu ),
					fv );
}

float perlin( float u, float v ) {
	return 
		(perlin_octave( u * 32.f, v * 32.f ) / 32.f +
		perlin_octave( u * 16.f, v * 16.f ) / 16.f +
		perlin_octave( u * 8.f, v * 8.f ) / 8.f +
		perlin_octave( u * 4.f, v * 4.f ) / 4.f +
		perlin_octave( u * 2.f, v * 2.f ) / 2.f +
		perlin_octave( u * 1.f, v * 1.f ) / 1.f) / 2.f;
}
