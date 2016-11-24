// future.c
#include "common.h"
#include "future.h"
//--------------------------------------------------------
#include "engine.h"
#include "worker.h"
#include "base/array.h"
#include "base/pair.h"
#include "mem/allocator.h"

vmutex futuresMutex = kMutexInitialiser;

IMPLEMENT_LIST(handler)
IMPLEMENT_LIST(future)

//futurelist* futures = NULL;
#define MaxFutures 16384
int futureCount = 0;
future* futures[MaxFutures];

future* future_onCompleteUNSAFE( future* f, handlerfunc hf, void* args ) {
	if ( f->complete )
		hf( f->value, args );
	else {
		handler* h = f->on_complete ? (handler*)mem_alloc(sizeof(handler)) : &f->h; // Use inline handler if first onComplete
		h->func = hf;
		h->args = args;
		if ( f->on_complete )
			f->on_complete = handlerlist_cons( h, f->on_complete );
		else {
			f->hl.head = h;
			f->hl.tail = NULL;
			f->on_complete = &f->hl;
		}
	}
	return f;
}

future* future_onComplete( future* f, handlerfunc hf, void* args ) {
	vmutex_lock( &futuresMutex ); {
		future_onCompleteUNSAFE( f, hf, args );
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
	future* f = (future*)mem_alloc( sizeof( future ));
	f->complete = false;
	f->execute = false;
	f->on_complete = NULL;
	f->hl.head = NULL;
	f->hl.tail = NULL;
	f->h.func = NULL;
	f->h.args = NULL;
	vmutex_lock( &futuresMutex ); {
		vAssert( futureCount + 1 < MaxFutures );
		arrayAdd(&futures, &futureCount, f );
	} vmutex_unlock( &futuresMutex );
	return f;
}

future* future_complete( future* f, const void* data ) {
	vmutex_lock( &futuresMutex ); {
		vAssert( !f->complete );
		f->value = data;
		f->complete = true;
		f->execute = true;
	} vmutex_unlock( &futuresMutex );
	return f;
}

void future_complete_( future* f ) { future_complete( f, NULL ); }

bool future_tryExecute( future* f ) {
	if (f->execute) {
		//printf( "Executing future " xPTRf "\n", (uintptr_t)f );
		vAssert( f->complete );
		f->execute = false;
		for ( handlerlist* hl = f->on_complete; hl; hl = hl->tail )
			if (hl->head) {
				handler handlr = *hl->head;
				(void)handlr;
				//printf( "tryExecute " xPTRf ":" xPTRf "\n", (uintptr_t)f, (uintptr_t)handlr->func );
				handlr.func(f->value, handlr.args);
				//mem_free( handlr.args ); /// TODO - ??
				arrayRemove(&futures, &futureCount, f );
			}
		return true;
	}
	return false;
}

void future_executeFutures() {
	vmutex_lock( &futuresMutex ); {
		// TODO - cleanup
		for ( int i = 0; i < futureCount; ++i )
			future_tryExecute( futures[i] );
	} vmutex_unlock( &futuresMutex );
}

void futures_tick( float dt ) {
	(void)dt;
	future_executeFutures();
}

void* completeWith( const void* data, void* ff ) {
	// NOTE - we don't lock here otherwise we hit re-entry
	future* f = (future*)ff;
	vAssert( !f->complete );
	f->value = data;
	f->complete = true;
	f->execute = true;
	return NULL;
}

void future_completeWith( future* f, future* other ) {
	future_onComplete( other, completeWith, f );
}

void* deferredOnComplete( const void* data, void* args ) {
	(void)data;
	future_onCompleteUNSAFE( (future*)_1(args), (handlerfunc)_2(args), _3(args)); 
	mem_free( args );
	return NULL;
}

future* futurePair_sequence( future* a, future* b ) {
	future* f = future_create();
	future_onComplete( a, deferredOnComplete, Triple(b, (void*)completeWith, f));
	return f;
}

future* future_(void* value) {
	return future_complete( future_create(), value );
}

future* futures_sequence( futurelist* fs ) {
	if (!fs)
		return future_( NULL );
	futurelist* next = fs->tail;
	future* f = fs->head;
	for ( ; next && next->head; next = next->tail )
		f = futurePair_sequence( next->head, f );
	return f;
}

void* runTask( const void* input, void* args ) {
	(void)input;
	worker_task* tsk = (worker_task*)args;
	worker_addTask( *tsk );
	mem_free( tsk );
	return NULL;
}
