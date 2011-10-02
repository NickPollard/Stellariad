// texture.c

#include "common.h"
#include "render/texture.h"
//---------------------
#include "maths.h"
#include "mem/allocator.h"
#include "render/render.h"
#include "system/file.h"
#include "system/string.h"
#include "system/thread.h"

// GLFW Libraries
#include <GL/glfw.h>

// Globals
GLuint g_texture_default = 0;
vmutex	texture_mutex = kMutexInitialiser;

void texture_init() {
	g_texture_default = texture_loadTGA( "assets/img/test64rgba.tga" );
	printf( "Loaded default texture as OpenGL texture name: %d\n", g_texture_default );
}

typedef struct textureRequest_s {
	GLuint* tex;
	const char* filename;
} textureRequest;

#define kMaxTextureRequests 16
textureRequest requests[kMaxTextureRequests];
int texture_request_count = 0;

// Load any waiting texture requests
void texture_tick() {
	vmutex_lock( &texture_mutex );
	{
		// TODO: This could be a lock-free queue
		for ( int i = 0; i < texture_request_count; i++ ) {
			*(requests[i].tex) = texture_loadTGA( requests[i].filename );
			mem_free( (void*)requests[i].filename );
		}
		texture_request_count = 0;
	}
	vmutex_unlock( &texture_mutex );
}

void texture_request( GLuint* tex, const char* filename ) {
	vmutex_lock( &texture_mutex );
	{
		vAssert( texture_request_count < kMaxTextureRequests );
		textureRequest* request = &requests[texture_request_count++];
		request->tex = tex;
//		request->filename = filename;
		request->filename = string_createCopy( filename );
	}
	vmutex_unlock( &texture_mutex );
}

uint8_t* read_tga( const char* file, int* w, int* h ) {
	int length = 0;
	u8* image_data = vfile_contents( file, &length );
	if ( image_data == 0 ) {
		printf( "Error reading TGA.\n" );
		vAssert( 0 );
	}

	tga_header* header = (tga_header*)image_data;
	u8* body = image_data + sizeof(tga_header);

	static const int kTrueColor_Uncompressed = 0x2;
	if ( header->image_type != kTrueColor_Uncompressed ) {
		printf( "Error. TGA is not uncompressed and in TrueColor.\n" );
		assert( 0 );
	}

	int width = header->width;
	int height = header->height;
	u8* id_field = body;
	(void)id_field;
	u8* color_map = body + header->id_length;
	tga_colormap_spec* mapspec = (tga_colormap_spec*)header->color_map_spec;
	u8* pixels = color_map + mapspec->entry_count;

	//printf( "width: %d, height: %d, bitdepth: %d.\n", width, height, header->pixel_depth );

//	printf( "TGA dimensions: %d * %d.\n", width, height );
//	printf( "TGA bitdepth: %d.\n", header->pixel_depth );

	int pixel_bytes = header->pixel_depth / 8;
	int size = width * height * pixel_bytes;
	uint8_t* tex = mem_alloc( size );
	memcpy( tex, pixels, size );

	bool swizzle = true;

	if ( swizzle ) {
		// Switch from BGRA to RGBA
		for ( int i = 0; i < width * height; i++ ) {
			uint8_t tmp = tex[i * pixel_bytes + 0];
			tex[i * pixel_bytes + 0] = tex[i * pixel_bytes + 2];
			tex[i * pixel_bytes + 2] = tmp;
		}
	}

	*w = width;
	*h = height;

//	printf( "TGA data segment size: %d\n", size );
	mem_free( image_data );
	return tex;
}

GLuint texture_loadTGA( const char* filename ) {
	vmutex_lock( &gl_mutex );
	printf( "Loading TGA \"%s\"\n" , filename );
	GLuint tex;
	int w, h;
	void* img = read_tga( filename, &w, &h );

	vAssert( isPowerOf2( w ) );
	vAssert( isPowerOf2( h ) );

	if ( !img )
		return 0;	// Failed to load the texture

	// Generate a texture name and bind to that
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );

	// Set up sampling parameters, use defaults for now
	// Bilinear interpolation, clamped
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT );

	glTexImage2D( GL_TEXTURE_2D,
		   			0,			// No Mipmaps for now
					GL_RGBA,	// 3-channel, 8-bits per channel (32-bit stride)
					(GLsizei)w, (GLsizei)h,
					0,			// Border, unused
					GL_RGBA,		// TGA uses BGR order internally
					GL_UNSIGNED_BYTE,	// 8-bits per channel
					img );

	glFlush();
	printf( "Loaded TGA as gluint %u\n", tex );

	mem_free( img );	// OpenGL copies the data, so we can free this here

	vmutex_unlock( &gl_mutex );
	return tex;
}
