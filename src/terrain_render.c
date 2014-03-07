// terrain_render.c
#include "common.h"
#include "terrain_render.h"
//-----------------------
#include "canyon.h"
#include "camera.h"
#include "maths/geometry.h"
#include "maths/vector.h"
#include "render/graphicsbuffer.h"
#include "render/texture.h"

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
		int index_b = indexFromUV( b, u_index + triangle_u_offset[i][0] , v_index + triangle_v_offset[i][0] );
		int index_c = indexFromUV( b, u_index + triangle_u_offset[i][1] , v_index + triangle_v_offset[i][1] );

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
#endif

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
void* canyonTerrain_nextVertexBuffer( canyonTerrain* t ) {
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
void* canyonTerrain_nextElementBuffer( canyonTerrain* t ) {
	vAssert( t->element_buffer_count < ( t->u_block_count * t->v_block_count * 2 ));
	void* buffer = t->element_buffers[t->element_buffer_count++];
	return buffer;
}
// Create GPU vertex buffer objects to hold our data and save transferring to the GPU each frame
// If we've already allocated a buffer at some point, just re-use it
void terrainBlock_initVBO( canyonTerrainBlock* b ) {
	int vert_count = canyonTerrainBlock_renderVertCount( b );
	b->vertex_VBO_alt	= render_requestBuffer( GL_ARRAY_BUFFER,			b->vertex_buffer,	sizeof( vertex )	* vert_count );
	b->element_VBO_alt	= render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, 	b->element_buffer,	sizeof( GLushort ) 	* b->element_count );
}

bool canyonTerrainBlock_render( canyonTerrainBlock* b, scene* s ) {
	// If we have new render buffers, free the old ones and switch to the new
	if (( b->vertex_VBO_alt && *b->vertex_VBO_alt ) && ( b->element_VBO_alt && *b->element_VBO_alt )) {
		render_freeBuffer( b->vertex_VBO );
		render_freeBuffer( b->element_VBO );

		b->vertex_VBO = b->vertex_VBO_alt;
		b->element_VBO = b->element_VBO_alt;
		b->element_count_render = b->element_count;
		b->vertex_VBO_alt = NULL;
		b->element_VBO_alt = NULL;
	}

	vector frustum[6];
	camera_calculateFrustum( s->cam, frustum );
	if ( frustum_cull( &b->bb, frustum ) )
		return false;

	int zone = b->canyon->current_zone;
	int first = ( zone + zone % 2 ) % b->canyon->zone_count;
	int second = ( zone + 1 - (zone % 2)) % b->canyon->zone_count;
	if ( b->vertex_VBO && *b->vertex_VBO && terrain_texture && terrain_texture_cliff ) {
		drawCall* draw = drawCall_create( &renderPass_main, resources.shader_terrain, b->element_count_render, b->element_buffer, b->vertex_buffer, b->canyon->zones[first].texture_ground->gl_tex, modelview );
		draw->texture_b = b->canyon->zones[first].texture_cliff->gl_tex;
		draw->texture_c = b->canyon->zones[second].texture_ground->gl_tex;
		draw->texture_d = b->canyon->zones[second].texture_cliff->gl_tex;
		draw->vertex_VBO = *b->vertex_VBO;
		draw->element_VBO = *b->element_VBO;
	}
	return true;
}


void canyonTerrain_render( void* data, scene* s ) {
	(void)s;
	canyonTerrain* t = data;
	render_resetModelView();
	matrix_mulInPlace( modelview, modelview, t->trans->world );

	int count = 0;
	for ( int i = 0; i < t->total_block_count; ++i ) {
		count += canyonTerrainBlock_render( t->blocks[i], s );
	}
}

#endif // CANYON_TERRAIN_INDEX
