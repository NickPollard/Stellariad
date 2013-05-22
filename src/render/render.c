// render.c

#include "common.h"
#include "render.h"
//-----------------------
#include "camera.h"
#include "font.h"
#include "light.h"
#include "input.h" // TODO - remove
#include "model.h"
#include "scene.h"
#include "skybox.h"
#include "vtime.h"
#include "debug/debuggraph.h"
#include "maths/vector.h"
#include "render/debugdraw.h"
#include "render/modelinstance.h"
#include "render/shader.h"
#include "render/texture.h"
#include "system/file.h"
#include "system/hash.h"
// temp
#include "engine.h"

#ifdef LINUX_X
#include <X11/Xlib.h>
#endif

// Rendering API declaration
#ifdef ANDROID
#define RENDER_OPENGL_ES
#define RENDER_GL_API EGL_OPENGL_ES_API
#endif

#ifdef LINUX_X
#define RENDER_OPENGL
#define RENDER_GL_API EGL_OPENGL_API
#endif

bool	render_initialised = false;

//#define GRAPH_GPU_FPS
#ifdef GRAPH_GPU_FPS
graph* gpu_fpsgraph; 
graphData* gpu_fpsdata;
frame_timer* gpu_fps_timer = NULL;
#endif // GRAPH_GPU_FPS

#define MAX_VERTEX_ARRAY_COUNT 1024

// *** Shader Pipeline

matrix modelview, camera_mtx, camera_inverse;
matrix perspective;
vector viewspace_up;
vector directional_light_direction;

gl_resources resources;

window window_main = { 1280, 720, 0, 0, 0, true };

GLuint render_glBufferCreate( GLenum target, const void* data, GLsizei size ) {
	//printf( "Allocating oGL buffer.\n" );
	GLuint buffer; // The OpenGL object handle we generate
	glGenBuffers( 1, &buffer );				// Generate a buffer name - effectively just a declaration
	glBindBuffer( target, buffer );			// Bind the buffer name to a target, creating the vertex buffer object
	// Usage hint can be: GL_[VARYING]_[USE]
	// Varying: STATIC / DYNAMIC / STREAM
	// Use: DRAW / READ / COPY
	// OpenGL ES only supports dynamic/static draw
	glBufferData( target, size, data, /*Usage hint*/ GL_STATIC_DRAW );	// Allocate the buffer, optionally copying data
	//glFinish();
	return buffer;
}

typedef struct bufferRequest_s {
	GLenum		target;
	const void*	data;
	GLsizei		size;
	GLuint*		ptr;
} bufferRequest;

typedef struct bufferCopyRequest_s {
	GLuint		buffer;
	GLenum		target;
	const void*	data;
	GLsizei		size;
} bufferCopyRequest;

#define	kMaxBufferRequests	512
bufferRequest	buffer_requests[kMaxBufferRequests];
int				buffer_request_count = 0;

#define	kMaxBufferCopyRequests	128
bufferCopyRequest	buffer_copy_requests[kMaxBufferCopyRequests];
int				buffer_copy_request_count = 0;

// Mutex for buffer requests
vmutex buffer_mutex = kMutexInitialiser;

bufferRequest* getBufferRequest() {
	vAssert( buffer_request_count < kMaxBufferRequests );
	bufferRequest* r = &buffer_requests[buffer_request_count++];
	return r;
}

bufferCopyRequest* getBufferCopyRequest() {
	vAssert( buffer_copy_request_count < kMaxBufferCopyRequests );
	bufferCopyRequest* r = &buffer_copy_requests[buffer_copy_request_count++];
	return r;
}

// Asynchronously copy data to a VertexBufferObject
void render_bufferCopy( GLenum target, GLuint buffer, const void* data, GLsizei size ) {
	bufferCopyRequest* b = NULL;
	vmutex_lock( &buffer_mutex );
	{
		b = getBufferCopyRequest();
		vAssert( b );
		b->buffer	= buffer;
		b->target	= target;
		b->data		= data;
		b->size		= size;
	}
	vmutex_unlock( &buffer_mutex );
}

