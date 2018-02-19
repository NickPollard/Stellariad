// Array.h
#pragma once

#include "mem/allocator.h"

// *** Array funcs
#define arrayContains( a, b, c ) array_contains( (void**)(a), (b), (void*)(c) )
bool array_contains( void** array, int count, void* ptr );
#define arrayAdd( a, b, c ) array_add( (void**)(a), (b), (c) )
void array_add( void** array, int* count, void* ptr );
#define arrayRemove( a, b, c ) array_remove( (void**)(a), (b), (c) )
void array_remove( void** array, int* count, void* ptr );
int array_find( void** array, int count, void* ptr );

/* 
 * Template Array
 *
 * Maintains a packed, unsorted array of elements for fast inserting, removing and iterating
 * Designed for efficient cache use when iterating elements in-order, not for random access
 */

namespace vitae {
  template <typename T> struct Array {
    // Must be used with POD types only
    //static_assert( std::is_pod<T>::value == true );

    // O(1) add
    // returns whether was able to successfully add (or not if array is full)
    bool add(T t);

    // O(n) remove
    // Returns whether element was in the array and succesfully removed
    bool remove(T t);

    // O(n) contains
    bool contains(T t);

    // O(n) find
    int find(T t);

    // O(1) size
    size_t size() { return count; }

    // O(1) isFull
    bool isFull() { return (count == max); }
    // O(1) isEmpty
    bool isEmpty() { return (count == 0); }

    // O(1) popBack
    T popBack();

    using iterator = T*;

    iterator begin() { return &buffer[0]; }
    iterator end() { return &buffer[count]; }

    T* asArray() { return buffer; }

    Array(size_t maxItems) : buffer((T*)mem_alloc(sizeof(T) * maxItems)), max(maxItems), count(0) {
			memset(buffer, 0, sizeof(T) * maxItems);
		}

    ~Array() { 
			mem_free( buffer ); 
		}

		Array(const Array<T>& other) = delete;
		void operator =(const Array<T>& other) = delete;

    private:
      T*     buffer;
      size_t max;
      size_t count;
  };


  template <typename T>
    bool Array<T>::add(T t) {
      if (!isFull()) {
	      buffer[count++] = t;
        return true;
      }
      return false;
    };

  template <typename T>
    bool Array<T>::remove(T t) {
	    int i = find( t );
	    if ( i != -1 ) {
        // switch with end
		    --count;
		    buffer[i] = buffer[count];
		    memset(&buffer[count], 0, sizeof(T)); // zero out removed mem
        return true;
	    }
      return false;
    };

  template <typename T>
    int Array<T>::find(T t) {
	    for ( size_t i = 0; i < count; ++i )
		    if ( buffer[i] == t )
			    return i;
      return -1;
    };

  template <typename T>
    bool Array<T>::contains(T t) { return (find(t) != -1); };

  template <typename T>
    T Array<T>::popBack() {
      return buffer[--count];
    };


  /* 
   * Fixed-sized immutable Array of elements
   * TODO - owning vs. non-owning?
   */
  template <typename T> struct Slice {
    public:
      const T& operator [](size_t index) const { return elems[index]; }

      size_t size() { return _size; }

      //static Slice<T> fromVec( const Vector<T>& v ) {
        //return Slice<T>( v.size(), v.elems() );
      //}

      Slice(size_t initialSize, T* ptr) : _size(initialSize), elems(ptr) {}
    private:
      size_t _size;
      T*     elems;
  };
}
