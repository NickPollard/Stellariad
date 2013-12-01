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
#include "render/graphicsbuffer.h"
#include "render/modelinstance.h"
#include "render/renderwindow.h"
#include "render/shader.h"
#include "render/texture.h"
#include "system/file.h"
#include "system/hash.h"
#include "system/string.h"
// temp
#include "engine.h"

#ifdef LINUX_X
#include <render/render_linux.h>
#endif
#ifdef ANDROID
#include <render/render_android.h>
#endif

#ifdef RENDER_OPENGL 
	#define kGlTextureUnits GL_MAX_TEXTURE_UNITS
#endif
#ifdef RENDER_OPENGL_ES
	#define kGlTextureUnits GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
#endif

// Rendering API declaration
bool	render_initialised = false;
bool	render_bloom_enabled = true;

#ifdef GRAPH_GPU_FPS
graph*			gpu_fpsgraph; 
graphData*		gpu_fpsdata;
frame_timer* 	gpu_fps_timer = NULL;
#endif // GRAPH_GPU_FPS

#define kMaxVertexArrayCount 1024

// *** Shader Pipeline
	matrix modelview, camera_mtx, camera_inverse, camera_matrix;
	matrix perspective;
	vector viewspace_up;
	vector directional_light_direction;

// *** Global rendering resources
	RenderResources resources;

	window window_main = { 1196, 720, 0, 0, 0, true };

// *** Properties of the GL implementation
	typedef struct GraphicsSystem_s {
		GLint maxTextureUnits;
	} GraphicsSystem;

	GraphicsSystem graphicsSystem = { 0 };

// *** RenderPasses
	renderPass renderPass_main;
	renderPass renderPass_alpha;
	renderPass renderPass_ui;
	renderPass renderPass_debug;
	renderPass renderPass_depth;

	map* callbatch_map = NULL;
	int callbatch_count = 0;

// *** SceneParams
	sceneParams sceneParams_main;