// Asynchronously create a VertexBufferObject
GLuint* render_requestBuffer( GLenum target, const void* data, GLsizei size ) {
//	printf( "RENDER: Buffer requested.\n" );
	bufferRequest* b = NULL;
	vmutex_lock( &buffer_mutex );
	{
		b = getBufferRequest();
		vAssert( b );
		b->target	= target;
		b->data		= data;
		b->size		= size;
		// Needs to allocate a GLuint somewhere
		// and return a pointer to that
		b->ptr = mem_alloc( sizeof( GLuint ));
		// Initialise this to 0, so we can ignore ones that haven't been set up yet
		*(b->ptr) = kInvalidBuffer;
	}
	vmutex_unlock( &buffer_mutex );
	return b->ptr;
}

void render_freeBuffer( void* buffer ) {
	mem_free( buffer );
}

// Load any waiting buffer requests
void render_bufferTick() {
	vmutex_lock( &buffer_mutex );
	{
		// TODO: This could be a lock-free queue
		for ( int i = 0; i < buffer_request_count; i++ ) {
			bufferRequest* b = &buffer_requests[i];
			*b->ptr = render_glBufferCreate( b->target, b->data, b->size );
			//printf( "Created buffer %x for request for %d bytes.\n", *b->ptr, b->size );
		}
		buffer_request_count = 0;
		
		for ( int i = 0; i < buffer_copy_request_count; i++ ) {
			bufferCopyRequest* b = &buffer_copy_requests[i];
			glBindBuffer( b->target, b->buffer );
			int origin = 0; // We're copyping the whole buffer
			glBufferSubData( b->target, origin, b->size, b->data );
			//printf( "Created buffer %x for request for %d bytes.\n", *b->ptr, b->size );
		}
		buffer_copy_request_count = 0;
	}
	vmutex_unlock( &buffer_mutex );
}

EGLNativeWindowType os_createWindow() {
#ifdef LINUX_X
	// Get the XServer display
	Display* display	= XOpenDisplay(NULL);

	int x = 0, y = 0;
	int border_width = 0;

	int white_color = XWhitePixel( display, 0 );
	int black_color = XBlackPixel( display, 0 );

	// Create the window
	Window window = XCreateSimpleWindow( display, DefaultRootWindow( display ), 
			x, y, 
			window_main.width, window_main.height,
		   	border_width, 
			black_color, black_color );

	// We want to get MapNotify events
	XSelectInput( display, window, ButtonPressMask|KeyPressMask|KeyReleaseMask|KeymapStateMask|StructureNotifyMask );

	// Setup client messaging to receive a client delete message
	Atom wm_delete=XInternAtom( display, "WM_DELETE_WINDOW", true );
	XSetWMProtocols( display, window, &wm_delete, 1 );

	GC gc = XCreateGC( display, window, 0, NULL );
	XSetForeground( display, gc, white_color );

	XStoreName( display, window, "Vitae");
	XMapWindow( display, window );

	// Wait for the MapNotify event
	for(;;) {
		XEvent e;
		XNextEvent( display, &e );
		if ( e.type == MapNotify )
			break;
	}

	xwindow_main.display = display;
	xwindow_main.window = window;
	xwindow_main.open = true;

	// TODO - this shouldn't happen here.
	// Window creation needs to be moved out of Render
	input_initKeyCodes( &xwindow_main );

	return window;
#endif
}

// Tear down the EGL context currently associated with the display.
void render_destroyWindow( window* w ) {
    if ( w->display != EGL_NO_DISPLAY ) {
        eglMakeCurrent( w->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
        if ( w->context != EGL_NO_CONTEXT ) {
            eglDestroyContext( w->display, w->context );
        }
        if ( w->surface != EGL_NO_SURFACE) {
            eglDestroySurface( w->display, w->surface );
        }
        eglTerminate( w->display );
    }
    w->display = EGL_NO_DISPLAY;
    w->context = EGL_NO_CONTEXT;
    w->surface = EGL_NO_SURFACE;
}

EGLNativeDisplayType render_getDefaultOSDisplay() {
#ifdef LINUX_X
	return XOpenDisplay(NULL);
#endif // LINUX_X
#ifdef ANDROID
	return EGL_DEFAULT_DISPLAY;
#endif // ANDROID
}

void render_createWindow( void* app, window* w ) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
#ifdef RENDER_OPENGL_ES
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
			EGL_DEPTH_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };

	// Ask for a GLES2 context	
	const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2, 
		EGL_NONE
	};
