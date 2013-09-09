// render.h
#pragma once
#include "scene.h"
#include "system/thread.h"

// External
#include "EGL/egl.h"

#define kVboCount 1
#define kInvalidBuffer 0

typedef struct renderPass_s renderPass;
typedef struct sceneParams_s sceneParams;

#define SHADER_UNIFORMS( f ) \
	f( projection ) \
	f( modelview ) \
	f( worldspace ) \
	f( light_position ) \
	f( light_diffuse ) \
	f( light_specular ) \
	f( tex ) \
	f( tex_b ) \
	f( tex_c ) \
	f( tex_d ) \
	f( tex_normal ) \
	f( tex_b_normal ) \
	f( fog_color ) \
	f( sky_color_top ) \
	f( sky_color_bottom ) \
	f( camera_space_sun_direction ) \
	f( sun_color ) \
	f( viewspace_up ) \
	f( directional_light_direction ) \
	f( screen_size )

#define DECLARE_AS_GLINT_P( var ) \
	GLint* var;

#define VERTEX_ATTRIBS( f ) \
	f( position ) \
	f( normal ) \
	f( uv ) \
	f( color )

#define VERTEX_ATTRIB_DISABLE_ARRAY( attrib ) \
	glDisableVertexAttribArray( *resources.attributes.attrib );

#define VERTEX_ATTRIB_LOOKUP( attrib ) \
	resources.attributes.attrib = (shader_findConstant( mhash( #attrib )));

#define VERTEX_ATTRIB_POINTER( attrib ) \
	glVertexAttribPointer( *resources.attributes.attrib, /*vec4*/ 4, GL_FLOAT, /*Normalized?*/GL_FALSE, sizeof( vertex ), (void*)offsetof( vertex, attrib) ); \
	glEnableVertexAttribArray( *resources.attributes.attrib );

typedef struct gl_resources_s {
	GLuint vertex_buffer[kVboCount];
	GLuint element_buffer[kVboCount];
	GLuint texture;

	struct {
		SHADER_UNIFORMS( DECLARE_AS_GLINT_P )
	} uniforms;
	struct {
		SHADER_UNIFORMS( DECLARE_AS_GLINT_P )
	} particle_uniforms;

	struct {
		VERTEX_ATTRIBS( DECLARE_AS_GLINT_P )
			/*
		GLint position;
		GLint normal;
		GLint uv;
		*/
	} attributes;

	// Shader objects
	GLuint vertex_shader, fragment_shader, program;
	GLuint particle_vertex_shader, particle_fragment_shader, particle_program;

	shader* shader_default;
	shader* shader_particle;
	shader* shader_terrain;
	shader* shader_skybox;
	shader* shader_ui;
	shader* shader_filter;
	shader* shader_debug;
	shader* shader_debug_2d;
	shader* shader_depth;
} gl_resources;

struct vertex_s {
	vector	position;
	vector	normal;
	vector	uv;
	vector	color;
	float	padding;
};

typedef struct vertex_s particle_vertex;

struct window_s {
	int width;
	int height;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	bool open;
};

extern gl_resources resources;
extern matrix modelview;
extern matrix camera_inverse;
extern matrix camera_mtx;
extern matrix perspective;
extern bool	render_initialised;
extern vmutex	gl_mutex;
extern renderPass renderPass_main;
extern renderPass renderPass_depth;
extern renderPass renderPass_alpha;
extern renderPass renderPass_ui;
extern renderPass renderPass_debug;
extern sceneParams sceneParams_main;
extern window window_main;

void render_setBuffers( float* vertex_buffer, int vertex_buffer_size, int* element_buffer, int element_buffer_size );

/*
 *
 *  Static Functions
 *
 */

void render_set2D();

void render_set3D( int w, int h );

void render_clear();

void render_swapBuffers();

// Iterate through each model in the scene
// Translate by their transform
// Then draw all the submeshes
void render_scene(scene* s);

void render_lighting(scene* s);

void render_applyCamera(camera* cam);

// Initialise the 3D rendering
void render_init( void* app );

// Terminate the 3D rendering
void render_terminate();

// Render the current scene
// This is where the business happens
void render( scene* s );

void render_resetModelView( );
void render_setUniform_matrix( GLuint uniform, matrix m );
void render_setUniform_texture( GLuint uniform, GLuint texture );
void render_setUniform_vector( GLuint uniform, vector* v );

void render_validateMatrix( matrix m );

typedef void (*func_getIV)( GLuint, GLenum, GLint* );
typedef void (*func_getInfoLog)( GLuint, GLint, GLint*, GLchar* );

void gl_dumpInfoLog( GLuint object, func_getIV getIV, func_getInfoLog getInfoLog );
GLuint render_glBufferCreate( GLenum target, const void* data, GLsizei size );

// Asynchronosuly create a GPU buffer
GLuint* render_requestBuffer( GLenum target, const void* data, GLsizei size );
// Free it
void render_freeBuffer( void* buffer );

// Asynchronously copy data to a GPU  buffer
void render_bufferCopy( GLenum target, GLuint buffer, const void* data, GLsizei size );

// Draw Calls

typedef struct drawCall_s {
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
} drawCall;

drawCall* drawCall_create( renderPass* pass, shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv );
void render_drawCall( drawCall* draw );
void* render_bufferAlloc( size_t size );

//
// *** The Rendering Thread itself
//
void* render_renderThreadFunc( void* args );

// Manage render windows
void render_destroyWindow( window* w );

