//#version 110
// Phong Fragment Shader
#ifdef GL_ES
precision mediump float;
#endif

// Varying
varying vec2 texcoord;
varying vec4 frag_position;
varying float height;

// Uniform
uniform sampler2D tex;
uniform vec4 fog_color;
uniform vec4 sky_color_top;
uniform vec4 sky_color_bottom;
uniform vec4 camera_space_sun_direction;

uniform vec4 sun_color;
const vec4 cloud_color = vec4( 1.0, 1.0, 1.0, 1.0 );

float sun_fog( vec4 local_sun_dir, vec4 fragment_position ) {
	return clamp( dot( local_sun_dir, normalize( fragment_position )), 0.0, 1.0 );
}

float sun( vec4 local_sun_dir, vec4 fragment_position ) {
	float g = max(0.1, smoothstep( 0.50, 1.0, abs(normalize(local_sun_dir).z)));
	float f = smoothstep( 1.0 - 0.33 * g, 1.0, clamp( dot( local_sun_dir, normalize( fragment_position )), 0.0, 1.0 ));
	//return pow(f, 4.0);
	return 0.0;
}

void main() {
	// light-invariant calculations
	vec4 material_diffuse = texture2D( tex, texcoord );
	//gl_FragColor = material_diffuse;

	// color = top * blue
	// then blend in cloud (white * green, blend alpha )
	vec4 fragColor = mix( sky_color_top * material_diffuse.z, cloud_color * material_diffuse.y, material_diffuse.w );
	// then add bottom * red
	fragColor = fragColor + sky_color_bottom * material_diffuse.x;
	fragColor.w = 1.0;

	// Fog
	float fog = clamp( 100.0 / ( max( 1.0, height * 0.5 + 40.0 )), 0.0, 1.0 );

	// sunlight on fog
	float fog_sun_factor = sun_fog( camera_space_sun_direction, frag_position );
	//float fog_sun_factor = 0.0;

	vec4 sunwhite = vec4(0.5, 0.5, 0.5, 1.0);
	float fog_actual_sun = sun( camera_space_sun_direction, frag_position );
	vec4 local_fog_color = fog_color + (sun_color * fog_sun_factor) + (sunwhite * fog_actual_sun);
	
	gl_FragColor = mix( fragColor, local_fog_color, fog );
	//gl_FragColor = texture2D( tex, texcoord );
	//gl_FragColor = vec4( 0.5, 0.5, 0.5, 1.0 );
}