#endif // OPENGL_ES
#ifdef RENDER_OPENGL
	const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
			EGL_DEPTH_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, // We want OpenGL, not OpenGL_ES
            EGL_NONE
    };
	// Don't need, as we're not using OpenGL_ES
	const EGLint context_attribs[] = {
		EGL_NONE
	};
#endif // RENDER_OPENGL

    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay( render_getDefaultOSDisplay() );
	EGLint minor = 0, major = 0;
    EGLBoolean result = eglInitialize( display, &major, &minor );
	vAssert( result == EGL_TRUE );

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
	printf( "EGL Choosing Config.\n" );
    eglChooseConfig( display, attribs, &config, 1, &numConfigs );
	vAssert( result == EGL_TRUE );

	// We need to create a window first, outside EGL
#ifdef ANDROID
    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    EGLint egl_visual_id;
    eglGetConfigAttrib( display, config, EGL_NATIVE_VISUAL_ID, &egl_visual_id );
    ANativeWindow_setBuffersGeometry( ((struct android_app*)app)->window, 0, 0, egl_visual_id );
	EGLNativeWindowType native_win = ((struct android_app*)app)->window;
#endif
#ifdef LINUX_X
	(void)app;
	EGLNativeWindowType native_win = os_createWindow();
#endif

	printf( "EGL Creating Surface.\n" );
    surface = eglCreateWindowSurface( display, config, native_win, NULL );
	if ( surface == EGL_NO_SURFACE ) {
		printf( "Unable to create EGL surface (eglError: %d)\n", eglGetError() );
		vAssert( 0 );
	}
	result = eglBindAPI( RENDER_GL_API );
	vAssert( result == EGL_TRUE );
  
	printf( "EGL Creating Context.\n" );
    context = eglCreateContext(display, config, NULL, context_attribs );

    result = eglMakeCurrent(display, surface, surface, context);
	vAssert( result == EGL_TRUE );

	// Store our EGL params with out render window
	w->display = display;
	w->surface = surface;
	w->context = context;
    eglQuerySurface(display, surface, EGL_WIDTH, &w->width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &w->height);
}

