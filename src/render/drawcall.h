#pragma once
#include "maths/matrix.h"
#include "render/vgl.h"

struct renderPass;

// Draw Calls
struct drawCall {
	shader*		vitae_shader;
	// Uniforms
	matrix		modelview;
	GLuint		texture;
	GLuint		texture_b;
	GLuint		texture_c;
	GLuint		texture_d;
	GLuint		texture_normal;
	GLuint		texture_b_normal;
	GLuint		texture_lookup;

	// Buffer data
	GLushort*	element_buffer;
	vertex*		vertex_buffer;

	GLuint		vertex_VBO;
	GLuint		element_VBO;
	uint16_t	element_count;
	uint16_t	element_buffer_offset;

	// *** Render flags - these could be packed in a byte/int maybe?
	GLenum		depth_mask;
	GLenum		elements_mode;
	
	drawCall() {}
	drawCall( shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv );

	static drawCall* createCached( shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv );
	static drawCall* create( renderPass* pass, shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv );
	drawCall* call( renderPass* pass, shader* vshader, matrix mv );

	drawCall* mode( GLenum elements_mode );
	drawCall* depth( bool depth );
};
