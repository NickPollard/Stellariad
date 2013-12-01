// texture.h
#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "render/vgl.h"

#define kInvalidGLTexture 0xffffffff

// Globals
extern texture* static_texture_default;
extern texture* static_texture_reflective;

void texture_staticInit();

GLuint texture_loadBitmap( int w, int h, int stride, uint8_t* bitmap, GLuint wrap_s, GLuint wrap_t );
GLuint texture_loadTGA(const char* filename);

	// TGA format
	//
	// 1 byte ID length
	// 1 byte color map (0 = no map, 1 = map)
	// 1 byte image type (uncompressed color = 0x2)
	// 5 bytes color map specification
	// 10 bytes image specification
	// ( 2 bytes - x origin )
	// ( 2 bytes - y origin )
	// ( 2 bytes - width )
	// ( 2 bytes - height )
	// ( 1 bytes - bit depth )
	// (ID length) bytes - Image ID
	// (from color map spec) bytes - Colour map
	// (width * height * bitdepth / 8) bytes - Image Data

typedef struct tga_colormap_spec_s {
	uint16_t first_entry;
	uint16_t entry_count;
	uint8_t pixel_depth;
} tga_colormap_spec;

typedef struct tga_header_s {
	uint8_t id_length;
	uint8_t color_map;
	uint8_t image_type;
	uint8_t color_map_spec[5];	// This can't be declared as a struct due to aliasing
	uint16_t origin_x;
	uint16_t origin_y;
	uint16_t width;
	uint16_t height;
	uint8_t pixel_depth;
	uint8_t image_desc;
} tga_header;

enum textureRequestType {
	kTextureFileRequest,
	kTextureMemRequest
};

struct texture_s {
	GLuint gl_tex;
	const char* filename;
};

typedef struct textureProperties_s {
	GLuint wrap_s;
	GLuint wrap_t;
} textureProperties;

void texture_tick();
void texture_requestFile( GLuint* tex, const char* filename, textureProperties* properties );

texture* texture_load( const char* filename );
texture* texture_loadWithProperties( const char* filename, textureProperties* properties );
texture* texture_loadFromMem( int w, int h, int stride, uint8_t* bitmap );
void texture_delete( texture* t );

// For offline skybox rendering, or anything else that needs source images
uint8_t* read_tga( const char* file, int* w, int* h );

void* texture_allocate( size_t size );
void texture_free( void* tex );

// Create a new texture using bilinear filtering and clamp-to-edge
GLuint textureBilinearClamped();
void textureUnbind();

#endif // __TEXTURE_H__
