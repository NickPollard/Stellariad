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

vec4 myNormalize( vec4 v ) {
	float scale = 1.f / sqrt( v.x * v.x +
		   	v.y * v.y +
		   	v.z * v.z +
		   	v.w * v.w );
	return vec4( v.xyz * scale, v.w );
}

float sun( vec4 local_sun_dir, vec4 fragment_position ) {
	float g = max(0.1, smoothstep( 0.50, 1.0, abs(normalize(local_sun_dir).z)));
	float f = smoothstep( 1.0 - 0.33 * g, 1.0, clamp( dot( local_sun_dir, normalize( fragment_position )), 0.0, 1.0 ));
	vec3 v = cross(normalize(fragment_position.xyz), local_sun_dir.xyz);
	return pow(f, 4.0) + (1.0 - pow(f, 4.0)) * 0.25 * max(0.0, 0.5 + 0.5 * cos(atan(v.x / v.y) * 20.0));//cos(angle);
}

void main() {
	vec4 material_diffuse = texture2D( tex, texcoord );

	// color = top * blue
	// then blend in cloud (white * green, blend alpha )
	vec4 fragColor = mix( sky_color_top * material_diffuse.z, cloud_color * material_diffuse.y, material_diffuse.w );
	// then add bottom * red
	fragColor = fragColor + sky_color_bottom * material_diffuse.x;
	fragColor.w = 1.0;

	// Fog
	float fog = clamp( 100.0 / ( max( 1.0, height * 0.5 + 40.0 )), 0.0, 1.0 );

	// sunlight on fog
	float fogSun = sun_fog( camera_space_sun_direction, frag_position );
	//float fogSun = sun_fog( camera_space_sun_direction, vec4( 0.0, 0.0, 1.0, 0.0 ) );
	//float fogSun = sun_fog( vec4( 0.0, 0.0, 1.0, 0.0 ), frag_position );

	//float fogSun = 0.0;

	vec4 sunwhite = vec4(0.5, 0.5, 0.5, 1.0);
	float sun = sun( camera_space_sun_direction, frag_position );
	vec4 local_fog_color = fog_color + (sun_color * fogSun) + (sunwhite * sun);
	//vec4 local_fog_color = fog_color + (vec4(1.0, 0.0, 0.0, 0.0) * fogSun) + (sunwhite * sun);
	
	gl_FragColor = mix( fragColor, local_fog_color, fog );
	//gl_FragColor = texture2D( tex, texcoord );
	//gl_FragColor = vec4( fogSun, 0.0, 0.0, 1.0 );
	//gl_FragColor = normalize( frag_position );
	//gl_FragColor = frag_position * 0.001;

	//gl_FragColor = normalize( frag_position );
	vec4 f = frag_position;
	//vec4 f = frag_position;
	//if ( frag_position.x > 0.0 ) {
		//gl_FragColor = myNormalize( f );
	//} else {
		//gl_FragColor = normalize( f );
	//}
}

