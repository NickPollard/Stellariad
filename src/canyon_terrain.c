// canyon_terrain.c
#include "common.h"
#include "canyon_terrain.h"
//-----------------------
#include "canyon.h"
#include "canyon_zone.h"
#include "collision.h"
#include "worker.h"
#include "maths/geometry.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/debugdraw.h"
#include "render/texture.h"

#define CANYON_TERRAIN_INDEXED 1
#define TERRAIN_USE_WORKER_THREAD 1

//#define TERRAIN_FORCE_NO_LOD
	
const float texture_scale = 0.0325f;
const float texture_repeat = 10.f;

texture* terrain_texture = 0;
texture* terrain_texture_cliff = 0;
texture* normal_flat = 0;
texture* cliff_normal = 0;

texture* terrain_texture_2 = 0;
texture* terrain_texture_cliff_2 = 0;


// *** Forward declarations
void canyonTerrainBlock_initVBO( canyonTerrainBlock* b );
void canyonTerrainBlock_calculateBuffers( canyonTerrainBlock* b );
void canyonTerrainBlock_createBuffers( canyonTerrainBlock* b );
void canyonTerrainBlock_calculateCollision( canyonTerrainBlock* b );
void canyonTerrainBlock_generate( canyonTerrainBlock* b );
void canyonTerrain_initVertexBuffers( canyonTerrain* t );
void canyonTerrain_initElementBuffers( canyonTerrain* t );

// *** Utility functions

int canyonTerrainBlock_renderIndexFromUV( canyonTerrainBlock* b, int u, int v ) {
	// Not adjusted as this is for renderable verts only
	vAssert( u >= 0 && u < b->u_samples );
	vAssert( v >= 0 && v < b->v_samples );
	return u + v * b->u_samples;
}
int canyonTerrainBlock_indexFromUV( canyonTerrainBlock* b, int u, int v ) {
	// Adjusted as we have a 1-vert margin for normal calculation at edges
	return ( u + 1 ) + ( v + 1 ) * ( b->u_samples + 2 );
}

// As this is just renderable verts, we dont have the extra buffer space for normal generation
int canyonTerrainBlock_renderVertCount( canyonTerrainBlock* b ) {
	return ( b->u_samples ) * ( b->v_samples );
}

// total verts, including those not rendered but that are generated for correct normal generation at block boundaries
int canyonTerrainBlock_vertCount( canyonTerrainBlock* b ) {
	return ( b->u_samples + 2 ) * ( b->v_samples + 2);
}

void canyonTerrainBlock_positionsFromUV( canyonTerrainBlock* b, int u_index, int v_index, float* u, float* v ) {
	int lod_ratio = b->u_samples / ( b->terrain->u_samples_per_block / 4 );
	if ( u_index == -1 )
		u_index = -lod_ratio;
	if ( u_index == b->u_samples )
		u_index = b->u_samples -1 + lod_ratio;
	if ( v_index == -1 )
		v_index = -lod_ratio;
	if ( v_index == b->v_samples )
		v_index = b->v_samples -1 + lod_ratio;

	float u_interval = ( b->u_max - b->u_min ) / (float)( b->u_samples - 1 );
	float v_interval = ( b->v_max - b->v_min ) / (float)( b->v_samples - 1 );
	*u = (float)u_index * u_interval + b->u_min;
	*v = (float)v_index * v_interval + b->v_min;
	//if ( u_index == 1 )
		//printf( "index %d, %d, position %.2f, %.2f, v_interval %.2f\n", u_index, v_index, *u, *v, v_interval );
}

int canyonTerrainBlock_triangleCount( canyonTerrainBlock* b ) {
	return ( b->u_samples - 1 ) * ( b->v_samples - 1 ) * 2;
}

bool boundsContains( int bounds[2][2], int coord[2] ) {
	return ( coord[0] >= bounds[0][0] &&
			coord[1] >= bounds[0][1] &&
			coord[0] <= bounds[1][0] &&
			coord[1] <= bounds[1][1] );
}

// Calculate the intersection of the two block bounds specified
void boundsIntersection( int intersection[2][2], int a[2][2], int b[2][2] ) {
	intersection[0][0] = max( a[0][0], b[0][0] );
	intersection[0][1] = max( a[0][1], b[0][1] );
	intersection[1][0] = min( a[1][0], b[1][0] );
	intersection[1][1] = min( a[1][1], b[1][1] );
}

// Mod the texcoord to a sensible value, based on the terrain block coords
// Used to stop our UV coordinates getting so big that floating point rounding issues cause
// aliasing in the texture
float canyon_uvMapped( float block_minimum, float f ) {
	//return f;
	float block_repeat = floorf( block_minimum / texture_repeat );
	return f - ( texture_repeat * block_repeat );
}

// ***
#define kMaxTerrainBlockWidth 40
#define kMaxTerrainBlockElements (kMaxTerrainBlockWidth * kMaxTerrainBlockWidth * 6)
unsigned short terrain_element_buffer[kMaxTerrainBlockElements];

void initialiseDefaultElementBuffer( ) {
	for ( int i = 0; i < kMaxTerrainBlockElements; i++ )
		terrain_element_buffer[i] = i;
}

void canyonTerrain_staticInit() {
	initialiseDefaultElementBuffer();
}

// ***


