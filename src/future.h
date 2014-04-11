// future.h
#include "base/list.h"

typedef void* (*handler)(const void*, void*);

DEF_LIST(handler)

typedef struct future_s {
	const void* value;
	bool complete;
	bool execute;
	handlerlist* on_complete;
	// TODO - GarbageCollect
} future;

DEF_LIST(future)

bool future_tryExecute( future* f );

void future_complete( future* f, const void* data );

void future_onComplete( future* f, handler h, void* args );

future* future_create();

void future_delete( future* f );

void future_completeWith( future* f, future* other );

future* futures_sequence( futurelist* fs );

void* runTask( const void* input, void* args );
