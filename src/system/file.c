// file.c
#include "common.h"
#include "file.h"
//-----------------------
#include "mem/allocator.h"
#include "system/hash.h"
#include "system/string.h"
#include "system/thread.h"
#include <assert.h>
#if defined(LINUX_X) || defined(ANDROID)
#include <sys/stat.h>
#endif // LINUX_X || ANDROID
#ifdef ANDROID
#include "zip.h"
#include <jni.h>
#endif // ANDROID

vmutex file_mutex = kMutexInitialiser;

void vfile_storeModifiedTime( const char* file );

//
// *** File
//

#define kVfileMaxPathLength 128

void vfile_assetPath( char* asset_path, const char* path ) {
	memset( asset_path, 0, kVfileMaxPathLength );
#ifdef ANDROID
	const char* prefix = "assets/";
	int prefix_length = strlen( prefix );
	vAssert(( strlen( path ) + prefix_length ) < kVfileMaxPathLength );
	strcpy( asset_path, prefix );
	strcpy( asset_path + prefix_length, path );
#else
	vAssert( strlen( path ) < kVfileMaxPathLength );
	strcpy( asset_path, path );
#endif // ANDROID
}

//
// *** Android specific functions
//

#ifdef ANDROID
struct zip* apk_archive = NULL;

#define kMaxApkPathLength 128
char apkPath[kMaxApkPathLength];

void vfile_loadApk( const char* path );

void Java_com_vitruvianinteractive_vitae_VNativeActivity_setApkPath( JNIEnv* env, jobject thiz, jstring path ) {
	printf( "Running setApkPath\n" );
	const char* aPath = (*env)->GetStringUTFChars( env, path, NULL /* isCopy */ );
	printf( "Path is: \"%s\"\n", aPath );
	int length = strlen( aPath );
	assert( length < (kMaxApkPathLength-1) );
	memcpy( apkPath, aPath, sizeof( char ) * length );
	apkPath[length] = '\0';
	(*env)->ReleaseStringUTFChars( env, path, aPath );

	vfile_loadApk( apkPath );
}

void vfile_loadApk( const char* path ) {
	printf( "Loading APK %s", path );
	apk_archive = zip_open( path, 0, NULL );
	if ( apk_archive == NULL ) {
		printf( "Error loading APK" );
		assert( 0 );
		return;
	}

#if 0
	//Just for debug, print APK contents
	int numFiles = zip_get_num_files(apk_archive);
	for (int i=0; i<numFiles; i++) {
		const char* name = zip_get_name(apk_archive, i, 0);
		if (name == NULL) {
			printf("Error reading zip file name at index %i : %s", zip_strerror(apk_archive));
			return;
		}
		printf("File %i : %s\n", i, name);
	}
#endif
}

struct zip_file* vfile_openApk( const char* path, const char* mode ) {
	char asset_path[kVfileMaxPathLength];
	vfile_assetPath( asset_path, path );

	//printf( "VFILE: Opening file from apk: \"%s\"\n", asset_path );

	struct zip_file* file = zip_fopen( apk_archive, asset_path, 0x0 /* Flags */ );
	if ( !file ) {
		printf( "Error loading file: \"%s\"\n", asset_path );
		assert( file );
		return NULL;
	}
	return file;
}

bool vfile_existsApk( const char* path ) {
	char asset_path[kVfileMaxPathLength];
	vfile_assetPath( asset_path, path );
	struct zip_file* file = zip_fopen( apk_archive, asset_path, 0x0 /* Flags */ );
	return (file != NULL);
}

bool vfile_exists( const char* path ) {
#ifdef ANDROID
	return vfile_existsApk( path );
#endif
	vAssert( 0 ); // NYI
}

// Load the entire contents of a file into a heap-allocated buffer of the same length
// returns a pointer to that buffer
// It its the caller's responsibility to free the buffer
void* vfile_contentsApk( const char* path, int* length ) {
	void *buffer;
	vmutex_lock( &file_mutex );
	{
		struct zip_file *f = vfile_openApk( path, "r" );

		// We need to get the asset path so we can zip_stat the length
		char asset_path[kVfileMaxPathLength];
		vfile_assetPath( asset_path, path );

		struct zip_stat stat;
		zip_stat( apk_archive, asset_path, 0x0 /* flags */, &stat );
		*length = (int)stat.size;
		buffer = mem_alloc( *length+1 );
		*length = zip_fread( f, buffer, *length );
		zip_fclose( f );
		((char*)buffer)[*length] = '\0'; // TODO should I be doing this? Distinction between binary and ASCII?
	}
	vmutex_unlock( &file_mutex );

    return buffer;
}