void render_buildShaders() {
	// Load Shaders								Vertex								Fragment
	resources.shader_default	= shader_load( "dat/shaders/phong.v.glsl",			"dat/shaders/phong.f.glsl" );
	resources.shader_particle	= shader_load( "dat/shaders/textured_phong.v.glsl",	"dat/shaders/textured_phong.f.glsl" );
	resources.shader_terrain	= shader_load( "dat/shaders/terrain.v.glsl",		"dat/shaders/terrain.f.glsl" );
	resources.shader_skybox		= shader_load( "dat/shaders/skybox.v.glsl",			"dat/shaders/skybox.f.glsl" );
	resources.shader_ui			= shader_load( "dat/shaders/ui.v.glsl",				"dat/shaders/ui.f.glsl" );
	resources.shader_filter		= shader_load( "dat/shaders/filter.v.glsl",			"dat/shaders/filter.f.glsl" );
	resources.shader_debug		= shader_load( "dat/shaders/debug_lines.v.glsl",	"dat/shaders/debug_lines.f.glsl" );
	resources.shader_debug_2d	= shader_load( "dat/shaders/debug_lines_2d.v.glsl",	"dat/shaders/debug_lines_2d.f.glsl" );
	resources.shader_depth		= shader_load( "dat/shaders/depth_pass.v.glsl",		"dat/shaders/depth_pass.f.glsl" );

#define GET_UNIFORM_LOCATION( var ) \
	resources.uniforms.var = shader_findConstant( mhash( #var )); \
	assert( resources.uniforms.var != NULL );
	SHADER_UNIFORMS( GET_UNIFORM_LOCATION )
	VERTEX_ATTRIBS( VERTEX_ATTRIB_LOOKUP );
}

#define kMaxDrawCalls 2048
#define kCallBufferCount 8		// Needs to be at least as many as we have shaders
// Each shader has it's own buffer for drawcalls
// This means drawcalls get batched by shader
drawCall	call_buffer[kCallBufferCount][kMaxDrawCalls];
// We store a list of the nextfree index for each buffer, so we can append new calls to the correct place
// This gets zeroed on each frame
int			next_call_index[kCallBufferCount];

map* callbatch_map = NULL;
int callbatch_count = 0;

struct renderPass_s {
	drawCall	call_buffer[kCallBufferCount][kMaxDrawCalls];
	int			next_call_index[kCallBufferCount];
};

// Parameters for the whole render operation
struct sceneParams_s {
	vector	fog_color;
	vector	sky_color;
	vector	sun_color;
};

#define kMaxRenderPasses 16
renderPass	render_pass[kMaxRenderPasses];

renderPass renderPass_main;
renderPass renderPass_alpha;
renderPass renderPass_ui;
renderPass renderPass_debug;
renderPass renderPass_depth;

sceneParams sceneParams_main;

void renderPass_clearBuffers( renderPass* pass ) {
	memset( pass->next_call_index, 0, sizeof( int ) * kCallBufferCount );
#if debug
	memset( pass->call_buffer, 0, sizeof( drawCall ) * kMaxDrawCalls * kCallBufferCount );
#endif
}

void render_clearCallBuffer( ) {
	renderPass_clearBuffers( &renderPass_main );
	renderPass_clearBuffers( &renderPass_depth );
	renderPass_clearBuffers( &renderPass_alpha );
	renderPass_clearBuffers( &renderPass_debug );
	renderPass_clearBuffers( &renderPass_ui );
}

#define		kRenderDrawBufferSize 1 * 1024 * 1024
uint8_t*	render_draw_buffer;
uint8_t*	render_draw_buffer_free;

// Temporary allocation that is only valid for a frame
void* render_bufferAlloc( size_t size ) {
	// TODO: Aligned!
	vAssert( ( render_draw_buffer_free + size ) < ( render_draw_buffer + kRenderDrawBufferSize ));
	void* p = render_draw_buffer_free;
	render_draw_buffer_free += size;
	return p;
}

void render_resetBufferBuffer() {
	render_draw_buffer_free = render_draw_buffer;
}
// Private Function declarations

void render_set3D( int w, int h ) {
	glViewport(0, 0, w, h);
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glDepthMask( GL_TRUE );
}

void render_set2D() {
	vAssert( 0 ); // NYI	
}

void render_handleResize() {
}

void render_swapBuffers( window* w ) {
	eglSwapBuffers( w->display, w->surface );
	glFlush();
}

// Iterate through each model in the scene
// Translate by their transform
// Then draw all the submeshes
void render_scene(scene* s) {
	for (int i = 0; i < s->model_count; i++) {
		modelInstance_draw( scene_model( s, i ), s->cam );
	}
	sceneParams_main.fog_color = scene_fogColor( s, transform_getWorldPosition( s->cam->trans ));
	sceneParams_main.sky_color = scene_skyColor( s, transform_getWorldPosition( s->cam->trans ));
	sceneParams_main.sun_color = scene_sunColor( s, transform_getWorldPosition( s->cam->trans ));
}

void render_lighting( scene* s ) {
	// Ambient Light

	// Point Lights	
	light_renderLights( s->light_count, s->lights );
}

// Clear information from last draw
void render_clear() {
	glClearColor( 0.f, 0.f, 0.f, 0.f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

	GLuint render_frame_buffer;
	GLuint render_texture;
	GLuint render_depth_buffer;

void render_initFrameBuffer( window w ) {
	glGenFramebuffers( 1, &render_frame_buffer );
	glGenTextures( 1, &render_texture );
	glGenRenderbuffers( 1, &render_depth_buffer );

	// Create Frame Buffer
	glBindFramebuffer( GL_FRAMEBUFFER, render_frame_buffer );
	// Create render texture
	glBindTexture( GL_TEXTURE_2D, render_texture );

	// Set up sampling parameters, use defaults for now
	// Bilinear interpolation, clamped
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE );

	int width = w.width;
	int height = w.height;

	glTexImage2D( GL_TEXTURE_2D,
		   			0,			// No Mipmaps for now
					GL_RGBA,	// 3-channel, 8-bits per channel (32-bit stride)
					width, height,
					0,			// Border, unused
					GL_RGBA,		// TGA uses BGR order internally
					GL_UNSIGNED_BYTE,	// 8-bits per channel
					NULL );


	// Attach render texture to framebuffer
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_texture, 0 );
	// Generate and Attach Depth Buffer to framebuffer
	glBindRenderbuffer( GL_RENDERBUFFER, render_depth_buffer );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_depth_buffer );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

// Initialise the 3D rendering
void render_init( void* app ) {
	render_createWindow( app, &window_main );

	printf("RENDERING: Initialising OpenGL rendering settings.\n");
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );	// Standard Alpha Blending

	// Backface Culling
	glFrontFace( GL_CW );
	glEnable( GL_CULL_FACE );

	texture_staticInit();
	shader_init();
	render_buildShaders();
	skybox_init();
	
	// Allocate space for buffers
	const GLsizei vertex_buffer_size = sizeof( vector ) * MAX_VERTEX_ARRAY_COUNT;
	const GLsizei element_buffer_size = sizeof( GLushort ) * MAX_VERTEX_ARRAY_COUNT;
	for ( int i = 0; i < kVboCount; i++ ) {
		resources.vertex_buffer[i]	= render_glBufferCreate( GL_ARRAY_BUFFER, NULL, vertex_buffer_size );
		resources.element_buffer[i]	= render_glBufferCreate( GL_ELEMENT_ARRAY_BUFFER, NULL, element_buffer_size );
	}

	// Allocate draw buffer
	render_draw_buffer = mem_alloc( kRenderDrawBufferSize );
	callbatch_map = map_create( kCallBufferCount, sizeof( unsigned int ));

	render_initFrameBuffer( window_main );

#ifdef GRAPH_GPU_FPS
#define kMaxGpuFPSFrames 256
	gpu_fpsdata = graphData_new( kMaxGpuFPSFrames );
	gpu_fpsgraph = graph_new( gpu_fpsdata, 100, 100, 640, 240, (float)kMaxGpuFPSFrames, 0.05f, color_blue );
	gpu_fps_timer = vtimer_create();
#endif // GRAPH_GPU_FPS
}

// Terminate the 3D rendering
void render_terminate() {
#ifndef ANDROID
	//glfwTerminate();
#endif
}

void render_perspectiveMatrix( matrix m, float fov, float aspect, float near, float far ) {
	matrix_setIdentity( m );
	m[0][0] = 1 / tan( fov );
	m[1][1] = aspect / tan( fov );
	m[2][2] = ( far + near )/( far - near );
	m[2][3] = 1.f;
	m[3][2] = (-2.f * far * near) / (far - near );
	m[3][3] = 0.f;
}

void render_validateMatrix( matrix m ) {
	bool should_assert = false;
	for ( int i = 0; i < 3; i++ ) {
		should_assert |= !isNormalized( matrix_getCol( m, i ) );
	}
	should_assert |= !( f_eq( m[0][3], 0.f ));
	should_assert |= !( f_eq( m[1][3], 0.f ));
	should_assert |= !( f_eq( m[2][3], 0.f ));
	should_assert |= !( f_eq( m[3][3], 1.f ));
	if ( should_assert ) {
		printf( "Validate matrix failed:\n" );
		matrix_print( m );
		vAssert( !should_assert );
	}
}

void render_resetModelView( ) {
	matrix_cpy( modelview, camera_inverse );
}

void render_setUniform_matrix( GLuint uniform, matrix m ) {
	glUniformMatrix4fv( uniform, 1, /*transpose*/false, (GLfloat*)m );
}

int render_current_texture_unit = 0;
// Takes a uniform and an OpenGL texture name (GLuint)
// It binds the given texture to an available texture unit
// and sets the uniform to that
void render_setUniform_texture( GLuint uniform, GLuint texture ) {
	GLint max_texture_units = 0;
#ifdef RENDER_OPENGL
	glGetIntegerv( GL_MAX_TEXTURE_UNITS, &max_texture_units );
#endif
#ifdef RENDER_OPENGL_ES
	glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units );
#endif
	vAssert( render_current_texture_unit < max_texture_units );

	// Activate a texture unit
	glActiveTexture( GL_TEXTURE0 + render_current_texture_unit );

	// Bind the texture to that texture unit
	glBindTexture( GL_TEXTURE_2D, texture );
	glUniform1i( uniform, render_current_texture_unit );
	render_current_texture_unit++;
}

