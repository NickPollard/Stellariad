// shader.c

#include "common.h"
#include "render/shader.h"
//---------------------
#include "mem/allocator.h"
#include "render/render.h"
#include "script/sexpr.h"
#include "system/file.h"
#include "system/inputstream.h"
#include "system/string.h"
#include "system/hash.h"

#define SHADER_DEBUG 0
#if SHADER_DEBUG
#define SHADER_DEBUGPRINT( ... ) printf( __VA_ARGS__ )
#else
#define SHADER_DEBUGPRINT( ... ) 
#endif // SHADER_DEBUG

#define kMaxShaderConstants 128
map* shader_constants = NULL;
#define kShaderMaxLogLength (16 << 10)

namespace Shader {
	// Lookup by filename
	shader** byName( const char* name ) {
		shader** s = shaderGet( name );
		if (s) return s; else return shaderGet("dat/shaders/default.s");
	}

	shader** defaultShader() { return byName( "dat/shaders/default.s" ); }
	shader** depth() { return byName( "dat/shaders/depth.s" ); }

	// Only set uniforms if we definitely have them - otherwise we might override aliased constants
	// in the current shader
	void Uniform( GLuint uniform, matrix m ) {
		if ( uniform != kShaderConstantNoLocation ) glUniformMatrix4fv( uniform, 1, /*transpose*/false, (GLfloat*)m ); 
	}

	// Takes a uniform and an OpenGL texture name (GLuint)
	// Binds the given texture to an available texture unit and sets the uniform
	void Uniform( GLuint uniform, GLuint texture ) {
		if ((int)uniform >= 0 ) {
			vAssert( render_current_texture_unit < graphicsSystem.maxTextureUnits );
			glActiveTexture( GL_TEXTURE0 + render_current_texture_unit );
			glBindTexture( GL_TEXTURE_2D, texture );
			glUniform1i( uniform, render_current_texture_unit );
			render_current_texture_unit++;
		}
	}

	void Uniform( GLuint uniform, vector* v ) {
		if ( uniform != kShaderConstantNoLocation ) glUniform4fv( uniform, 1, (GLfloat*)v );
	}

	void Uniform( GLuint uniform, vector v ) { Uniform( uniform, &v ); }

