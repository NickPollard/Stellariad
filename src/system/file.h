// file.h
#pragma once

extern heapAllocator* global_string_heap;

// *** Stream Reading

bool token_isFloat( const char* token );
bool token_isString( const char* token );
const char* sstring_create( const char* token );

int isNewLine( char c );

// *** File Operations

bool vfile_exists( const char* path );
FILE* vfile_open( const char* path, const char* mode );
void* vfile_contents(const char *path, size_t *length);
void vfile_writeContents( const char* path, void* buffer, int length );

bool vfile_modifiedSinceLast( const char* file );

// Static init
void vfile_systemInit();


// *** Testing

void test_sfile( );
