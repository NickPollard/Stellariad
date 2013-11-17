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

// *** Uniform
uniform mat4 worldspace;
uniform vec4 directional_light_direction;
uniform vec4 sun_color;
uniform sampler2D tex;
uniform vec4 fog_color;
uniform vec4 camera_space_sun_direction;

const vec4 ambient = vec4( 0.2, 0.2, 0.3, 0.0 );
const vec4 directionalDiffuse = vec4( 1.0, 1.0, 0.8, 1.0 );
const vec4 directionalSpecular = vec4( 0.4, 0.4, 0.4, 1.0 );

float sun_fog( vec4 local_sun_dir, vec4 fragment_position ) {
	return clamp( dot( normalize(local_sun_dir), normalize( fragment_position )), 0.0, 1.0 );
}

void main() {
	vec4 material = texture2D( tex, texcoord );

	vec4 light_direction = normalize( worldspace * directional_light_direction );
	float diffuse_amount = max( 0.0, dot( -light_direction, fragNormal )) * 1.0;
	vec4 diffuse = directionalDiffuse * diffuse_amount;

	vec4 bounce = reflect( light_direction, fragNormal );
	float spec = clamp( dot( bounce, -normalize(fragPosition)), 0.0, 1.0 );
	vec4 specular = directionalSpecular * pow( spec, 10.0 ) * material.a;

	vec4 fragColor = (ambient + specular + diffuse) * material;
	vec4 local_fog_color = fog_color + (sun_color * sun_fog( camera_space_sun_direction, fragPosition ));
	gl_FragColor = vec4(mix( fragColor, local_fog_color, fog ).xyz, 1.0);
}
