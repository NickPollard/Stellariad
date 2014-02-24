// graphicsbuffer.c
#include "common.h"
#include "graphicsbuffer.h"
//-----------------------
#include "render.h"

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

#define	kMaxBufferRequests	1024
bufferRequest	buffer_requests[kMaxBufferRequests];
int				buffer_request_count = 0;

#define	kMaxBufferCopyRequests	128
bufferCopyRequest	buffer_copy_requests[kMaxBufferCopyRequests];
int				buffer_copy_request_count = 0;

vmutex buffer_mutex = kMutexInitialiser;

GLuint render_bufferCreate( GLenum target, const void* data, GLsizei size ) {
	GLuint buffer;							// The OpenGL object handle we generate
	glGenBuffers( 1, &buffer );				// Generate a buffer name - effectively just a declaration
	glBindBuffer( target, buffer );			// Bind the buffer name to a target, creating the vertex buffer object
	// Usage hint can be: GL_[VARYING]_[USE]
	// Varying: STATIC / DYNAMIC / STREAM
	// Use: DRAW / READ / COPY
	// OpenGL ES only supports dynamic/static draw
	static const int usageHint = GL_STATIC_DRAW;
	glBufferData( target, size, data, usageHint );	// Allocate the buffer, optionally copying data
	return buffer;
}


bufferRequest* newBufferRequest() {
	vAssert( buffer_request_count < kMaxBufferRequests );
	bufferRequest* r = &buffer_requests[buffer_request_count++];
	return r;
}

bufferCopyRequest* newBufferCopyRequest() {
	vAssert( buffer_copy_request_count < kMaxBufferCopyRequests );
	bufferCopyRequest* r = &buffer_copy_requests[buffer_copy_request_count++];
	return r;
}

// Asynchronously copy data to a VertexBufferObject
void render_bufferCopy( GLenum target, GLuint buffer, const void* data, GLsizei size ) {
	vmutex_lock( &buffer_mutex );
	{
		bufferCopyRequest* b = newBufferCopyRequest();
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
		b = newBufferRequest();
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

void render_freeBuffer( void* buffer ) { if ( buffer ) mem_free( buffer ); }

// Load any waiting buffer requests
void render_bufferTick() {
	vmutex_lock( &buffer_mutex );
	{
		// TODO: This could be a lock-free queue
		for ( int i = 0; i < buffer_request_count; i++ ) {
			bufferRequest* b = &buffer_requests[i];
			*b->ptr = render_bufferCreate( b->target, b->data, b->size );
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


/*
graphicsBuffer* graphicsBuffer_createStatic() {
	glGenBuffers();
	glBindBuffer( TARGET );
	// Create a buffer of the correct size
	// Not dependent on anything, should not sync
	// Just takes time to allocate (which is unavoidable)
	glBufferData( TARGET, size, NULL );
	// We know we're not using this buffer yet
	// So we can do an unsynchronised copy using mapBufferRange
	glMapBufferRange( UNSYNCHED );
	memcpy();
	glUnmap();
}

graphicsBuffer* graphicsBuffer_createDynamic() {
}

graphicsBuffer* graphicsBuffer_createStream() {
}

void graphicsBuffer_fill( void* data ) {
	glBindBuffer( buffer->target );
	glBufferSubData( buffer->size, data );

	glBindBuffer( buffer->target );
	data = glMapBuffer( buffer->target );
	memcpy( data, buffer->data, buffer->size );
	glUnmapBuffer( buffer->target );
}

void graphicsBuffer_bind( graphicsBuffer* buffer ) {
	glBindBuffer( buffer->target, buffer->object );
}

void graphicsBuffer_unbind( graphicsBuffer* buffer ) {
	glBindBuffer( buffer->target, INVALID_BUFFER );
}
*/

/* Example usage: Models

init:
   model_buffer = graphicsBuffer_createStatic( buffer_size, buffer_data );

draw:
   draw_call->buffer = block_buffer;

render:
	graphicsBuffer_bind( draw_call->buffer );


   */

/* Example usage: Terrain

init:
   block_buffer = graphicsBuffer_createStatic( buffer_size, NULL );
   graphicsBuffer_fill( buffer_data );

draw:
   draw_call->buffer = block_buffer;

render:
	graphicsBuffer_bind( draw_call->buffer );

   */

/* Example usage: Particles

   */
