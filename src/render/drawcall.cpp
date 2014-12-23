// drawcall.cpp
#include "common.h"
#include "render/drawcall.h"
//---------------------
#include "maths/mathstypes.h"
#include "maths/matrix.h"
#include "render/render.h"

drawCall::drawCall( shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv ) {
	vAssert( vshader );

	vitae_shader = vshader;
	element_buffer = elements;
	vertex_buffer = verts;
	element_count = count;
	texture = tex;
	element_buffer_offset = 0;
	vertex_VBO	= resources.vertex_buffer;
	element_VBO	= resources.element_buffer;
	depth_mask = GL_TRUE;
	elements_mode = GL_TRIANGLES;

	matrix_cpy( modelview, mv );
}

/*
drawCall* drawCall_createCached( renderPass* pass, shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv ) {
	vAssert( pass );
	vAssert( vshader );

	drawCall* draw = (drawCall*)mem_alloc( sizeof( drawCall ));
	draw->vitae_shader = vshader;
	draw->element_buffer = elements;
	draw->vertex_buffer = verts;
	draw->element_count = count;
	draw->texture = tex;
	draw->element_buffer_offset = 0;
	draw->vertex_VBO	= resources.vertex_buffer;
	draw->element_VBO	= resources.element_buffer;
	draw->depth_mask = GL_TRUE;
	draw->elements_mode = GL_TRIANGLES;

	matrix_cpy( draw->modelview, mv );
	return draw;
}

*/
drawCall* drawCall::call( renderPass* pass, shader* vshader, matrix mv ) {
	vAssert( pass );
	vAssert( vshader );

	// TODO - should this be per-pass?
	int buffer = render_findDrawCallBuffer( vshader ); // TODO - optimize findDrawCallBuffer - better hashing
	int call = pass->next_call_index[buffer]++;
	vAssert( call < kMaxDrawCalls );

	drawCall* draw = &pass->call_buffer[buffer][call];
	memcpy( draw, this, sizeof( drawCall ));

	matrix_cpy( draw->modelview, mv );
	return draw;
}

/*
drawCall* drawCall_create( renderPass* pass, shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv ) {
	vAssert( pass );
	vAssert( vshader );

	int buffer = render_findDrawCallBuffer( vshader );
	int call = pass->next_call_index[buffer]++;
	vAssert( call < kMaxDrawCalls );
	drawCall* draw = &pass->call_buffer[buffer][call];

	draw->vitae_shader = vshader;
	draw->element_buffer = elements;
	draw->vertex_buffer = verts;
	draw->element_count = count;
	draw->texture = tex;
	draw->element_buffer_offset = 0;
	draw->vertex_VBO	= resources.vertex_buffer;
	draw->element_VBO	= resources.element_buffer;
	draw->depth_mask = GL_TRUE;
	draw->elements_mode = GL_TRIANGLES;

	matrix_cpy( draw->modelview, mv );
	return draw;
}
*/
