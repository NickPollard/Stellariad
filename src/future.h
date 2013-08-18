// future.h
#include "base/list.h"

typedef void* (*handler)(const void*);

DEF_LIST(handler)

typedef struct future_s {
	const void* value;
	bool complete;
	bool execute;
	handlerlist* on_complete;
} future;

bool future_tryExecute( future* f );

void future_complete( future* f, const void* data );

void future_onComplete( future* f, handler h );

future* future_create();

void future_delete( future* f );
