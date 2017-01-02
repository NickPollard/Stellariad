// canyon.c
#include "common.h"
#include "canyon.h"
//-----------------------
#include "base/window_buffer.h"
#include "maths/geometry.h"
#include "maths/maths.h"
#include "maths/vector.h"
#include "render/debugdraw.h"
#include "render/render.h"
#include "terrain/cache.h"

// *** New Terrain Canyon

/*
   The new canyon is made from a quadratic Bezier-spline. We (randomly) generate piecewise linear segments
   and then make each contiguous trio into a quadratic Bezier curve. By splitting every segment in half, we
   can ensure that the control points for each consecutive quadratic curve lie upon the same line so that we
   do not have a discontinuity in the first derivative - so the canyon will be smooth.
   
   ***
   
   To deal with procedural generation of a section of an infinite canyon, we will have a sliding window
   of points/segments loaded at any one time. This window reflects just part of the underlying infinite
   stream of points.

   It is planned that this window will only ever move forwards, not backwards, though this could change

   The window is implemented as a ring buffer, so that the first element will not always be at index 0,
   but at index M, and the last element will be (M + Size - 1) % Size
   */

const float canyon_base_radius = 40.f;
const float canyon_width = 20.f;
const float canyon_height = 40.f;

long int initial_seed = 0x0;

// *** Forward declarations
vector terrain_newCanyonPoint( canyon* c, vector current, vector previous );
void canyon_generateInitialPoints( canyon* c );
// ***


void canyon_seedRandom( canyon* c, long int seed ) { deterministic_seedRandSeq( seed, &c->randomSeed ); }

// Returns a point from the buffer that corresponds to absolute stream position STREAM_INDEX
static inline vector canyon_point( window_buffer* buffer, size_t stream_index ) {
	size_t mapped_index = windowBuffer_mappedPosition( buffer, stream_index );
	return canyonBuffer_point( buffer, mapped_index );
}

static inline vector cached_point_normal( window_buffer* buffer, size_t stream_index ) {
	size_t mapped_index = windowBuffer_mappedPosition( buffer, stream_index );
	return canyonBuffer_normal( buffer, mapped_index );
}

vector segmentNormal( vector from, vector to ) {
	vector segment = vector_sub( to, from );
	return normalized(Vector( -segment.coord.z, 0.f, segment.coord.x, 0.f ));
}

// Idempotently generate points to fill up empty space in the buffer
void canyonBuffer_generatePoints( canyon* c, size_t from, size_t to ) {
	window_buffer* b = c->canyon_streaming_buffer;
	vector before = canyonBuffer_point( b, windowBuffer_index( b, b->tail - 1 ));
	vector current = canyonBuffer_point( b, windowBuffer_index( b, b->tail ));
	while ( from < to ) {
		size_t i = windowBuffer_mappedPosition( b, from );
		vector next = terrain_newCanyonPoint( c, current, before );
		vector normal = normalized( vector_add( segmentNormal( before, current), segmentNormal( current, next )));

		before = current;
		current = next;
		canyonData data = { next, normal };
		canyonBuffer_write( b, i, sizeof( canyonData ), &data );
		++from;
	}
	b->tail = ( b->head + b->window_size - 1 ) % b->window_size;
}

// Seek the canyon window_buffer forward so that its stream_position is now SEEK_POSITION
void canyonBuffer_seekForward( canyon* c, size_t seek_position ) {
	window_buffer* b = c->canyon_streaming_buffer;
	vAssert( seek_position >= b->stream_position );
	size_t stream_end_position = windowBuffer_endPosition( b );
	while ( stream_end_position < seek_position ) {
		// The seek target is not in the window so we flush everything and need to generate
		// enough points to reach the seek_target before doing final canyonBuffer_generatePoints
		size_t new_head = windowBuffer_index( b, b->tail - 2 );
		b->stream_position = windowBuffer_streamPosition( b, new_head );
		b->head = new_head;
		size_t from = windowBuffer_streamPosition( b, b->tail ) + 1;
		size_t to = b->stream_position + b->window_size;
		canyonBuffer_generatePoints( c, from, to);
		stream_end_position = windowBuffer_endPosition( b );
	}

	// The seek target is already in the window
	b->head = windowBuffer_mappedPosition( b, seek_position );
	b->stream_position = seek_position;

	// Update all data
	size_t from = windowBuffer_streamPosition( b, b->tail ) + 1;
	size_t to = b->stream_position + b->window_size;
	canyonBuffer_generatePoints( c, from, to );
}