void render_buildShaders() {
	// Load Shaders					Vertex												Fragment
	resources.shader_default		= shader_load( "dat/shaders/phong.v.glsl",			"dat/shaders/phong.f.glsl" );
	resources.shader_reflective		= shader_load( "dat/shaders/reflective.v.glsl",		"dat/shaders/reflective.f.glsl" );
	resources.shader_refl_normal	= shader_load( "dat/shaders/refl_normal.v.glsl",	"dat/shaders/refl_normal.f.glsl" );
	resources.shader_particle		= shader_load( "dat/shaders/textured_phong.v.glsl",	"dat/shaders/textured_phong.f.glsl" );
	resources.shader_terrain		= shader_load( "dat/shaders/terrain.v.glsl",		"dat/shaders/terrain.f.glsl" );
	resources.shader_skybox			= shader_load( "dat/shaders/skybox.v.glsl",			"dat/shaders/skybox.f.glsl" );
	resources.shader_ui				= shader_load( "dat/shaders/ui.v.glsl",				"dat/shaders/ui.f.glsl" );
	resources.shader_gaussian		= shader_load( "dat/shaders/gaussian.v.glsl",		"dat/shaders/gaussian.f.glsl" );
	resources.shader_gaussian_vert	= shader_load( "dat/shaders/gaussian_vert.v.glsl",	"dat/shaders/gaussian_vert.f.glsl" );
	resources.shader_filter			= shader_load( "dat/shaders/filter.v.glsl",			"dat/shaders/filter.f.glsl" );
	resources.shader_debug			= shader_load( "dat/shaders/debug_lines.v.glsl",	"dat/shaders/debug_lines.f.glsl" );
	resources.shader_debug_2d		= shader_load( "dat/shaders/debug_lines_2d.v.glsl",	"dat/shaders/debug_lines_2d.f.glsl" );
	resources.shader_depth			= shader_load( "dat/shaders/depth_pass.v.glsl",		"dat/shaders/depth_pass.f.glsl" );

#define GET_UNIFORM_LOCATION( var ) \
	resources.uniforms.var = shader_findConstant( mhash( #var )); \
	assert( resources.uniforms.var != NULL );
	SHADER_UNIFORMS( GET_UNIFORM_LOCATION )
	VERTEX_ATTRIBS( VERTEX_ATTRIB_LOOKUP );
}

// TODO - hashmap
shader* render_shaderByName( const char* name ) {
	if (string_equal(name, "phong")) return resources.shader_default;
	else if (string_equal(name, "reflective")) return resources.shader_reflective;
	else if (string_equal(name, "refl_normal")) return resources.shader_refl_normal;
	else return resources.shader_default;
}

#define kMaxDrawCalls 2048
#define kCallBufferCount 12		// Needs to be at least as many as we have shaders
// Each shader has it's own buffer for drawcalls
// This means drawcalls get batched by shader
drawCall	call_buffer[kCallBufferCount][kMaxDrawCalls];
// We store a list of the nextfree index for each buffer, so we can append new calls to the correct place
// This gets zeroed on each frame
int			next_call_index[kCallBufferCount];

struct renderPass_s {
	drawCall	call_buffer[ kCallBufferCount ][ kMaxDrawCalls ];
	int			next_call_index[ kCallBufferCount ];
};
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

// Iterate through each model in the scene, translate by their transform, then draw all the submeshes
void render_scene(scene* s) {
	sceneParams_main.fog_color = scene_fogColor( s, transform_getWorldPosition( s->cam->trans ));
	sceneParams_main.sky_color = scene_skyColor( s, transform_getWorldPosition( s->cam->trans ));
	sceneParams_main.sun_color = scene_sunColor( s, transform_getWorldPosition( s->cam->trans ));
	for (int i = 0; i < s->model_count; i++) {
		modelInstance_draw( scene_model( s, i ), s->cam );
	}
}

void render_lighting( scene* s ) {
	light_renderLights( s->light_count, s->lights );
}

// Clear information from last draw
void render_clear() {
	glClearColor( 0.f, 0.f, 0.f, 0.f );
	glClear( GL_DEPTH_BUFFER_BIT );
}

typedef struct frameBuffer_s {
	GLuint frame_buffer;
	GLuint texture;
	GLuint depth_texture;
	GLuint depth_buffer;
	int width;
	int height;
} FrameBuffer;

FrameBuffer* render_first_buffer	= nullptr;
FrameBuffer* render_second_buffer	= nullptr;
FrameBuffer* render_third_buffer	= nullptr;
FrameBuffer* render_fourth_buffer	= nullptr;

GLuint frameBuffer() {
	GLuint buffer;
	glGenFramebuffers( 1, &buffer );
	glBindFramebuffer( GL_FRAMEBUFFER, buffer );
	return buffer;
}

FrameBuffer* newFrameBuffer( int width, int height ) {
	FrameBuffer* buffer = mem_alloc(sizeof( FrameBuffer ));
	buffer->width = width;
	buffer->height = height;
	buffer->frame_buffer = frameBuffer();
	buffer->texture = textureBilinearClamped();
	glTexImage2D( GL_TEXTURE_2D,
		   			0,			// No Mipmaps for now
					GL_RGB,	// 3-channel, 8-bits per channel (32-bit stride)
					width, height,
					0,			// Border, unused
					GL_RGB,		// TGA uses BGR order internally
					GL_UNSIGNED_BYTE,	// 8-bits per channel
					NULL );
	textureUnbind();

	buffer->depth_texture = textureBilinearClamped();
	glTexImage2D( GL_TEXTURE_2D,
		   			0,						// No Mipmaps for now
					GL_DEPTH_COMPONENT,
					width, height,
					0,						// Border, unused
					GL_DEPTH_COMPONENT,
					GL_UNSIGNED_SHORT,		// 16-bits
					NULL );

	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer->texture, 0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, buffer->depth_texture, 0 );
	vAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	textureUnbind();
	return buffer;
}

// Initialise the 3D rendering
void render_init( void* app ) {
	render_createWindow( app, &window_main );

	printf("RENDERING: Initialising OpenGL rendering settings.\n");
	const char* extension_string = (const char*)glGetString( GL_EXTENSIONS );
	printf( "Extensions supported: %s\n", extension_string );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );	// Standard Alpha Blending

	// Backface Culling
	glFrontFace( GL_CW );
	glEnable( GL_CULL_FACE );

	glGetIntegerv( kGlTextureUnits, &graphicsSystem.maxTextureUnits );

	texture_staticInit();
	shader_init();
	render_buildShaders();
	skybox_init();
	
	// Allocate space for buffers
	for ( int i = 0; i < kVboCount; i++ ) {
		resources.vertex_buffer[i]	= render_bufferCreate( GL_ARRAY_BUFFER, NULL, sizeof( vector ) * kMaxVertexArrayCount);
		resources.element_buffer[i]	= render_bufferCreate( GL_ELEMENT_ARRAY_BUFFER, NULL, sizeof( GLushort ) * kMaxVertexArrayCount);
	}

	callbatch_map = map_create( kCallBufferCount, sizeof( unsigned int ));

	const int downscale = 8;
	int w = window_main.width / downscale;
	int h = window_main.height / downscale;
	render_first_buffer = newFrameBuffer( window_main.width, window_main.height );
	render_second_buffer = newFrameBuffer( w, h );
	render_third_buffer = newFrameBuffer( w, h );
	render_fourth_buffer = newFrameBuffer( w, h );

#ifdef GRAPH_GPU_FPS
#define kMaxGpuFPSFrames 256
	gpu_fpsdata = graphData_new( kMaxGpuFPSFrames );
	vector graph_blue = Vector( 0.2f, 0.2f, 0.8f, 1.f );
	gpu_fpsgraph = graph_new( gpu_fpsdata, 100, 100, 640, 240, (float)kMaxGpuFPSFrames, 0.033f, graph_blue );
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
	if ((int)uniform >= 0 ) {
		vAssert( render_current_texture_unit < graphicsSystem.maxTextureUnits );

		// Activate a texture unit
		glActiveTexture( GL_TEXTURE0 + render_current_texture_unit );

		// Bind the texture to that texture unit
		glBindTexture( GL_TEXTURE_2D, texture );
		glUniform1i( uniform, render_current_texture_unit );
		render_current_texture_unit++;
	}
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
	
	matrix_setIdentity( modelview );

	camera* cam = s->cam;
	cam->aspect = ((float)window_main.width) / ((float)window_main.height);
	render_perspectiveMatrix( perspective, cam->fov, cam->aspect, cam->z_near, cam->z_far );

	vector frustum[6];
	camera_calculateFrustum( cam, frustum );

	render_validateMatrix( cam->trans->world );
	matrix_cpy( camera_mtx, cam->trans->world );
	matrix_inverse( camera_inverse, cam->trans->world );
	matrix_cpy( camera_matrix, cam->trans->world );
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
	int* found = map_find( callbatch_map, key );
	int index;
	if ( !found ) {
		vAssert( callbatch_count < kCallBufferCount );
		map_add( callbatch_map, key, &callbatch_count );
		index = callbatch_count++;
	} else
		index = *found;

	return index;
}

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
	draw->vertex_VBO	= resources.vertex_buffer[0];
	draw->element_VBO	= resources.element_buffer[0];
	draw->depth_mask = GL_TRUE;
	draw->elements_mode = GL_TRIANGLES;

	matrix_cpy( draw->modelview, mv );
	return draw;
}

