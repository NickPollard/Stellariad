// string.h

#pragma once

// Initialise the string system (builds the global string heap)
void string_staticInit();

// Create a new copy of the string pointed to by SRC
// The new string is mem_alloced on the global heap
const char* string_createCopy( const char* src );

// Return TRUE if the contents (i.e. text) of string A and B are identical
bool string_equal( const char* a, const char* b );

// Return a new copy of string SRC with leading and trailing whitespace removed
// The new copy is mem_alloced on the global heap
const char* string_trim( const char* src );

// Copy string SRC to DST with leading and trailing whitespace removed
const char* string_trimCopy( char* dst, const char* src );

bool charset_contains( const char* charset, char c );

typedef struct streamWriter_s {
	char* string;
	char* write_head;
	char* end;
} streamWriter;

streamWriter* streamWriter_create( char* buffer, char* end );
void stream_printf( streamWriter* stream, const char* fmt, ... );

void test_string( );
