// future.c

IMPLEMENT_LIST(handler)

void future_onComplete( future* f, handler* h ) {
	if ( f->complete )
		h();
	else
		f->on_complete = cons( h, f->on_complete );	
}

void future_delete( future* f ) {
	list_delete( f->on_complete );
	mem_free( f )
}

future* future_create() {
	future* f = mem_alloc( sizeof( future ));
	f->complete = false;
	return f;
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
