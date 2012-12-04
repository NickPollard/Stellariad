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

sexpr* parse( const char* string ) {
	(void)string;
	return NULL;
}

sexpr* parse_file( const char* filename ) {
	size_t length;
	return parse( vfile_contents( filename, &length ));
}

int sexpr_countChildrenByType( sexpr* s, const char* name ) {
	(void)s; (void)name;
	return 0;
}

mesh* load_mesh( sexpr* s ) {
	(void)s;
	return NULL;
}

bool sexpr_named( const char* name, sexpr* s ) {
	return string_equal( s->value, name );
}

model* load_model( sexpr* s ) {
	int mesh_count = sexpr_countChildrenByType( s, "mesh" );
	model* m = model_createModel( mesh_count );
	sexpr* child = s->child;
	while ( child ) {
		if ( sexpr_named( "mesh", child ) ) {
			model_addMesh( m, load_mesh( child ));
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
	return NULL;
}
