// drawcall.cpp
#include "common.h"
#include "render/drawcall.h"
//---------------------
#include "maths/mathstypes.h"
#include "maths/matrix.h"
#include "render/render.h"
#include <new>

/*
   Right now, drawCall creates by default in the render drawCall arrrays, which are 
   automatically wiped each frame (ephemeral).
   drawCall_createCached creates a heap-allocated drawCall, that can then be sent
   to the render each frame without needing recreation (so just a memcpy)
   */

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

drawCall* drawCall::createCached( shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv ) {
	void* data = mem_alloc( sizeof( drawCall ));
	drawCall* draw = new(data) drawCall( vshader, count, elements, verts, tex, mv );
	return draw;
}

drawCall* drawCall::create( renderPass* pass, shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv ) {
	int buffer = render_findDrawCallBuffer( vshader );
	int call = pass->next_call_index[buffer]++;
	vAssert( call < kMaxDrawCalls );
	void* data = (void*)&pass->call_buffer[buffer][call];
	drawCall* draw = new(data) drawCall( vshader, count, elements, verts, tex, mv );
	return draw;
}

drawCall* drawCall::call( renderPass* pass, shader* vshader, matrix mv ) {
	vAssert( pass );
	vAssert( vshader );

	int buffer = render_findDrawCallBuffer( vshader ); // TODO - optimize findDrawCallBuffer - better hashing
	int call = pass->next_call_index[buffer]++;
	vAssert( call < kMaxDrawCalls );

	drawCall* draw = &pass->call_buffer[buffer][call];
	memcpy( draw, this, sizeof( drawCall ));

	matrix_cpy( draw->modelview, mv );
	return draw;
}

GLenum glBool( bool b ) { return b ? GL_TRUE : GL_FALSE; }

drawCall* drawCall::mode( GLenum elements_mode ) { this->elements_mode = elements_mode; return this; }
drawCall* drawCall::depth( bool depth ) { this->depth_mask = glBool(depth); return this; }
