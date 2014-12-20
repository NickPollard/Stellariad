#pragma once

#include "render/vgl.h"

// Draw Calls
struct drawCall {
	// Shader
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
	vector		fog_color;

	// Buffer data
	GLushort*	element_buffer;
	vertex*		vertex_buffer;

	GLuint		vertex_VBO;
	GLuint		element_VBO;
	unsigned int	element_count;
	unsigned int	element_buffer_offset;
	GLenum		depth_mask;
	GLenum		elements_mode;
};
