// parse.c

#include "common.h"
#include "parse.h"
//-----------------------
#include "light.h"
#include "maths.h"
#include "model.h"
#include "scene.h"
#include "mem/allocator.h"
#include "render/modelinstance.h"
#include "system/hash.h"
#include "system/string.h"

/*
   A heap specifically for string allocations
   This is to keep string allocations organised,
   prevent them from fragmenting the main pool,
   and let me track them (string allocations should *only* be required
   for parsing or scripting
   */
#define kStringHeapSize 4 * 1024
heapAllocator* global_string_heap = NULL;

//
// *** Parsing
//

// Is the character a whitespace character?
// eg. space, tab
int isWhiteSpace( char c ) {
	return ( c == ' ' || c == '\n' || c == '\t' );
}

// Is the character a terminator character
// Eg. NULL-terminator or end of line
int isTerminator( char c ) {
	return ( c == '\0' /*|| c == '\n'*/ ); // Or EoF
}

int isListStart( char c ) {
	return c == '(';
}

int isListEnd( char c ) {
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
	while ( !isWhiteSpace( *ptr ) && !isTerminator( *ptr )) {
		if ( isListStart( *ptr ) || isListEnd( *ptr ) ) {
			if ( ( ptr - stream->stream ) == 0 ) // if parenthesis is first char, push pas it
				ptr++;	
			break;
		}
		ptr++;
	}
	// make a copy of it
	int length = ptr - stream->stream;
	char* token = mem_alloc( sizeof( char ) * (length + 1) );
	strncpy( token, stream->stream, length );
	token[length] = '\0';
	stream->stream = ptr; // Advance past the end of the token
	return token;
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
		in->stream++;
}
// Advance to just past the next end-of-line
void inputStream_nextLine( inputStream* in ) {
	inputStream_advancePast( in, '\n' );
}

//
// *** S-Expressions
//

bool isAtom( sterm* s ) {
	return ( s->type == typeAtom );
}

bool isList( sterm* s ) {
	return ( s->type == typeList );
}

bool isString( sterm* s ) {
	return ( s->type == typeString );
}

bool isPropertyType( sterm* s, const char* property_name ) {
	return ( isAtom( s ) && s->tail && string_equal( s->head, property_name ));
}

bool isTransform( sterm* s ) {
	return ( s->type == typeTransform );
}

bool isVector( sterm* s ) {
	return ( s->type == typeVector );
}

bool token_isString( const char* token ) {
	size_t len = strlen( token );
	vAssert( len < 256 );	// Sanity check
	return (( token[0] == '"' ) && ( token[len-1] == '"' ));
}

sterm* sterm_create( int tag, void* ptr ) {
	sterm* term = mem_alloc( sizeof( sterm ) );
	term->type = tag;
	term->head = ptr;
	term->tail = NULL;
	return term;
}

// Create a string from a string-form token
// Allocated in the string memory pool
const char* sstring_create( const char* token ) {
	size_t len = strlen( token );
	char* buffer = heap_allocate( global_string_heap, sizeof( char ) * (len-1) );
	memcpy( buffer, &token[1], len-2 );
	buffer[len-2] = '\0';
	return buffer;
};

void* property_value( sterm* property ) {
//	return ((sterm*)property->tail->head)->head;
	return property->tail->head;
}

// Read a token at a time from the inputstream, advancing the read head,
// and build it into an slist of atoms
sterm* parse( inputStream* stream ) {
	char* token = inputStream_nextToken( stream );
	if ( isListEnd( *token ) ) {
		mem_free( token ); // It's a bracket, discard it
		return NULL;
	}
	if ( isListStart( *token ) ) {
		mem_free( token ); // It's a bracket, discard it
		sterm* list = sterm_create( typeList, NULL );
		sterm* s = list;

		s->head = parse( stream );
		if ( !s->head ) { // The Empty list () 
			return list;
		}

		while ( true ) {
			sterm* sub_expr = parse( stream );				// parse a subexpr
			if ( sub_expr ) {								// If a valid return
				s->tail = sterm_create( typeList, NULL );	// Add it to the tail
				s = s->tail;
				s->head = sub_expr;
			} else {
				return list;
			}
		}
	}
	// When it's an atom, we keep the token, don't free it

	// Actually not necessarily an atom, let's read it in as whichever base data type it is
	// either: Atom, String, Number
	if ( token_isString( token )) {
//		printf( "Found String: %s\n", token );
		const char* string = sstring_create( token );
		return sterm_create( typeString, (char*)string );
	}
		
//	if ( atom )
		return sterm_create( typeAtom, (void*)token );
//	if ( number )
//		return sterm_create( typeNumber );
}