void render_printShader( shader* s ) {
	if ( s == resources.shader_default )	printf( "shader: default\n" );
	if ( s == resources.shader_particle )	printf( "shader: particle\n" );
	if ( s == resources.shader_terrain )	printf( "shader: terrain\n" );
	if ( s == resources.shader_skybox )		printf( "shader: skybox\n" );
	if ( s == resources.shader_ui )			printf( "shader: ui\n" );
	if ( s == resources.shader_filter )		printf( "shader: filter\n" );
}

GLuint current_VBO = -1;

void render_drawCall_draw( drawCall* draw ) {
	vAssert( draw->element_count > 0 );
	bool new_buffer = (draw->vertex_VBO != current_VBO);

	if ( new_buffer ) {
		current_VBO = draw->vertex_VBO;
		glBindBuffer( GL_ARRAY_BUFFER, draw->vertex_VBO );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, draw->element_VBO );
	}	
	
	// If required, copy our data to the GPU
	if ( draw->vertex_VBO == resources.vertex_buffer[0] ) {
		GLsizei vertex_buffer_size	= draw->element_count * sizeof( vertex );
		GLsizei element_buffer_size	= draw->element_count * sizeof( GLushort );
		glBufferData( GL_ARRAY_BUFFER, vertex_buffer_size, draw->vertex_buffer, GL_DYNAMIC_DRAW );// OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, element_buffer_size, draw->element_buffer, GL_DYNAMIC_DRAW ); // OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW
		/*
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
		*/
	}

	VERTEX_ATTRIBS( VERTEX_ATTRIB_POINTER );
	glDrawElements( draw->elements_mode, draw->element_count, GL_UNSIGNED_SHORT, (void*)(uintptr_t)draw->element_buffer_offset );
}

