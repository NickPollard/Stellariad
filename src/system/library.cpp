#include "common.h"
#include "library.h"
//---------------------
#include "system/hash.h"

// The refcount is stored just before the asset
// ie sizeof(int) bytes before
//				  PTR
//	| refcount	| asset -------- |
int* getRefCount( void* asset ) {
	return (((int*)asset) - 1);
}

void addref( void* asset ) {
	++(*getRefCount( asset ));
}

void unref( void* asset ) {
	--(*getRefCount( asset ));
}

void* library_find( library* l, const char* asset_name ) {
	return map_find( l->_map, mhash( asset_name ));
}

void library_add( library* l, const char* asset_name, void* asset ) {
	map_add( l->_map, mhash( asset_name ), asset );
	addref( asset );
}

/*
   Given a filename for a texture to load, looks it up in the library
   If it's already there, just return a handle to it
   otherwise load it and then return a handle to it
   */
void* library_load( library* l, const char* asset_name ) {
	void* asset = library_find( l, asset_name );
	if ( !asset ) {
		asset = l->load_func( asset_name );
		library_add( l, asset_name, asset );
		return asset;
	}
	else {
		addref( asset );
		return asset;
	}
}

/*
	Unload an asset, as we are finished with it
	If the assets are referenced counted, this will just decrement the 
	reference count.
   */
void library_unload( library* l, void* asset ) {
	(void)l;
	unref( asset );
	if ( *getRefCount( asset ) == 0 ) {
		// Unload the asset
	}
}