void render_setUniform_vector( GLuint uniform, vector* v ) {
	// Only set uniforms if we definitely have them - otherwise we might override aliased constants
	// in the current shader
	if ( uniform != SHADER_CONSTANT_UNBOUND_LOCATION )
		glUniform4fv( uniform, 1, (GLfloat*)v );
}

// Shader version
void render( scene* s ) {
	render_clearCallBuffer();
	render_resetBufferBuffer();
	
	matrix_setIdentity( modelview );

	float aspect = ((float)window_main.width) / ((float)window_main.height);
	camera* cam = s->cam;
	render_perspectiveMatrix( perspective, cam->fov, aspect, cam->z_near, cam->z_far );

	vector frustum[6];
	camera_calculateFrustum( cam, frustum );

	render_validateMatrix( cam->trans->world );
	matrix_cpy( camera_mtx, cam->trans->world );
	matrix_inverse( camera_inverse, cam->trans->world );
	render_resetModelView();
	render_validateMatrix( modelview );

	viewspace_up = matrix_vecMul( modelview, &y_axis );
	vector light_direction = Vector( 1.f, -0.5f, 0.5f, 0.f );
	// TODO this probably needs going per shader/draw batch
	directional_light_direction = normalized( matrix_vecMul( modelview, &light_direction ));
	
	render_lighting( s );

	render_scene( s );
}