sterm* parse_string( const char* string ) {
	inputStream* stream = inputStream_create( string );
	sterm* s = parse( stream );
	mem_free( stream );
	return s;
}

sterm* parse_file( const char* filename ) {
	int length = 0;
	printf( "FILE: Loading File \"%s\" for parsing.\n", filename );
	char* contents = vfile_contents( filename, &length );
	vAssert( (contents) );
	vAssert( (length != 0) );
	sterm* s = parse_string( contents );
	mem_free( contents );
	return s;
}

// *** Map functions

typedef void (*function)( void* );
typedef void (*function_v)( void*, void* );
typedef void (*function_vv)( void*, void*, void* );

void map_vv( sterm* list, function_vv f, void* data, void* data_ ) {
	f( list->head, data, data_ );
	if ( list->tail )
		map_vv( list->tail, f, data, data_ );
}
void map_v( sterm* list, function_v f, void* data ) {
	f( list->head, data );
	if ( list->tail )
		map_v( list->tail, f, data );
}
void map_( sterm* list, function f ) {
	f( list->head );
	if ( list->tail )
		map_( list->tail, f );
}

// Load a scene from a .sc file (s-expression based)

void* s_print( sterm* s );
void* s_concat( sterm* s );
void* s_modelInstance( sterm* s );
void* s_light( sterm* s );
void* s_light_create( sterm* s );
void* s_object( sterm* s );
void* s_transform( sterm* s );
void* s_scene( sterm* s );
void* s_translation( sterm* s );
void* s_diffuse( sterm* s );
void* s_specular( sterm* s );
void* s_filename( sterm* s );
void* s_vector( sterm* s );
void* s_model( sterm* s );

void scene_processObject( void* object_, void* scene_, void* transform_ );

#define S_FUNC( atom, func )	if ( string_equal( (const char*)data->head, atom ) ) { \
									sterm* s = sterm_create( typeFunc, func ); \
									return s; \
								}

// TODO PLACEHOLDER
// ( should be a hash lookup or similar )
void* lookup( sterm* data ) {
	S_FUNC( "print", s_print )
	S_FUNC( "concat", s_concat )
	S_FUNC( "model-instance", s_modelInstance )
	S_FUNC( "model", s_model )
	S_FUNC( "light", s_light )
	S_FUNC( "light_create", s_light_create )
	S_FUNC( "object", s_object )
	S_FUNC( "transform", s_transform )
	S_FUNC( "scene", s_scene )
	S_FUNC( "translation", s_translation )
	S_FUNC( "diffuse", s_diffuse )
	S_FUNC( "specular", s_specular )
	S_FUNC( "vector", s_vector )
	S_FUNC( "filename", s_filename )

	return data;
}

bool isFunction( sterm* s ) {
	return s->type == typeFunc;
}

const char* getAtom( sterm* term ) {
	assert( term->type = typeAtom );
	return (const char*)term->head;
}

void debug_sterm_print( sterm* term ) {
	if ( isList( term ) ) {
		if ( isList( term->head ))
			printf( "(" );
		debug_sterm_print( term->head );
		if ( isList( term->head ))
			printf( ")" );
	}

	if ( isAtom( term ) )
		printf( "%s ", (const char*)term->head );
	if ( term->tail )
		debug_sterm_print( term->tail );
}

void debug_sterm_printList( sterm* term ) {
	printf( "List: " );
	if ( isList( term ))
		printf( "(" );
	debug_sterm_print( term );
	if ( isList( term ))
		printf( ")" );
	printf( "\n" );
}

sterm* cons( void* head, sterm* tail ) {
	sterm* term = mem_alloc( sizeof( sterm ));
	term->type = typeList;
	term->head = head;
	term->tail = tail;
	return term;
}

