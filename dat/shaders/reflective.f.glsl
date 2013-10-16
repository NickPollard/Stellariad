//#version 110
// Reflective Fragment Shader

#ifdef GL_ES
precision mediump float;
#endif

//#define POINT_LIGHTS

// Varying
varying vec4 frag_position;
varying vec4 cameraSpace_frag_normal;
varying vec2 texcoord;
varying float fog;
const int LIGHT_COUNT = 2;

// Uniform
uniform mat4 worldspace;
uniform mat4 camera_to_world;
uniform vec4 directional_light_direction;
uniform vec4 sun_color;
#ifdef POINT_LIGHTS
uniform vec4 light_position[LIGHT_COUNT];
uniform vec4 light_diffuse[LIGHT_COUNT];
uniform vec4 light_specular[LIGHT_COUNT];
#endif // POINT_LIGHTS
uniform sampler2D tex;
uniform sampler2D tex_b;
uniform vec4 fog_color;
uniform vec4 camera_space_sun_direction;

// Test Light values
const vec4 light_ambient = vec4( 0.2, 0.2, 0.3, 0.0 );
// Directional Light
const vec4 directional_light_diffuse = vec4( 1.0, 1.0, 0.8, 1.0 );
const vec4 directional_light_specular = vec4( 0.4, 0.4, 0.4, 1.0 );

const float light_radius = 20.0;

float sun_fog( vec4 local_sun_dir, vec4 fragment_position ) {
	return max( 0.0, dot( local_sun_dir, normalize( fragment_position )));
}

void main() {
#if 0
	gl_FragColor = vec4( 0.0, 1.0, 0.0, 1.0 );
#else
	// light-invariant calculations
	vec4 view_direction = normalize( frag_position );
	vec4 material_diffuse = texture2D( tex, texcoord );
	vec4 total_light_color = light_ambient;

	// Directional light
#if 1
		// Ambient + Diffuse
		vec4 light_direction = normalize( worldspace * directional_light_direction );
		float diffuse = max( 0.0, dot( -light_direction, cameraSpace_frag_normal )) * 1.0;
		total_light_color += directional_light_diffuse * diffuse;

		// Specular
		vec4 spec_bounce = reflect( light_direction, cameraSpace_frag_normal );
		float spec = max( 0.0, dot( spec_bounce, -view_direction ));
		float shininess = 10.0;
		//float specular = pow( spec, shininess );
		total_light_color += directional_light_specular * pow( spec, shininess );
	
#endif

		// reflection
		//vec4 dir = vec4( 0.0, 0.0, 1.0, 0.0 );
		//vec4 dir = normalize
		view_direction.w = 0.0;
		vec4 r = reflect( view_direction, normalize( cameraSpace_frag_normal ));
		r.w = 0.0;
		vec4 refl_bounce = camera_to_world * r;
		vec2 refl_coord = vec2( refl_bounce.x, abs(refl_bounce.y));
		vec4 reflection = texture2D( tex_b, refl_coord ) * 1.4 * material_diffuse.a;
#ifdef POINT_LIGHTS	
	for ( int i = 0; i < LIGHT_COUNT; i++ ) {
		// Per-light calculations
		vec4 cs_light_position = worldspace * light_position[i];
		vec4 light_direction = normalize( frag_position - cs_light_position );
		float light_distance = length( frag_position - cs_light_position );

		// Ambient + Diffuse
		float diffuse = max( 0.0, dot( -light_direction, cameraSpace_frag_normal )) * max( 0.0, ( light_radius - light_distance ) / ( light_distance - 0.0 ) );
		vec4 diffuse_color = light_diffuse[i] * diffuse;
		total_diffuse_color += diffuse_color;

		// Specular
		vec4 spec_bounce = reflect( light_direction, cameraSpace_frag_normal );
		float spec = max( 0.0, dot( spec_bounce, -view_direction ));
		float shininess = 10.0;
		float specular = pow( spec, shininess );
		vec4 specular_color = light_specular[i] * specular;
		total_specular_color += specular_color;
	}
#endif

	vec4 fragColor = total_light_color * material_diffuse * 0.7 + reflection;
	//vec4 fragColor = reflection;
	
	// sunlight on fog
	float fog_sun_factor = sun_fog( camera_space_sun_direction, frag_position );
	vec4 local_fog_color = fog_color + (sun_color * fog_sun_factor);

	gl_FragColor = mix( fragColor, local_fog_color, fog );
	gl_FragColor.w = 1.0;
#endif

}
