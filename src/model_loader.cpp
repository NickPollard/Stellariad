#include "common.h"
#include "model_loader.h"
//--------------------------------------------------------
#include "scene.h"
#include "model.h"
#include "script/lisp.h"
#include "script/sexpr.h"
#include "string/stringops.h"
#include "system/file.h"
#include "system/inputstream.h"
#include "system/string.h"

mesh* mesh_loadObj( const char* filename ) {
	//println("Loading Mesh: " + $(filename));
	// Load the raw data
	int vertCount = 0, index_count = 0, normal_count = 0, uv_count = 0;
	// Lets create these arrays on the heap, as they need to be big
	// TODO: Could make these static perhaps?
	vector* vertices	= (vector*)mem_alloc( sizeof( vector ) * kObjMaxVertices );
	vector* normals		= (vector*)mem_alloc( sizeof( vector ) * kObjMaxVertices );
	vector* uvs			= (vector*)mem_alloc( sizeof( vector ) * kObjMaxVertices );
	uint16_t* indices			= (uint16_t*)mem_alloc( sizeof( uint16_t ) * kObjMaxIndices );
	uint16_t* normal_indices	= (uint16_t*)mem_alloc( sizeof( uint16_t ) * kObjMaxIndices );
	uint16_t* uv_indices		= (uint16_t*)mem_alloc( sizeof( uint16_t ) * kObjMaxIndices );

#define array_clear( array, size ) \
	memset( array, 0, sizeof( array[0] ) * size );

	// Initialise to 0;
	array_clear( vertices, kObjMaxVertices );
	array_clear( normals, kObjMaxVertices );
	array_clear( uvs, kObjMaxVertices );
	array_clear( indices, kObjMaxIndices );
	array_clear( normal_indices, kObjMaxIndices );
	array_clear( uv_indices, kObjMaxIndices );

	size_t file_length = -1;
	const char* file_buffer = (const char*)vfile_contents( filename, &file_length );
	inputStream* stream = inputStream_create( file_buffer );

	while ( !inputStream_endOfFile( stream )) {
		char* token = inputStream_nextToken( stream );
		if ( string_equal( token, "v" )) {
			assert( vertCount < kObjMaxVertices );
			// Vertex
			for ( int i = 0; i < 3; i++ ) {
				inputStream_freeToken( stream, token );
				token = inputStream_nextToken( stream );
				vertices[vertCount].val[i] = strtof( token, NULL );
			}
			vertices[vertCount].coord.w = 1.f; // Force 1.0 w value for all vertices.
			vertCount++;
		}
		if ( string_equal( token, "vn" )) {
			assert( normal_count < kObjMaxVertices );
			// Vertex Normal
			for ( int i = 0; i < 3; i++ ) {
				inputStream_freeToken( stream, token );
				token = inputStream_nextToken( stream );
				normals[normal_count].val[i] = strtof( token, NULL );
			}
			normals[normal_count].coord.w = 0.f; // Force 0.0 w value for all normals
			normal_count++;
		}
		if ( string_equal( token, "vt" )) {
			assert( uv_count < kObjMaxVertices );
			// Vertex Texture Coord (UV)
			for ( int i = 0; i < 2; i++ ) {
				inputStream_freeToken( stream, token );
				token = inputStream_nextToken( stream );
				uvs[uv_count].val[i] = strtof( token, NULL );
			}
			uv_count++;
		}
		if ( string_equal( token, "f" )) {
			// Face (indices)
			for ( int i = 0; i < 3; i++ ) {
				assert( index_count < kObjMaxIndices );

				inputStream_freeToken( stream, token );
				token = inputStream_nextToken( stream );
				// Need to split into 3 parts (vert/tex/normal) by /
				int len = strlen( token );
				char vert[8];
				char uv[8];
				char norm[8];
				int j = 0;
				const char* string = token;
				while ( string[j] != '/' && j < len ) {
					assert( j >= 0 );
					assert( j < 8 );
					vert[j] = string[j];
					j++;
				}
				vert[j] = '\0';
				string = &string[j+1];
				j = 0;
				len = strlen( string );
				while ( string[j] != '/' && j < len ) {
					assert( j >= 0 );
					assert( j < 8 );
					uv[j] = string[j];
					j++;
				}
				uv[j] = '\0';
				string = &string[j+1];
				j = 0;
				len = strlen( string );
				while ( string[j] != '/' && j < len ) {
					assert( j >= 0 );
					assert( j < 8 );
					norm[j] = string[j];
					j++;
				}
				norm[j] = '\0';

				indices[index_count]		= atoi( vert ) - 1; // -1 as obj uses 1-based indices, not 0-based as we do
				normal_indices[index_count]	= atoi( norm ) - 1; // -1 as obj uses 1-based indices, not 0-based as we do
				uv_indices[index_count]		= atoi( uv ) - 1; // -1 as obj uses 1-based indices, not 0-based as we do
				index_count++;
			}
		}
		inputStream_freeToken( stream, token );
		inputStream_nextLine( stream );
	}
	mem_free( (void*)file_buffer );
	//printf( "MESH_LOAD: Parsed .obj file \"%s\" with %d verts, %d faces, %d normals, %d uvs.\n", filename, vertCount, index_count / 3, normal_count, uv_count );

	// Copy our loaded data into the Mesh structure
	mesh* msh = mesh_createMesh( vertCount, index_count, index_count, uv_count, vertices, indices, normals, normal_indices, uvs, uv_indices );

	mem_free(vertices);
	mem_free(normals);
	mem_free(uvs);
	mem_free(indices);
	mem_free(normal_indices);
	mem_free(uv_indices);

	return msh;
}

model* model_load( const char* filename ) {
	model* mdl = (model*)sexpr_loadFile( filename );
	if ( mdl->meshCount > 0 )
		mdl->_obb = obb_calculate( mdl->meshes[0]->vertCount, mdl->meshes[0]->verts );
#if DEBUG
	mdl->filename = string_createCopy(filename);
#endif // DEBUG
	return mdl;
}