void render_sceneParams( sceneParams* params ) {
	render_setUniform_vector( *resources.uniforms.fog_color, &params->fog_color );
	render_setUniform_vector( *resources.uniforms.sky_color_bottom, &params->fog_color );
	render_setUniform_vector( *resources.uniforms.sky_color_top, &params->sky_color );
	render_setUniform_vector( *resources.uniforms.sun_color, &params->sun_color );

	const vector world_space_sun_dir = {{ 0.f, 0.f, 1.f, 0.f }};
	vector sun_dir = matrix_vecMul( modelview, &world_space_sun_dir );
	render_setUniform_vector( *resources.uniforms.camera_space_sun_direction, &sun_dir );
}

int render_findDrawCallBuffer( shader* vshader ) {
	uintptr_t key = (uintptr_t)vshader;
	int* i_ptr = map_find( callbatch_map, key );
	int i;
	if ( !i_ptr ) {
		vAssert( callbatch_count < kCallBufferCount );
		map_add( callbatch_map, key, &callbatch_count );
		i = callbatch_count;
		++callbatch_count;
	} else {
		i = *i_ptr;
	}

	return i;
}

drawCall* drawCall_create( renderPass* pass, shader* vshader, int count, GLushort* elements, vertex* verts, GLint tex, matrix mv ) {
	vAssert( pass );
	vAssert( vshader );

	// Lookup drawcall buffer from shader
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
	draw->vertex_VBO	= resources.vertex_buffer[0];
	draw->element_VBO	= resources.element_buffer[0];
	draw->depth_mask = GL_TRUE;
	draw->elements_mode = GL_TRIANGLES;

	matrix_cpy( draw->modelview, mv );
	return draw;
}

