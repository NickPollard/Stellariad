// render.h
#pragma once
#include "scene.h"
#include "system/thread.h"
#include "render/drawcall.h"
#include "render/shader_attributes.h"
#include <memory> // unique_ptr

// External
#include "EGL/egl.h"

#define kVboCount 1
#define kInvalidBuffer 0

// *** Forward Declaration
typedef struct sceneParams_s sceneParams;

using std::unique_ptr;
using std::move;

#define kMaxDrawCalls 2048
#define kCallBufferCount 24		// Needs to be at least as many as we have shaders

void render_drawPass( window* w, const renderPass& pass );

// Wrapper around a sequence of renderpasses
struct PassSequence {
	unique_ptr<PassSequence> prev;
	renderPass& ths;

	PassSequence(renderPass& p, PassSequence* s) : prev(s), ths(p) {}
	PassSequence(PassSequence&& p) : prev(move(p.prev)), ths(p.ths) { }

	PassSequence operator >> (renderPass& p) {
		return PassSequence(p, new PassSequence(move(*this)));
	}

	void render(window* w);
};

struct renderPass {
	bool		alphaBlend;
	bool		colorMask;
	bool		depthMask;
	bool		depthTest;
	bool		clearDepth;
	drawCall	call_buffer[ kCallBufferCount ][ kMaxDrawCalls ];
	int			next_call_index[ kCallBufferCount ];
	renderPass* next;

	renderPass() : clearDepth(false), next(NULL) {}
	renderPass(renderPass* n) : next(n) {}
	renderPass& alpha(bool a) { alphaBlend = a; return *this; }
	renderPass& color(bool c) { colorMask = c; return *this; }
	renderPass& depth(bool d) { depthMask = d; return *this; }
	renderPass& test(bool t) { depthTest = t; return *this; }
	renderPass& clear(bool c) { clearDepth = c; return *this; }

	PassSequence operator >> (renderPass& n) {
		PassSequence* p = new PassSequence(*this, NULL);
		return PassSequence(n, p);
	}

	void render(window* w);
};

//PassSequence::PassSequence(const PassSequence&& p) {
//		ths = p.ths;
//		prev = p.prev;
//}
//PassSequence::PassSequence(const PassSequence&& p) : prev(p.prev), ths (p.ths) {
//}




struct RenderResources {
	// Default general buffers
	GLuint vertex_buffer;
	GLuint element_buffer;

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

/*
 *
 *  Static Functions
 *
 */

void render_set2D();

void render_set3D( int w, int h );

void render_applyCamera(camera* cam);

// Initialise the 3D rendering
void render_init( void* app );

// Terminate the 3D rendering
void render_terminate();

// Render the current scene
// This is where the business happens
void render( window* w, scene* s );

void render_resetModelView( );

void render_validateMatrix( matrix m );

// *** The Rendering Thread itself
void* render_renderThreadFunc( void* args );

// *** Manage render windows
void render_destroyWindow( window* w );

int render_findDrawCallBuffer( shader* vshader );
