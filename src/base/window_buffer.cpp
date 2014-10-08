// window_buffer.c
#include "common.h"
#include "window_buffer.h"
//-------------------------
#include "canyon.h"
#include "maths/vector.h"
#include "mem/allocator.h"

void window_bufferInit( window_buffer* buffer, size_t size, void* elements ) {
	buffer->head = 0;
	buffer->tail = 0;
	buffer->stream_position = 0;
	buffer->window_size = size;
	buffer->elements = elements;
}

window_buffer* window_bufferCreate( size_t elementSize, size_t count ) {
	window_buffer* w = (window_buffer*)mem_alloc( sizeof( window_buffer ));
	window_bufferInit( w, count, mem_alloc( elementSize * count ));
	return w;
}

// Return the last stream position currently mapped in the window
size_t windowBuffer_endPosition( window_buffer* w ) {
	return w->stream_position + w->window_size - 1;
}

// Find where in the window_buffer, the underly stream POSITION is mapped
size_t windowBuffer_mappedPosition( window_buffer* w, size_t position ) {
	return ( position - w->stream_position + w->head ) % w->window_size;
}

// Find where in the underlying stream, the window buffer POSITION is from
size_t windowBuffer_streamPosition( window_buffer* w, size_t position ) {
	return ( position - w->head + w->window_size ) % w->window_size + w->stream_position;
}

size_t windowBuffer_index( window_buffer* w, size_t index ) {
	return ( index + w->window_size ) % w->window_size;
}


// TODO - do we need these?
void canyonBuffer_write( window_buffer* buffer, int i, size_t size, void* data ) {
	void* values = buffer->elements;
	void* pos = ((uint8_t*)values) + size * i;
	memcpy( pos, data, size );
}

canyonData canyonBuffer_value( window_buffer* buffer, int i ) {
	canyonData* values = (canyonData*)buffer->elements;
	return values[i];
}
vector canyonBuffer_point( window_buffer* buffer, int i ) { return canyonBuffer_value( buffer, i ).point; }
vector canyonBuffer_normal( window_buffer* buffer, int i ) { return canyonBuffer_value( buffer, i ).normal; }

void windowBuffer_setHead( window_buffer* buffer, size_t h ) {
	buffer->stream_position = windowBuffer_streamPosition( buffer, h );
	buffer->head = h;
}


// Seek the canyon window_buffer forward so that its stream_position is now SEEK_POSITION
void windowBuffer_seekForward( canyon* c, window_buffer* buffer, size_t seek_position ) {
	vAssert( seek_position >= buffer->stream_position );
	size_t stream_end_position = windowBuffer_endPosition( buffer );
	while ( stream_end_position < seek_position ) {
		// The seek target is not in the window so we flush everything and need to generate
		// enough points to reach the seek_target before doing final canyonBuffer_generatePoints
		windowBuffer_setHead( buffer, windowBuffer_index( buffer, buffer->tail - 2 ));
		size_t from = windowBuffer_streamPosition( buffer, buffer->tail ) + 1;
		size_t to = buffer->stream_position + buffer->window_size;
		canyonBuffer_generatePoints( c, from, to );
		stream_end_position = windowBuffer_endPosition( buffer );
	}

	// The seek target is now in the window
	buffer->head = windowBuffer_mappedPosition( buffer, seek_position );
	buffer->stream_position = seek_position;
	size_t from = windowBuffer_streamPosition( buffer, buffer->tail ) + 1;
	size_t to = buffer->stream_position + buffer->window_size;
	canyonBuffer_generatePoints( c, from, to );
}
