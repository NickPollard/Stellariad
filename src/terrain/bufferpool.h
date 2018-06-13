// bufferpool.h
template<typename T>
struct BufferPool {
  const int poolSize;
  const int bufferSize;
  int count;
  T** buffers;

  BufferPool( int ps, int bs ) : poolSize(ps), bufferSize(bs) {}
  static   BufferPool& init( int poolSize, int bufferSize );

  T*       takeBuffer();
  void     releaseBuffer( T* buffer );
};

template<typename T>
BufferPool<T>& BufferPool<T>::init( int poolSize, int bufferSize ) {
  void* mem = mem_alloc(sizeof(BufferPool<T>));
  BufferPool<T>& pool = *(new(mem) BufferPool<T>( poolSize, bufferSize ));

  pool.buffers = (T**)mem_alloc( poolSize * sizeof( T* ));
  pool.count = 0;
  for ( int i = 0; i < poolSize; i++ )
    pool.buffers[i] = (T*)mem_alloc( sizeof( T ) * bufferSize );
  return pool;
}

template<typename T>
void BufferPool<T>::releaseBuffer( T* buffer ) {
  // Find the buffer in the list, switch it with the last
  const int last = count-1;
  const int i = array_find( (void**)buffers, count, buffer );
  vAssert(i >= 0 && i <= last);
  if (i != last) {
    buffers[i] = buffers[last];
    buffers[last] = buffer;
  }
  --count;
}

template<typename T>
T* BufferPool<T>::takeBuffer() {
  vAssert( count < poolSize );
  T* buffer = buffers[count++];
  return buffer;
}
