// texture.c

#include "common.h"
#include "render/texture.h"
//---------------------
#include "future.h"
#include "worker.h"
#include "maths/maths.h"
#include "mem/allocator.h"
#include "render/render.h"
#include "system/file.h"
#include "system/hash.h"
#include "system/string.h"
#include "system/thread.h"

#define RENDER_USE_MIPMAPPING
#define RENDER_TEXTURE_LOAD_ASYNC
#define kTextureHeapSize 1024*1024*48

// *** Forward Declarations
uint8_t* read_tga( const char* file, int* w, int* h );
void texture_requestMem( GLuint* tex, int w, int h, int stride, uint8_t* bitmap, GLuint wrap_s, GLuint wrap_t );

// Globals
texture* static_texture_default = NULL;
texture* static_texture_reflective = NULL;
vmutex	texture_mutex = kMutexInitialiser;
static heapAllocator*	texture_heap = NULL;

typedef struct textureRequest_s {
	GLuint* tex;
	enum textureRequestType type;
	const char* filename;
	uint8_t*	bitmap;
	int			width;
	int			height;
	int			stride;
	textureProperties properties;
	future*		future;
} textureRequest;

#define kMaxTextureRequests 32
textureRequest requests[kMaxTextureRequests];
int texture_request_count = 0;

// Load any waiting texture requests
void texture_tick() {
	vmutex_lock( &texture_mutex );
	{
		// TODO: This could be a lock-free queue
		for ( int i = 0; i < texture_request_count; i++ ) {
			textureRequest r = requests[i];
			switch ( requests[i].type ) {
				case kTextureFileRequest:
					*(r.tex) = texture_loadTGA( r.filename );
					mem_free( (void*)r.filename );
					break;
				case kTextureMemRequest:
					vAssert( r.bitmap );
					*(r.tex) = texture_loadBitmap( r.width, r.height, r.stride, r.bitmap, r.properties.wrap_s, r.properties.wrap_t );
					// Need to clear up where we free this - we won't always want to do it but we might
					texture_free( r.bitmap );
					break;
			}
			//vAssert( r.future );
			//future_complete( r.future, r.filename );
			// TODO - refcount
			//future_delete( r.future );
			//r.future = NULL;
		}
		texture_request_count = 0;
	}
	vmutex_unlock( &texture_mutex );
}

void* texture_workerLoadFile( void* args ) {
	void** arg_ptrs = args;
	GLuint*				tex			= arg_ptrs[0];
	const char*			filename	= arg_ptrs[1];
	textureProperties*	properties	= arg_ptrs[2];
	//printf( "Run worker task: Tex: " xPTRf ", filename: %s\n", (uintptr_t)tex, filename );

	int w, h;
	void* bitmap = read_tga( filename, &w, &h );

	int stride = 4; // Currently we only support RGBA8
	texture_requestMem( tex, w, h, stride, bitmap, properties->wrap_s, properties->wrap_t );
	// texture_requestMem copies image data, so we can safely free now
	texture_free( bitmap );

	// Args were allocated when task was created, so we can safely free them here
	mem_free( args );
	return NULL;
}
		
textureRequest* textureRequest_new() {
	textureRequest* r = &requests[texture_request_count++];
	//vAssert( !r->future );
	//r->future = future_create();
	return r;
}

// Load a texture from an internal block of memory as a bitmap
void texture_requestMem( GLuint* tex, int w, int h, int stride, uint8_t* bitmap, GLuint wrap_s, GLuint wrap_t ) {
	vmutex_lock( &texture_mutex );
	{
		vAssert( texture_request_count < kMaxTextureRequests );
		textureRequest* request = textureRequest_new();

		request->tex = tex;
		*request->tex = kInvalidGLTexture;
		request->type = kTextureMemRequest;
		size_t bitmap_size = sizeof( uint8_t ) * w * h * stride;
		request->bitmap = texture_allocate( bitmap_size );
		memcpy( request->bitmap, bitmap, bitmap_size );
		request->width = w;
		request->height = h;
		request->stride = stride;
		request->properties.wrap_s = wrap_s;
		request->properties.wrap_t = wrap_t;
	}
	vmutex_unlock( &texture_mutex );
}

