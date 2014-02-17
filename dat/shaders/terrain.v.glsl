//#version 110
#ifdef GL_ES
precision highp float;
#endif

//#define NORMAL_MAPPING

// Attributes
attribute vec4 position;
attribute vec4 normal;
attribute vec4 uv;
attribute vec4 color;

// Varying
varying vec4 frag_position;
varying vec4 cameraSpace_frag_normal;
varying vec4 frag_normal;
varying vec2 texcoord;
varying vec2 cliff_texcoord;
varying vec2 cliff_texcoord_b;
varying vec4 vert_color;
varying float fog;
varying vec4 local_fog_color;
varying float cliff;
#ifdef NORMAL_MAPPING
#else
varying float specular;
#endif


// Uniform
uniform	mat4 projection;
uniform	mat4 modelview;
uniform vec4 camera_space_sun_direction;
uniform vec4 fog_color;
uniform vec4 sun_color;
uniform vec4 viewspace_up;
uniform vec4 directional_light_direction;

float sun_fog( vec4 local_sun_dir, vec4 view_direction ) {
	return max( 0.0, dot( local_sun_dir, view_direction ));
}

void main() {
	gl_Position = projection * modelview * position;
#if 1
	frag_position = modelview * position;
	cameraSpace_frag_normal = modelview * normal;
	frag_normal = normal;
	texcoord = uv.xz;
	cliff_texcoord = uv.zy;
	cliff_texcoord_b = uv.xy;

	vert_color = vec4( color.xyz, 1.0 );
	
	vec4 view_direction = normalize( frag_position );

	// We can calculate fog per vertex as we know polys will be small for terrain
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

	// sunlight on fog
	float fog_sun_factor = sun_fog( camera_space_sun_direction, view_direction );
	local_fog_color = fog_color + ( sun_color * fog_sun_factor );
	
	// Cliff
	float edge_ground = 0.03;
	float edge_cliff = 0.15;
	float d = 1.0 - max( normal.y, 0.0 );
	cliff = smoothstep( edge_ground, edge_cliff, d );

	// lighting
	// Specular
#ifdef NORMAL_MAPPING
#else
	vec4 spec_bounce = reflect( directional_light_direction, cameraSpace_frag_normal );
	specular = max( 0.0, dot( spec_bounce, -view_direction ));
#endif // NORMAL_MAPPING
#endif
}
