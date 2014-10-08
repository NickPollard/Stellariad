// debugtext.c

#include "common.h"
#include "debugtext.h"
//-----------------------
#include "mem/allocator.h"
#include "font.h"

void PrintDebugText( debugtextframe* frame, const char* string ) {
	assert( strlen(string) < kDebugTextLineLength );
	strncpy( frame->lines[frame->lineCount], string, kDebugTextLineLength );
	frame->lines[frame->lineCount][kDebugTextLineLength-1] = '\0'; // Ensure null-termination
	frame->lineCount++;
}

debugtextframe* debugtextframe_create( float x, float y, float lineHeight) {
	debugtextframe* f = mem_alloc( sizeof( debugtextframe ));
	memset( f, 0, sizeof( debugtextframe ));
	f->x = x;
	f->y = y;
	f->lineHeight = lineHeight;
	return f;
}

void debugtextframe_tick( void* entity, float dt ) {
}

void debugtextframe_render( void* entity ) {
	assert( 0 );
	glEnable(GL_TEXTURE_2D);

	debugtextframe* f = (debugtextframe*)entity;
	float x = f->x;
	float y = f->y;
	for ( int i = 0; i < f->lineCount; i++) {
//		printf( "debugtext rendering: %d \"%s\"\n", i, f->lines[i] );
		font_renderString( x, y, f->lines[i] );
		y += f->lineHeight;
	}
	f->lineCount = 0;
	glDisable(GL_TEXTURE_2D);
}
