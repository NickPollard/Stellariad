// terrain.c
#include "common.h"
#include "terrain.h"
//-----------------------
#include "canyon.h"
#include "noise.h"
#include "canyon_terrain.h"

float terrain_mountainFunc( float x ) {
	return cosf( x - 0.5 * sinf( 2 * x ));
}

// Returns a value between 0.f and height_m
float terrain_mountainHeight( float x, float z, float u, float v ) {
	(void)v;
	const float mountain_gradient_scale = 0.00001f;
	const float base_mountain_scale = 2.f;
	const float offset = ( max( 0.0, fabsf( u ) - canyon_base_radius ));
	const float canyon_height_scale = offset * offset * mountain_gradient_scale + base_mountain_scale;

	const float scale_m_x = 40.f;
	const float scale_m_z = 40.f;
	const float mountain_height = 10.f * canyon_height_scale;
	return (1.0f + terrain_mountainFunc( x / scale_m_x ) * terrain_mountainFunc( z / scale_m_z )) * 0.5f * mountain_height;
}

float shelf( float n, float shelf, float force ) {
	float delta = fclamp( n - shelf, 0.f, force );
 	return n - delta;
}

float terrain_detailHeight( float u, float v ) {
	//return 0.f;
	(void)u;(void)v;
	float scale = 1.f;
	float amplitude = 16.f;
	return /*shelf( */amplitude * perlin( u * scale, v * scale ) +
			amplitude * 1.5f * perlin( u * scale * 0.7f, v * scale * 0.7f ) + 
				amplitude * 2.5f * perlin( u * scale * 0.13f, v * scale * 0.13f );/*, 
					20.f, 15.f );
					*/
}

float curvestep( float in, float step ) {
	float m = fmodf( in, step ) / step - 0.5f;
	float delta = tan( m * PI * 0.25f );
	return (floorf(in / step) + delta) * step;
}


// Sample the canyon height (Y) at a given world X and Z
float terrain_canyonHeight( float u, float v ) {
	(void)v;
	u = ( fmaxf( fabsf( u ) - canyon_base_radius, 0.f) ) * ( u > 0.f ? 1.f : -1.f );

	const float incline_scale = 0.0004f;
	const float flat_radius = 20.f;
	const float offset = fmaxf( 0.0, fabsf( u ) - canyon_base_radius ) - flat_radius;
   	const float incline = offset * offset * incline_scale;
	
	// Turn the canyon-space U into a height
	float mask = cos( fclamp( u / canyon_width, -PI/2.f, PI/2.f ));
	return (1.f - fclamp( powf( u / canyon_width, 4.f ), 0.f, 1.f )) * mask * canyon_height - incline;
}

// The procedural function
float canyonTerrain_sampleUV( float u, float v ) {
	float detail = terrain_detailHeight( u, v ) * 0.5f;
	float scale = 0.435f;
	float cliff = terrain_detailHeight( u * scale, v * scale );
	float canyon = terrain_canyonHeight( u, v );
	(void)canyon;(void)cliff;
	return detail - curvestep(cliff * 2.f, 50.f) - canyon;
}


