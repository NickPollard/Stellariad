// future.c
#include "common.h"
#include "future.h"
//--------------------------------------------------------
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"

// TODO - MAKE THREADSAFE!!

IMPLEMENT_LIST(handler)
IMPLEMENT_LIST(future)

void future_onComplete( future* f, handler h, void* args ) {
	if ( f->complete )
		h( f->value, args );
	else
		f->on_complete = handlerlist_cons( &h, f->on_complete ); // TODO - ARGH
}

void future_delete( future* f ) {
	handlerlist_delete( f->on_complete );
	mem_free( f );
}

future* future_create() {
	future* f = mem_alloc( sizeof( future ));
	f->complete = false;
	f->execute = false;
	return f;
}

void future_complete( future* f, const void* data ) {
	vAssert( !f->complete );
	f->value = data;
	f->complete = true;
	f->execute = true;
	printf( "Future complete! Data: %s\n", (const char*)data );
}

bool future_tryExecute( future* f ) {
	if (f->execute) {
		vAssert( f->complete );
		f->execute = false;
		//foreach( f->oncomplete, apply() );
		return true;
	}
	return false;
}

void future_executeFutures() {	/*
	for ( f : each future ) {
		future_tryExecute( f );
	}
	*/
}
		
/*
	texture* t = new_texture()
	future* f = new_future()
	...
	f->value = t

	f_tex = load_texture()
	future_onComplete( f_tex, set_texture )
	future_onComplete( lua_callback )
   */

void* completeWith( const void* data, void* f ) {
	future_complete( f, data );
	return NULL;
}

void future_completeWith( future* f, future* other ) {
	future_onComplete( f, completeWith, other );
}

void* deferredOnComplete( const void* data, void* args ) {
	(void)data;
	future_onComplete( _1(args), _1(_2(args)), _2(_2(args))); 
	return NULL;
}

future* futurePair_sequence( future* a, future* b ) {
	future* f = future_create();
	future_onComplete( a, deferredOnComplete, Pair(b, Pair(deferredOnComplete, f)) );
	return f;
}

future* futures_sequence( futurelist* fs ) {
	if (!fs) {
		future* now = future_create();
		future_complete( now, NULL );
		return now;
	}
	futurelist* next = fs->tail;
	if (!next) {
		return fs->head;
	}
	future* f = fs->head;
	while ( next && next->head ) {
		f = futurePair_sequence( next->head, f );
		next = next->tail;
	}
	return f;
}

void* runTask( const void* input, void* args ) {
	(void)input;
	worker_task* tsk = args;
	worker_addTask( *tsk );
	mem_free( tsk );
	return NULL;
}
