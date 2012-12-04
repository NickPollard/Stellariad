// inputstream.h
#pragma once
#include "system/file.h"

#define kStreamTokenBufferLength 64

typedef struct inputStream_s {
	const char* source; // Never changes once initialised
	const char* stream; // Read head
	const char* end;	// One-past-the-end
	char token_buffer[kStreamTokenBufferLength];
} inputStream;

inputStream*	inputStream_create( const char* source );
char*	inputStream_nextToken( inputStream* stream );
void	inputStream_freeToken( inputStream* stream, const char* token );
void	inputStream_skipToken( inputStream* stream );
bool	inputStream_endOfFile( inputStream* in );
void	inputStream_nextLine( inputStream* in );
void	inputStream_skipPast( inputStream* stream, const char* string );

bool isListStart( char c );
bool isListEnd( char c );
