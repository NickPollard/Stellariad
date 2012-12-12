// canyon.c
#include "common.h"
#include "canyon.h"
//-----------------------
#include "vtime.h"
#include "maths/geometry.h"
#include "maths/maths.h"
#include "maths/vector.h"
#include "render/debugdraw.h"
#include "render/render.h"

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

// *** Forward declarations
vector terrain_newCanyonPoint( vector current, vector previous );

window_buffer canyon_streaming_buffer;
vector	canyon_points[kMaxCanyonPoints];

randSeq canyon_random_seed;
long int initial_seed = 0x0;

void canyonBuffer_init( window_buffer* buffer, size_t size, void* elements ) {
	buffer->head = 0;
	buffer->tail = 0;
	buffer->stream_position = 0;
	buffer->window_size = size;
	buffer->elements = elements;
}

void canyon_seedRandom( long int seed ) {
	deterministic_seedRandSeq( seed, &canyon_random_seed );
}

void canyon_staticInit() {
	canyonBuffer_init( &canyon_streaming_buffer, kMaxCanyonPoints, &canyon_points );
	canyon_seedRandom( initial_seed );
}

// Return the last stream position currently mapped in the window
size_t windowBuffer_endPosition( window_buffer* w ) {
	return w->stream_position + w->window_size - 1;
}

// Find where in the window_buffer, the underly stream POSITION is mapped
static inline size_t windowBuffer_mappedPosition( window_buffer* w, size_t position ) {
	return ( position - w->stream_position + w->head ) % w->window_size;
}

// Find where in the underlying stream, the window buffer POSITION is from
size_t windowBuffer_streamPosition( window_buffer* w, size_t position ) {
	return ( position - w->head + w->window_size ) % w->window_size + w->stream_position;
}

size_t windowBuffer_index( window_buffer* w, size_t index ) {
	return ( index + w->window_size ) % w->window_size;
}

void canyonBuffer_setValue( window_buffer* buffer, int i, vector value ) {
	vector* values = buffer->elements;
	values[i] = value;
}

static inline vector canyonBuffer_value( window_buffer* buffer, int i ) {
	vector* values = buffer->elements;
	return values[i];
}

// Returns a point from the buffer that corresponds to absolute stream position STREAM_INDEX
static inline vector canyon_point( window_buffer* buffer, size_t stream_index ) {
	size_t mapped_index = windowBuffer_mappedPosition( buffer, stream_index );
	return canyonBuffer_value( buffer, mapped_index );
}

// Generate points to fill up empty space in the buffer
void canyonBuffer_generatePoints( window_buffer* buffer ) {
	vector before = canyonBuffer_value( buffer, windowBuffer_index( buffer, buffer->tail - 1 ));
	vector current = canyonBuffer_value( buffer, windowBuffer_index( buffer, buffer->tail ));
	size_t next_stream_position = windowBuffer_streamPosition( buffer, buffer->tail ) + 1;
	size_t end_position = buffer->stream_position + buffer->window_size;
	while ( next_stream_position < end_position ) {
		//printf( "Next stream: " dPTRf ", end stream: " dPTRf ".\n", next_stream_position, end_position );
		size_t i = windowBuffer_mappedPosition( buffer, next_stream_position );
		vector next = terrain_newCanyonPoint( current, before );
		before = current;
		current = next;
		canyonBuffer_setValue( buffer, i, next );
		++next_stream_position;
	}
	buffer->tail = ( buffer->head + buffer->window_size - 1 ) % buffer->window_size;
}

// Seek the canyon window_buffer forward so that its stream_position is now SEEK_POSITION
void canyonBuffer_seekForward( window_buffer* buffer, size_t seek_position ) {
	vAssert( seek_position >= buffer->stream_position );
	size_t stream_end_position = windowBuffer_endPosition( buffer );
	while ( stream_end_position < seek_position ) {
		// The seek target is not in the window so we flush everything and need to generate
		// enough points to reach the seek_target before doing final canyonBuffer_generatePoints
		size_t new_head = windowBuffer_index( buffer, buffer->tail - 2 );
		buffer->stream_position = windowBuffer_streamPosition( buffer, new_head );
		buffer->head = new_head;
		canyonBuffer_generatePoints( buffer );
		stream_end_position = windowBuffer_endPosition( buffer );
	}

	// The seek target is already in the window
	buffer->head = windowBuffer_mappedPosition( buffer, seek_position );
	buffer->stream_position = seek_position;

	// Update all data
	canyonBuffer_generatePoints( buffer );
}

