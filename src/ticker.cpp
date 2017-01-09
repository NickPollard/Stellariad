// ticker.c
#include "src/common.h"
#include "src/ticker.h"
//---------------------
#include "engine.h"
#include "base/array.h"
#include "base/delegate.h"
#include "base/sequence.h"
#include "mem/allocator.h"
#include <assert.h>

// Tick all tickers in the tick list!
// All tickers in a list share the same tick function
// This is to (hopefully) improve cache usage and debugging
void delegate_tick(delegate* d, float dt, engine* eng ) {
	for ( int i = 0; i < d->count; i++ ) {
		((tickfunc)d->tick)( d->data[i], dt, eng );
	}
}

void delegate_render( delegate* d, scene* s ) {
	for ( int i = 0; i < d->count; i++ ) {
		((renderfunc)d->tick)( d->data[i], s );
	}
}

void delegate_input( delegate* d, input* in ) {
	for ( int i = 0; i < d->count; i++ ) {
		((inputfunc)d->tick)( d->data[i], in );
	}
}

delegate* delegate_create(void* func, int size) {
	delegate* d = (delegate*)mem_alloc(sizeof(delegate));
	d->tick = func;
	d->count = 0;
	d->data = (void**)mem_alloc(size * sizeof(void*));
	d->max = size;

	return d;
}

int delegate_add(delegate* d, void* entry) {
	if (d->count < d->max) {
		d->data[d->count++] = entry;
		return true;
	}
	return false;
}

void delegate_remove( delegate* d, void* entry ) {
	array_remove( d->data, &d->count, entry );
}

int delegate_isFull( delegate* d ) {
	return d->count == d->max;
}

void f(int a, int b) {
  mem_alloc(a + b);
}

void g(int a, int b, bool d) {
  if (d)
    mem_alloc(a + b);
  else
    mem_alloc(a);
}

void h(bool a, int b, bool d) {
  if (a && d)
    mem_alloc(b);
  else
    mem_alloc(0);
}

void testD() {
  using vitae::Array;
  using vitae::Delegate;
  using vitae::DelegateList;
  using vitae::Sequence;
  auto& d = *(new Delegate<int,int>(32, f));
  d.add(1);
  d.add(2);
  d.call(12);

  auto& e = *(new Delegate<int,int,bool>(32, g));
  e.add(1);
  e.add(2);
  e.call(12,false);

  auto& dl = *(new DelegateList<int,bool>());
  dl.add(1, g);
  dl.add(2, g);
  dl.add(false, h);
  dl.call(12,false);
  dl.remove(false, h);

  auto& intArray = *(new Array<int>(32));
  intArray.add(1);
  intArray.add(2);
  for (auto& i : intArray) {
    i += 1;
  }

  auto& intSequence = *(new Sequence<int>(32));
  intSequence.add(1);
  intSequence.add(2);
  for (auto& i : intSequence) {
    i += 1;
  }
}
