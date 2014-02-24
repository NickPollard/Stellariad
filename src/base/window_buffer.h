// window_buffer.h

struct window_buffer_s {
	// This is the index of the window buffer that corresponds to the first element in the window
	size_t head;
	// This is the index of the window buffer that corresponds to the last element in the window
	// (most of the time this will be (head + size - 1) % size
	size_t tail;
	// This is the index of the underlying stream that corresponds to the first element in the window
	// (That is, elements[head])
	size_t stream_position;
	// The actual buffer
	void* elements;
	// How many elements there are
	size_t window_size;

	size_t element_size; // Size of contained elements
};

window_buffer* window_bufferCreate( size_t elementSize, size_t count );

// TODO - should this be exposed?
size_t windowBuffer_streamPosition( window_buffer* w, size_t position );
size_t windowBuffer_endPosition( window_buffer* w );
size_t windowBuffer_index( window_buffer* w, size_t index );
size_t windowBuffer_mappedPosition( window_buffer* w, size_t position );

canyonData canyonBuffer_value( window_buffer* buffer, int i );
vector canyonBuffer_point( window_buffer* buffer, int i );
vector canyonBuffer_normal( window_buffer* buffer, int i );
void canyonBuffer_write( window_buffer* buffer, int i, size_t size, void* data );