void* eval( sterm* data ) {
	if ( isAtom( data )) {
		// It's either a function, or a number
		return lookup( data );
	}
	else if ( isList( data )) {
		// If evaluating a list, the head must eval to an atom
		sterm* func = eval( data->head );
		if ( isFunction( func ))
			return ((script_func)func->head)( data->tail );
		else
			return data;
	}
	else if ( isString( data )) {
		return data;
	}
	else {
		printf( "Unrecognised Sterm type: %d.\n", data->type );
		assert( 0 );
	}
	return NULL;
}

sterm* eval_list( sterm* s ) {
	sterm* result = NULL;
	if ( s )
		result = cons( eval( s->head ), eval_list( s->tail ) );
	return result;
}

void sterm_free( sterm* s ) {
//	printf( "FILE: sterm_free.\n" );
	if ( !isList( s ) ) {
		if ( isString( s ))
			heap_deallocate( global_string_heap, s->head );
		else
			mem_free( s->head );
		mem_free( s );
	}
	else {
		if ( s->tail )
			sterm_free( s->tail );
		if ( s->head )
			sterm_free( s->head );
		mem_free( s );
	}
}

void* s_concat( sterm* s ) {
	int size = 0;
	char* string = NULL;
	while( s ) {
		// TODO - fix this, need separate string type
		// For now just assume atom
		sterm* stext = eval( s->head );
		const char* text = stext->head;
		size_t extra = strlen( text );
		char* tmp = mem_alloc( sizeof( char ) * ( size + extra + 1 ) );
		strncpy( tmp, string, size );
		strncpy( tmp + size, text, extra );
		if ( string )
			mem_free( string );
		string = tmp;
		size += extra;
		string[size] = '\0';
		s = s->tail;
	}
	return string;
}

void* s_print( sterm* s ) {
	printf( "> s_print(): " );
	while( s ) {
		const char* string = eval( s->head );
		printf( "%s", string );
		s = s->tail;
	}
	printf( "\n" );
	return NULL;
}


// TODO - could this be removed, just an slist but with the term above having the typeTransform?
typedef struct transformData_s {
	sterm* elements;
	vector	translation;
} transformData;

transformData* transformData_create() {
	transformData* t =  (transformData*)mem_alloc( sizeof( transformData ));
	t->elements = NULL;
	t->translation = Vector( 0.f, 0.f, 0.f, 1.f );
	return t;
}

void transformData_processElement( void* e, void* data ) {
	sterm* element = e;
	transformData* t = data;
	if ( isPropertyType( element, "modelInstance" ) || isTransform( element ) || isPropertyType( element, "light" )) {
		t->elements = cons( element, t->elements );
	}
	// If it's a translation, copy the vector to the transformData
	if ( isPropertyType( element, "translation" )) {
		vector* translation = (vector*)property_value( element );
		t->translation = *translation;
	}
}

// Creates a transformdata
// with a list of all modelInstancs passed into it
// and all other sub transforms
// So returns the whole sub tree from this point
void* s_transform( sterm* raw_elements ) {
	transformData* tData = transformData_create();
	if ( raw_elements ) {
		sterm* elements = eval_list( raw_elements );
		map_v( elements, transformData_processElement, tData );
	}
	sterm* t = sterm_create( typeTransform, tData );
	return t;
}

// Creates an sterm-form property
// This takes the form of a list where the first item is the property name
// and the second item is the property value
// ie.
//	p->head = property name
//	p->tail->head = property value
//	p->tail->tail = NULL
sterm* sterm_createProperty( const char* property_name, int property_type, void* property_data ) {
	sterm* property = sterm_create( typeAtom, (char*)property_name );
	sterm* value = sterm_create( property_type, property_data );
	property->tail =value;
	return property;
	/*
	return cons(	sterm_create( typeAtom, (void*)property_name ), 
					cons(	sterm_create( property_type, property_data ), 
							NULL ));
							*/
}

void* s_readVector( const char* property_name, sterm* raw_elements ) {
	assert( raw_elements );
	sterm* elements = eval_list( raw_elements );
	// Should be a list of one single vector
	assert( isVector( (sterm*)elements->head ));
	// For now, copy the vector head from the vector sterm
	sterm* property = sterm_createProperty( property_name, typeVector, ((sterm*)elements->head)->head );
	return property;
}

