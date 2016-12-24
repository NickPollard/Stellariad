// canyon.h
#pragma once
#include "canyon_zone.h"
#include "maths/vector.h"

// The maximum canyon points loaded in the buffer at one time
#define MaxCanyonPoints 96
// How long (in world space units) each canyon segment is
#define CanyonSegmentLength 40.f
// How many segments behind the current position to keep in the buffer
#define TrailingCanyonSegments 30

struct canyon_s { 
	canyonZone  zones[kNumZones];
	int         zone_count;
	int         current_zone;
	vector      zone_sample_point;
	scene*      _scene;
	window_buffer* canyon_streaming_buffer;
};

struct canyonData_s {
	vector point;
	vector normal;
};

// TODO move these to canyon members
extern const float canyon_base_radius;
extern const float canyon_width;
extern const float canyon_height;

// Canyon functions
canyon* canyon_create( scene* s, const char* file );
void canyon_tick( void* canyon_data, float dt, engine* eng );
//void canyon_generateInitialPoints( canyon* c );
void canyon_staticInit(); // TODO - does this actually need to be static?
void canyon_seekForWorldPosition( canyon* c, vector position );
// Convert world-space X and Z coords into canyon space U and V
void canyonSpaceFromWorld( canyon* c, float x, float z, float* u, float* v );
// Convert canyon-space U and V coords into world space X and Z
void terrain_worldSpaceFromCanyon( canyon* c, float u, float v, float* x, float* z );

void canyonBuffer_generatePoints( canyon* c, size_t from, size_t to );
void canyonBuffer_seek( canyon* c, size_t seek_position );

// *** DEBUG
void terrain_debugDraw( canyon* c, window* w );

void canyon_test();
