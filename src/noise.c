// noise.c
#include "common.h"
#include "noise.h"
//-----------------------
#include "maths/maths.h"

float noiseTexture[64][64];
static const int NoiseResolution = 64;

// *** Forward Declaration
float perlin_raw( float u, float v );

void noise_staticInit() {
#if 0
	for ( int x = 0; x < NoiseResolution; ++x )
		for ( int y = 0; y < NoiseResolution; ++y )
			noiseTexture[x][y] = perlin_raw(((float)x), ((float)y));
#else 
	memset( noiseTexture, 0, 64 * 64 * sizeof( float ));	
#endif
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

float noise2D_wrapped( float u, float v, float scale ) {
	return noise2D( fmodf( u, (float)NoiseResolution * scale), fmodf( v, (float)NoiseResolution * scale));
}

float noiseLookup( float u, float v ) {
	const int iu = (int)u;
	const int iv = (int)v;
	//const int uu = iu + NoiseResolution * 16;
//	const int vv = iv + NoiseResolution * 16;
	const int uu = (iu + (abs(iu) / NoiseResolution + 1) * NoiseResolution);
	const int vv = (iv + (abs(iv) / NoiseResolution + 1) * NoiseResolution);
	return noiseTexture[uu % NoiseResolution][vv % NoiseResolution];
}

float perlin( float u, float v ) {
	const float iu = floorf( u );
	const float iv = floorf( v );
	const float fu = fractf( u );
	const float fv = fractf( v );
	
	const float a = noiseLookup(iu, iv);
	const float b = noiseLookup(iu + 1.f, iv);
	const float c = noiseLookup(iu, iv + 1.f);
	const float d = noiseLookup(iu + 1.f, iv + 1.f);

	return sinerp( sinerp( a, b, fu ),
					sinerp( c, d, fu ),
					fv );
}

float perlin_octave( float u, float v, float scale ) {
	const float u_ = u / 32.f;
	const float v_ = v / 32.f;
	const float iu = floorf( u_ * scale );
	const float iv = floorf( v_ * scale );
	const float fu = fractf( u_ * scale );
	const float fv = fractf( v_ * scale );
	
	//printf( "u: %.2f, wrapped: %.2f\n", 2.f, fmodf( 2.f, 2.f ));
	const float s = (float)NoiseResolution * scale / 32.f;
	
	const float a = noise2D(fmodf(iu, s),fmodf(iv,s));
	const float b = noise2D(fmodf(iu + 1.f,s),fmodf(iv, s));
	const float c = noise2D(fmodf(iu,s),fmodf(iv + 1.f,s));
	const float d = noise2D(fmodf(iu + 1.f,s),fmodf(iv + 1.f,s));

	return sinerp( sinerp( a, b, fu ),
					sinerp( c, d, fu ),
					fv );
}

float perlin_raw( float u, float v ) {
	return 
		//perlin_octave( u, v, 1.f ) / 1.f;
		(perlin_octave( u, v, 32.f ) / 32.f +
		perlin_octave( u, v, 16.f ) / 16.f +
		perlin_octave( u, v, 8.f ) / 8.f +
		perlin_octave( u, v, 4.f ) / 4.f +
		perlin_octave( u, v, 2.f ) / 2.f +
		perlin_octave( u, v, 1.f ) / 1.f ) / 2.f;
}
