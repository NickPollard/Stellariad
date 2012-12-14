//#version 110
#ifdef GL_ES
precision mediump float;
#endif
// Phong Vertex Shader
// Attributes
attribute vec4 position;
attribute vec4 normal;
attribute vec4 uv;
attribute vec4 color;

// Varying
varying vec2 texcoord;
varying vec4 frag_position;
varying float fog;
varying vec4 local_fog_color;

// Uniform
uniform	mat4 projection;
uniform	mat4 modelview;
uniform vec4 camera_space_sun_direction;
uniform vec4 sun_color;
uniform vec4 fog_color;

float sun_fog( vec4 local_sun_dir, vec4 position ) {
	//return max( 0.0, dot( local_sun_dir, normalize( position )));
	float scale = length( position );
	return ( max( 0.0, dot( local_sun_dir, position ))) / scale;
}

void main() {
	gl_Position = projection * modelview * position;
	frag_position = modelview * position;
	texcoord = uv.xy;

	float fog_sun_factor = sun_fog( camera_space_sun_direction, frag_position );
	local_fog_color = fog_color + (sun_color * fog_sun_factor);

	// We can calculate fog per vertex as long as the verts are close enough together (fine for sky-dome)
	float fog_height	= 160.0;
	float fog_max		= 0.4;

	//float distance = sqrt( frag_position.z * frag_position.z + frag_position.x * frag_position.x );
	fog = clamp( ( fog_height - position.y ) / fog_height, 0.0, 1.0 );
	//fog = clamp( 100.0 / ( max( 1.0, position.y + 120.0 )), 0.0, 1.0 );
}
