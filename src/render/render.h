// render.h
#pragma once
#include "scene.h"
#include "system/thread.h"
#include "render/shader_attributes.h"

// External
#include "EGL/egl.h"

#define kVboCount 1
#define kInvalidBuffer 0

// *** Forward Declaration
struct drawCall;

typedef struct renderPass_s renderPass;
typedef struct sceneParams_s sceneParams;

struct RenderResources {
	// TODO - do these even need to be arrays now? We spawn individual buffers for models, only one general one
	GLuint vertex_buffer[kVboCount];
	GLuint element_buffer[kVboCount];

	ShaderUniforms uniforms;
	VertexAttribs attributes;
};

struct vertex_s {
	vector		position; // 4 x 32bit = 128bit
	vector		normal;		// 4 x 32bit = 128bit
	vec2			uv;				// 2 x 32bit = 64bit // Maybe 2 x 16bit?
	uint32_t	color;		// 4 x 8bit = 32bit
	// TODO - Should these be smaller than 32bit floats in these vectors? UVs/Colors at least? (And normals?)
	// What's the smallest feasible size? 10x 32bit?
};

struct window_s {
	int width;
	int height;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	bool open;
};

// *** Properties of the GL implementation/Hardware/Device we are running on
struct GraphicsSystem {
	GLint maxTextureUnits;
	GLint maxVaryingParams;
};

// *** Parameters for the whole render scene
struct sceneParams_s {
	vector	fog_color;
	vector	sky_color;
	vector	sun_color;
};

extern RenderResources resources;
extern matrix modelview;
extern matrix camera_inverse;
extern matrix camera_mtx;
extern matrix perspective;
extern bool	render_initialised;
extern renderPass renderPass_main;
extern renderPass renderPass_depth;
extern renderPass renderPass_alpha;
extern renderPass renderPass_ui;
extern renderPass renderPass_debug;
extern sceneParams sceneParams_main;
extern window window_main;
extern bool render_bloom_enabled;
extern int render_current_texture_unit;
extern GraphicsSystem graphicsSystem;

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
void render( window* w, scene* s );

void render_resetModelView( );
void render_setUniform_matrix( GLuint uniform, matrix m );
void render_setUniform_texture( GLuint uniform, GLuint texture );
void render_setUniform_vector( GLuint uniform, vector* v );

void render_validateMatrix( matrix m );

// TODO - move this debugging stuff out somewhere?
typedef void (*func_getIV)( GLuint, GLenum, GLint* );
typedef void (*func_getInfoLog)( GLuint, GLint, GLint*, GLchar* );
void gl_dumpInfoLog( GLuint object, func_getIV getIV, func_getInfoLog getInfoLog );

void* render_bufferAlloc( size_t size );

//
// *** The Rendering Thread itself
//
void* render_renderThreadFunc( void* args );

// Manage render windows
void render_destroyWindow( window* w );

// *** drawCalls
drawCall* drawCall_create( renderPass* pass, shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv );
void render_drawCall( drawCall* draw );
drawCall* drawCall_createCached( renderPass* pass, shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv );
drawCall* drawCall_callCached( renderPass* pass, shader* vshader, drawCall* cached, matrix mv );
