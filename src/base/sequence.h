// sequence.h
#pragma once

#include <forward_list>
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
    //static_assert( std::is_pod<T>::value == true );
      
    // amortised O(1) add
    // Adds the element to the latest array, adding an array if required
    void add(T t);
   
    // O(n) remove
    // Returns whether element was in the array and succesfully removed
    bool remove(T t);
   
    // O(n) contains
    bool contains(T t);

    struct Iterator {
      Iterator( typename forward_list<Array<T>>::iterator arr,
                typename Array<T>::Iterator iter,
                typename forward_list<Array<T>>::iterator _end) :
                   array( arr ), end( _end ), iterator( iter ) {}

      T& operator * () { return *iterator; }

      bool operator != (Iterator other) {
        return (array != other.array) || (iterator != other.iterator);
      }

      void operator ++() {
        ++iterator;
        if ( iterator == array->end() ) {
          ++array;
          iterator = (array == end) ? nullptr : array->begin();
          //if ( !( array == end ))
            //iterator = array->begin();
          //else
            //iterator = nullptr;
        }
      }

      private:
        typename       forward_list<Array<T>>::iterator array;
        const typename forward_list<Array<T>>::iterator end;
        typename       Array<T>::Iterator 						  iterator;
    };

    Iterator begin() { 
			typename forward_list<Array<T>>::iterator first = arrays.begin();
			if (first->isEmpty()) {
				++first;
			}
			return Iterator( first, first->begin(), arrays.end() ); 
		}
    Iterator end() { return Iterator( arrays.end(), nullptr, arrays.end() ); }
    
    static const size_t defaultArraySize = 32;

		// TODO - delete copy constructor/assignment op

    Sequence() : arraySize(defaultArraySize) {
      arrays.emplace_front(arraySize); // TODO - emplace
		}
    Sequence(size_t size) : arraySize(size) {
      arrays.emplace_front(arraySize); // TODO - emplace
		}

		Sequence(const Sequence<T>& other) = delete;
		void operator =(const Sequence<T>& other) = delete;
  
    private:
      forward_list<Array<T>> arrays;
      size_t arraySize;
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
