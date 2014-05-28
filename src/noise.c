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
	for ( int x = 0; x < NoiseResolution; ++x )
		for ( int y = 0; y < NoiseResolution; ++y )
			noiseTexture[x][y] = perlin_raw(((float)x), ((float)y));
}

double msin( double d ) {
	return (0.5 + 0.5 * sin(d));
}

float noise( float f ) {
	const double d = f;
	return fractf((float)sin(d * 43276.1432));
}

float noise2D( float u, float v ) {
	return noise( u + 1000.f * v );
}

float noise2D_wrapped( float u, float v, float scale ) {
	return noise2D( fmodf( u, (float)NoiseResolution * scale), fmodf( v, (float)NoiseResolution * scale));
}

float perlin( float u, float v ) {
	const int iu = (int)floorf( u );
	const int iv = (int)floorf( v );
	
	const int n = NoiseResolution;
	const int iuu = (n + iu % n) % n;
	const int ivv = (n + iv % n) % n;
	const int iuu_ = (n + (iu+1) % n) % n;
	const int ivv_ = (n + (iv+1) % n) % n;
	const float a = noiseTexture[ iuu  ][ ivv  ];
	const float b = noiseTexture[ iuu_ ][ ivv  ];
	const float c = noiseTexture[ iuu  ][ ivv_ ];
	const float d = noiseTexture[ iuu_ ][ ivv_ ];

	return sinerp( sinerp( a, b, fractf( u )),
					sinerp( c, d, fractf( u )),
						fractf( v ));
}

float perlin_octave( float u, float v, float scale ) {
	const float u_ = u / 32.f;
	const float v_ = v / 32.f;
	const float iu = floorf( u_ * scale );
	const float iv = floorf( v_ * scale );
	const float fu = fractf( u_ * scale );
	const float fv = fractf( v_ * scale );
	
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
		(perlin_octave( u, v, 32.f ) / 32.f +
		perlin_octave( u, v, 16.f ) / 16.f +
		perlin_octave( u, v, 8.f ) / 8.f +
		perlin_octave( u, v, 4.f ) / 4.f +
		perlin_octave( u, v, 2.f ) / 2.f +
		perlin_octave( u, v, 1.f ) / 1.f ) / 2.f;
}