	void loadShaders() {
		shaderLoad( "dat/shaders/default.s", true );
		shaderLoad( "dat/shaders/depth.s", true );
		shaderLoad( "dat/shaders/refl_normal.s", true );
		shaderLoad( "dat/shaders/reflective.s", true );
		shaderLoad( "dat/shaders/terrain.s", true );
		shaderLoad( "dat/shaders/gaussian.s", true );
		shaderLoad( "dat/shaders/gaussian_vert.s", true );
		shaderLoad( "dat/shaders/ui.s", true );
		shaderLoad( "dat/shaders/dof.s", true );
		shaderLoad( "dat/shaders/ssao.s", true );
		shaderLoad( "dat/shaders/skybox.s", true );
		shaderLoad( "dat/shaders/particle.s", true );

#define GET_UNIFORM_LOCATION( var ) \
		resources.uniforms.var = shader_findConstant( mhash( #var )); \
		assert( resources.uniforms.var != NULL );
		SHADER_UNIFORMS( GET_UNIFORM_LOCATION )
			VERTEX_ATTRIBS( VERTEX_ATTRIB_LOOKUP );
	}
}

// Find the program location for a named Uniform variable in the given program
GLint shader_getUniformLocation( GLuint program, const char* name ) { return glGetUniformLocation( program, name ); }

// Find the program location for a named Attribute variable in the given program
GLint shader_getAttributeLocation( GLuint program, const char* name ) { return glGetAttribLocation( program, name ); }

// Create a binding for the given variable within the given program
shaderConstantBinding shader_createBinding( GLuint shader_program, const char* variable_type, const char* variable_name ) {
	// For each one create a binding
	shaderConstantBinding binding;
	binding.program_location = shader_getAttributeLocation( shader_program, variable_name );
	if ( binding.program_location == -1 )
		binding.program_location = shader_getUniformLocation( shader_program, variable_name );
	binding.value = (GLint*)map_findOrAdd( shader_constants, mhash( variable_name ));

	// TODO: Make this into an easy lookup table
	binding.type = uniform_unknown;
	if ( string_equal( "vec4", variable_type ))
		binding.type = uniform_vector;
	if ( string_equal( "mat4", variable_type ))
		binding.type = uniform_matrix;
	if ( string_equal( "sampler2D", variable_type ))
		binding.type = uniform_tex2D;

	SHADER_DEBUGPRINT( "SHADER: Created Shader binding 0x" xPTRf " for \"%s\" at location 0x%x, type: %s\n", (uintptr_t)binding.value, variable_name, binding.program_location, variable_type );
	return binding;
}

void shaderDictionary_addBinding( shaderDictionary* d, shaderConstantBinding b ) {
	assert( d->count < kMaxShaderConstantBindings );
	d->bindings[d->count++] = b;
}

// Find a list of uniform variable names in a shader source file
void shader_buildDictionary( shaderDictionary* dict, GLuint shader_program, const char* src ) {
	// Find a list of uniform variable names
	inputStream* stream = inputStream_create( src );
	char* token;
	while ( !inputStream_endOfFile( stream )) {
		token = inputStream_nextToken( stream );
		if (( string_equal( token, "uniform" ) || string_equal( token, "attribute" )) && !inputStream_endOfFile( stream )) {
			// Advance two tokens (the next is the type declaration, the second is the variable name)
			inputStream_freeToken( stream, token );
			token = inputStream_nextToken( stream );
			#define kMaxVariableLength 64
			char type[kMaxVariableLength];
			char name[kMaxVariableLength];
			string_trimCopy( type, token );
			inputStream_freeToken( stream, token );
			token = inputStream_nextToken( stream );
			string_trimCopy( name, token );
			// If it's an array remove the array specification
			char* c = (char*)name;
			int length = strlen(name );
			while ( c < name + length ) {
				if ( *c == '[' )
					*c = '\0';
				c++;
			}

			shaderDictionary_addBinding( dict, shader_createBinding( shader_program, type, name ));
		}
		inputStream_freeToken( stream, token );
	}
}

typedef void (*func_getIV)( GLuint, GLenum, GLint* );
typedef void (*func_getInfoLog)( GLuint, GLint, GLint*, GLchar* );

void gl_dumpInfoLog( GLuint object, func_getIV getIV, func_getInfoLog getInfoLog ) {
	GLint length = -1;
	char* log;
	getIV( object, GL_INFO_LOG_LENGTH, &length );
	if ( length != -1 ) {
		length = min( length, kShaderMaxLogLength );
		log = (char*)mem_alloc( sizeof( char ) * length );
		log[0] = '\0';
		GLint length_read;
		getInfoLog( object, length, &length_read, log );
		printf( "--- shader compiler output ---\n" );
		printf( "%s", log );
		printf( "------\n" );
		mem_free( log );
	}
}

// Compile a GLSL shader object from the given source code
// Based on code from Joe's Blog: http://duriansoftware.com/joe/An-intro-to-modern-OpenGL.-Chapter-2.2:-Shaders.html
GLuint shader_compile( GLenum type, const char* path, const char* source ) {
	GLint length = strlen( source );
	GLuint glShader;
	GLint shader_ok;

	if ( !source ) {
		printError( "Cannot create Shader. File %s not found.", path );
		assert( 0 );
	}

	glShader = glCreateShader( type );
	glShaderSource( glShader, 1, (const GLchar**)&source, &length );
	glCompileShader( glShader );

	glGetShaderiv( glShader, GL_COMPILE_STATUS, &shader_ok );
	if ( !shader_ok ) {
		printError( "Failed to compile Shader from File %s.", path );
		gl_dumpInfoLog( glShader, glGetShaderiv,  glGetShaderInfoLog );
		return 0;
		//assert( 0 );
	}

	return glShader;
}

// Link two given shader objects into a full shader program
GLuint shader_link( GLuint vertex_shader, GLuint fragment_shader ) {
	GLint program_ok;

	GLuint program = glCreateProgram();
	glAttachShader( program, vertex_shader );
	glAttachShader( program, fragment_shader );
	glLinkProgram( program );
	glValidateProgram( program );

	glGetProgramiv( program, GL_LINK_STATUS, &program_ok );
	if ( !program_ok ) {
		printf( "Failed to link shader program.\n" );
		gl_dumpInfoLog( program, glGetProgramiv, glGetProgramInfoLog );
		assert( 0 );
	}

	return program;
}

// Build a GLSL shader program from given vertex and fragment shader source pathnames
GLuint buildShader( const char* vertex_path, const char* fragment_path, const char* vertex_file, const char* fragment_file ) {
	GLuint vertex_shader = shader_compile( GL_VERTEX_SHADER, vertex_path, vertex_file );
	GLuint fragment_shader = shader_compile( GL_FRAGMENT_SHADER, fragment_path, fragment_file );
	bool err = (vertex_shader == 0 || fragment_shader == 0); 
	return err ? 0 : shader_link( vertex_shader, fragment_shader );
}

void shader_init() {
	shader_constants = map_create( kMaxShaderConstants, sizeof( GLint ));
	Shader::loadShaders();
}

void shader_bindConstant( shaderConstantBinding binding ) {
	*binding.value = binding.program_location;
}

GLint* shader_findConstant( int key ) {
	return (GLint*)map_find( shader_constants, key );
}

// Clear all constants so that they are unbound
void shader_clearConstants() {
#define CLEAR_SHADER_UNIFORM( var ) \
	*resources.uniforms.var = kShaderConstantNoLocation;
	SHADER_UNIFORMS( CLEAR_SHADER_UNIFORM )
}

void shader_bindConstants( shader* s ) {
	assert( s->dict.count < kMaxShaderConstantBindings );
	for ( int i = 0; i < s->dict.count; i++ )
		shader_bindConstant( s->dict.bindings[i] );
}

// Activate the shader for use in rendering
void shader_activate( shader* s ) {
	vAssert( s );
	glUseProgram( s->program );

	shader_clearConstants();
	shader_bindConstants( s );
}

map* shaderMap = NULL;

shader** shaderGet( const char* shaderName ) {
	void* s = map_find(shaderMap, mhash(shaderName));
	if (s) return (shader**)s;
	else return nullptr;
}

DEF_LIST(shaderInfo);
IMPLEMENT_LIST(shaderInfo);

shaderInfolist* shadersLoaded = nullptr;

bool shaderAlreadyLoaded( const char* shaderName ) {
	(void)shaderName;
	return false;
}

void shaderLoad( const char* shaderName, bool required ) {
	printf( "[SHADER] loading %s.\n", shaderName );
	if (shaderMap == NULL)
		shaderMap = map_create(128, sizeof( shader* ) );
	shaderInfo* sInfo = (shaderInfo*)sexpr_loadFile( shaderName );
	sInfo->name = shaderName;
	shader* s = shader_load( sInfo->vertex, sInfo->fragment );
	if (!s) {
		printf( "Could not load shader %s\n", shaderName );
		vAssert( !required || s );
	}
	else
		map_addOverride(shaderMap, mhash(shaderName), &s);

	// Add to loaded list
	// if no already in it
	if (!shaderAlreadyLoaded(shaderName)) {
		shadersLoaded = shaderInfolist_cons( sInfo, shadersLoaded );
	}
}

void shadersReloadAll() {
	for ( shaderInfolist* l = shadersLoaded; l && l->head; l = l->tail )
		if (vfile_modifiedSinceLast( l->head->fragment) || vfile_modifiedSinceLast( l->head->vertex) || vfile_modifiedSinceLast( l->head->name ))
			shaderLoad( l->head->name, false );
}

// Load a shader from GLSL files
shader* shader_load( const char* vertex_name, const char* fragment_name ) {
	SHADER_DEBUGPRINT( "SHADER: Loading Shader (Vertex: \"%s\", Fragment: \"%s\")\n", vertex_name, fragment_name );
	shader* s = (shader*)mem_alloc( sizeof( shader ));
	memset( s, 0, sizeof( shader ));
	s->dict.count = 0;

	// Load source code
	size_t length = 0;
	const char* vertex_file = (const char*)vfile_contents( vertex_name, &length );
	const char* fragment_file = (const char*)vfile_contents( fragment_name, &length );

	s->program = buildShader( vertex_name, fragment_name, vertex_file, fragment_file );
	if (s->program == 0) { // If we couldn't build it properly
		mem_free( s );
		return nullptr;
	}

	shader_buildDictionary( &s->dict, s->program, vertex_file );
	shader_buildDictionary( &s->dict, s->program, fragment_file );

	mem_free( (void*)vertex_file );			// Cast away const to free, we allocated this ourselves
	mem_free( (void*)fragment_file	);		// Cast away const to free, "

	vAssert( s );
	return s;
}
