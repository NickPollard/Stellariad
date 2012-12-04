// inputstream.c
#include "common.h"
#include "inputstream.h"
//-----------------------
#include "mem/allocator.h"
#include "system/string.h"

//
// *** Parsing
//

// Is the character a whitespace character?
// eg. space, tab
int isWhiteSpace( char c ) {
	return ( c == ' ' || 
			// c == '\n' || 
			 c == '\t' );
}

// Is the character a newline?
int isNewLine( char c ) {
	return ( c == '\n' );
	}

// Is the character a terminator character
// Eg. NULL-terminator or end of line
int isTerminator( char c ) {
	return ( c == '\0' /*|| c == '\n'*/ ); // Or EoF
}

bool isListStart( char c ) {
	return c == '(';
}

bool isListEnd( char c ) {
	return c == ')';
}

//
// *** Stream Reading
//

void inputStream_reset( inputStream* in ) {
	in->stream = in->source;
}

inputStream* inputStream_create( const char* source ) {
	inputStream* in = mem_alloc( sizeof( inputStream ));
	assert( in );
	in->source = source;
	in->stream = in->source;
	in->end = in->source + strlen( in->source );
	return in;
}

// Returns the next token as a c string, advances the inputstream to the token's end
char* inputStream_nextToken( inputStream* stream ) {
	// parse leading whitespace
	while ( isWhiteSpace( *stream->stream ) )
			stream->stream++;
	// measure the length of the token - ptr should become one-past-the-end
	const char* ptr = stream->stream;
	// Special case for newlines
	if ( isNewLine( *ptr ))
	{
		++ptr;
	}
	else {
		while ( !isWhiteSpace( *ptr ) && !isNewLine( *ptr ) && !isTerminator( *ptr )) {
			if ( isListStart( *ptr ) || isListEnd( *ptr ) ) {
				if ( ( ptr - stream->stream ) == 0 ) // if parenthesis is first char, push pas it
					++ptr;	
				break;
			}
			++ptr;
		}
	}
	// make a copy of it
	int length = ptr - stream->stream;
	char* token;
	if ( length < kStreamTokenBufferLength ) {
		token = &stream->token_buffer[0];
	}
	else {
		token = mem_alloc( sizeof( char ) * (length + 1) );
	}
	strncpy( token, stream->stream, length );
	token[length] = '\0';
	stream->stream = ptr; // Advance past the end of the token
	return token;
}

void inputStream_freeToken( inputStream* stream, const char* token ) {
	if ( token != &stream->token_buffer[0] )
		mem_free( (char*)token );
}

// Advance the stream forward just passed the first instance of [string]
void inputStream_skipPast( inputStream* stream, const char* string ) {
	char* token = NULL;
	while ( !inputStream_endOfFile( stream ) && 
			(!token || !string_equal( token, string ))) {
		if ( token )
			inputStream_freeToken( stream, token );
		token = inputStream_nextToken( stream );
		}
	if ( token )
		inputStream_freeToken( stream, token );
}

// Returns the next token as a c string, advances the inputstream to the token's end
void inputStream_skipToken( inputStream* stream ) {
	// parse leading whitespace
	while ( isWhiteSpace( *stream->stream ) )
			stream->stream++;
	// measure the length of the token - ptr should become one-past-the-end
	const char* ptr = stream->stream;
	while ( !isWhiteSpace( *ptr ) && !isTerminator( *ptr )) {
		if ( isListStart( *ptr ) || isListEnd( *ptr ) ) {
			if ( ( ptr - stream->stream ) == 0 ) // if parenthesis is first char, push pas it
				ptr++;	
			break;
		}
		ptr++;
	}
	stream->stream = ptr; // Advance past the end of the token
}
// Check whether we have reached our end pointer
bool inputStream_endOfFile( inputStream* in ) {
	return in->stream >= in->end;
}


void inputStream_advancePast( inputStream* in, char c ) {
	while ( *in->stream != c && !inputStream_endOfFile( in ))
		++in->stream;
	++in->stream;
}
// Advance to just past the next end-of-line
void inputStream_nextLine( inputStream* in ) {
	inputStream_advancePast( in, '\n' );
}