void canyonTerrainBlock_render( canyonTerrainBlock* b ) {
	// If we have new render buffers, free the old ones and switch to the new
	if (( b->vertex_VBO_alt && *b->vertex_VBO_alt ) &&
			( b->element_VBO_alt && *b->element_VBO_alt )) {
		if ( b->vertex_VBO )
			render_freeBuffer( b->vertex_VBO );
		if ( b->element_VBO )
		render_freeBuffer( b->element_VBO );

		b->vertex_VBO = b->vertex_VBO_alt;
		b->element_VBO = b->element_VBO_alt;
		b->element_count_render = b->element_count;

		b->vertex_VBO_alt = NULL;
		b->element_VBO_alt = NULL;
	}

	int zone = b->canyon->current_zone;
	int first = zone % 2;
	int second = 1 - first;
	first = ( zone + first ) % b->canyon->zone_count;
	second = ( zone + second ) % b->canyon->zone_count;
	if ( b->vertex_VBO && *b->vertex_VBO && terrain_texture && terrain_texture_cliff ) {
		drawCall* draw = drawCall_create( &renderPass_main, resources.shader_terrain, b->element_count_render, b->element_buffer, b->vertex_buffer, b->canyon->zones[first].texture_ground->gl_tex, modelview );
		draw->texture_b = b->canyon->zones[first].texture_cliff->gl_tex;
		draw->texture_c = b->canyon->zones[second].texture_ground->gl_tex;
		draw->texture_d = b->canyon->zones[second].texture_cliff->gl_tex;
		draw->texture_lookup = b->canyon->canyonZone_lookup_texture->gl_tex;
	
		// TEST
		draw->texture_normal = normal_flat->gl_tex;
		draw->texture_b_normal = cliff_normal->gl_tex;

		draw->vertex_VBO = *b->vertex_VBO;
		draw->element_VBO = *b->element_VBO;

		//drawCall_create( &renderPass_depth, resources.shader_depth, b->element_count_render, b->element_buffer, b->vertex_buffer, b->canyon->zones[first].texture_ground->gl_tex, modelview );
	}
}

int canyonTerrain_blockIndexFromUV( canyonTerrain* t, int u, int v ) {
	return u + v * t->u_block_count;
}

void canyonTerrain_render( void* data ) {
	canyonTerrain* t = data;
	render_resetModelView();
	matrix_mulInPlace( modelview, modelview, t->trans->world );

	for ( int i = 0; i < t->total_block_count; ++i ) {
		canyonTerrainBlock_render( t->blocks[i] );
	}
}

canyonTerrainBlock* canyonTerrainBlock_create( canyonTerrain* t ) {
	canyonTerrainBlock* b = mem_alloc( sizeof( canyonTerrainBlock ));
	memset( b, 0, sizeof( canyonTerrainBlock ));
	b->vertex_VBO = NULL;
	b->element_VBO = NULL;
	b->vertex_VBO_alt = NULL;
	b->element_VBO_alt = NULL;
	b->u_samples = t->u_samples_per_block;
	b->v_samples = t->v_samples_per_block;
	b->terrain = t;
	return b;
}

void canyonTerrain_blockContaining( int coord[2], canyonTerrain* t, vector* point ) {
	float u, v;
	terrain_canyonSpaceFromWorld( point->coord.x, point->coord.z, &u, &v );
	float block_width = (2 * t->u_radius) / (float)t->u_block_count;
	float block_height = (2 * t->v_radius) / (float)t->v_block_count;
	coord[0] = fround( u / block_width, 1.f );
	coord[1] = fround( v / block_height, 1.f );
}

int canyonTerrain_lodLevelForBlock( canyonTerrain* t, int coord[2] ) {
	int block[2];
	canyonTerrain_blockContaining( block, t, &t->sample_point );
	//int centre_u = ( t->u_block_count + 1 ) / 2 + t->bounds[0][0];
	//int centre_v = ( t->v_block_count + 1 ) / 2 + t->bounds[0][1];
	int u_distance = abs( coord[0] - block[0] );
	int v_distance = abs( coord[1] - block[1] );
	vAssert( t->lod_interval_u > 0 );
	vAssert( t->lod_interval_v > 0 );
	return min( 2, ( u_distance / t->lod_interval_u ) + ( v_distance / t->lod_interval_v ));
}

void canyonTerrainBlock_calculateSamplesForLoD( canyonTerrainBlock* b, canyonTerrain* t, int coord[2] ) {
#ifdef TERRAIN_FORCE_NO_LOD
	(void)coord;
	int lod_level = 0;
#else
	int lod_level = canyonTerrain_lodLevelForBlock( t, coord );
#endif
	// Set samples based on U offset
	// We add one so that we always get a centre point
	switch( lod_level ) {
		case 0:
			b->u_samples = t->u_samples_per_block + 1;
			b->v_samples = t->v_samples_per_block + 1;
			break;
		case 1:
			b->u_samples = t->u_samples_per_block / 2 + 1;
			b->v_samples = t->v_samples_per_block / 2 + 1;
			break;
		case 2:
			b->u_samples = t->u_samples_per_block / 4 + 1;
			b->v_samples = t->v_samples_per_block / 4 + 1;
			break;
		default:
			vAssert( 0 );
	}
	b->lod_level = lod_level;
}