// Seek the canyon window_buffer so that its stream_position is now SEEK_POSITION - can seek forwards or backwards
void canyonBuffer_seek( canyon* c, size_t seek_position ) {
	window_buffer* b = c->canyon_streaming_buffer;
	if ( seek_position < b->stream_position ) {
		b->head = b->tail = b->stream_position = 0;
		canyon_generateInitialPoints( c );
	}
	canyonBuffer_seekForward( c, seek_position );
}




// *** Canyon Geometry

float canyon_v( int segment_index, float segment_prog ) {
	return ( segment_prog + (float)segment_index ) * CanyonSegmentLength;
}

// Estimate the closest z point, based on an even distribution of Zs
int canyon_estimatePointForZ( window_buffer* buffer, float z ) {
	float min_z = canyonBuffer_point( buffer, buffer->head ).coord.z;
	float max_z = canyonBuffer_point( buffer, buffer->tail ).coord.z;
	return (int)( fclamp(( z - min_z ) / ( max_z - min_z ), 0.f, 1.f ) * (float)buffer->window_size ) + buffer->stream_position;
}

int canyon_findStartZ( canyon* c, int default_start, float z, float closest_d, int closest_i ) {
	// Then walk backwards until the earliest possible Z
	float earliest_z = z - sqrt( closest_d );
	int start = closest_i;
	float current_z = canyon_point( c->canyon_streaming_buffer, start ).coord.z;
	// TODO - just binary search this?
	while ( current_z > earliest_z && start > default_start ) {
		int min_delta = (int)fmax( 1.f, ( current_z - earliest_z ) / CanyonSegmentLength );
		start = max( start - min_delta, default_start );
		vAssert( start >= default_start );
		current_z = canyon_point( c->canyon_streaming_buffer, start ).coord.z;
	}
	return start;
}

// Convert world-space X and Z coords into canyon space U and V
int canyon_findClosestPoint( canyon* c, float x, float z ) {
	vector point = Vector( x, 0.f, z, 1.f );
	int closest_i = 0;	// Closest point - in absolute stream position
	float closest_d = FLT_MAX;
	// We need to find the closest segment
	int default_start = c->canyon_streaming_buffer->stream_position + 1;
	int default_end = c->canyon_streaming_buffer->stream_position + c->canyon_streaming_buffer->window_size;

	// Estimate the closest z point, based on an even distribution of Zs
	// initialize the closest distance from that
	closest_i = max( c->canyon_streaming_buffer->stream_position + 1, canyon_estimatePointForZ( c->canyon_streaming_buffer, z ));
	vector closest_point = canyon_point( c->canyon_streaming_buffer, closest_i );
	vector displacement = vector_sub( point, closest_point );
	closest_d = vector_lengthSq( &displacement );

	// Then walk backwards until the earliest possible Z
	int start = canyon_findStartZ( c, default_start, z, closest_d, closest_i );
	int end = default_end;

	// Iterate from there
	for ( size_t i = (size_t)start; i + 1 < (size_t)end; ++i ) {
		vector test_point = canyon_point( c->canyon_streaming_buffer, i );
		
		// We know that Z values are always increasing as we force canyon segments in the forward arc
		// If the distance from Z-component alone is greater than closest, we can break out of the loop
		// as all later points must be at least that far away
		// * Use fabsf on one argument to preserve the sign of the square
		if ( fabsf(test_point.coord.z - point.coord.z) * (test_point.coord.z - point.coord.z) > closest_d ) {
			//printf( "Finishing canyon point test at index %d, skipping %d, total %d.\n", (int)i, (int)(end - i), (int)(end - start));
			break;
		}

		// TODO - do we skip everything behind or have we already done that?
		vector displacement = vector_sub( point, test_point );
		float d = vector_lengthSq( &displacement );
		if ( d < closest_d ) {
			closest_d = d;
			closest_i = i;
		}
	}
	return closest_i;
}

void canyonSpaceFromWorld( canyon* c, float x, float z, float* u, float* v ) {
	int closest_i = canyon_findClosestPoint( c, x, z );
	vector point = Vector( x, 0.f, z, 1.f );

	// find closest points on the two segments using that point
	vector closest_a, closest_b;
	float seg_pos_a = segment_closestPoint( canyon_point( c->canyon_streaming_buffer, closest_i ), canyon_point( c->canyon_streaming_buffer, closest_i+1 ), point, &closest_a );
	float seg_pos_b = segment_closestPoint( canyon_point( c->canyon_streaming_buffer, closest_i-1 ), canyon_point( c->canyon_streaming_buffer, closest_i ), point, &closest_b );
	// use the closest
	float length_a = vector_lengthI( vector_sub( point, closest_a ));
	float length_b = vector_lengthI( vector_sub( point, closest_b ));
	float sign = vector_sub( point, closest_a ).coord.x < 0.f ? 1.f : -1.f;
	*u = length_a * sign;
	*v = ( length_a < length_b ) ? canyon_v( closest_i, seg_pos_a ) : canyon_v( closest_i - 1, seg_pos_b );
}

