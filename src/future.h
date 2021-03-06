// future.h
#include "base/list.h"
#include "system/thread.h"

extern vmutex futuresMutex;

typedef void* (*handlerfunc)(const void*, void*);

typedef struct handler_s { 
	handlerfunc func;
	void* args;
} handler;

DEF_LIST(handler)

struct future_s {
	const void* value;
	bool complete;
	bool execute;
	handlerlist* on_complete;
	// Memory optimization; we have one handler&list inline
	handlerlist hl;
	handler h;
	// TODO - GarbageCollect
};

DEF_LIST(future)

bool future_tryExecute( future* f );

future* future_complete( future* f, const void* data );

void future_complete_( future* f );

future* future_onComplete( future* f, handlerfunc h, void* args );

future* future_create();

void future_delete( future* f );

void future_completeWith( future* f, future* other );

future* futures_sequence( futurelist* fs );

void* runTask( const void* input, void* args );

void futures_tick( float dt );