void texture_queueWorkerTextureLoad( GLuint* tex, const char* filename, textureProperties* properties ) {
	const void** args = mem_alloc( sizeof( void* ) * 3 );
	args[0] = tex;
	args[1] = filename;
	args[2] = properties;
	//printf( "Queue worker task: Tex: " xPTRf ", filename: %s\n", (uintptr_t)tex, filename );
	worker_task texture_load_task;
	texture_load_task.func = texture_workerLoadFile;
	texture_load_task.args = args;
	worker_addTask( texture_load_task );
}

void texture_requestFile( GLuint* tex, const char* filename, textureProperties* properties ) {
	texture_queueWorkerTextureLoad( tex, filename, properties );
}

void texture_init( texture* t, const char* filename ) {
	t->gl_tex = kInvalidGLTexture;
	t->filename = string_createCopy( filename );
}

// Hash map of pointers to textures
#define kTextureCacheMaxEntries 256
map* texture_cache = NULL;

void textureCache_init() {
	vAssert( texture_cache == NULL );
	texture_cache = map_create( kTextureCacheMaxEntries, sizeof( texture* ));
}

texture* textureCache_find( const char* filename ) {
	void** result = map_find( texture_cache, mhash( filename ));
	if ( result )
		return *((texture**)result);
	else
		return NULL;
}

void textureCache_add( texture* t, const char* filename ) {
	map_add( texture_cache, mhash( filename ), &t );
}

#define kTexturesMax 1024
texture textures[kTexturesMax];
int texture_count = 0;

texture* texture_nextEmpty() {
	return &textures[texture_count++];
}

// Get a texture matching a given filename
// Pulls it from cache if existing, otherwise loads it asynchronously
texture* texture_load( const char* filename ) {
	textureProperties* properties = mem_alloc( sizeof( textureProperties ));
	properties->wrap_s = GL_REPEAT;
	properties->wrap_t = GL_REPEAT;
	return texture_loadWithProperties( filename, properties );
}


// Get a texture matching a given filename
// Pulls it from cache if existing, otherwise loads it asynchronously
texture* texture_loadWithProperties( const char* filename, textureProperties* properties ) {
	texture* t;
	t = textureCache_find( filename );
	if ( t ) {
		return t;
	}
	else {
		//printf( "Loading Texture \"%s\".\n", filename );
		t = texture_nextEmpty();
		texture_init( t, filename );
		textureCache_add( t, filename );
#ifdef RENDER_TEXTURE_LOAD_ASYNC
		texture_requestFile( &t->gl_tex, filename, properties );
#else	
		const void** args = mem_alloc( sizeof( void* ) * 3 );
		args[0] = &t->gl_tex;
		args[1] = filename;
		args[2] = properties;
		texture_workerLoadFile( args );
#endif // RENDER_TEXTURE_LOAD_ASYNC
	}
	return t;
}



