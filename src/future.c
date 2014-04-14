// future.c
#include "common.h"
#include "future.h"
//--------------------------------------------------------
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"

// TODO - MAKE THREADSAFE!!

vmutex futuresMutex = kMutexInitialiser;

IMPLEMENT_LIST(handler)
IMPLEMENT_LIST(future)

futurelist* futures = NULL;

future* future_onComplete( future* f, handlerfunc hf, void* args ) {
	vmutex_lock( &futuresMutex ); {
		if ( f->complete )
			hf( f->value, args );
		else {
			handler* h = mem_alloc(sizeof(handler));
			h->func = hf;
			h->args = args;
			f->on_complete = handlerlist_cons( h, f->on_complete ); // TODO - ARGH
		}
	} vmutex_unlock( &futuresMutex );
	return f;
}

void future_delete( future* f ) {
	vmutex_lock( &futuresMutex ); {
		handlerlist_delete( f->on_complete );
		mem_free( f );
	} vmutex_unlock( &futuresMutex );
}

future* future_create() {
	future* f = mem_alloc( sizeof( future ));
	f->complete = false;
	f->execute = false;
	futures = futurelist_cons( f, futures );
	f->on_complete = NULL;
	return f;
}

void future_complete( future* f, const void* data ) {
	vmutex_lock( &futuresMutex ); {
		vAssert( !f->complete );
		f->value = data;
		f->complete = true;
		f->execute = true;
		//printf( "Future complete! Data: %s\n", (const char*)data );
	} vmutex_unlock( &futuresMutex );
}

void future_complete_( future* f ) {
	future_complete( f, NULL );
}

bool future_tryExecute( future* f ) {
	if (f->execute) {
		vAssert( f->complete );
		f->execute = false;
		for ( handlerlist* hl = f->on_complete; hl; hl = hl->tail )
			if (hl->head) {
				handler handlr = *hl->head;
				(void)handlr;
				//printf( "tryExecute " xPTRf ":" xPTRf "\n", (uintptr_t)f, (uintptr_t)handlr->func );
				handlr.func(f->value, handlr.args);
				//mem_free( handlr.args ); /// TODO - ??
			}
		return true;
	}
	return false;
}

void future_executeFutures() {
	vmutex_lock( &futuresMutex ); {
		futurelist* fl = futures;
		while ( fl ) {
			if ( fl->head )
				future_tryExecute( fl->head );
			fl = fl->tail;
		}
	} vmutex_unlock( &futuresMutex );
}

void futures_tick( float dt ) {
	(void)dt;
	future_executeFutures();
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

void* completeWith( const void* data, void* ff ) {
	// NOTE - we don't lock here otherwise we hit re-entry
	future* f = ff;
	vAssert( !f->complete );
	f->value = data;
	f->complete = true;
	f->execute = true;
	return NULL;
}

void future_completeWith( future* f, future* other ) {
	future_onComplete( other, completeWith, f );
}

future* future_onCompleteUNSAFE( future* f, handlerfunc hf, void* args ) {
	if ( f->complete )
		hf( f->value, args );
	else {
		handler* h = mem_alloc(sizeof(handler));
		h->func = hf;
		h->args = args;
		f->on_complete = handlerlist_cons( h, f->on_complete ); // TODO - ARGH
	}
	return f;
}

// Args -> Pair( future, Pair( func, args ))
void* deferredOnComplete( const void* data, void* args ) {
	(void)data;
	future_onCompleteUNSAFE( _1(args), _1(_2(args)), _2(_2(args))); 
	return NULL;
}

future* futurePair_sequence( future* a, future* b ) {
	future* f = future_create();
	future_onComplete( a, deferredOnComplete, Pair(b, Pair(completeWith, f)));
	return f;
}

future* futures_sequence( futurelist* fs ) {
	if (!fs) {
		future* now = future_create();
		future_complete( now, NULL );
		return now;
	}
	futurelist* next = fs->tail;
	if (!next)
		return fs->head;
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