void render_drawTextureBatch( drawCall* draw ) {
	render_setUniform_matrix( *resources.uniforms.modelview, draw->modelview );
	render_drawCall_draw( draw );
}

int compareTexture( const void* a, const void* b ) {
	const drawCall* draw_a = a;
	const drawCall* draw_b = b;
	return ((int)draw_a->texture) - ((int)draw_b->texture);
}

void render_drawShaderBatch( window* w, int count, drawCall* calls ) {
	glDepthMask( calls[0].depth_mask );
	shader_activate( calls[0].vitae_shader );
	render_lighting( theScene );
	render_setUniform_matrix( *resources.uniforms.projection,	perspective );
	render_setUniform_matrix( *resources.uniforms.worldspace,	modelview );
	render_setUniform_matrix( *resources.uniforms.camera_to_world, camera_matrix );
	render_setUniform_vector( *resources.uniforms.viewspace_up, &viewspace_up );
	render_setUniform_vector( *resources.uniforms.directional_light_direction, &directional_light_direction );
	vector screen_size = Vector( w->width, w->height, 0.f, 0.f );
	render_setUniform_vector( *resources.uniforms.screen_size, &screen_size );

	render_sceneParams( &sceneParams_main );

	drawCall* sorted = mem_alloc( sizeof(drawCall) * count );
	memcpy( sorted, calls, sizeof(drawCall) * count );
	bool ui_shader = calls[0].vitae_shader != resources.shader_ui;
	if ( ui_shader ) {
		qsort( sorted, count, sizeof(drawCall), &compareTexture );
	}

	// Reset the current texture unit so we have as many as we need for this batch
	render_current_texture_unit = 0;
	// Only draw if we have a valid texture
	GLuint current_texture = -1;
	drawCall* draw = &sorted[0];
	if ( draw->texture != kInvalidGLTexture ) {
		for ( int i = 0; i < count; i++ ) {
			drawCall* draw = &sorted[i];
			if (draw->texture != current_texture || ui_shader) {
				current_texture = draw->texture;
				render_current_texture_unit = 0;
				render_setUniform_texture( *resources.uniforms.tex, draw->texture );
				if ( *resources.uniforms.tex_b ) render_setUniform_texture( *resources.uniforms.tex_b, draw->texture_b );
				if ( *resources.uniforms.tex_c ) render_setUniform_texture( *resources.uniforms.tex_c, draw->texture_c );
				if ( *resources.uniforms.tex_d ) render_setUniform_texture( *resources.uniforms.tex_d, draw->texture_d );
				if ( *resources.uniforms.tex_normal ) render_setUniform_texture( *resources.uniforms.tex_normal, draw->texture_normal );
			}
			render_drawTextureBatch( &sorted[i] );
		}
	}
	mem_free( sorted );
}

// Draw each batch of drawcalls
void render_drawPass( window* w, renderPass* pass ) {
	for ( int i = 0; i < kCallBufferCount; i++ ) {
		drawCall* batch = pass->call_buffer[i];
		int count = pass->next_call_index[i];
		if ( count > 0 )
			render_drawShaderBatch( w, count, batch );	
	}
}

void render_attachFrameBuffer(FrameBuffer* buffer) {
	glBindFramebuffer( GL_FRAMEBUFFER, buffer->frame_buffer );
	//render_clear();
	glViewport(0, 0, buffer->width, buffer->height);
}

