// sequence.h
#pragma once

#include <forward_list>
#include <type_traits>
#include "base/array.h"

/*
 * Template Sequence
 *
 * Maintains a growable sequence of elements, backed by zero or more vitae::Arrays
 * Fast insertion, removal, and traversal
 * Use when you want a dynamically sized container instead of a simple vitae::Array
 */

using std::forward_list;

namespace vitae {

  template <typename T> struct Sequence {
    // Must be used with POD types only
    static_assert( std::is_pod<T>::value );
      
    // amortised O(1) add
    // Adds the element to the latest array, adding an array if required
    void add(T t);
   
    // O(n) remove
    // Returns whether element was in the array and succesfully removed
    bool remove(T t);
   
    // O(n) contains
    bool contains(T t);

    // forward iterators
    struct iterator;
    iterator begin();
    iterator end();

    struct iterator {
      using ListIterator = typename forward_list<Array<T>>::iterator;
      using ArrayIterator = typename Array<T>::iterator;

      ArrayIterator resetArrayIter() {
        return (array == end) ? nullptr : array->begin();
      }

      iterator( ListIterator arr, ListIterator _end ) : array( arr ), end( _end ), arrIter( resetArrayIter() ) {}

      T& operator * () { return *arrIter; }

      bool operator != (iterator other) {
        return (array != other.array) || (arrIter != other.arrIter);
      }

      void operator ++() {
        ++arrIter;
        if ( arrIter == array->end() ) {
          ++array;
          arrIter = resetArrayIter();
        }
      }

      private:
        ListIterator  array;
        ListIterator  end;
        ArrayIterator arrIter;
    };
    
    static const size_t defaultArraySize = 32;

    Sequence() : arraySize(defaultArraySize) {
      arrays.emplace_front(arraySize);
		}
    Sequence(size_t size) : arraySize(size) {
      arrays.emplace_front(arraySize);
		}

		Sequence(const Sequence<T>& other) = delete;
		void operator =(const Sequence<T>& other) = delete;
  
    private:
      forward_list<Array<T>> arrays;
      size_t arraySize;
  };

  template <typename T>
    typename Sequence<T>::iterator Sequence<T>::begin() { 
	    auto first = arrays.begin();
	    if (first->isEmpty())
			++first;
		  return iterator( first, arrays.end() ); 
	  };
  template <typename T>
    typename Sequence<T>::iterator Sequence<T>::end() { 
      return iterator( arrays.end(), arrays.end() ); 
    };

  template <typename T>
    void Sequence<T>::add(T t) {
      // Add at the end (elements are contiguous)
      // invariant: last array is never full
      auto& last = arrays.front(); // TODO - what if empty?
      last.add(t);
      if (last.isFull()) {
        arrays.emplace_front(arraySize); // TODO - emplace
      }
    };

  template <typename T>
    bool Sequence<T>::remove(T t) {
      auto& last = arrays.front();
      for ( auto& a : arrays ) {
        if ( a.remove( t )) {
          if ( &a != &last ) { // if a == last, don't switch, just remove from end
            arrays.pop_front();
            auto& newlast = arrays.front();
            // invariant: newlast is now nonEmpty
            if ( &a != &newlast ) {
              a.add(newlast.popBack());
            }
          }
          return true;
        }
      }
      return false;
    };

}