void render_printShader( shader* s ) {
	if ( s == resources.shader_default )
		printf( "shader: default\n" );
	if ( s == resources.shader_particle )
		printf( "shader: particle\n" );
	if ( s == resources.shader_terrain )
		printf( "shader: terrain\n" );
	if ( s == resources.shader_skybox )
		printf( "shader: skybox\n" );
	if ( s == resources.shader_ui )
		printf( "shader: ui\n" );
	if ( s == resources.shader_filter )
		printf( "shader: filter\n" );
}

void render_drawCall_draw( drawCall* draw ) {
	// Bind Correct buffers
	glBindBuffer( GL_ARRAY_BUFFER, draw->vertex_VBO );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, draw->element_VBO );
	
	// If required, copy our data to the GPU
	if ( draw->vertex_VBO == resources.vertex_buffer[0] ) {
		GLsizei vertex_buffer_size	= draw->element_count * sizeof( vertex );
		GLsizei element_buffer_size	= draw->element_count * sizeof( GLushort );
#if 1
		glBufferData( GL_ARRAY_BUFFER, vertex_buffer_size, draw->vertex_buffer, GL_DYNAMIC_DRAW );// OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, element_buffer_size, draw->element_buffer, GL_DYNAMIC_DRAW ); // OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW
#else
		glBufferData( GL_ARRAY_BUFFER, vertex_buffer_size, NULL, GL_DYNAMIC_DRAW );// OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, element_buffer_size, NULL, GL_DYNAMIC_DRAW ); // OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW

		void* buffer = glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
		vAssert( buffer != NULL );
		memcpy( buffer, draw->vertex_buffer, vertex_buffer_size );
		glUnmapBuffer( GL_ARRAY_BUFFER );
		
		buffer = glMapBuffer( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY );
		vAssert( buffer != NULL );
		memcpy( buffer, draw->element_buffer, element_buffer_size );
		glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
#endif
	}

	// Now Draw!
	VERTEX_ATTRIBS( VERTEX_ATTRIB_POINTER );
	vAssert( draw->element_count > 0 );
	/*
	render_printShader( draw->vitae_shader );
	printf( "Vertex buffer " xPTRf ", element_buffer " xPTRf ".\n", (uintptr_t)draw->vertex_buffer, (uintptr_t)draw->element_buffer );
	printf( "element count %d, element_buffer_offset %d.\n", draw->element_count, draw->element_buffer_offset );
	*/
	glDrawElements( draw->elements_mode, draw->element_count, GL_UNSIGNED_SHORT, (void*)(uintptr_t)draw->element_buffer_offset );
	VERTEX_ATTRIBS( VERTEX_ATTRIB_DISABLE_ARRAY );
}

void render_drawBatch( drawCall* draw ) {
	// Reset the current texture unit so we have as many as we need for this batch
	render_current_texture_unit = 0;
	// Only draw if we have a valid texture
	if ( draw->texture != kInvalidGLTexture ) {
		render_setUniform_texture( *resources.uniforms.tex,			draw->texture );
		if ( *resources.uniforms.tex_b ) {
			render_setUniform_texture( *resources.uniforms.tex_b,		draw->texture_b );
		}
		if ( *resources.uniforms.tex_c ) {
			render_setUniform_texture( *resources.uniforms.tex_c,		draw->texture_c );
		}
		if ( *resources.uniforms.tex_d ) {
			render_setUniform_texture( *resources.uniforms.tex_d,		draw->texture_d );
		}
		/*
		if ( *resources.uniforms.tex_normal ) {
			render_setUniform_texture( *resources.uniforms.tex_normal,		draw->texture_normal );
		}
		if ( *resources.uniforms.tex_b_normal ) {
			render_setUniform_texture( *resources.uniforms.tex_b_normal,	draw->texture_b_normal );
		}
		if ( *resources.uniforms.tex_lookup ) {
			render_setUniform_texture( *resources.uniforms.tex_lookup,		draw->texture_lookup );
		}
		*/
		render_setUniform_matrix( *resources.uniforms.modelview,	draw->modelview );
		render_drawCall_draw( draw );
	}
}