// Seek the canyon window_buffer so that its stream_position is now SEEK_POSITION - can seek forwards or backwards
void canyonBuffer_seek( window_buffer* buffer, size_t seek_position ) {
	if ( seek_position >= buffer->stream_position ) {
		canyonBuffer_seekForward( buffer, seek_position );
	}
	else {
		// go back to the beginning and seek forward from there
		buffer->head = 0;
		buffer->tail = 0;
		buffer->stream_position = 0;
		canyon_generateInitialPoints();
		canyonBuffer_seekForward( buffer, seek_position );
	}
}




// *** Canyon Geometry

float canyon_v( int segment_index, float segment_prog ) {
	return ( segment_prog + (float)segment_index ) * kCanyonSegmentLength;
}

// Estimate the closest z point, based on an even distribution of Zs
int canyon_estimatePointForZ( window_buffer* buffer, float z ) {
	float min_z = canyonBuffer_value( buffer, buffer->head ).coord.z;
	float max_z = canyonBuffer_value( buffer, buffer->tail ).coord.z;
	return (int)( fclamp(( z - min_z ) / ( max_z - min_z ), 0.f, 1.f ) * (float)buffer->window_size ) + buffer->stream_position;
}

int canyon_findStartZ( int default_start, float z, float closest_d, int closest_i ) {
	// Then walk backwards until the earliest possible Z
	float earliest_z = z - sqrt( closest_d );
	int start = closest_i;
	float current_z = canyon_point( &canyon_streaming_buffer, start ).coord.z;
	// TODO - just binary search this?
	while ( current_z > earliest_z && start > default_start ) {
		int min_delta = (int)fmax( 1.f, ( current_z - earliest_z ) / kCanyonSegmentLength );
		start = max( start - min_delta, default_start );
		vAssert( start >= default_start );
		current_z = canyon_point( &canyon_streaming_buffer, start ).coord.z;
	}
	return start;
}