void canyonTerrainBlock_calculateExtents( canyonTerrainBlock* b, canyonTerrain* t, int coord[2] ) {
	float u_size = (2 * t->u_radius) / (float)t->u_block_count;
	float v_size = (2 * t->v_radius) / (float)t->v_block_count;
	b->u_min = ((float)coord[0] - 0.5f) * u_size;
	b->v_min = ((float)coord[1] - 0.5f) * v_size;
	b->u_max = b->u_min + u_size;
	b->v_max = b->v_min + v_size;

	canyonTerrainBlock_calculateSamplesForLoD( b, t, coord );
}

// Calculate the block bounds for the terrain, at a given sample point
void canyonTerrain_calculateBounds( int bounds[2][2], canyonTerrain* t, vector* sample_point ) {
	/*
	   Find the block we are in
	   Then extend by the correct radius

	   The center block [0] is from -half_block_width to half_block_width
	   */
	int block[2];
	canyonTerrain_blockContaining( block, t, sample_point );
	int rx = ( t->u_block_count - 1 ) / 2;
	int ry = ( t->v_block_count - 1 ) / 2;
	bounds[0][0] = block[0] - rx;
	bounds[0][1] = block[1] - ry;
	bounds[1][0] = block[0] + rx;
	bounds[1][1] = block[1] + ry;
}

void canyonTerrain_createBlocks( canyonTerrain* t ) {
	vAssert( !t->blocks );

	t->total_block_count = t->u_block_count * t->v_block_count;
	vAssert( t->total_block_count > 0 );

	t->blocks = mem_alloc( sizeof( canyonTerrainBlock* ) * t->total_block_count );

	// Ensure the block bounds are initialised;
	canyonTerrain_calculateBounds( t->bounds, t, &t->sample_point );

	// Calculate block extents
	for ( int v = 0; v < t->v_block_count; v++ ) {
		for ( int u = 0; u < t->u_block_count; u++ ) {
			int i = canyonTerrain_blockIndexFromUV( t, u, v );
			t->blocks[i] = canyonTerrainBlock_create( t );
			canyonTerrainBlock* b = t->blocks[i];
			b->coord[0] = t->bounds[0][0] + u;
			b->coord[1] = t->bounds[0][1] + v;
			canyonTerrainBlock_generate( b );
			t->blocks[i]->canyon = t->canyon;
		}
	}
}

canyonTerrain* canyonTerrain_create( canyon* c, int u_blocks, int v_blocks, int u_samples, int v_samples, float u_radius, float v_radius ) {
	canyonTerrain* t = mem_alloc( sizeof( canyonTerrain ));
	memset( t, 0, sizeof( canyonTerrain ));
	t->canyon = c;
	t->u_block_count = u_blocks;
	t->v_block_count = v_blocks;
	vAssert( u_samples <= kMaxTerrainBlockWidth );
	vAssert( v_samples <= kMaxTerrainBlockWidth );
	t->u_samples_per_block = u_samples;
	t->v_samples_per_block = v_samples;
	t->u_radius = u_radius;
	t->v_radius = v_radius;
	t->lod_interval_u = 3;
	t->lod_interval_v = 2;

	canyonTerrain_initVertexBuffers( t );
#if CANYON_TERRAIN_INDEXED
	canyonTerrain_initElementBuffers( t );
#endif // CANYON_TERRAIN_INDEX
	canyonTerrain_createBlocks( t );

	t->trans = transform_create();

	if ( !terrain_texture ) {
		terrain_texture = texture_load( "dat/img/terrain/grass.tga" );
	}
	if ( !terrain_texture_cliff ) {
		terrain_texture_cliff = texture_load( "dat/img/terrain/cliff_grass.tga" );
	}
	if ( !normal_flat ) {
		normal_flat = texture_load( "dat/img/normal_flat.tga" );
	}
	if ( !cliff_normal ) {
		cliff_normal = texture_load( "dat/img/terrain/cliff_normal.tga" );
	}

	// Temp
	if ( !terrain_texture_2 ) {
		terrain_texture_2 = texture_load( "dat/img/terrain/ground_industrial.tga" );
	}
	if ( !terrain_texture_cliff_2 ) {
		terrain_texture_cliff_2 = texture_load( "dat/img/terrain/cliff_industrial.tga" );
	}

	return t;
}

void canyonTerrain_setLodIntervals( canyonTerrain* t, int u, int v ) {
	t->lod_interval_u = u;
	t->lod_interval_v = v;
}

bool canyonTerrainBlock_triangleInvalid( canyonTerrainBlock* b, int u_index, int v_index, int u_offset, int v_offset ) {
	u_offset = u_offset / 2 + u_offset % 2;
	u_offset = min( u_offset, 0 );
	return ( u_index + u_offset >= b->u_samples - 1 ) ||
		( u_index + u_offset < 0 ) ||
		( v_index + v_offset >= b->v_samples - 1 ) ||
		( v_index + v_offset < 0 );
}

