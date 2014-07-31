#ifdef GL_ES
precision highp float;
#endif

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
varying vec2 screenCoord;
varying float specular;

// Uniform
uniform sampler2D tex;
uniform sampler2D tex_b;
uniform sampler2D tex_c;
uniform sampler2D tex_d;
uniform sampler2D ssao_tex;

uniform vec4 directional_light_direction;

// Test Light values
const vec4 light_ambient = vec4( 0.2, 0.2, 0.3, 1.0 );
// Directional Light
const vec4 directional_light_diffuse = vec4( 1.0, 1.0, 0.8, 1.0 );
const vec4 directional_light_specular = vec4( 0.4, 0.4, 0.4, 1.0 );

void main() {
	// light-invariant calculations
	float ssao = texture2D( ssao_tex, screenCoord ).x;
	vec4 cliff_color_2 = texture2D( tex_d, cliff_texcoord ); // TODO - like above!
	//float ssao = ssaov.x;
	vec4 view_direction = normalize( frag_position );

	vec4 normal = cameraSpace_frag_normal;

	// Ambient + Diffuse
	// TODO: Can this be done in the vertex shader?
	// how does dot( -light_direction, normal ) vary accross a poly?
	float diffuse = max( 0.0, dot( -directional_light_direction, normal ));
	vec4 total_light_color = (light_ambient + directional_light_diffuse * diffuse) * ssao; 
	total_light_color += directional_light_specular * specular;

	vec4 ground_color = texture2D( tex, texcoord );
	vec4 cliff_color = mix( texture2D( tex_b, cliff_texcoord ),
			texture2D( tex_b, cliff_texcoord_b ),
			smoothstep(0.2, 0.8, (abs(frag_normal.z) / ( abs(frag_normal.x) + abs(frag_normal.z)))));
	float darken = smoothstep(0.0, 1.0, clamp(0.6 + abs(cliff - 0.4), 0.0, 1.0));
	vec4 tex_color = mix( ground_color, cliff_color, cliff ) * darken;

	vec4 ground_color_2 = texture2D( tex_c, texcoord );
	vec4 tex_color_2 = mix( ground_color_2, cliff_color_2, cliff ) * darken;

	vec4 fragColor = total_light_color * mix( tex_color, tex_color_2, vert_color.x );
	gl_FragColor = mix( fragColor, local_fog_color, fog );
	gl_FragColor.w = 1.0;
}
