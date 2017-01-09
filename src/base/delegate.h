// delegate.h
#pragma once


/*
 * Templated delegate
 *
 *
 */

#include "base/array.h"
#include "base/sequence.h"
#include <map>

using std::make_pair;

namespace vitae {

  // Existential in object type
  template <typename... Args> struct SomeDelegate {
    virtual void call(Args... args) = 0; 
  };

  template <typename T, typename... Args> struct Delegate : SomeDelegate<Args...> {
    typedef void (*Func)( T, Args... );

    virtual void call( Args... args ) {
      for (auto& el : elems)
        f( el, args... );
    }

    void add(T t)    { elems.add(t); }
    void remove(T t) { elems.remove(t); }

    Delegate(size_t size, Func _f) : f(_f), elems(size) {}

		// *** NOCOPY
		Delegate(const Delegate<T, Args...>& other) = delete;
		void operator = (const Delegate<T, Args...>& other) = delete;

    private:
      Func f; 
      Sequence<T> elems;
  };

  template <typename... Args> struct DelegateList {
    using SomeFunc = void*;
    template <typename T>
      using Func = void (*)(T, Args...);

    template <typename T>
      auto asSomeFunc(Func<T> f) -> SomeFunc { return (SomeFunc)f; }

    template <typename T>
      void add(T t, Func<T> f) {
        static const int delegateSize = 32;
        auto sf = asSomeFunc(f);
        auto i = delegates.find(sf);
        if (i == delegates.end()) { // It wasn't there, so create it
          auto* d = new Delegate<T,Args...>( delegateSize, f );
          delegates.insert( make_pair( sf, d ));
          d->add(t);
        } else {
          // This case *must* be safe, as we've found it via the correct func
          auto& d = *static_cast<Delegate<T, Args...>*>( i->second );
          d.add(t);
        }
      };

    template <typename T>
      void remove(T t, Func<T> f) {
        auto sf = asSomeFunc(f);
        auto i = delegates.find(sf);
        if (i != delegates.end()) {
          auto& d = *static_cast<Delegate<T, Args...>*>( i->second );
          d.remove(t);
        }
      };

		//	TODO - bug when calling empty array (perhaps that isn't first?)
    void call(Args... args) {
      for ( auto d : delegates )
        d.second->call( args... );
    }

		DelegateList() {}
		// *** NOCOPY
		DelegateList(const DelegateList<Args...>& other) = delete;
		void operator = (const DelegateList<Args...>& other) = delete;

    private:
      std::map<SomeFunc, SomeDelegate<Args...>*> delegates;
  };


}
