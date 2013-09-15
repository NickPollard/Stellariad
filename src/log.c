// log.c

#include "common.h"
#include "log.h"
//---------------------
#include <stdarg.h>

#define kMaxLogLength 256
#define kFileModeAppend "a"

FILE* log_file;
bool log_initialized = false;

void log_init( const char* log_filename ) {
	log_file = fopen( log_filename, kFileModeAppend );
	log_initialized = true;
}

void log_deinit() {
	fclose( log_file );
}

void vlog( const char* format, ... ) {
	vAssert( log_initialized == true );
	va_list args;
	va_start( args, format );
	char buffer[kMaxLogLength];
	vsprintf( buffer, format, args );
	printf( "%s", buffer );
	fprintf( log_file, "%s", buffer );
	va_end( args );
}
