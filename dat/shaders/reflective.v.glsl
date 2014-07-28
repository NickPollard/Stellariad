//#version 110
// Reflective Vertex Shader

#ifdef GL_ES
precision mediump float;
#endif

attribute vec4 position;
attribute vec4 normal;
attribute vec4 uv;
attribute vec4 color;

varying vec4 frag_position;
varying vec4 cameraSpace_frag_normal;
varying vec2 texcoord;
varying float fog;
varying vec2 screenCoord;

uniform	mat4 projection;
uniform	mat4 modelview;

void main() {
	gl_Position = projection * modelview * position;
	screenCoord = gl_Position.xy / gl_Position.w;
	frag_position = modelview * position;
	cameraSpace_frag_normal = modelview * normal;
	texcoord = uv.xy;

	float fog_far		= 450.0;
	float fog_near		= 200.0;
	float fog_distance	= 250.0;			// fog_far - fog_near
	float fog_height	= 160.0;
	float fog_max		= 0.4;
	float distant_fog_near		= 600.0;
	float distant_fog_far		= 700.0;
	float distant_fog_distance	= 100.0; // distant_fog_far - distant_fog_near

	float distance = sqrt( frag_position.z * frag_position.z + frag_position.x * frag_position.x );
	float height_factor = clamp( ( fog_height - position.y ) / fog_height, 0.0, 1.0 );
	float near_fog = min(( distance - fog_near ) / fog_distance, fog_max ) * height_factor;
	float far_fog = ( distance - distant_fog_near ) / distant_fog_distance;
	fog = clamp( max( near_fog, far_fog ), 0.0, 1.0 );
}
