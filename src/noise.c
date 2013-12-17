// noise.c
#include "common.h"
#include "noise.h"
//-----------------------
#include "maths/maths.h"

float noiseTexture[256][256];
static const int NoiseResolution = 256;

// *** Forward Declaration
float perlin_raw( float u, float v );

void noise_staticInit() {
	for ( int x = 0; x < NoiseResolution; ++x )
		for ( int y = 0; y < NoiseResolution; ++y )
			noiseTexture[x][y] = perlin_raw(((float)x) / 32.f, ((float)y) / 32.f);
}

double msin( double d ) {
	return (0.5 + 0.5 * sin(d));
}

float noise( float f ) {
	const double d = f;
	return fractf((float)sin(d * 43276.1432));
}

float noise2D( float u, float v ) {
	(void)v;
	return noise( u + 1000.f * v );
}

float noiseLookup( float u, float v ) {
	const int iu = (int)u;
	const int iv = (int)v;
	const int uu = (iu + (abs(iu) / NoiseResolution + 1) * NoiseResolution);
	const int vv = (iv + (abs(iv) / NoiseResolution + 1) * NoiseResolution);
	return noiseTexture[uu % NoiseResolution][vv % NoiseResolution];
}

float perlin( float u, float v ) {
	const float iu = floorf( u );
	const float iv = floorf( v );
	const float fu = fractf( u );
	const float fv = fractf( v );
	
	const float a = noiseLookup(iu,iv);
	const float b = noiseLookup(iu + 1.f,iv);
	const float c = noiseLookup(iu,iv + 1.f);
	const float d = noiseLookup(iu + 1.f,iv + 1.f);

	return sinerp( sinerp( a, b, fu ),
					sinerp( c, d, fu ),
					fv );
}

float perlin_octave( float u, float v ) {
	const float iu = floorf( u );
	const float iv = floorf( v );
	const float fu = fractf( u );
	const float fv = fractf( v );
	
	const float a = noise2D(iu,iv);
	const float b = noise2D(iu + 1.f,iv);
	const float c = noise2D(iu,iv + 1.f);
	const float d = noise2D(iu + 1.f,iv + 1.f);

	return sinerp( sinerp( a, b, fu ),
					sinerp( c, d, fu ),
					fv );
}

float perlin_raw( float u, float v ) {
	return 
		(perlin_octave( u * 32.f, v * 32.f ) / 32.f +
		perlin_octave( u * 16.f, v * 16.f ) / 16.f +
		perlin_octave( u * 8.f, v * 8.f ) / 8.f +
		perlin_octave( u * 4.f, v * 4.f ) / 4.f +
		perlin_octave( u * 2.f, v * 2.f ) / 2.f +
		perlin_octave( u * 1.f, v * 1.f ) / 1.f) / 2.f;
}
