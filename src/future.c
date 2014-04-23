// future.c
#include "common.h"
#include "future.h"
//--------------------------------------------------------
#include "engine.h"
#include "worker.h"
#include "base/pair.h"
#include "mem/allocator.h"

// TODO - MAKE THREADSAFE!!

vmutex futuresMutex = kMutexInitialiser;

IMPLEMENT_LIST(handler)
IMPLEMENT_LIST(future)

//futurelist* futures = NULL;
#define MaxFutures 1024
int futureCount = 0;
future* futures[MaxFutures];

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
	f->on_complete = NULL;
	vmutex_lock( &futuresMutex ); {
		arrayAdd(&futures, &futureCount, f );
		//futures = futurelist_cons( f, futures );
	} vmutex_unlock( &futuresMutex );
	return f;
}

void future_complete( future* f, const void* data ) {
	vmutex_lock( &futuresMutex ); {
		vAssert( !f->complete );
		f->value = data;
		f->complete = true;
		f->execute = true;
	} vmutex_unlock( &futuresMutex );
}

void future_complete_( future* f ) {
	future_complete( f, NULL );
}

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
	future* f = ff;
	vAssert( !f->complete );
	f->value = data;
	f->complete = true;
	f->execute = true;
	return NULL;
}
void* completeWithSequence( const void* data, void* ff ) {
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
	future_onComplete( a, deferredOnComplete, Pair(b, Pair(completeWithSequence, f)));
	return f;
}

future* future_(void* value) {
	future* now = future_create();
	future_complete( now, value );
	return now;
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
	worker_task* tsk = args;
	worker_addTask( *tsk );
	mem_free( tsk );
	return NULL;
}

void debugPrintFutures() {
	/*
	for ( futurelist* fs = futures; fs && fs->head; fs = fs->tail ) {
		if ( !fs->head->complete )
		printf( "Future " xPTRf " incomplete.\n", (uintptr_t)fs->head );
	}
	*/
}