#endif

// file open wrapper that asserts on failure
FILE* vfile_open( const char* path, const char* mode ) {
	char asset_path[kVfileMaxPathLength];
	vfile_assetPath( asset_path, path );

	FILE* file = fopen( asset_path, mode );
	if ( !file ) {
		printf( "Error loading file: \"%s\"\n", asset_path );
		assert( file );
		return NULL;
	}
	
	// Store last modified time
	vfile_storeModifiedTime( asset_path );

	return file;
}


// Load the entire contents of a file into a heap-allocated buffer of the same length
// returns a pointer to that buffer
// It its the caller's responsibility to free the buffer
void* vfile_contents( const char* path, size_t* length ) {
#ifdef ANDROID
	return vfile_contentsApk( path, length );
#endif

    FILE *f = vfile_open( path, "r" );
    void *buffer;

    fseek(f, 0, SEEK_END);
    *length = ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer = mem_alloc( (*length) + 1 );
//	printf( "FILE: reading contents of file of size: %d.\n", *length );
    *length = fread( buffer, 1, *length, f );
    fclose(f);
    ((char*)buffer)[*length] = '\0'; // TODO should I be doing this? Distinction between binary and ASCII?

    return buffer;
}

void vfile_writeContents( const char* path, void* buffer, int length ) {
	FILE *f = fopen( path, "w+" );

    if ( !f ) {
        printf("vfile: Unable to open %s for writing\n", path);
		assert( 0 );
        return;
    }

	fwrite( buffer, 1, length, f );
	fclose( f );
}

// *** inputstream funcs
bool token_isString( const char* token ) {
	size_t len = strlen( token );
	vAssert( len < 256 );	// Sanity check
	return (( token[0] == '"' ) && ( token[len-1] == '"' ));
}

bool token_isFloat( const char* token ) {
	// check every character is a digit, - or .
	int length = strlen( token );
	for ( int i = 0; i < length; i++ ) {
		if ( !charset_contains( "0123456789-.", token[i] )) {
			return false;
			}
		}
	errno = 0;
	float f = strtof( token, NULL );
	(void)f;
	return ( errno == 0 );
	}

// Create a string from a string-form token
// Allocated in the string memory pool
const char* sstring_create( const char* token ) {
	size_t len = strlen( token );
	char* buffer = heap_allocate( global_string_heap, sizeof( char ) * (len-1), NULL );
	memcpy( buffer, &token[1], len-2 );
	buffer[len-2] = '\0';
	return buffer;
};

#define kMaxFileEntries 1024
map* file_modified_times = NULL;

void vfile_systemInit() {
	vAssert( !file_modified_times );
	file_modified_times = map_create( kMaxFileEntries, sizeof( time_t ));
}

void vfile_storeModifiedTime( const char* file ) {
	if ( !file_modified_times )
		return;
	time_t modified_time;
	struct stat file_stat;
	int error = stat( file, &file_stat );
	vAssert( error == 0 );
	modified_time = file_stat.st_mtime;
	
	int key = mhash( file );
	map_addOverride( file_modified_times, key, &modified_time );
}

time_t vfile_lastModifiedTime( const char* file ) {
	int key = mhash( file );
	time_t* modified_time = map_find( file_modified_times, key );
	if (!modified_time )
		return 0;
	else
		return *modified_time;
}

bool vfile_modifiedSinceLast( const char* file ) {
#if defined(LINUX_X)
	time_t time_new, time_old;
	struct stat file_stat;
	int error = stat( file, &file_stat );
	if ( error != 0 )
		return false;
	vAssert( error == 0 );

	time_old = vfile_lastModifiedTime( file );

	time_new = file_stat.st_mtime;
	return ( time_new > time_old );
#else
	// TODO - implement for other platforms
	return false;
#endif
}
