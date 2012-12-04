#include "common.h"
#include "sexpr.h"
//--------------------------------------------------------
#include "model.h"
#include "model_loader.h"
#include "maths/maths.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "system/hash.h"
#include "system/file.h"
#include "system/inputstream.h"
#include "system/string.h"

int isListStart( char c ) {
	return c == '(';
}

int isListEnd( char c ) {
	return c == ')';
}

int isLineComment( const char* token ) {
	return 

sexpr* sexpr_create( int type, sexpr* value ) {

}

char* sexpr_nextToken( inputStream* stream ) {
	char* token = inputStream_nextToken( stream );
	while ( isLineComment( token ) || isNewLine( *token )) {
		// skip the line
		if ( isLineComment( token )) {
			//printf( " # Comment found; skipping line.\n" );
			inputStream_skipPast( stream, "\n" );
		}
		inputStream_freeToken( stream, token );
		token = inputStream_nextToken( stream );
	}
	return token;
}

sexpr* parse_stream( inputStream stream ) {
	if ( inputStream_endOfFile( stream )) {
		printf( "ERROR: End of file reached during sexpr parse, with incorrect number of closing parentheses.\n" );
		vAssert( 0 );
		return NULL;
	}

	char* token = sexpr_nextToken( stream );

	if ( isListEnd( *token ) ) {
		inputStream_freeToken( stream, token ); // It's a bracket, discard it
		return NULL;
	}
	else if ( isListStart( *token ) ) {
		inputStream_freeToken( stream, token ); // It's a bracket, discard it
		token = sexpr_nextToken( stream );
		sexpr* parent = sexpr_create( typeAtom, (void*)string_createCopy( token ));
		parent->child = NULL;
		sexpr* last_child = NULL;
		while ( children ) {
			sexpr* child = parse_stream( stream );
			if ( !parent->child ) {
				parent->child = child;
			}
			else {
				last_child->next = child;
			}
			last_child = child;
		}
		return parent;
	}
	else {
		// When it's an atom, we keep the token, don't free it
		return sexpr_create( typeAtom, (void*)string_createCopy( token ));
	}

		
		
		
		
		

		sexpr* list = sexpr_create( typeList, sexpr_parse( stream ));
		sexpr* s = list;

		if ( !s->head ) { // The Empty list () 
			return list;
		}

		while ( true ) {
			sexpr* sub_expr = sexpr_parse( stream );				// parse a subexpr
			if ( sub_expr ) {								// If a valid return
				s->tail = sexpr_create( typeList, sub_expr );	// Add it to the tail
				s = s->tail;
			} else {
				return list;
			}
		}
	}
}

sexpr* parse( const char* string ) {
	inputStream* stream = inputStream_create( string );
	sexpr* s = parse_stream( stream );
	mem_free( stream );
	return s;
}

sexpr* parse_file( const char* filename ) {
	size_t length;
	return parse( vfile_contents( filename, &length ));
}

bool sexpr_named( const char* name, sexpr* s ) {
	return string_equal( s->value, name );
}

int sexpr_countChildrenByType( sexpr* s, const char* name ) {
	sexpr* child = s->child;
	int count = 0;
	while ( child ) {
		if ( sexpr_named( name, child ))
			++count;
		child = child->next;
	}
	return count;
}

mesh* load_mesh( sexpr* s ) {
	(void)s;
	const char* filename = sexpr_findChildNamed( "filename", s )->next->value;
	const char* filename = "dat/model/smoothsphere2.obj";
	mesh* m = mesh_loadObj( filename );
	return NULL;
}

model* load_model( sexpr* s ) {
	int mesh_index = 0;
	int mesh_count = sexpr_countChildrenByType( s, "mesh" );
	printf( "SEXPR: Creating model with %d meshes.\n", mesh_count );
	model* m = model_createModel( mesh_count );
	sexpr* child = s->child;
	while ( child ) {
		if ( sexpr_named( "mesh", child ) ) {
			model_addMesh( m, mesh_index++, load_mesh( child ));
		}
		child = child->next;
	}
	return m;
}


particleEmitter* load_particleEmitter( sexpr* s ) {
	(void)s;
	return NULL;
}

transform* load_transform( sexpr* s ) {
	(void)s;
	transform* t = transform_create();
	return t;
}