// Creates a translation
void* s_translation( sterm* raw_elements ) {
	return s_readVector( "translation", raw_elements );
}

void* s_diffuse( sterm* raw_elements ) {
	return s_readVector( "diffuse", raw_elements );
}

void* s_specular( sterm* raw_elements ) {
	return s_readVector( "specular", raw_elements );
}

void* s_vector( sterm* raw_elements ) {
	if ( raw_elements ) {
		sterm* elements = eval_list( raw_elements );
		vector* v = mem_alloc( sizeof( vector ));
		memset( v, 0, sizeof( vector ));
		sterm* element = elements;
		int i = 0;
		while ( element && i < 4 ) {
			// dereference head twice, as we have a list of atoms of floats
			v->val[i] = strtof( (const char*)((sterm*)element->head)->head, NULL );
			i++;
			element = element->tail;
		}
		sterm* sv = sterm_create( typeVector, v );
		return sv;
	}
	vAssert( 0 );
	return NULL;
}

// Process a Filename
// ( filename <string> )
void* s_readString( const char* property_name, sterm* raw_elements ) {
	vAssert( raw_elements );
	sterm* elements = eval_list( raw_elements );
	// Should be a single string, so check the head
	vAssert( isString( elements->head ));
	// The string is stored in the string heap
	sterm* property = sterm_createProperty( property_name, typeString, ((sterm*)elements->head)->head );
	return property;
}

void* s_filename( sterm* raw_elements) {
	return s_readString( "filename", raw_elements );
}

#define kMaxObjectTypes	16
#define kMaxProperties	16
map* object_offsets = NULL;

void register_propertyOffset( const char* type, const char* property, int offset ) {
	int t = mhash( type );
	map* m = NULL;
	map** m_ptr = map_find( object_offsets, t );
	if ( m_ptr )
		m = *m_ptr;
	else {
		printf( "Creating offset hashmap for type \"%s\".\n", type );
		m = map_create( kMaxProperties, sizeof( int ));
		map_add( object_offsets, t, &m );
	}
	vAssert( m ); // We should have a valid map by now

	int p = mhash( property);
	printf( "Adding offset ( \"%s\", \"%s\", %d )\n", type, property, offset );
	map_add( m, p, &offset );
}

int propertyOffset( const char* type, const char* property ) {
	map** m_ptr = map_find( object_offsets, mhash( type ));
	vAssert( m_ptr && *m_ptr );

	int* offset_ptr = map_find( *m_ptr, mhash( property ));
	vAssert( offset_ptr && (*offset_ptr >= 0) );
	return (*offset_ptr);
}

void parse_init() {
	global_string_heap = heap_create( kStringHeapSize );

	printf( "Creating Object_offsets hashmap.\n" );
	object_offsets = map_create( kMaxObjectTypes, sizeof( map* ));

	// Light
	register_propertyOffset( "light", "diffuse", offsetof( light, diffuse_color ));
	register_propertyOffset( "light", "specular", offsetof( light, specular_color ));
}

// Generic property process function
// Process a list of properties
// Compare them to the property list of the type
// Set appropriately
void processProperty( void* p, void* object, /* (const char*) */ void* type_name ) {
	// We need the object type name and the property name
	sterm* property = p;

	const char*	property_name	= property->head;
	sterm*		value_term		= property->tail;
	int			property_type	= value_term->type;

	void* data = (uint8_t*)object + propertyOffset( type_name, property_name );

	switch ( property_type ) {
		case typeVector:
			*(vector*)data = *(vector*)value_term->head;
			break;
		case typeString:
			*(const char**)data = value_term->head;
			break;
	}
}

// Creates a model instance
// Returns that model instance
void* s_modelInstance( sterm* raw_properties ) {
	modelHandle handle = -1;
	if ( raw_properties ) {
		sterm* properties = eval_list( raw_properties );
		while ( properties ) {
			sterm* property = properties->head;
			if ( isPropertyType( property, "filename" )) {
				printf( "Modelinstance filename \"%s\"\n", (char*)property->tail->head );
				handle = model_getHandleFromFilename( property->tail->head );
			}
			properties = properties->tail;
		}
	}
	
	vAssert( handle != -1 );
	modelInstance* m = modelInstance_create( handle );
	return m;
}

