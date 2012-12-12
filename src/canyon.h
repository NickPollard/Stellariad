// canyon.h
#pragma once
#include "canyon_zone.h"
#include "maths/vector.h"

// The maximum canyon points loaded in the buffer at one time
#define kMaxCanyonPoints 96
// How long (in world space units) each canyon segment is
#define kCanyonSegmentLength 40.f
// How many segments behind the current position to keep in the buffer
#define kTrailingCanyonSegments 30

typedef struct window_buffer_s {
	// This is the index of the window buffer that corresponds to the first element in the window
	size_t head;
	// This is the index of the window buffer that corresponds to the last element in the window
	// (most of the time this will be (head + size - 1) % size
	size_t tail;
	// This is the index of the underlying stream that corresponds to the first element in the window
	// (That is, elements[head])
	size_t stream_position;
	// The actual buffer
	void* elements;
	// How many elements there are
	size_t window_size;
} window_buffer;

struct canyon_s { 
	int			zone_count;
	canyonZone	zones[kNumZones];
	int			current_zone;

	texture*	canyonZone_lookup_texture;
	texture*	canyonZone_lookup_pending;

	vector		zone_sample_point;
	scene*		scene;
};

extern const float canyon_base_radius;
extern const float canyon_width;
extern const float canyon_height;

// Canyon functions
void terrain_debugDraw( window* w );
void canyon_generateInitialPoints();
float terrain_canyonHeight( float x, float z, float u, float v );
void canyon_staticInit();
void canyon_seekForWorldPosition( vector position );
// For colouring
void terrain_canyonSpaceFromWorld( float x, float z, float* u, float* v );

// Convert canyon-space U and V coords into world space X and Z
void terrain_worldSpaceFromCanyon( float u, float v, float* x, float* z );
void canyon_tick( void* canyon_data, float dt, engine* eng );
canyon* canyon_create();

