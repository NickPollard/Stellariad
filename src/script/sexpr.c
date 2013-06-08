#include "common.h"
#include "sexpr.h"
//--------------------------------------------------------
#include "model.h"
#include "model_loader.h"
#include "particle.h"
#include "ribbon.h"
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

sexpr* sexpr_createFloat( float number ) {
	sexpr* s = sexpr_create( sexprTypeFloat, NULL );
	s->number_value = number;
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
	char spacing[256];
	sexpr_formatSpacing( spacing, depth );
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
		//printf( "Sexpr parse: %s%s\n", spacing, token );
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
		//printf( "Adding child %s%s\n", spacing, token );
		return sexpr_create( sexprTypeString, (void*)sstring_create( token ));
	}
	else if ( token_isFloat( token )) {
		//printf( "Adding child %s%2f (float )\n", spacing, strtof( token, NULL ));
		return sexpr_createFloat( strtof( token, NULL ));
	}
	else {
		// When it's an atom, we keep the token, don't free it
		//printf( "Adding child %s%s\n", spacing, token );
		return sexpr_create( sexprTypeAtom, (void*)string_createCopy( token ));
	}
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

vector sexpr_loadVector( sexpr* s ) {
	vector v = Vector( 0.0, 0.0, 0.0, 1.0 );
	if ( sexpr_named( "vector", s )) {
		sexpr* child = s->child;
		for ( int i = 0; i < 4; ++i ) {
			vAssert( child );
			vAssert( child->type == sexprTypeFloat );
			float n = child->number_value;
			v.val[i] = n;

			child = child->next;
		}
	}
	return v;
}

particleEmitter* sexpr_loadParticleEmitter( sexpr* s ) {
	sexpr* fileterm = sexpr_findChildNamed( "filename", s );
	if ( fileterm ) {
		vAssert( fileterm->child );
		const char* filename = fileterm->child->value;
		particleEmitterDef* def = particle_loadAsset( filename );
		particleEmitter* emitter = particle_newEmitter( def );
		return emitter;
	}
	return NULL;
}

ribbonEmitter* sexpr_loadRibbonEmitter( sexpr* s ) {
	(void)s;
	//printf( "Sexpr Loading ribbon emitter.\n" );
	sexpr* fileterm = sexpr_findChildNamed( "filename", s );
	if ( fileterm ) {
		vAssert( fileterm->child );
		const char* filename = fileterm->child->value;
		ribbonEmitterDef* def = ribbon_loadAsset( filename );
		ribbonEmitter* emitter = ribbonEmitter_create( def );
		return emitter;
	}
	vAssert( 0 );
	return NULL;
}

property* sexpr_loadVectorProperty( sexpr* s ) {
	property* p = property_create( 5 );
	sexpr* child = s->child;
	while ( child ) {
		vAssert( sexpr_named( "property_key", child ));
		vAssert( child->child );
		vAssert( child->child->type == sexprTypeFloat );
		float time = child->child->number_value;
		vector v = sexpr_loadVector( child->child->next );
		//vector_print( &v );
		//printf ( "\n" );
		property_addfv( p, time, (float*)&v );
		child = child->next;
	}
	return p;
}

ribbonEmitterDef* sexpr_loadRibbonEmitterDef( sexpr* s ) {
	(void)s;
	//printf( "Sexpr Loading ribbon emitter def.\n" );
	ribbonEmitterDef* def = ribbonEmitterDef_create();
	sexpr* color_term = sexpr_findChildNamed( "color", s );
	if ( color_term ) {
		sexpr* property_term = sexpr_findChildNamed( "vector_property", color_term );
		def->color = sexpr_loadVectorProperty( property_term );
	}
	
	sexpr* static_term = sexpr_findChildNamed( "static_texture", s );
	if ( static_term ) {
		def->static_texture = string_equal( static_term->child->value, "true" );
	}
	
	sexpr* diffuse_term = sexpr_findChildNamed( "diffuse_texture", s );
	if ( diffuse_term ) {
		const char* texture_path = diffuse_term->child->value;
		textureProperties* properties = mem_alloc( sizeof( textureProperties ));
		if ( !def->static_texture ) {
			properties->wrap_s = GL_CLAMP_TO_EDGE;
			properties->wrap_t = GL_CLAMP_TO_EDGE;
		}
		def->diffuse = texture_loadWithProperties( texture_path, properties );
	}

	sexpr* radius_term = sexpr_findChildNamed( "radius", s );
	if ( radius_term ) {
		def->radius = radius_term->child->number_value;
	}

	sexpr* lifetime_term = sexpr_findChildNamed( "lifetime", s );
	if ( lifetime_term ) {
		def->lifetime = lifetime_term->child->number_value;
	}
	
	return def;
}

transform* sexpr_loadModelTransform( model* m, sexpr* s ) {
	(void)s;
	//printf( "Adding transform!\n" );
	transform* t = transform_create();
	model_addTransform( m, t );

	sexpr* trans_term = sexpr_findChildNamed( "translation", s );
	if ( trans_term ) {
		vAssert( trans_term->child );
		vector translation = sexpr_loadVector( trans_term->child );
		transform_setLocalTranslation( t, &translation );
	}

	sexpr* child = s->child;
	while ( child ) {
		if ( sexpr_named( "particleEmitter", child ) ) {
			particleEmitter* emitter =  sexpr_loadParticleEmitter( child );
			model_addParticleEmitter( m, emitter );
			emitter->trans = (transform*)(uintptr_t)model_transformIndex( m, t );
		}
		
		if ( sexpr_named( "ribbonEmitter", child ) ) {
			ribbonEmitter* emitter = sexpr_loadRibbonEmitter( child );
			model_addRibbonEmitter( m, emitter );
			emitter->trans = (transform*)(uintptr_t)model_transformIndex( m, t );
		}

		child = child->next;
	}

	return t;
}

model* sexpr_loadModel( sexpr* s ) {
	int mesh_index = 0;
	int mesh_count = sexpr_countChildrenByType( s, "mesh" );
	model* m = model_createModel( mesh_count );
	sexpr* child = s->child;
	while ( child ) {
		if ( sexpr_named( "mesh", child ) ) {
			model_addMesh( m, mesh_index++, sexpr_loadMesh( child ));
		}
		else if ( sexpr_named( "transform", child )) {
			// This will add the transform to the model
			sexpr_loadModelTransform( m, child );
		}

		child = child->next;
	}
	return m;
}

void* sexpr_load( sexpr* s ) {
	if ( sexpr_named( "model", s )) {
		return sexpr_loadModel( s );
	}
	if ( sexpr_named( "ribbonEmitterDef", s )) {
		return sexpr_loadRibbonEmitterDef( s );
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
//	vAssert( 0 );
}
