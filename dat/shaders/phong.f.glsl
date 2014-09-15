//#version 110
// Phong Fragment Shader

#ifdef GL_ES
precision mediump float;
#endif

// *** Varying
varying vec4 fragPosition;
varying vec4 fragNormal;
varying vec2 texcoord;
varying float fog;
varying vec2 screenCoord;

// *** Uniform
uniform mat4 worldspace;
uniform vec4 directional_light_direction;
uniform vec4 sun_color;
uniform sampler2D tex;
uniform sampler2D ssao_tex;
uniform vec4 fog_color;
uniform vec4 camera_space_sun_direction;

const vec4 ambient = vec4( 0.2, 0.2, 0.3, 0.0 );
const vec4 directionalDiffuse = vec4( 1.0, 1.0, 0.8, 1.0 );
const vec4 directionalSpecular = vec4( 0.4, 0.4, 0.4, 1.0 );

float sun_fog( vec4 local_sun_dir, vec4 fragment_position ) {
	return clamp( dot( normalize(local_sun_dir), normalize( fragment_position )), 0.0, 1.0 );
}

float sun( vec4 sunDir, vec4 fragPosition ) {
	vec4 sun = normalize( sunDir );
	vec4 dir = normalize( fragPosition );
	float g = max(0.1, smoothstep( 0.50, 1.0, abs(sun.z)));
	float f = smoothstep( 1.0 - 0.33 * g, 1.0, clamp( dot( sun, dir ), 0.0, 1.0 ));
	vec3 v = cross(dir.xyz, sun.xyz);
	return pow(f, 4.0) + (1.0 - pow(f, 4.0)) * 0.25 * max(0.0, 0.5 + 0.5 * cos(atan(v.x / v.y) * 20.0));//cos(angle);
}

void main() {
	vec4 ssao = texture2D( ssao_tex, screenCoord * 0.5 + vec2( 0.5, 0.5 ) );
	vec4 material = texture2D( tex, texcoord );

	vec4 light_direction = normalize( worldspace * directional_light_direction );
	float diffuse_amount = max( 0.0, dot( -light_direction, fragNormal )) * 1.0;
	vec4 diffuse = directionalDiffuse * diffuse_amount;

	vec4 bounce = reflect( light_direction, fragNormal );
	float spec = clamp( dot( bounce, -normalize(fragPosition)), 0.0, 1.0 );
	vec4 specular = directionalSpecular * pow( spec, 10.0 ) * material.a;

	vec4 fragColor = ((ambient + diffuse) * ssao.x + specular ) * material;
	//vec4 local_fog_color = fog_color + (sun_color * sun_fog( camera_space_sun_direction, fragPosition ));

	vec4 sunwhite = vec4(0.5, 0.5, 0.5, 1.0);
	float sun = sun( camera_space_sun_direction, normalize(fragPosition) );
	float fogSun = sun_fog( camera_space_sun_direction, fragPosition );
	vec4 local_fog_color = fog_color + (sun_color * fogSun) + (sunwhite * sun);

	//gl_FragColor = vec4(mix( fragColor, local_fog_color, fog ).xyz, 1.0);
	gl_FragColor = vec4( 1.0, 1.0, 1.0, 1.0 );
}