int terrainCanyon_segmentAtDistance( float v ) {
	return (float)floorf(v / CanyonSegmentLength);
}

// Convert canyon-space U and V coords into world space X and Z
void terrain_worldSpaceFromCanyon( canyon* c, float u, float v, float* x, float* z ) {
	vmutex_lock( &terrainMutex ); {
	int i = max(0, terrainCanyon_segmentAtDistance( v ));
	float segment_position = ( v - (float)i * CanyonSegmentLength ) / CanyonSegmentLength; 
	
	// For this segment
	vector canyon_next = canyon_point( c->canyon_streaming_buffer, i + 1 );
	// TODO - pull down points and normals together for i -> i+1
	vector canyon_previous = canyon_point( c->canyon_streaming_buffer, i );
	vector canyon_position = vector_lerp( &canyon_previous, &canyon_next, segment_position );

	/* We use a warped grid system, where near the canyon the U-axis lines run perpendicular to the canyon
	   but further away (more than max_perpendicular_u) they run parallel to the X axis.
	   This is to prevent overlapping artifacts in the terrain generation.
	   We actually lerp the perpendicular U-axis lines to blend from the previous and next segements
	   to prevent overlapping.  */

	// Begin and end vectors for tweening the normal
	vector normal_begin = cached_point_normal( c->canyon_streaming_buffer, i + 0); // Incremented again because point normals trail behind now
	vector normal_end = cached_point_normal( c->canyon_streaming_buffer, i + 1 );

	const float max_perpendicular_u = 100.f;
	// TODO - Try pointer versions?
	//vector target_begin = vector_add( canyon_previous, vector_scaled( normal_begin, max_perpendicular_u ));
	//vector target_end = vector_add( canyon_next, vector_scaled( normal_end, max_perpendicular_u ));
	vector target_begin, target_end;
	vector a = vector_scaled( normal_begin, max_perpendicular_u );
   	Add( &target_begin, &canyon_previous, &a );
	vector b = vector_scaled( normal_end, max_perpendicular_u );
	Add( &target_end, &canyon_next, &b );
	vector target = vector_lerp( &target_begin, &target_end, segment_position );
	vector sampled_normal = normalized( vector_sub( target, canyon_position ));
	




	// What am I doing here? Ah - it's because after so far out (max_perp) we switch back to U-as-X axis)
	float perpendicular_u = fclamp( u, -max_perpendicular_u, max_perpendicular_u );
	vector u_offset = vector_scaled( sampled_normal, perpendicular_u );
	//vector flat_u_offset = Vector( perpendicular_u - u, 0.f, 0.f, 0.f );
	u_offset.coord.x += perpendicular_u - u;
	vector position = vector_add( canyon_position, u_offset);
	*x = position.coord.x;
	*z = position.coord.z; 
	} vmutex_unlock( &terrainMutex );
}

vector terrain_newCanyonPoint( canyon* c, vector current, vector previous ) {
	float previous_angle = acosf( normalized(vector_sub( current, previous )).coord.x );
	const float max_angle_delta = PI / 8.f;
	float min_angle = fmaxf( PI * 0.25f, previous_angle - max_angle_delta );
	float max_angle = fminf( PI * 0.75f, previous_angle + max_angle_delta );
	float angle = deterministic_frand( &c->randomSeed, min_angle, max_angle );
	vector offset = Vector( cosf( angle ) * CanyonSegmentLength, 0.f, sinf( angle ) * CanyonSegmentLength, 0.f );
	return vector_add( current, offset );
}

// Generate n points of the canyon
void canyon_generateInitialPoints( canyon* c ) {
	canyon_seedRandom( c, initial_seed );
	window_buffer* b = c->canyon_streaming_buffer;
	int initial_straight_segments = 2;
	for ( int i = 0; i < initial_straight_segments + 1; ++i ) {
		canyonData d;
		d.point = Vector( 0.f, 0.f, i * CanyonSegmentLength, 1.f );
		d.normal = Vector( -1.f, 0.f, 0.f, 1.f );
		canyonBuffer_write( b, i, sizeof( canyonData ), &d );
	}
	b->tail = initial_straight_segments;
}