void render_drawCallBatch( int count, drawCall* calls ) {
	glDepthMask( calls[0].depth_mask );
	shader_activate( calls[0].vitae_shader );
	render_lighting( theScene );
	// Set up uniform matrices
	render_setUniform_matrix( *resources.uniforms.projection,	perspective );
	render_setUniform_matrix( *resources.uniforms.worldspace,	modelview );
	render_setUniform_vector( *resources.uniforms.viewspace_up, &viewspace_up );
	render_setUniform_vector( *resources.uniforms.directional_light_direction, &directional_light_direction );

	render_sceneParams( &sceneParams_main );

	for ( int i = 0; i < count; i++ ) {
		render_drawBatch( &calls[i] );
	}
}

void render_drawPass( renderPass* pass ) {
	// Draw each batch of drawcalls
	for ( int i = 0; i < kCallBufferCount; i++ ) {
		drawCall* batch = pass->call_buffer[i];
		int count = pass->next_call_index[i];
		if ( count > 0 )
			render_drawCallBatch( count, batch );	
	}
}

void render_attachFrameBuffer() {
	glBindFramebuffer( GL_FRAMEBUFFER, render_frame_buffer );
}

void render_unattachFrameBuffer() {
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void render_draw( window* w, engine* e ) {
	(void)e;
	render_set3D( w->width, w->height );
	render_clear();
	
	/*
	memcpy( &renderPass_depth, &renderPass_main, sizeof( renderPass ));
	for ( int i = 0; i < kCallBufferCount; ++i ) {
		for ( int j = 0; j < kMaxDrawCalls; ++j ) {
			renderPass_depth.call_buffer[i][j].vitae_shader = resources.shader_depth;
		}
	}
	*/

	glEnable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
	render_drawPass( &renderPass_depth );

	glEnable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	render_drawPass( &renderPass_main );

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	render_drawPass( &renderPass_alpha );
	
	// No depth-test for ui
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	render_drawPass( &renderPass_ui );

	// No depth-test for debug
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	render_drawPass( &renderPass_debug );

	render_swapBuffers( w );
}

void render_waitForEngineThread() {
	vthread_waitCondition( start_render );
}

void render_renderThreadTick( engine* e ) {
	PROFILE_BEGIN( PROFILE_RENDER_TICK );
	texture_tick();
	render_resetModelView();
#ifdef GRAPH_GPU_FPS
	graph_render( gpu_fpsgraph );
	timer_getDelta(gpu_fps_timer);
#endif
	render_draw( &window_main, e );
#ifdef GRAPH_GPU_FPS
	//glFlush();
	glFinish();
	float delta = timer_getDelta(gpu_fps_timer);
	static int framecount = 0;
	++framecount;
	graphData_append( gpu_fpsdata, (float)framecount, delta );
	printf( "GPU Delta %.8f\n", delta );
#endif
	// Indicate that we have finished
	vthread_signalCondition( finished_render );
	PROFILE_END( PROFILE_RENDER_TICK );
}

//
// *** The Rendering Thread itself
//
void* render_renderThreadFunc( void* args ) {
	printf( "RENDER THREAD: Hello from the render thread!\n" );
	engine* e = NULL;
#ifdef ANDROID
	struct android_app* app = args;
	e = app->userData;
#else
	e = args;
	void* app = NULL;
#endif

	render_init( app );
	render_initialised = true;
	printf( "RENDER THREAD: Render system initialised.\n");
	vthread_signalCondition( finished_render );

	while( true ) {
		render_bufferTick();
		render_waitForEngineThread();
		render_renderThreadTick( e );
	}

	return NULL;
}