/*
	(def s_object
		( sterm-createProperty (map (processProperty object object-type) (eval-list args) )))
   */
/*
void* s_object( void* object, const char* object_type, sterm* raw_properties ) {
	if ( raw_properties ) {
		sterm* properties = eval_list( raw_properties );
		map_vv( properties, processProperty, object, (char*)object_type );
	}
	return sterm_createProperty( object_type, typeObject, object );
}
*/

void* s_object( /* void* object, const char* object_type,*/ sterm* raw_args ) {
	vAssert ( raw_args );
	sterm* args = eval_list( raw_args );
	// The object and object_type come first
	void* object = args->head; // We just have a native light type in the list, not a property
	args = args->tail;
	const char* object_type = ((sterm*)args->head)->head;
	printf( "s_object called with type \"%s\"\n", object_type );
	args = args->tail;

	map_vv( args, processProperty, object, (char*)object_type );

	return sterm_createProperty( object_type, typeObject, object );
}

void append( sterm* before, sterm* after ) {
	sterm* s = before;
	while ( s->tail)
		s = s->tail;
	s->tail = after;
}
/*
	(def s_light
		( s_object light-create "light" args ))
   */
void* s_light( sterm* args ) {
//	return eval( parse_string( "(object light_create \"light\" args)" ));
	sterm* s = parse_string( "(object (light_create) \"light\" )" );
	append( s, args );
	return eval( s );
//	return s_object( light_create(), "light", args );
}

void* s_light_create( sterm* args ) {
	printf( "light_create\n");
	return light_create();
}

void scene_processTransform( scene* s, transform* parent, transformData* tData ) {
	transform* t = transform_create();
	matrix_setTranslation( t->local, &tData->translation );
	t->parent = parent;
	scene_addTransform( s, t );

	// If it has children, process those
	if ( tData->elements )
		map_vv( tData->elements, scene_processObject, s, t );
}

void scene_processModel( scene* s, transform* parent, modelInstance* m ) {
	m->trans = parent;
	scene_addModel( s, m );
}

void scene_processLight( scene* s, transform* parent, light* l ) {
	l->trans = parent;
	scene_addLight( s, l );
}

void scene_processObject( void* object_, void* scene_, void* transform_ ) {
	sterm* object = object_;
	scene* s = scene_;
	transform* parent = transform_;
	if ( isTransform( object ))
		scene_processTransform( s, parent, object->head );
	if ( isPropertyType( object, "modelInstance" ))
		scene_processModel( s, parent, object->tail->head );
	if ( isPropertyType( object, "light" )) {
		printf( "Scene processing light.\n" );
		scene_processLight( s, parent, object->tail->head );
	}
}

/*
	call s_scene, with a list of sterms
	creates a scene, then evals the list and calls scene_processObjects with that list

	evalling the list causes s_transform to be called, with a list of sterms
	that creates the transformData
   */
void* s_scene( sterm* raw_scene_objects ) {
	scene* s = scene_create();
	sterm* scene_objects = eval_list( raw_scene_objects ); // TODO: Could eval_list be part of just eval?
	map_vv( scene_objects, scene_processObject, s, NULL );
	return s;
}

// *** Testing

// Tests s_concat, eval, parse, inputStream
void test_s_concat() {
	sterm* s = parse_string( "(concat Hello World)" );
	char* result = eval( s );

	assert( strcmp( result, "HelloWorld" ) == 0 );
	assert( strcmp( result, "HellooWorld" ) != 0 );

	sterm_free( s );
	return;
}

// Creates a model def
// Returns that model def
void* s_model( sterm* raw_properties ) {
	return NULL;
}

// pass through to model?
/*
void* s_mesh( sterm* raw_properties ) {

}
*/

void test_smodel() {
	const char* test_string = "(model (mesh \"dat/model/ship_hd_2.obj\"))";
	sterm* s = parse_string( test_string );
	model* result = eval( s );
	(void)result;
}

void test_sfile( ) {
	test_smodel();
	/*
	printf( "FILE: Beginning test: test dat/test2.s\n" );
	sterm* s = parse_file( "dat/test2.s" );
	eval( s );
	sterm_free( s );
	*/

	printf( "FILE: Beginning test: test concat\n" );
	test_s_concat();
}