void canyon_seekForWorldPosition( canyon* c, vector position ) {
	float u, v;
	canyonSpaceFromWorld( c, position.coord.x, position.coord.z, &u, &v );
	size_t seek_position = max( 0, terrainCanyon_segmentAtDistance( v ) - TrailingCanyonSegments );
	canyonBuffer_seek( c, seek_position );
}

void canyon_tickZone( canyon* c, float v, int zone ) {
	int current = zone % c->zone_count;
	int next = ( zone + 1 ) % c->zone_count;
	canyonZone* a = &c->zones[current];
	canyonZone* b = &c->zones[next];
	vector sky_color, fog_color, sun_color;
	canyonZone_skyFogBlend( a, b, canyonZone_blend( v ), &sky_color, &fog_color, &sun_color );
	scene_setFogColor( c->_scene, &fog_color );
	scene_setSkyColor( c->_scene, &sky_color );
	scene_setSunColor( c->_scene, &sun_color );
}

void canyon_tick( void* canyon_data, float dt, engine* eng ) {
	(void)eng;
	canyon* c = (canyon*)canyon_data;
	float u, v;
	canyonSpaceFromWorld( c, zone_sample_point.coord.x, zone_sample_point.coord.z, &u, &v );
	canyon_tickZone( c, v, canyon_zone( v )); // TODO - Huh??
	canyonZone_tick( c, dt );
}

// Sample the data at this point - will memcache
/*
canyonData* canyon_sample( canyon* c, int index ) {
	void* data = window_bufferSample( c->canyon_streaming_buffer, index );
	return (canyonData*)data;
}
*/

canyon* canyon_create( scene* s, const char* file ) {
	canyon* c = (canyon*)mem_alloc( sizeof( canyon ));
	memset( c, 0, sizeof( canyon ));
	c->_scene = s;
	canyonZone_load( c, file );
	c->canyon_streaming_buffer = window_bufferCreate( sizeof( canyonData ), MaxCanyonPoints );
	canyon_generateInitialPoints( c );
	canyonBuffer_seek( c, 0 );
	return c;
}

void canyon_test() {
	canyon* c = canyon_create( NULL, "dat/script/lisp/canyon_zones.s");
	canyon_seekForWorldPosition(c, Vector( 0.f, 0.f, 0.f, 1.f ));
	canyon_seekForWorldPosition(c, Vector( 0.f, 0.f, 100.f, 1.f ));
	canyon_seekForWorldPosition(c, Vector( 0.f, 0.f, 200.f, 1.f ));
	canyon_seekForWorldPosition(c, Vector( 0.f, 0.f, 300.f, 1.f ));
	canyon_seekForWorldPosition(c, Vector( 0.f, 0.f, 0.f, 1.f ));
	printf( "Seek Successful\n" );
	vAssert( 0 );
}



// *** DEBUG
vector  terrainSegment_toScreen( window* w, vector world ) {
	const float visible_width = 4000.f;
	const float visible_length = 4000.f;
	const float x_pos = 400.f;
	const float y_pos = 50.f;
	vector screen = Vector( x_pos + ( world.coord.x / visible_width ) * w->width,
		   	y_pos + ( world.coord.z / visible_length ) * w->height,
		   	0.f,
		   	1.f );
	return screen;
}

void terrain_drawSegment( window* w, vector a, vector b ) {
	vector a_2d = terrainSegment_toScreen( w, a );
	vector b_2d = terrainSegment_toScreen( w, b );
	debugdraw_line2d( a_2d, b_2d, color_green );
}

// Draw debug lines on the screen showing the path of the canyon we have generated
void terrain_debugDraw( canyon* c, window* w ) {
	int total_points = MaxCanyonPoints;

	for ( int i = 0; i+1 < total_points; ++i ) {
		// draw line segment from (i) to (i+1)
		terrain_drawSegment( w,
				canyon_point( c->canyon_streaming_buffer, c->canyon_streaming_buffer->head + i ),
			   	canyon_point( c->canyon_streaming_buffer, c->canyon_streaming_buffer->head + i + 1 ));
	}
}

void canyon_debugPrint( canyon* c ) {
	for ( int i = 0; i+1 < MaxCanyonPoints; ++i ) {
		// draw line segment from (i) to (i+1)
		vector v = canyon_point( c->canyon_streaming_buffer, c->canyon_streaming_buffer->head + i );
		//vector v = *(c->canyon_streaming_buffer->head + i)
		printf( "%d : %.2f, %.2f\n", i, v.coord.x, v.coord.z );
	}
}
