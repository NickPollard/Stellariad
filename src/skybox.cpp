// skybox.c

#include "common.h"
#include "skybox.h"
//-----------------------
#include "model.h"
#include "model_loader.h"
#include "maths/vector.h"
#include "render/render.h"
#include "render/shader.h"
#include "render/texture.h"
#include "system/hash.h"

// *** Forward Declarations
uint8_t* skybox_generateSkybox( vector sky_color_bottom, vector sky_color_top, uint8_t* image );

model*		skybox_model = NULL;
texture*	skybox_texture = NULL;

uint8_t*	skybox_image = NULL;	// For offline CPU rendering
int			skybox_width = 0;
int			skybox_height = 0;

// Initialise static data for the skybox system
void skybox_init( ) {
	//skybox_texture = texture_load( "dat/img/vitae_sky2_export_flattened.tga" );
	//skybox_image = read_tga( "dat/img/vitae_sky2_export_flattened.tga", &skybox_width, &skybox_height );
	skybox_image = read_tga( "dat/img/vitae_sky_dome.tga", &skybox_width, &skybox_height );
	vector sky_color_bottom	= Vector( 0.f, 0.4f, 0.3f, 1.f );
	vector sky_color_top	= Vector( 0.4f, 0.55f, 0.7f, 1.f );
	static const int stride = 4;
	(void)sky_color_bottom; (void)sky_color_top;

#if 1
	skybox_texture = texture_loadFromMem( skybox_width, skybox_height, stride, skybox_image );
#else
	uint8_t* bitmap = skybox_generateSkybox( sky_color_bottom, sky_color_top, skybox_image );
	skybox_texture = texture_loadFromMem( skybox_width, skybox_height, stride, bitmap );
	texture_free( bitmap );
#endif

	texture_free( skybox_image );

	skybox_model = model_load( "dat/model/skydome.s" );
	skybox_model->meshes[0]->texture_diffuse = skybox_texture;
	skybox_model->meshes[0]->_shader = Shader::byName( "dat/shaders/skybox.s" );
	skybox_model->meshes[0]->cachedDraw = NULL;
	skybox_model->meshes[0]->dontCache = true;
}

#define SKYBOX_VERTEX_ATTRIB_POINTER( attrib ) \
	glVertexAttribPointer( attrib, /*vec4*/ 4, GL_FLOAT, /*Normalized?*/GL_FALSE, sizeof( skybox_vertex ), (void*)offsetof( skybox_vertex, attrib) ); \
	glEnableVertexAttribArray( attrib );

// Render the skybox
void skybox_render( void* data ) {
	(void)data;
	// Skybox does not write to the depth buffer

	render_resetModelView();
	vector v = Vector( 0.f, 0.f, 0.f, 1.f );
	matrix_setTranslation( modelview, &v );

	// TEMP: Force texture again as delayed tex loading from model can override this
	//skybox_model->meshes[0]->cachedDraw = NULL;
	skybox_model->meshes[0]->texture_diffuse = skybox_texture;

	mesh_render( skybox_model->meshes[0] );
}

vector skybox_fragmentShader( vector src_color, vector sky_color_bottom, vector sky_color_top ) {
	static vector cloud_color = {{ 1.f, 1.f, 1.f, 1.f }};
	// color = top * blue
	// then blend in cloud (white * green, blend alpha )
	vector frag_color = vector_add(	vector_scaled( sky_color_top, src_color.coord.z * ( 1.f - src_color.coord.w )),
			vector_scaled( cloud_color, src_color.coord.y * src_color.coord.w ));
	// then add bottom * red
	//vector frag_color = color_black;
	//(void)cloud_color; (void)sky_color_top;
	frag_color = vector_add( frag_color, vector_scaled( sky_color_bottom , src_color.coord.x ));
	//vector_printf( "color: ", &src_color );
	frag_color.coord.w = 1.0;
	frag_color.coord.x = fminf( 1.f, frag_color.coord.x );
	frag_color.coord.y = fminf( 1.f, frag_color.coord.y );
	frag_color.coord.z = fminf( 1.f, frag_color.coord.z );
	frag_color.coord.w = fminf( 1.f, frag_color.coord.w );
	return frag_color;
}

void skybox_shadeSkybox( int width, int height, uint8_t* src_image, uint8_t* dst_image, vector sky_color_bottom, vector sky_color_top ) {
	static const int stride = 4;
	for ( int x = 0; x < width; ++x ) {
		for ( int y = 0; y < height; ++y ) {
			int index = (x + ( y * width )) * stride;
			uint8_t r = src_image[index + 0];
			uint8_t g = src_image[index + 1];
			uint8_t b = src_image[index + 2];
			uint8_t a = src_image[index + 3];
			//printf( "color %d %d %d %d.\n", r, g, b, a);
			vector src_color = Vector( ((float)r)/255.f, ((float)g)/255.f, ((float)b)/255.f, ((float)a)/255.f );
			vector dst_color = skybox_fragmentShader( src_color, sky_color_bottom, sky_color_top );
			//vector_printf( "dst color: ", &dst_color );
			dst_image[index + 0] = dst_color.coord.x * 255.f;
			dst_image[index + 1] = dst_color.coord.y * 255.f;
			dst_image[index + 2] = dst_color.coord.z * 255.f;
			dst_image[index + 3] = dst_color.coord.w * 255.f;
		}
	}
}

uint8_t* skybox_generateSkybox( vector sky_color_bottom, vector sky_color_top, uint8_t* image ) {
	int w = skybox_width;
	int h = skybox_height;
	static const int stride = 4;
	uint8_t* dst_image = (uint8_t*)mem_alloc( sizeof( uint8_t ) * w * h * stride );
	memset( dst_image, 0, sizeof( uint8_t ) * stride * w * h );
	skybox_shadeSkybox( w, h, image, dst_image, sky_color_bottom, sky_color_top );
	return dst_image;
}
