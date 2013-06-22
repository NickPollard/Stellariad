// ticker.c
#include "src/common.h"
#include "src/ticker.h"
//---------------------
#include "engine.h"
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
	d->data = mem_alloc(size * sizeof(void*));
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

//////////////
//
// Tick Test
//
//////////////

void tick_tester_tick(void* t, float dt) {
	(void)dt;
	tick_tester* tester = (tick_tester*)t;
	tester->tickcount += tester->tickinc;
	printf("Tick Tester: Tick count is %d.\n", tester->tickcount);
}

///////////////
