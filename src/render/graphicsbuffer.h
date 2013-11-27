// graphicsbuffer.h

#include "render/vgl.h"

struct graphicsBuffer_s {
};

typedef struct graphicsBuffer_s graphicsBufer;

GLuint render_glBufferCreate( GLenum target, const void* data, GLsizei size );

// Asynchronosuly create a GPU buffer
GLuint* render_requestBuffer( GLenum target, const void* data, GLsizei size );
// Free it
void render_freeBuffer( void* buffer );

// Asynchronously copy data to a GPU  buffer
void render_bufferCopy( GLenum target, GLuint buffer, const void* data, GLsizei size );

// Tick the buffer system (to load buffers asynchronously). Should only be called from the render thread!
void render_bufferTick();
