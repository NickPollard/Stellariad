// Canyon_zone.c
#include "common.h"
#include "canyon_zone.h"
//-------------------------
#include "canyon.h"
#include "scene.h"
#include "maths/maths.h"
#include "mem/allocator.h"
#include "render/texture.h"
#include "script/lisp.h"

int canyon_current_zone = 0;
vector zone_sample_point;

// What Zone are we in at a given distance V down the canyon? This forever increases
int canyon_zone( float v ) {
	return (int)floorf( v / kZoneLength );
}

// What ZoneType are we in at a given distance V down the canyon? This is canyon_zone modulo zone count
// (so repeating zones of the same type have the same index)
int canyon_zoneType( canyon* c, float v ) {
	return canyon_zone( v ) % c->zone_count;
}

// How far through the current zone are we?
float canyonZone_distance( float v ) {
	return v - floorf( v / kZoneLength ) * kZoneLength;
}

// A float ratio for how strongly in the zone we are, use for zone terrain colouring
// See-saws back and forth between 1.f and 0.f to deal with our texture building
float canyonZone_terrainBlend( float v ) {
	int zone = canyon_zone( v );
	if (zone < 1 ) return 0.f;
	float this = (float)(zone % 2);
	float next = 1.f - this;
	return (zone < 1) ? 0.f : lerp( this, next, canyonZone_blend( v ));
}

// Returns a blend value from x to x + 1.f, of how much we should blend to the next zone
// Where x is the current zone index
float canyonZone_totalBlend( float v ) {
	float base = floorf( v / kZoneLength );
	float distance_remaining = kZoneLength - canyonZone_distance( v );
	float blend = 1.f - fclamp( distance_remaining / kZoneBlendDistance, 0.f, 1.f );
	return base + blend;
}

// Returns a blend value from 0.f to 1.f, of how much we should blend to the next zone
float canyonZone_blend( float v ) {
	float distance_remaining = kZoneLength - canyonZone_distance( v );
	float blend = 1.f - fclamp( distance_remaining / kZoneBlendDistance, 0.f, 1.f );
	return blend;
}

float canyonZone_progress( float v ) {
	return v / kZoneLength;
}

// What color is the standard terrain for ZONE?
vector canyonZone_terrainColor( canyon* c, int zone ) {
	return c->zones[zone % c->zone_count].terrain_color;
}

vector canyonZone_cliffColor( canyon* c, int zone ) {
	return c->zones[zone % c->zone_count].cliff_color;
}

vector canyonZone_edgeColor( canyon* c, int zone ) {
	return c->zones[zone % c->zone_count].edge_color;
}

vector canyonZone_terrainColorAtV( canyon* c, float v ) {
	return canyonZone_terrainColor( c, canyon_zone( v ));
}

vector canyonZone_cliffColorAtV( canyon* c, float v ) {
	return canyonZone_cliffColor( c, canyon_zone( v ));
}

vector canyonZone_edgeColorAtV( canyon* c, float v ) {
	return canyonZone_edgeColor( c, canyon_zone( v ));
}

texture* canyonZone_buildTexture( canyonZone a, canyonZone b ) {
	uint8_t* bitmap = mem_alloc( sizeof( uint8_t ) * kZoneTextureWidth * kZoneTextureHeight * kZoneTextureStride );
	for ( int y = 0; y < kZoneTextureHeight; ++y ) {
		for ( int x = 0; x < kZoneTextureWidth; ++x ) {
			int index = ( x + y * kZoneTextureWidth ) * kZoneTextureStride;
			float zone = (float)x / (float)kZoneTextureWidth;
			float cliff = (float)y / (float)kZoneTextureHeight;
			vector terrain_color = vector_lerp( &a.terrain_color, &b.terrain_color, zone );
			vector cliff_color = vector_lerp( &a.cliff_color, &b.cliff_color, zone );
			//vector edge_color = vector_lerp( &a.edge_color, &b.edge_color, zone );
			vector color = vector_lerp( &terrain_color, &cliff_color, cliff );
			bitmap[index + 0] = (uint8_t)( color.coord.x * 255.f );
			bitmap[index + 1] = (uint8_t)( color.coord.y * 255.f );
			bitmap[index + 2] = (uint8_t)( color.coord.z * 255.f );
			bitmap[index + 3] = 1.f;
		}
	}
	texture* t = texture_loadFromMem( kZoneTextureWidth, kZoneTextureHeight, kZoneTextureStride, bitmap );
	return t;
}

void canyonZone_skyFogBlend( canyonZone* a, canyonZone* b, float blend, vector* sky_color, vector* fog_color, vector* sun_color ) {
	*sky_color = vector_lerp( &a->sky_color, &b->sky_color, blend );
	*fog_color = vector_lerp( &a->fog_color, &b->fog_color, blend );
	*sun_color = vector_lerp( &a->sun_color, &b->sun_color, blend );
}

void canyonZone_tick( canyon* c, float dt ) {
	(void)dt;
	float u, v;
	canyonSpaceFromWorld( c, zone_sample_point.coord.x, zone_sample_point.coord.z, &u, &v );
	int zone = canyon_zone( v );
	if ( zone != c->current_zone ) {
		c->current_zone = zone;
	}
}

void canyonZone_load( canyon* c, const char* filename ) {
	context* lisp_context = lisp_newContext();
	term* t = lisp_eval_file( lisp_context, filename );
	// We should now have a list of zones
	term_takeRef( t );
	term* zone = t;
	int zone_count = list_length( t );
	//vAssert( zone_count < kNumZones );
	int i = 0;
	while ( zone ) { 
		c->zones[i] = *(canyonZone*)( head( zone )->data );
		zone = zone->tail;
		++i;
	}
	// TODO - add this back and get it working
	//term_deref( t );
	c->zone_count = zone_count;
}

canyonZone* canyonZone_create() {
	canyonZone* z = mem_alloc( sizeof( canyonZone ));
	memset( z, 0, sizeof( canyonZone ));
	return z;
}