texture* texture_loadFromMem( int w, int h, int stride, uint8_t* bitmap ) {
	texture* t = texture_nextEmpty();
	texture_requestMem( &t->gl_tex, w, h, stride, bitmap, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
	return t;
}

void texture_delete( texture* t ) {
	(void)t;
	// Do nothing right now?
	// TODO - this should potentially free the GL_TEX if no longer referenced
	// Also update which is the next_free texture	
}

uint8_t* read_tga( const char* file, int* w, int* h ) {
	size_t length = 0;
	u8* image_data = vfile_contents( file, &length );
	if ( image_data == 0 ) {
		printf( "ERROR: Error reading TGA.\n" );
		vAssert( 0 );
	}

	tga_header* header = (tga_header*)image_data;
	u8* body = image_data + sizeof(tga_header);

	static const int kTrueColor_Uncompressed = 0x2;
	if ( header->image_type != kTrueColor_Uncompressed ) {
		printf( "ERROR: TGA is not uncompressed and in TrueColor.\n" );
		assert( 0 );
	}

	int width = header->width;
	int height = header->height;
	u8* id_field = body;
	(void)id_field;
	u8* color_map = body + header->id_length;
	tga_colormap_spec* mapspec = (tga_colormap_spec*)header->color_map_spec;
	u8* pixels = color_map + mapspec->entry_count;

	int pixel_bytes = header->pixel_depth / 8;
	int size = width * height * pixel_bytes;
	//printf( "Read TGA: Loading file \"%s\"\n", file );
	uint8_t* tex = texture_allocate( size );
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

	mem_free( image_data );
	return tex;
}

GLuint texture_loadTGA( const char* filename ) {
	//printf( "TEXTURE: Loading TGA \"%s\"\n" , filename );
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
#ifdef RENDER_USE_MIPMAPPING
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
#else
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
#endif // RENDER_USE_MIPMAPPING
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

#ifdef RENDER_USE_MIPMAPPING
	glGenerateMipmap( GL_TEXTURE_2D );
#endif // RENDER_USE_MIPMAPPING

	texture_free( img );	// OpenGL copies the data, so we can free this here

	return tex;
}

GLuint texture_loadBitmap( int w, int h, int stride, uint8_t* bitmap, GLuint wrap_s, GLuint wrap_t ) {
	GLuint tex;

	vAssert( isPowerOf2( w ) );
	vAssert( isPowerOf2( h ) );

	vAssert( bitmap );
	vAssert( stride == 4 ); // Only support RGBA8 right now

	// Generate a texture name and bind to that
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );

	// Set up sampling parameters, use defaults for now
	// Bilinear interpolation, clamped
#ifdef RENDER_USE_MIPMAPPING
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
#else
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
#endif // RENDER_USE_MIPMAPPING
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	// TODO - set this properly. For now force to clamp for loadBitmap, for the terrain lookup texture
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     wrap_s );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     wrap_t );
	
	glTexImage2D( GL_TEXTURE_2D,
		   			0,			// No Mipmaps for now
					GL_RGBA,	// 3-channel, 8-bits per channel (32-bit stride)
					(GLsizei)w, (GLsizei)h,
					0,			// Border, unused
					GL_RGBA,		// 
					GL_UNSIGNED_BYTE,	// 8-bits per channel
					bitmap );

#ifdef RENDER_USE_MIPMAPPING
	glGenerateMipmap( GL_TEXTURE_2D );
#endif // RENDER_USE_MIPMAPPING

	return tex;
}

void* texture_allocate( size_t size ) {
	void* mem = heap_allocate( texture_heap, size );
	//printf( "TEXTURE: Allocating " dPTRf " bytes as 0x" xPTRf "\n", size, (uintptr_t)mem );
	return mem;
}

void texture_free( void* tex ) {
	//printf( "TEXTURE: Freeing 0x" xPTRf " bytes\n", (uintptr_t)tex );
	heap_deallocate( texture_heap, tex );
}

void texture_staticInit() {
	texture_heap = heap_create( kTextureHeapSize );
	assert( texture_heap->total_allocated == 0 );

	textureCache_init();

	static_texture_default = texture_load( "dat/img/test64rgba.tga" );
	static_texture_reflective = texture_load( "dat/img/CGSkies_0037_free.tga" );

	memset( requests, 0, sizeof( textureRequest ) * kMaxTextureRequests );
}

void textureBilinear() {
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
}

void textureNearest() {
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
}

void textureClamped() {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE );
}

GLuint textureBilinearClamped() {
	GLuint t;
	glGenTextures( 1, &t );
	glBindTexture( GL_TEXTURE_2D, t );
	textureBilinear();
	textureClamped();
	return t;
}

void textureUnbind() {
	glBindTexture( GL_TEXTURE_2D, 0 );
}
