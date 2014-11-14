#pragma once

// A generic asset load function
typedef void* (*loadFunc)( const char* asset_name );

typedef struct library_s {
	map*	_map;			// The map that contains lookup information for entries	
	heapAllocator*	heap;	// Where the assets are allocated
	loadFunc		load_func;	// Which function to use to load assets
} library;

void*	library_load( library* l, const char* asset_name );
void	library_unload( library* l, void* asset );