void render_unattachFrameBuffer() {
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

GLuint* postProcess_vertex_VBO = 0;
GLuint* postProcess_element_VBO = 0;

void render_drawFrameBuffer( window* w, FrameBuffer* buffer, shader* s, float alpha ) {
	shader_activate( s );
	render_setUniform_texture( *resources.uniforms.tex,	buffer->texture );
	vector screen_size = Vector( w->width, w->height, 0.f, 0.f );
	render_setUniform_vector( *resources.uniforms.screen_size, &screen_size );

	vertex vertex_buffer[4];
	float width = w->width;
	float h = w->height;
	vertex_buffer[0].position = Vector(0.f, 0.f, 0.f, 1.f);
	vertex_buffer[1].position = Vector(width, 0.f, 0.f, 1.f);
	vertex_buffer[2].position = Vector(0.f, h, 0.f, 1.f);
	vertex_buffer[3].position = Vector(width, h, 0.f, 1.f);
	vertex_buffer[0].uv = Vector(0.f, 0.f, 0.f, 1.f);
	vertex_buffer[1].uv = Vector(1.f, 0.f, 0.f, 1.f);
	vertex_buffer[2].uv = Vector(0.f, 1.f, 0.f, 1.f);
	vertex_buffer[3].uv = Vector(1.f, 1.f, 0.f, 1.f);
	vertex_buffer[0].color = Vector(alpha, alpha, alpha, alpha);
	vertex_buffer[1].color = Vector(alpha, alpha, alpha, alpha);
	vertex_buffer[2].color = Vector(alpha, alpha, alpha, alpha);
	vertex_buffer[3].color = Vector(alpha, alpha, alpha, alpha);
	short element_buffer[6] = {0, 2, 1, 1, 2, 3};
	int element_count = 6;
	if (!postProcess_vertex_VBO) {
		postProcess_vertex_VBO = mem_alloc(sizeof(GLuint));
		*postProcess_vertex_VBO = resources.vertex_buffer[0];
	}
	if (!postProcess_element_VBO) postProcess_element_VBO = render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, element_buffer, element_count * sizeof(GLushort));
	if ( *postProcess_vertex_VBO != kInvalidBuffer && *postProcess_element_VBO != kInvalidBuffer ) {
		glBindBuffer( GL_ARRAY_BUFFER, *postProcess_vertex_VBO );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, *postProcess_element_VBO );
		GLsizei vertex_buffer_size	= element_count * sizeof( vertex );
		GLsizei element_buffer_size	= element_count * sizeof( GLushort );
		glBufferData( GL_ARRAY_BUFFER, vertex_buffer_size, vertex_buffer, GL_DYNAMIC_DRAW );// OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, element_buffer_size, element_buffer, GL_DYNAMIC_DRAW ); // OpenGL ES only supports DYNAMIC_DRAW or STATIC_DRAW
		VERTEX_ATTRIBS( VERTEX_ATTRIB_POINTER );
		glDrawElements( GL_TRIANGLES, element_count, GL_UNSIGNED_SHORT, (void*)(uintptr_t)0 );
		//VERTEX_ATTRIBS( VERTEX_ATTRIB_DISABLE_ARRAY );
		current_VBO = *postProcess_vertex_VBO;
	}
}

void render_draw( window* w, engine* e ) {
	(void)e;
	render_set3D( w->width, w->height );
	render_clear();

	// Draw normally AND to the frame buffer
	render_attachFrameBuffer(render_first_buffer);
	{
		render_clear();
		//glEnable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );
		glDepthMask( GL_TRUE );
		glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		render_drawPass( w, &renderPass_depth );

		//glDepthMask( GL_TRUE );
		//glEnable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		render_drawPass( w, &renderPass_main );

		//glEnable( GL_DEPTH_TEST );
		glEnable( GL_BLEND );
		render_drawPass( w, &renderPass_alpha );
	}
	render_unattachFrameBuffer();
	glViewport(0, 0, w->width, w->height);
	render_drawFrameBuffer( w, render_first_buffer, resources.shader_ui, 1.f );

	// No depth-test for ui
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );

	if ( render_bloom_enabled ) {
		// downscale pass
		render_current_texture_unit = 0;
		render_attachFrameBuffer( render_second_buffer ); 
		{
	   		render_drawFrameBuffer( w, render_first_buffer, resources.shader_ui, 1.f );
		}
		render_unattachFrameBuffer();
		// horizontal pass
		render_attachFrameBuffer( render_third_buffer ); 
		{
	   		render_drawFrameBuffer( w, render_first_buffer, resources.shader_gaussian, 1.f );
		}
		render_unattachFrameBuffer();
		// vertical pass
		render_attachFrameBuffer( render_fourth_buffer ); 
		{
	   		render_drawFrameBuffer( w, render_third_buffer, resources.shader_gaussian_vert, 1.f );
		}
		render_unattachFrameBuffer();

		glViewport(0, 0, w->width, w->height);
	   	render_drawFrameBuffer( w, render_fourth_buffer, resources.shader_ui, 0.3f );
	}
	render_drawPass( w, &renderPass_ui );

	// No depth-test for debug
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	render_drawPass( w, &renderPass_debug );

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
	glFinish();
	float delta = timer_getDelta(gpu_fps_timer);
	static int framecount = 0;
	++framecount;
	graphData_append( gpu_fpsdata, (float)framecount, delta );
	printf( "GPU Delta %.4f\n", delta );
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
