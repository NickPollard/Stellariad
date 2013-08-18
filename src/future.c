// future.c
#include "common.h"
#include "future.h"
//--------------------------------------------------------
#include "mem/allocator.h"

IMPLEMENT_LIST(handler)

void future_onComplete( future* f, handler h ) {
	if ( f->complete )
		h( f->value );
	else
		f->on_complete = handlerlist_cons( &h, f->on_complete );	
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
