// shader.h
#pragma once

#include "render/render.h"

// The maximum number of constants that a single shader can use
#define kMaxShaderConstantBindings 32

// Value signifying shader constant is unbound (and hence should not be set for this shader)
#define kShaderConstantNoLocation 0xffffffff

enum uniform_type {
	uniform_unknown,
	uniform_matrix,
	uniform_vector,
	uniform_tex2D,
	uniform_int
};

typedef struct shaderConstantBinding_s {
	GLint*		value;
	GLint		program_location;
	int			type;
} shaderConstantBinding;

typedef struct shaderDictionary_s {
	int count;
	shaderConstantBinding bindings[kMaxShaderConstantBindings];
} shaderDictionary;

struct shader_s {
	GLuint program;				// The Linked OpenGL shader program, containing vertex and fragment shaders;
	shaderDictionary	dict;	// Dictionary of shader constant lookups
};

struct shaderInfo_s {
	const char* name;
	const char* fragment;
	const char* vertex;
};

typedef struct shaderInfo_s shaderInfo;

// *** Static

void shader_init();


// **** Member

// Compile a GLSL shader object from the given source code
GLuint shader_compile( GLenum type, const char* path, const char* source );

// Load a shader from GLSL files
shader* shader_load( const char* vertex_name, const char* fragment_name );
// * NEW
shader** shaderGet( const char* shaderName );
void shaderLoad( const char* shader, bool required );
void shadersReloadAll();

// Find the program location for a named Uniform variable in the given program
GLint shader_getUniformLocation( GLuint program, const char* name );

// Find the program location for a named Attribute variable in the given program
GLint shader_getAttributeLocation( GLuint program, const char* name );

// Activate the shader for use in rendering
void shader_activate( shader* s );

GLint* shader_findConstant( int key );
