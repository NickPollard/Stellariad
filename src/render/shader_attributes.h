#pragma once

#define DECLARE_AS_GLINT_P( var ) GLint* var;
#define VERTEX_ATTRIB_DISABLE_ARRAY( attrib ) glDisableVertexAttribArray( *resources.attributes.attrib );
#define VERTEX_ATTRIB_LOOKUP( attrib ) resources.attributes.attrib = (shader_findConstant( mhash( #attrib )));

#define VERTEX_ATTRIB_POINTER( attrib ) \
	if ( *resources.attributes.attrib >= 0 ) { \
		glVertexAttribPointer( *resources.attributes.attrib, /*vec4*/ 4, GL_FLOAT, /*Normalized?*/GL_FALSE, sizeof( vertex ), (void*)offsetof( vertex, attrib )); \
		glEnableVertexAttribArray( *resources.attributes.attrib ); \
	}

#define SHADER_UNIFORMS( f ) \
	f( projection ) \
	f( modelview ) \
	f( worldspace ) \
	f( camera_to_world ) \
	f( light_position ) \
	f( light_diffuse ) \
	f( light_specular ) \
	f( tex ) \
	f( tex_b ) \
	f( tex_c ) \
	f( tex_d ) \
	f( tex_normal ) \
	f( ssao_tex ) \
	f( fog_color ) \
	f( sky_color_top ) \
	f( sky_color_bottom ) \
	f( camera_space_sun_direction ) \
	f( sun_color ) \
	f( viewspace_up ) \
	f( directional_light_direction ) \
	f( screen_size )

#define VERTEX_ATTRIBS( f ) \
	f( position ) \
	f( normal ) \
	f( uv ) \
	f( color )

struct ShaderUniforms { SHADER_UNIFORMS( DECLARE_AS_GLINT_P ) };
struct VertexAttribs { VERTEX_ATTRIBS( DECLARE_AS_GLINT_P ) };
