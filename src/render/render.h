// render.h
#pragma once
#include "scene.h"

#define SHADER_UNIFORMS( f ) \
	f( projection ) \
	f( modelview ) \
	f( worldspace ) \
	f( light_position ) \
	f( light_diffuse ) \
	f( light_specular )


#define DECLARE_AS_GLINT_P( var ) \
	GLint* var;

#define VERTEX_ATTRIBS( f ) \
	f( position ) \
	f( normal ) \
	f( uv )

#define VERTEX_ATTRIB_DISABLE_ARRAY( attrib ) \
	glDisableVertexAttribArray( attrib );

#define VERTEX_ATTRIB_LOOKUP( attrib ) \
	GLint attrib = *(shader_findConstant( mhash( #attrib )));

#define VERTEX_ATTRIB_POINTER( attrib ) \
	glVertexAttribPointer( attrib, /*vec4*/ 4, GL_FLOAT, /*Normalized?*/GL_FALSE, sizeof( vertex ), (void*)offsetof( vertex, attrib) ); \
	glEnableVertexAttribArray( attrib );

typedef struct gl_resources_s {
	GLuint vertex_buffer, element_buffer;
	GLuint texture;

	struct {
		SHADER_UNIFORMS( DECLARE_AS_GLINT_P )
	} uniforms;
	struct {
		SHADER_UNIFORMS( DECLARE_AS_GLINT_P )
	} particle_uniforms;

	struct {
		GLint position;
		GLint normal;
		GLint uv;
	} attributes;

	// Shader objects
	GLuint vertex_shader, fragment_shader, program;
	GLuint particle_vertex_shader, particle_fragment_shader, particle_program;

	shader* shader_default;
	shader* shader_particle;
	shader* shader_terrain;
} gl_resources;

extern gl_resources resources;
extern matrix modelview;
extern matrix camera_inverse;
extern matrix perspective;

void render_setBuffers( float* vertex_buffer, int vertex_buffer_size, int* element_buffer, int element_buffer_size );

/*
 *
 *  Static Functions
 *
 */

void render_set2D();

void render_set3D( int w, int h );

void render_clear();

#ifdef ANDROID
void render_swapBuffers( egl_renderer* egl );
#else
void render_swapBuffers();
#endif

// Iterate through each model in the scene
// Translate by their transform
// Then draw all the submeshes
void render_scene(scene* s);

void render_lighting(scene* s);

void render_applyCamera(camera* cam);

// Initialise the 3D rendering
void render_init();

// Terminate the 3D rendering
void render_terminate();

// Render the current scene
// This is where the business happens
void render( scene* s , int w, int h );

void render_resetModelView( );
void render_setUniform_matrix( GLuint uniform, matrix m );
void render_setUniform_texture( GLuint uniform, GLuint texture );

void render_validateMatrix( matrix m );

typedef void (*func_getIV)( GLuint, GLenum, GLint* );
typedef void (*func_getInfoLog)( GLuint, GLint, GLint*, GLchar* );

void gl_dumpInfoLog( GLuint object, func_getIV getIV, func_getInfoLog getInfoLog );
