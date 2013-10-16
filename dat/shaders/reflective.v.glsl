//#version 110

#ifdef GL_ES
precision mediump float;
#endif

// Reflective Vertex Shader
// Attributes
attribute vec4 position;
attribute vec4 normal;
attribute vec4 uv;
attribute vec4 color;

// Varying
varying vec4 frag_position;
varying vec4 cameraSpace_frag_normal;
varying vec2 texcoord;
varying float fog;

// Uniform
uniform	mat4 projection;
uniform	mat4 modelview;

void main() {
	gl_Position = projection * modelview * position;
	frag_position = modelview * position;
	cameraSpace_frag_normal = modelview * normal;
	texcoord = uv.xy;
	// TODO - need to get a proper world position, not model position
	// This means we need the model matrix separate from the combined model-view
	//fog = fog( position, frag_position );

	// We can calculate fog per vertex as we know polys will be small for terrain
	float fog_far		= 450.0;
	float fog_near		= 200.0;
	float fog_distance	= 250.0;			// fog_far - fog_near
	float fog_height	= 160.0;
	float fog_max		= 0.4;
	float distant_fog_near		= 600.0;
	float distant_fog_far		= 700.0;
	float distant_fog_distance	= 100.0; // distant_fog_far - distant_fog_near

	float height_factor = clamp( ( fog_height - position.y ) / fog_height, 0.0, 1.0 );
	float distance = sqrt( frag_position.z * frag_position.z + frag_position.x * frag_position.x );
	float near_fog = clamp(( distance - fog_near ) / fog_distance, 0.0, fog_max ) * height_factor;
	float far_fog = clamp(( frag_position.z - distant_fog_near ) / distant_fog_distance, 0.0, 1.0 );
	fog = max( near_fog, far_fog );
}