// Convert world-space X and Z coords into canyon space U and V
int canyon_findClosestPoint( float x, float z ) {
	vector point = Vector( x, 0.f, z, 1.f );
	int closest_i = 0;	// Closest point - in absolute stream position
	float closest_d = FLT_MAX;
	// We need to find the closest segment
	int default_start = canyon_streaming_buffer.stream_position + 1;
	int default_end = canyon_streaming_buffer.stream_position + canyon_streaming_buffer.window_size;

	// Estimate the closest z point, based on an even distribution of Zs
	// initialize the closest distance from that
	closest_i = max( canyon_streaming_buffer.stream_position + 1, canyon_estimatePointForZ( &canyon_streaming_buffer, z ));
	vector closest_point = canyon_point( &canyon_streaming_buffer, closest_i );
	vector displacement = vector_sub( point, closest_point );
	closest_d = vector_lengthSq( &displacement );

	// Then walk backwards until the earliest possible Z
	int start = canyon_findStartZ( default_start, z, closest_d, closest_i );
	int end = default_end;

	/*
	float earliest_z = z - sqrt( closest_d );
	start = search_index;
	float current_z = canyon_point( &canyon_streaming_buffer, start ).coord.z;
	// TODO - just binary search this?
	while ( current_z > earliest_z && start > default_start ) {
		int min_delta = (int)fmax( 1.f, ( current_z - earliest_z ) / kCanyonSegmentLength );
		start = max( start - min_delta, default_start );
		vAssert( start >= default_start );
		current_z = canyon_point( &canyon_streaming_buffer, start ).coord.z;
	}
	*/

	//printf( "search z: %.2f, min z: %.2f.\n", z, current_z );
	//vAssert( start < end );
	// TEMP
	// Test all points we wouldn't normally test
	/*
	   for ( size_t i = default_start; i < (size_t)start; ++i ) {
	   vector test_point = canyon_point( &canyon_streaming_buffer, i );
	   vector displacement = vector_sub( point, test_point );
	   vAssert( vector_lengthSq( &displacement ) > closest_d );
	   }
	   */
	//vAssert( end == default_end );

	// Iterate from there
	for ( size_t i = (size_t)start; i + 1 < (size_t)end; ++i ) {
		vector test_point = canyon_point( &canyon_streaming_buffer, i );
		
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

void terrain_canyonSpaceFromWorld( float x, float z, float* u, float* v ) {
	int closest_i = canyon_findClosestPoint( x, z );
	vector point = Vector( x, 0.f, z, 1.f );

	// find closest points on the two segments using that point
	vector closest_a, closest_b;
	float seg_pos_a = segment_closestPoint( canyon_point( &canyon_streaming_buffer, closest_i ), canyon_point( &canyon_streaming_buffer, closest_i+1 ), point, &closest_a );
	float seg_pos_b = segment_closestPoint( canyon_point( &canyon_streaming_buffer, closest_i-1 ), canyon_point( &canyon_streaming_buffer, closest_i ), point, &closest_b );
	// use the closest
	float length_a = vector_lengthI( vector_sub( point, closest_a ));
	float length_b = vector_lengthI( vector_sub( point, closest_b ));
	if ( length_a < length_b ) {
		//vector_printf( "Closest point: ", &closest_a );
		*u = length_a;
		*v = canyon_v( closest_i, seg_pos_a );
	} else {
		//vector_printf( "Closest point: ", &closest_b );
		*u = length_a;
		*v = canyon_v( closest_i - 1, seg_pos_b );
	}
	//printf( "terrain_canyonSpaceFromWorld: x,z ( %.2f, %.2f ) to u, v ( %.2f, %.2f )\n", x, z, *u, *v );
}

int terrainCanyon_segmentAtDistance( float v ) {
	return (float)floorf(v / kCanyonSegmentLength);
}

vector segment_normal( window_buffer* b, int i ) {
	vector next = canyon_point( b, i + 1 );
	vector previous = canyon_point( b, i );
	vector segment = normalized( vector_sub( next, previous ));
	vector normal = Vector( -segment.coord.z, 0.f, segment.coord.x, 0.f );
	//vAssert( isNormalized( &normal ));
	return normal;
}

// Convert canyon-space U and V coords into world space X and Z
void terrain_worldSpaceFromCanyon( float u, float v, float* x, float* z ) {
	int i = terrainCanyon_segmentAtDistance( v );
	if ( i < 0 ) {
		i = 0;
		//segment_position = 0.f;
	}
	float segment_position = ( v - (float)i * kCanyonSegmentLength ) / kCanyonSegmentLength; 
	
	// For this segment
	vector canyon_next = canyon_point( &canyon_streaming_buffer, i + 1 );
	vector canyon_previous = canyon_point( &canyon_streaming_buffer, i );

	//vAssert( segment_position >= 0.f );
	vAssert( segment_position <= 1.f );

	vector canyon_position = vector_lerp( &canyon_previous, &canyon_next, segment_position );

	vector normal = segment_normal( &canyon_streaming_buffer, i );

	/*
	vector segment = normalized( vector_sub( canyon_next, canyon_previous ));
	vector normal = Vector( -segment.coord.z, 0.f, segment.coord.x, 0.f );
	vAssert( isNormalized( &normal ));
	*/

	/*
	   We use a warped grid system, where near the canyon the U-axis lines run perpendicular to the canyon
	   but further away (more than max_perpendicular_u) they run parallel to the X axis.
	   This is to prevent overlapping artifacts in the terrain generation.
	   We actually lerp the perpendicular U-axis lines to blend from the previous and next segements
	   to prevent overlapping.
	   */
	vector normal_next = segment_normal( &canyon_streaming_buffer, i + 1);
	vector normal_previous;
	if ( i == 0 )
		normal_previous = normal;
	else
		normal_previous = segment_normal( &canyon_streaming_buffer, i - 1);

	// Begin and end vectors for tweening the normal
	vector normal_begin = normalized( vector_add( normal_previous, normal ));
	vector normal_end = normalized( vector_add( normal_next, normal ));
	//vector sampled_normal = vector_lerp( &normal_begin, &normal_end, segment_position );

	const float max_perpendicular_u = 100.f;
	vector target_begin = vector_add( canyon_previous, vector_scaled( normal_begin, max_perpendicular_u ));
	vector target_end = vector_add( canyon_next, vector_scaled( normal_end, max_perpendicular_u ));
	vector target = vector_lerp( &target_begin, &target_end, segment_position );
	vector sampled_normal = normalized( vector_sub( target, canyon_position ));

	float perpendicular_u = fclamp( u, -max_perpendicular_u, max_perpendicular_u );
	float flat_u = u - perpendicular_u;

	vector u_offset = vector_scaled( sampled_normal, perpendicular_u );
	vector flat_u_offset = vector_scaled( x_axis, -flat_u );
	vector position = vector_add( canyon_position, vector_add( u_offset, flat_u_offset ));
	*x = position.coord.x;
	*z = position.coord.z;
}

// Sample the canyon height (Y) at a given world X and Z
float terrain_canyonHeight( float x, float z, float u, float v ) {
	(void)x; (void)z; (void)v;
	u = ( u < 0.f ) ? fminf( u + canyon_base_radius, 0.f ) : fmaxf( u - canyon_base_radius, 0.f );

	const float incline_scale = 0.0004f;
	const float flat_radius = 20.f;
	const float offset = max( 0.0, fabsf( u ) - canyon_base_radius ) - flat_radius;
   	const float incline = offset * offset	* incline_scale;
	
	// Turn the canyon-space U into a height
	float mask = cos( fclamp( u / canyon_width, -PI/2.f, PI/2.f ));
	return (1.f - fclamp( powf( u / canyon_width, 4.f ), 0.f, 1.f )) * mask * canyon_height - incline;
}

vector terrain_newCanyonPoint( vector current, vector previous ) {
	// We want to generate a point in front
	// Generate a forward facing angle
	vector direction = vector_sub( current, previous );
	Normalize( &direction, &direction );
	float previous_angle = acosf( direction.coord.x );

	//float previous_angle = PI / 2.f;
	const float max_angle_delta = PI / 8.f;

	float min_angle = fmaxf( PI * 0.25f, previous_angle - max_angle_delta );
	float max_angle = fminf( PI * 0.75f, previous_angle + max_angle_delta );
	//float angle = frand( min_angle, max_angle );
	float angle = deterministic_frand( &canyon_random_seed, min_angle, max_angle );
	vector offset = Vector( cosf( angle ) * kCanyonSegmentLength, 0.f, sinf( angle ) * kCanyonSegmentLength, 0.f );

	vector next = vector_add( current, offset );
	return next;
}


const vector canyon_start = {{ 0.f, 0.f, 0.f, 1.f }};

// Generate X points of the canyon
void canyon_generateInitialPoints() {
	canyon_seedRandom( initial_seed );
	canyon_points[0] = canyon_start;
	int initial_straight_segments = 2;
	for ( int i = 1; i < initial_straight_segments + 1; ++i ) {
		canyon_points[i] = Vector( 0.f, 0.f, i * kCanyonSegmentLength, 1.f );
	}
	canyon_streaming_buffer.tail = initial_straight_segments;
	canyonBuffer_generatePoints( &canyon_streaming_buffer );	
}


void canyon_seekForWorldPosition( vector position ) {
	float u, v;
	terrain_canyonSpaceFromWorld( position.coord.x, position.coord.z, &u, &v );
	size_t seek_position = max( 0, terrainCanyon_segmentAtDistance( v ) - kTrailingCanyonSegments );
	canyonBuffer_seek( &canyon_streaming_buffer, seek_position );
}

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
void terrain_debugDraw( window* w ) {
	int total_points = kMaxCanyonPoints;

	for ( int i = 0; i+1 < total_points; ++i ) {
		// draw line segment from (i) to (i+1)
		terrain_drawSegment( w,
				canyon_point( &canyon_streaming_buffer, canyon_streaming_buffer.head + i ),
			   	canyon_point( &canyon_streaming_buffer, canyon_streaming_buffer.head + i + 1 ));
	}
}

void canyon_tick( void* canyon_data, float dt, engine* eng ) {
	(void)eng;
	canyon* c = canyon_data;
	float u, v;
	terrain_canyonSpaceFromWorld( zone_sample_point.coord.x, zone_sample_point.coord.z, &u, &v );
	int zone = canyon_zone( v );

	int current = zone % c->zone_count;
	int next = ( zone + 1 ) % c->zone_count;
	canyonZone* a = &c->zones[current];
	canyonZone* b = &c->zones[next];
	vector sky_color, fog_color, sun_color;
	canyonZone_skyFogBlend( a, b, canyonZone_blend( v ), &sky_color, &fog_color, &sun_color );
	scene_setFogColor( c->scene, &fog_color );
	scene_setSkyColor( c->scene, &sky_color );
	scene_setSunColor( c->scene, &sun_color );

	canyonZone_tick( c, dt );
}

canyon* canyon_create() {
	canyon* c = mem_alloc( sizeof( canyon ));
	memset( c, 0, sizeof( canyon ));
	return c;
}
