//#version 110
// Phong Fragment Shader
#ifdef GL_ES
precision mediump float;
#endif

// Varying
varying vec2 texcoord;
varying vec4 frag_position;
varying float fog;
varying vec4 local_fog_color;

// Uniform
uniform sampler2D tex;
uniform vec4 fog_color;
uniform vec4 sky_color_top;
uniform vec4 sky_color_bottom;
uniform vec4 camera_space_sun_direction;

uniform vec4 sun_color;
const vec4 cloud_color = vec4( 1.0, 1.0, 1.0, 1.0 );

void main() {
	vec4 tex_color = texture2D( tex, texcoord );
	gl_FragColor = mix( tex_color, local_fog_color, fog );
}