#if CANYON_TERRAIN_INDEXED
#else
// Given a vertex that has been generated, fill in the correct triangles for it after it has been unrolled
void canyonTerrainBlock_fillTrianglesForVertex( canyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert ) {
	// Each vertex is in a maximum of 6 triangles
	// The triangle indices can be computed as: (where row == ( u_samples - 1 ) * 2)
	//  first row:
	//    row * (v_index - 1) + 2 * u_index - 1
	//    row * (v_index - 1) + 2 * u_index
	//    row * (v_index - 1) + 2 * u_index + 1
	//	second row:
	//    row * v_index + 2 * u_index - 2
	//    row * v_index + 2 * u_index - 1
	//    row * v_index + 2 * u_index
	// (discarding any that fall outside the range 0 -> tri_count)
	// Triangle vert index:
	// top: 0, 2, 1
	// bottom: 1, 2, 0
	// finished index = triangle_index * 3 + triangle_vert_index
	const int triangle_vert_indices[6] = { 0, 2, 1, 1, 2, 0 };
	const int v_offset[6] = { -1, -1, -1, 0, 0, 0 };
	const int u_offset[6] = { -1, 0, 1, -2, -1, 0 };
	
	int triangles_per_row = ( b->u_samples - 1 ) * 2;
	
	for ( int i = 0; i < 6; ++i ) {
		// Calculate triangle index
		int row = v_index + ( v_offset[i] );
		int column = ( 2 * u_index ) + ( u_offset[i] );
		int triangle_index = triangles_per_row * row + column; 

		if ( canyonTerrainBlock_triangleInvalid( b, u_index, v_index, u_offset[i], v_offset[i]) )
			continue;

		// if it's a valid triangle (not out-of-bounds)
		int vert_index = triangle_index * 3 + triangle_vert_indices[i];
		vertices[vert_index] = *vert;

		// Cliff coloring
		const int triangle_u_offset[6][2] = { { 0, -1 }, { 1, 0 }, { 1, 1 }, { -1, -1 }, { -1, 0 }, { 0, 1 } };
		const int triangle_v_offset[6][2] = { { -1, 0 }, { -1, -1 }, { 0, -1 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };
		
		// Calculate indices of other 2 triangle verts
		int index_b = canyonTerrainBlock_indexFromUV( b, u_index + triangle_u_offset[i][0] , v_index + triangle_v_offset[i][0] );
		int index_c = canyonTerrainBlock_indexFromUV( b, u_index + triangle_u_offset[i][1] , v_index + triangle_v_offset[i][1] );

		vector v_b = positions[index_b];
		vector v_c = positions[index_c];

		vector normal;
		float d;
		plane( vertices[vert_index].position, v_b, v_c, &normal, &d );
		float angle = acosf( Dot( &normal, &y_axis ));
		const float cliff_angle = PI / 4.f;
		bool cliff = angle > cliff_angle;
		if ( cliff ) {
			vertices[vert_index].color = Vector( 0.2f, 0.3f, 0.5f, 1.f );
		} else {
			vertices[vert_index].color = Vector( 0.8f, 0.9f, 1.0f, 0.f );
		}
	}
}

void canyonTerrainBlock_generateVertices( canyonTerrainBlock* b, vector* verts, vector* normals ) {
	for ( int v = 0; v < b->v_samples; v ++ ) {
		for ( int u = 0; u < b->u_samples; u ++  ) {
			int i = canyonTerrainBlock_indexFromUV( b, u, v );
			vertex vert;
			vert.position = verts[i];
			vert.normal = normals[i];

			float u_pos, v_pos;
			canyonTerrainBlock_positionsFromUV( b, u, v, &u_pos, &v_pos );

			vert.uv = Vector( ( vert.position.coord.x * texture_scale ),
				   	canyon_uvMapped( b->v_min * texture_scale, vert.position.coord.z * texture_scale ),
				   	canyon_uvMapped( b->v_min * texture_scale, v_pos * texture_scale ),
				   	vert.position.coord.y * texture_scale );
			canyonTerrainBlock_fillTrianglesForVertex( b, verts, b->vertex_buffer, u, v, &vert );
		}
	}
}
#endif // CANYON_TERRAIN_INDEXED

// Generate Normals
void canyonTerrainBlock_calculateNormals( canyonTerrainBlock* block, int vert_count, vector* verts, vector* normals ) {
	(void)vert_count;

	int lod_ratio = block->u_samples / ( block->terrain->u_samples_per_block / 4 );

	for ( int v = 0; v < block->v_samples; ++v ) {
		for ( int u = 0; u < block->u_samples; ++u ) {
			int index;
			if ( v == block->v_samples - 1 )
				index = canyonTerrainBlock_indexFromUV( block, u, v - lod_ratio );
			else
				index = canyonTerrainBlock_indexFromUV( block, u, v - 1 );
			vector left		= verts[index];

			if ( v == 0 )
				index = canyonTerrainBlock_indexFromUV( block, u, v + lod_ratio );
			else
				index = canyonTerrainBlock_indexFromUV( block, u, v + 1 );
			vector right	= verts[index];

			if ( u == block->u_samples - 1 )
				index = canyonTerrainBlock_indexFromUV( block, u - lod_ratio, v );
			else
				index = canyonTerrainBlock_indexFromUV( block, u - 1, v );
			vector top		= verts[index];

			if ( u == 0 )
				index = canyonTerrainBlock_indexFromUV( block, u + lod_ratio, v );
			else
				index = canyonTerrainBlock_indexFromUV( block, u + 1, v );
			vector bottom	= verts[index];
			
			int i = canyonTerrainBlock_indexFromUV( block, u, v );

			vector a, b, c, x, y;
			// Calculate vertical vector
			// Take cross product to calculate normals
			Sub( &a, &bottom, &top );
			Cross( &x, &a, &y_axis );
			Cross( &b, &x, &a );

			// Calculate horizontal vector
			// Take cross product to calculate normals
			Sub( &a, &right, &left );
			Cross( &y, &a, &y_axis );
			Cross( &c, &y, &a );

			Normalize( &b, &b );
			Normalize( &c, &c );

			// Average normals
			vector total = Vector( 0.f, 0.f, 0.f, 0.f );
			Add( &total, &total, &b );
			Add( &total, &total, &c );
			total.coord.w = 0.f;
			Normalize( &total, &total );
			vAssert( i < vert_count );
			normals[i] = total;

#if CANYON_TERRAIN_INDEXED
			int buffer_index = canyonTerrainBlock_renderIndexFromUV( block, u, v );
			block->vertex_buffer[buffer_index].normal = normals[i];
#endif // CANYON_TERRAIN_INDEXED
		}
	}
}

void* canyonTerrain_allocVertexBuffer( canyonTerrain* t ) {
#if CANYON_TERRAIN_INDEXED
	int max_vert_count = ( t->u_samples_per_block + 1 ) * ( t->v_samples_per_block + 1 );
	return mem_alloc( sizeof( vertex ) * max_vert_count );
#else
	int max_element_count = t->u_samples_per_block * t->v_samples_per_block * 6;
	return mem_alloc( sizeof( vertex ) * max_element_count );
#endif // CANYON_TERRAIN_INDEX
}

void canyonTerrain_initVertexBuffers( canyonTerrain* t ) {
	// Init w*h*2 buffers that we can use for vertex_buffers
	vAssert( t->vertex_buffers == 0 );
	int count = t->u_block_count * t->v_block_count * 2;
	t->vertex_buffers = mem_alloc( count * sizeof( vertex* ));
	for ( int i = 0; i < count; i++ ) {
		t->vertex_buffers[i] = canyonTerrain_allocVertexBuffer( t );
	}
}
void canyonTerrain_freeVertexBuffer( canyonTerrain* t, unsigned short* buffer ) {
	// Find the buffer in the list
	// Switch it with the last
	int count = t->vertex_buffer_count;
	int i = array_find( (void**)t->vertex_buffers, count, buffer );
	t->vertex_buffers[i] = t->vertex_buffers[count-1];
	t->vertex_buffers[count-1] = buffer;
	--t->vertex_buffer_count;
}
void* canyonTerrain_nextVertexBuffer( canyonTerrain* t  ) {
	vAssert( t->vertex_buffer_count < ( t->u_block_count * t->v_block_count * 2 ));
	void* buffer = t->vertex_buffers[t->vertex_buffer_count++];
	return buffer;
}

#if CANYON_TERRAIN_INDEXED
void* canyonTerrain_allocElementBuffer( canyonTerrain* t ) {
	int max_element_count = t->u_samples_per_block * t->v_samples_per_block * 6;
	return mem_alloc( sizeof( unsigned short ) * max_element_count );
}
void canyonTerrain_initElementBuffers( canyonTerrain* t ) {
	// Init w*h*2 buffers that we can use for element_buffers
	vAssert( t->element_buffers == 0 );
	int count = t->u_block_count * t->v_block_count * 2;
	t->element_buffers = mem_alloc( count * sizeof( unsigned short* ));
	for ( int i = 0; i < count; i++ ) {
		t->element_buffers[i] = canyonTerrain_allocElementBuffer( t );
	}
}
void canyonTerrain_freeElementBuffer( canyonTerrain* t, unsigned short* buffer ) {
	// Find the buffer in the list
	// Switch it with the last
	int count = t->element_buffer_count;
	int i = array_find( (void**)t->element_buffers, count, buffer );
	t->element_buffers[i] = t->element_buffers[count-1];
	t->element_buffers[count-1] = buffer;
	--t->element_buffer_count;
}
void* canyonTerrain_nextElementBuffer( canyonTerrain* t  ) {
	vAssert( t->element_buffer_count < ( t->u_block_count * t->v_block_count * 2 ));
	void* buffer = t->element_buffers[t->element_buffer_count++];
	return buffer;
}
#endif // CANYON_TERRAIN_INDEX



void canyonTerrainBlock_createBuffers( canyonTerrainBlock* b ) {
	b->element_count = canyonTerrainBlock_triangleCount( b ) * 3;
	vAssert( b->element_count > 0 );
	vAssert( b->element_count <= kMaxTerrainBlockElements );

#if CANYON_TERRAIN_INDEXED
	if ( !b->element_buffer )
		b->element_buffer = canyonTerrain_nextElementBuffer( b->terrain );
	if ( !b->vertex_buffer )
		b->vertex_buffer = canyonTerrain_nextVertexBuffer( b->terrain );
#else
	if ( !b->vertex_buffer )
		b->vertex_buffer = canyonTerrain_nextVertexBuffer( b->terrain );
	b->element_buffer = terrain_element_buffer;
#endif // CANYON_TERRAIN_INDEXED
}

void canyonTerrainBlock_calculateBuffers( canyonTerrainBlock* b ) {
	int vert_count = canyonTerrainBlock_vertCount( b );
	
	vector* verts = alloca( sizeof( vector ) * vert_count );
	memset( verts, 0, sizeof( vector ) * vert_count );
	vector* normals = alloca( sizeof( vector ) * vert_count );

	// Generate initial mesh vertices
	for ( int v_index = -1; v_index < b->v_samples + 1; ++v_index ) {
		for ( int u_index = -1; u_index < b->u_samples + 1; ++u_index ) {
			float u, v, vert_x, vert_y, vert_z;
			int i = canyonTerrainBlock_indexFromUV( b, u_index, v_index );
			vAssert( i < vert_count );
			canyonTerrainBlock_positionsFromUV( b, u_index, v_index, &u, &v );
			terrain_worldSpaceFromCanyon( u, v, &vert_x, &vert_z );
			vert_y = canyonTerrain_sampleUV( vert_x, vert_z, u, v );
			verts[i] = Vector( vert_x, vert_y, vert_z, 1.f );
			normals[i] = y_axis;
		}
	}

#if 1
	// Force low-LOD edges
	int lod_ratio = b->u_samples / ( b->terrain->u_samples_per_block / 4 );

	for ( int v_index = -1; v_index < b->v_samples + 1; ++v_index ) {
		for ( int u_index = -1; u_index < b->u_samples + 1; ++u_index ) {
			// Generate a vertex
			int i = canyonTerrainBlock_indexFromUV( b, u_index, v_index );
			vAssert( i < vert_count );
			float u, v;
			canyonTerrainBlock_positionsFromUV( b, u_index, v_index, &u, &v );

			// If it's an intermediary value
			if ( ( v_index <= 0 || v_index >= b->v_samples - 1 ) && 
					( u_index % lod_ratio != 0 ) &&
					( u_index >= 0 && u_index < b->u_samples )) {
				int prev_index = canyonTerrainBlock_indexFromUV( b, u_index - ( u_index % lod_ratio ), v_index );
				int next_index = canyonTerrainBlock_indexFromUV( b, u_index - ( u_index % lod_ratio ) + lod_ratio, v_index );

				if ( prev_index >= 0 && prev_index < vert_count &&
						next_index >= 0 && next_index < vert_count ) {
					float factor = (float)( u_index % lod_ratio ) / (float)lod_ratio;
					vector previous = verts[prev_index];
					vector next = verts[next_index];
					verts[i] = vector_lerp( &previous, &next, factor );
				}
			}

			if ( ( u_index <= 0 || u_index >= b->u_samples - 1 ) && 
					( v_index % lod_ratio != 0 ) &&
					( v_index >= 0 && v_index < b->v_samples )) {
				int prev_index = canyonTerrainBlock_indexFromUV( b, u_index, v_index - ( v_index % lod_ratio ));
				int next_index = canyonTerrainBlock_indexFromUV( b, u_index, v_index - ( v_index % lod_ratio ) + lod_ratio );

				if ( prev_index >= 0 && prev_index < vert_count &&
						next_index >= 0 && next_index < vert_count ) {
					float factor = (float)( v_index % lod_ratio ) / (float)lod_ratio;
					vector previous = verts[prev_index];
					vector next = verts[next_index];
					verts[i] = vector_lerp( &previous, &next, factor );
				}
			}

#if CANYON_TERRAIN_INDEXED
			if ( v_index >= 0 && v_index < b->v_samples && u_index >= 0 && u_index < b->u_samples ) {
				int buffer_index = canyonTerrainBlock_renderIndexFromUV( b, u_index, v_index );
				vAssert( buffer_index < canyonTerrainBlock_renderVertCount( b ));
				vAssert( buffer_index >= 0 );
				b->vertex_buffer[buffer_index].position = verts[i];
				b->vertex_buffer[buffer_index].uv = Vector( ( verts[i].coord.x * texture_scale ),
					   										canyon_uvMapped( b->v_min * texture_scale, verts[i].coord.z * texture_scale ),
														    canyon_uvMapped( b->v_min * texture_scale, v * texture_scale ),
														   	verts[i].coord.y * texture_scale );
				b->vertex_buffer[buffer_index].color = Vector( canyonZone_terrainBlend( v ), 0.f, 0.f, 1.f );
			}
#endif // CANYON_TERRAIN_INDEXED
		}
	}
#endif

	canyonTerrainBlock_calculateNormals( b, vert_count, verts, normals );

#if 1
	// Force low-LOD edges
	for ( int v_index = 0; v_index < b->v_samples; ++v_index ) {
		for ( int u_index = 0; u_index < b->u_samples; ++u_index ) {
			int i = canyonTerrainBlock_indexFromUV( b, u_index, v_index );
			// If it's an intermediary value
			if ( ( v_index == 0 || v_index == b->v_samples - 1 ) && 
					( u_index % lod_ratio != 0 ) &&
					( u_index >= 0 && u_index < b->u_samples )) {
				int prev_index = canyonTerrainBlock_indexFromUV( b, u_index - ( u_index % lod_ratio ), v_index );
				int next_index = canyonTerrainBlock_indexFromUV( b, u_index - ( u_index % lod_ratio ) + lod_ratio, v_index );

				if ( prev_index >= 0 && prev_index < vert_count &&
						next_index >= 0 && next_index < vert_count ) {
					float factor = (float)( u_index % lod_ratio ) / (float)lod_ratio;
					vector previous = normals[prev_index];
					vector next = normals[next_index];
					normals[i] = vector_lerp( &previous, &next, factor );
				}
			}

			if ( ( u_index == 0 || u_index == b->u_samples - 1 ) && 
					( v_index % lod_ratio != 0 ) &&
					( v_index >= 0 && v_index < b->v_samples )) {
				int prev_index = canyonTerrainBlock_indexFromUV( b, u_index, v_index - ( v_index % lod_ratio ));
				int next_index = canyonTerrainBlock_indexFromUV( b, u_index, v_index - ( v_index % lod_ratio ) + lod_ratio );

				if ( prev_index >= 0 && prev_index < vert_count &&
						next_index >= 0 && next_index < vert_count ) {
					float factor = (float)( v_index % lod_ratio ) / (float)lod_ratio;
					vector previous = normals[prev_index];
					vector next = normals[next_index];
					normals[i] = vector_lerp( &previous, &next, factor );
				}
			}

#if CANYON_TERRAIN_INDEXED
			if ( v_index >= 0 && v_index < b->v_samples && u_index >= 0 && u_index < b->u_samples ) {
				int buffer_index = canyonTerrainBlock_renderIndexFromUV( b, u_index, v_index );
				b->vertex_buffer[buffer_index].normal = normals[i];
			}
#endif // CANYON_TERRAIN_INDEXED
		}
	}
#endif


#if CANYON_TERRAIN_INDEXED
	int triangle_count = canyonTerrainBlock_triangleCount( b );
	int i = 0;
	for ( int v = 0; v + 1 < b->v_samples; ++v ) {
		for ( int u = 0; u + 1 < b->u_samples; ++u ) {
			vAssert( i * 3 + 5 < b->element_count );
			vAssert( canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 ) < canyonTerrainBlock_renderVertCount( b ) );
			b->element_buffer[ i * 3 + 0 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v );
			b->element_buffer[ i * 3 + 1 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
			b->element_buffer[ i * 3 + 2 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
			i++;
			b->element_buffer[ i * 3 + 0 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
			b->element_buffer[ i * 3 + 1 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 );
			b->element_buffer[ i * 3 + 2 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
			i++;
		}
	}
	vAssert( i == triangle_count );
#else
	// Unroll Verts
	canyonTerrainBlock_generateVertices( b, verts, normals );
#endif // CANYON_TERRAIN_INDEXED
	
	//mem_free( verts );
	//mem_free( normals );

	canyonTerrainBlock_initVBO( b );
}

// Create GPU vertex buffer objects to hold our data and save transferring to the GPU each frame
// If we've already allocated a buffer at some point, just re-use it
void canyonTerrainBlock_initVBO( canyonTerrainBlock* b ) {
#if CANYON_TERRAIN_INDEXED
	int vert_count = canyonTerrainBlock_renderVertCount( b );
#else
	int vert_count = b->element_count;
#endif // CANYON_TERRAIN_INDEXED
	b->vertex_VBO_alt	= render_requestBuffer( GL_ARRAY_BUFFER,			b->vertex_buffer,	sizeof( vertex )	* vert_count );
	b->element_VBO_alt	= render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, 	b->element_buffer,	sizeof( GLushort ) 	* b->element_count );
}

void canyonTerrainBlock_generate( canyonTerrainBlock* b ) {
	canyonTerrainBlock_calculateExtents( b, b->terrain, b->coord );
	canyonTerrainBlock_createBuffers( b );
	canyonTerrainBlock_calculateBuffers( b );
	canyonTerrainBlock_calculateCollision( b );
}

void* canyonTerrain_workerGenerateBlock( void* args ) {
	//printf( "Worker generate terrain block.\n" );
	canyonTerrainBlock* b = args;
	if ( b->pending ) {
		canyonTerrainBlock_generate( b );
		b->pending = false;
	}
	return NULL;
}

// Set up a task for the worker thread to generate the terrain block
void canyonTerrain_queueWorkerTaskGenerateBlock( canyonTerrainBlock* b ) {
	worker_task terrain_block_task;
	terrain_block_task.func = canyonTerrain_workerGenerateBlock;
	terrain_block_task.args = b;
	worker_addTask( terrain_block_task );
}

void canyonTerrain_updateBlocks( canyonTerrain* t ) {
	/*
	   We have a set of current blocks, B
	   We have a set of projected blocks based on the new position, B'

	   Calculate the intersection I = B n B';
	   All blocks ( b | b is in I ) we keep, shifting their pointers to the correct position
	   All other blocks fill up the empty spaces, then are recalculated

	   We are not freeing or allocating any blocks here; only reusing existing ones
	   The block pointer array remains sorted
	   */

	int bounds[2][2];
	int intersection[2][2];
	canyonTerrain_calculateBounds( bounds, t, &t->sample_point );

	// If the bounds are the exact same as before, we don't need to do *any* updating
	if ( bounds[0][0] == t->bounds[0][0] &&
			bounds[0][1] == t->bounds[0][1] &&
			bounds[1][0] == t->bounds[1][0] &&
			bounds[1][1] == t->bounds[1][1] )
		return;

	boundsIntersection( intersection, bounds, t->bounds );

	// Using alloca for dynamic stack allocation (just moves the stack pointer up)
	canyonTerrainBlock** new_blocks = alloca( sizeof( canyonTerrainBlock* ) * t->total_block_count );

	int empty_index = 0;
	// For Each old block
	for ( int i = 0; i < t->total_block_count; i++ ) {
		int coord[2];
		coord[0] = t->bounds[0][0] + ( i % t->u_block_count );
		coord[1] = t->bounds[0][1] + ( i / t->u_block_count );
		// if in new bounds
		if ( boundsContains( intersection, coord )) {
			// copy to new array;
			int new_u = coord[0] - bounds[0][0];
			int new_v = coord[1] - bounds[0][1];
			int new_index = new_u + ( new_v * t->u_block_count );
			vAssert( new_index >= 0 );
			vAssert( new_index < t->total_block_count );
			new_blocks[new_index] = t->blocks[i];
		}
		else {
			// Copy unused blocks
			// Find next empty index
			int new_coord[2];
			while ( true ) {
				new_coord[0] = bounds[0][0] + ( empty_index % t->u_block_count );
				new_coord[1] = bounds[0][1] + ( empty_index / t->u_block_count );
				if ( !boundsContains( intersection, new_coord ))
					break;
				empty_index++;
			}
			new_blocks[empty_index] = t->blocks[i];
			empty_index++;
		}
	}

	// For each new block
	for ( int i = 0; i < t->total_block_count; i++ ) {
		int coord[2];
		coord[0] = bounds[0][0] + ( i % t->u_block_count );
		coord[1] = bounds[0][1] + ( i / t->u_block_count );
		canyonTerrainBlock* b = new_blocks[i];
		// if not in old bounds
		// Or if we need to change lod level
#ifdef TERRAIN_FORCE_NO_LOD
		if ( !boundsContains( intersection, coord ))
#else
		if ( !boundsContains( intersection, coord ) || 
				( !b->pending && ( b->lod_level != canyonTerrain_lodLevelForBlock( t, coord ))))
#endif
		{
			memcpy( b->coord, coord, sizeof( int ) * 2 );
			// mark it as new, buffers will be filled in later
			b->pending = true;
		}
	}
	memcpy( t->bounds, bounds, sizeof( int ) * 2 * 2 );
	memcpy( t->blocks, new_blocks, sizeof( canyonTerrainBlock* ) * t->total_block_count );

	for ( int i = 0; i < t->total_block_count; ++i ) {
		canyonTerrainBlock* b = t->blocks[i];
		if ( b->pending ) {
#if TERRAIN_USE_WORKER_THREAD
			canyonTerrain_queueWorkerTaskGenerateBlock( b );
#else
			canyonTerranBlock_generate( b );
			b->pending = false;
			break;
#endif // TERRAIN_USE_WORKER_THREAD
		}
	}
}

void canyonTerrain_tick( void* data, float dt, engine* eng ) {
	(void)dt;
	(void)eng;
	canyonTerrain* t = data;
	canyonTerrain_updateBlocks( t );
}

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

float terrain_detailHeight( float u, float v ) {
	return	0.5 * sinf( u ) * sinf( v ) +
			sinf( u / 3.f ) * sinf( v / 3.f ) +
			5 * sinf( u / 10.f ) * sinf( v / 10.f ) * sinf( u / 10.f ) * sinf( v / 10.f );
	/*
	return 3.f * ( sinf( u / 23.f ) * sinf( v / 23.f ) * sinf( u / 23.f ) * sinf( v / 23.f ) +
			sinf( u / 17.f ) * sinf( v / 17.f ) +
		   sinf( u / 13.f ) * sinf( v / 13.f ));
		   */

}

// The procedural function
float canyonTerrain_sampleUV( float x, float z, float u, float v ) {
	float mountains = terrain_mountainHeight( x, z, u, v );
	float detail = terrain_detailHeight( u, v );
	float canyon = terrain_canyonHeight( x, z, u, v );
	return mountains + detail - canyon;
}

float canyonTerrain_sample( float x, float z ) {
	float u, v;
	terrain_canyonSpaceFromWorld( x, z, &u, &v );
	return canyonTerrain_sampleUV( x, z, u, v );
}


void canyonTerrainBlock_removeCollision( canyonTerrainBlock* b ) {
	transform_delete( b->collision->trans );
	collision_removeBody( b->collision );
	b->collision = NULL;
}

void canyonTerrainBlock_calculateCollision( canyonTerrainBlock* b ) {
	if ( b->collision) {
		canyonTerrainBlock_removeCollision( b );
	}

	heightField* h = heightField_create( b->u_max - b->u_min, 
											b->v_max - b->v_min, 
											b->u_samples, 
											b->v_samples );
	for ( int i = 0; i < b->u_samples; i++ ) {
		for ( int j = 0; j < b->v_samples; j++ ) {
			int index = canyonTerrainBlock_renderIndexFromUV( b, i, j );
			h->verts[i + j * b->u_samples] = b->vertex_buffer[index].position;
		}
	}
	heightField_calculateAABB( h );
	shape* s = shape_heightField_create( h );
	b->collision = body_create( s, transform_create());
	b->collision->layers |= kCollisionLayerTerrain;
	//b->collision->collide_with |= kCollisionLayerPlayer;
	collision_addBody( b->collision );
}
