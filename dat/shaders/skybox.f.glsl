//#version 110
// Phong Fragment Shader
#ifdef GL_ES
precision highp float;
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

float sun_fog( vec4 sunLocal, vec4 fragment_position ) {
	return clamp( dot( normalize( sunLocal ), normalize( fragment_position )), 0.0, 1.0 );
}

const float sunMin = 0.5;
const float sunMax = 1.4;
const float rayFreq = 14.0;
const vec4 sunwhite = vec4(0.5, 0.5, 0.3, 1.0);

float sun( vec4 local_sun_dir, vec4 fragment_position ) {
	float g = max(0.1, smoothstep( sunMin, sunMax, abs(normalize(local_sun_dir).z)));
	float f = smoothstep( 1.0 - 0.33 * g, 1.0, clamp( dot( local_sun_dir, normalize( fragment_position )), 0.0, 1.0 ));
	vec3 v = cross(normalize(fragment_position.xyz), local_sun_dir.xyz);
	return pow(f, 4.0) + (1.0 - pow(f, 4.0)) * 0.25 * max(0.0, 0.5 + 0.5 * cos(atan(v.x / v.y) * rayFreq));
}

void main() {
#if 1
	//vec4 material_diffuse = texture2D( tex, texcoord );

	// color = top * blue
	// then blend in cloud (white * green, blend alpha )
//	vec4 fragColor = mix( sky_color_top * material_diffuse.z, cloud_color * material_diffuse.y, material_diffuse.w );
	//vec4 fragColor = mix( sky_color_top * 1.0, cloud_color * material_diffuse.y, 0.0 );
	vec4 fragColor = sky_color_top;
	// then add bottom * red
	fragColor = fragColor + sky_color_bottom * 0.0;
	fragColor.w = 1.0;

	// Fog
	float fog = clamp( 100.0 / ( max( 1.0, height * 0.5 + 40.0 )), 0.0, 1.0 );

	float fogSun = sun_fog( camera_space_sun_direction, frag_position );

	float sun = sun( camera_space_sun_direction, frag_position );
	vec4 local_fog_color = fog_color + (sun_color * fogSun) + (sunwhite * sun);
	
	gl_FragColor = mix( fragColor, local_fog_color, fog );

	vec4 f = frag_position;
#else
	gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
#endif
}

