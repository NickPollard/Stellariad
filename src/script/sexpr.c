#include "common.h"
#include "sexpr.h"
//--------------------------------------------------------
#include "model.h"
#include "model_loader.h"
#include "maths/maths.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/texture.h"
#include "system/hash.h"
#include "system/file.h"
#include "system/inputstream.h"
#include "system/string.h"

bool isLineComment( const char* token ) {
	return string_equal( "#", token );	
}

sexpr* sexpr_create( enum sexprType type, const char* name ) {
	sexpr* s = mem_alloc( sizeof( sexpr ));
	s->type = type;
	s->value = name;
	s->child = NULL;
	s->next = NULL;
	return s;
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

void sexpr_formatSpacing( char* buffer, int depth ) {
	for ( int i = 0; i < depth; ++i ) {
		buffer[i] = ' ';
	}
	buffer[depth] = '\0';
}

sexpr* sexpr_parseStream( inputStream* stream, int depth ) {
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
		sexpr* parent = sexpr_create( sexprTypeAtom, (void*)string_createCopy( token ));
		char spacing[256];
		sexpr_formatSpacing( spacing, depth );
		printf( "Sexpr parse: %s%s\n", spacing, token );
		parent->child = NULL;
		sexpr* last_child = NULL;
		sexpr* child = sexpr_parseStream( stream, depth + 2 );
		while ( child ) {
			if ( !parent->child ) {
				parent->child = child;
			}
			else {
				last_child->next = child;
			}
			last_child = child;
			child = sexpr_parseStream( stream, depth + 2 );
		}
		return parent;
	}
	else if ( token_isString( token )) {
		// When it's an atom, we keep the token, don't free it
		char spacing[256];
		sexpr_formatSpacing( spacing, depth );
		printf( "Adding child %s%s\n", spacing, token );
		return sexpr_create( sexprTypeString, (void*)sstring_create( token ));
	}
	else {
		// When it's an atom, we keep the token, don't free it
		char spacing[256];
		sexpr_formatSpacing( spacing, depth );
		printf( "Adding child %s%s\n", spacing, token );
		return sexpr_create( sexprTypeAtom, (void*)string_createCopy( token ));
	}

		
		
		
		
	/*
		

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
	*/
}

sexpr* sexpr_parse( const char* string ) {
	inputStream* stream = inputStream_create( string );
	sexpr* s = sexpr_parseStream( stream, 0 );
	mem_free( stream );
	return s;
}

sexpr* sexpr_parseFile( const char* filename ) {
	size_t length;
	return sexpr_parse( vfile_contents( filename, &length ));
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

sexpr* sexpr_findChildNamed( const char* name, sexpr* parent ) {
	sexpr* child = parent->child;
	while ( child ) {
		if ( sexpr_named( name, child )) {
			return child;
		}
		child = child->next;
	}
	return NULL;
}

mesh* sexpr_loadMesh( sexpr* s ) {
	(void)s;
	const char* filename = "dat/model/smoothsphere2.obj";

	sexpr* fileterm = sexpr_findChildNamed( "filename", s );
	if ( fileterm ) {
		vAssert( fileterm->child );
		filename = fileterm->child->value;
	}
	
	mesh* m = mesh_loadObj( filename );
	
	sexpr* diffuse_term = sexpr_findChildNamed( "diffuse_texture", s );
	if ( diffuse_term ) {
		vAssert( diffuse_term->child );
		m->texture_diffuse = texture_load( diffuse_term->child->value );
	}
	
	return m;
}

model* sexpr_loadModel( sexpr* s ) {
	int mesh_index = 0;
	int mesh_count = sexpr_countChildrenByType( s, "mesh" );
	printf( "SEXPR: Creating model with %d meshes.\n", mesh_count );
	model* m = model_createModel( mesh_count );
	sexpr* child = s->child;
	while ( child ) {
		if ( sexpr_named( "mesh", child ) ) {
			model_addMesh( m, mesh_index++, sexpr_loadMesh( child ));
		}
		child = child->next;
	}
	return m;
}

void* sexpr_load( sexpr* s ) {
	if ( sexpr_named( "model", s )) {
		return sexpr_loadModel( s );
	}
	return NULL;
}

void* sexpr_loadFile( const char* filename ) {
	return sexpr_load( sexpr_parseFile( filename ));
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

void test_sexpr() {
	sexpr* s = sexpr_parse( "(model)" );
	(void)s;
	s = sexpr_parse( "(model (transform (particle )))" );
	s = sexpr_parse( "(model (transform (particle)) (transform (particle )))" );
	model* m = sexpr_loadModel( sexpr_parse( "(model (mesh))" ));
	model* m_ = sexpr_loadModel( sexpr_parse( "(model (mesh (filename \"dat/model/ship_hd_2.obj\")))" ));
	(void)m; (void)m_;
//	vAssert( 0 );
}
