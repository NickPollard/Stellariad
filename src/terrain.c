// terrain.c
#include "common.h"
#include "terrain.h"
//-----------------------
#include "mem/allocator.h"
#include "render/render.h"
#include "render/shader.h"
#include "render/texture.h"
#include "system/hash.h"

// Create a procedural terrain
terrain* terrain_create() {
	terrain* t = mem_alloc( sizeof( terrain ));
	memset( t, 0, sizeof( terrain ));
	t->u_samples = 1;
	t->v_samples = 1;
	return t;
}

// The procedural function
float terrain_sample( float u, float v ) {
//	return sin( u ) * sin( v );

	return (
			0.5 * sin( u ) * sin( v ) +
			sin( u / 3.f ) * sin( v / 3.f )
		   );
}

void terrain_updateIntervals( terrain* t ) {
	t->u_interval = t->u_radius / ((float)(t->u_samples - 1) * 0.5);
	t->v_interval = t->v_radius / ((float)(t->v_samples - 1) * 0.5);
}

void terrain_setSize( terrain* t, float u, float v ) {
	t->u_radius = u;
	t->v_radius = v;
	terrain_updateIntervals( t );
}

void terrain_setResolution( terrain* t, int u, int v ) {
	t->u_samples = u;
	t->v_samples = v;
	terrain_updateIntervals( t );
}

// Calculate vertex and element buffers from procedural definition
void terrain_calculateBuffers( terrain* t ) {

/*
	We have a grid of x * y points
	So (x-1) * (y-1) quads
	So (x-1) * (y-1) * 2 tris
	So (x-1) * (y-1) * 6 indices
	*/

	int triangle_count = ( t->u_samples - 1 ) * ( t->v_samples - 1 ) * 2;
	t->index_count = triangle_count * 3;
	int vert_count = t->u_samples * t->v_samples;

	if ( !t->vertex_buffer ) {
		t->vertex_buffer = mem_alloc( t->index_count * sizeof( vertex ));
	}
	if ( !t->element_buffer ) {
		t->element_buffer = mem_alloc( t->index_count * sizeof( GLushort ));
	}

	vector* verts = mem_alloc( vert_count * sizeof( vector ));

/*
	We want to draw the mesh from ( -u_radius, -v_radius ) to ( u_radius, v_radius )
	We sample at an interval of ( u_interval, v_interval )
   */

	int vert_index = 0;
	for ( float u = -t->u_radius; u < t->u_radius + ( 0.5 * t->u_interval ); u+= t->u_interval ) {
		for ( float v = -t->v_radius; v < t->v_radius + ( 0.5 * t->v_interval ); v+= t->v_interval ) {
			float h = terrain_sample( u, v );
			verts[vert_index++] = Vector( u, h, v, 1.f );
			vAssert( u != 0.f || v != 0.f || h != 0.f );
			
			printf( "vert %d: Vert:", vert_index-1 );
			vector_print( &verts[vert_index-1] );
			printf( "\n" );
		}
	}

	assert( vert_index == vert_count );

	int element_count = 0;
	// Calculate elements
	int tw = ( t->u_samples - 1 ) * 2; // Triangles per row
	for ( int i = 0; i < triangle_count; i++ ) {
		GLushort tl = ( i / 2 ) + i / tw;
		GLushort tr = tl + 1;
		GLushort bl = tl + t->u_samples;
		GLushort br = bl + 1;

		vAssert( tl >= 0 );
		vAssert( tr >= 0 );
		vAssert( bl >= 0 );
		vAssert( br >= 0 );

		vAssert( tl < vert_count );
		vAssert( tr < vert_count );
		vAssert( bl < vert_count );
		vAssert( br < vert_count );

		if ( i % 2 ) {
			// bottom right triangle - br, bl, tr
			t->element_buffer[element_count++] = br;
			t->element_buffer[element_count++] = bl;
			t->element_buffer[element_count++] = tr;
		}
		else {
			// top left triangle - tl, tr, bl
			t->element_buffer[element_count++] = tl;
			t->element_buffer[element_count++] = tr;
			t->element_buffer[element_count++] = bl;
		}
	}
/*
	// Test print
	for ( int i = 0; i < t->index_count; i++ ) {
		printf( "Index %d: %u.\n", i, t->element_buffer[i] );
	}
*/
	// For each element index
	// Unroll the vertex/index bindings
	for ( int i = 0; i < t->index_count; i++ ) {
		// Copy the required vertex position, normal, and uv
		t->vertex_buffer[i].position = verts[t->element_buffer[i]];
		t->vertex_buffer[i].normal = Vector( 0.f, 1.f, 0.f, 0.f );
		t->vertex_buffer[i].uv = verts[t->element_buffer[i]];
		t->element_buffer[i] = i;
	}
/*	
	// Test print
	for ( int i = 0; i < t->index_count; i++ ) {
		printf( "Index %d: Vert:", i );
		vector_print( &t->vertex_buffer[i].position );
		printf( "\n" );
	}
*/
	mem_free( verts );
}

// Send the buffers to the GPU
void terrain_render( void* data ) {
	terrain* t = data;
	glDepthMask( GL_TRUE );
	// Copy our data to the GPU
	// There are now <index_count> vertices, as we have unrolled them
	GLsizei vertex_buffer_size = t->index_count * sizeof( vertex );
	GLsizei element_buffer_size = t->index_count * sizeof( GLushort );

	// Textures
	GLint* tex = shader_findConstant( mhash( "tex" ));
	if ( tex )
		render_setUniform_texture( *tex, g_texture_default );

	VERTEX_ATTRIBS( VERTEX_ATTRIB_LOOKUP );
	// *** Vertex Buffer
	glBindBuffer( GL_ARRAY_BUFFER, resources.vertex_buffer );
	glBufferData( GL_ARRAY_BUFFER, vertex_buffer_size, t->vertex_buffer, GL_DYNAMIC_DRAW );// OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW
	VERTEX_ATTRIBS( VERTEX_ATTRIB_POINTER );
	// *** Element Buffer
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, element_buffer_size, t->element_buffer, GL_DYNAMIC_DRAW ); // OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW

	// Draw!
	glDrawElements( GL_TRIANGLES, t->index_count, GL_UNSIGNED_SHORT, (void*)0 );

	// Cleanup
	VERTEX_ATTRIBS( VERTEX_ATTRIB_DISABLE_ARRAY )
}

void terrain_delete( terrain* t ) {
	if ( t->vertex_buffer )
		mem_free( t->vertex_buffer );
	if ( t->element_buffer )
		mem_free( t->element_buffer );
	mem_free( t );
}

void test_terrain() {
	terrain* t = terrain_create();
	terrain_setSize( t, 15.f, 15.f );
	terrain_setResolution( t, 30, 30 );
	terrain_calculateBuffers( t );
	terrain_delete( t );
//	vAssert( 0 );
}
