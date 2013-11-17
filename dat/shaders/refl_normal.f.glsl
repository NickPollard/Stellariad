//#version 110
// Reflective Fragment Shader

#ifdef GL_ES
precision mediump float;
#endif

#define FRESNEL 1

varying vec4 frag_position;
varying vec4 cameraSpace_frag_normal;
varying vec4 frag_normal;
varying vec2 texcoord;
varying float fog;

uniform mat4 worldspace;
uniform mat4 camera_to_world;
uniform vec4 directional_light_direction;
uniform vec4 sun_color;

uniform sampler2D tex;
uniform sampler2D tex_b;
uniform sampler2D tex_normal;
uniform vec4 fog_color;
uniform vec4 camera_space_sun_direction;
uniform	mat4 modelview;

const vec4 light_ambient = vec4( 0.2, 0.2, 0.3, 0.0 );
const vec4 directional_light_diffuse = vec4( 1.0, 1.0, 0.8, 1.0 );
const vec4 directional_light_specular = vec4( 0.4, 0.4, 0.4, 1.0 );

float sun_fog( vec4 local_sun_dir, vec4 view_direction ) {
	return clamp( dot( local_sun_dir, view_direction ), 0.0, 1.0 );
}

void main() {
	// light-invariant calculations
	vec4 view_direction = vec4( normalize( frag_position ).xyz, 0.0 );
	vec4 material_diffuse = texture2D( tex, texcoord );

	vec4 texture_normal = texture2D( tex_normal, texcoord );
	float x = texture_normal.x - 0.5 * 2.0; 
	float z = texture_normal.z - 0.5 * 2.0; 
	float y = 1.0 - ( x*x + z*z );
	vec4 image_normal = vec4( x, y, z, 0.0 );
	//vec4 imagespace_normal = normalize( image_normal );

	vec4 binormal = vec4( 0.0, 1.0, 0.0, 0.0 );
	vec4 tangent = vec4( cross( binormal.xyz, frag_normal.xyz ), 0.0 );
	mat4 tangent_space = mat4( tangent, binormal, frag_normal, vec4( 0.0, 0.0, 0.0, 1.0 ) );
	vec4 normal = modelview * tangent_space * image_normal;
	//vec4 normal = cameraSpace_frag_normal;

	// *** Directional light
	// Ambient + Diffuse
	vec4 light_direction = normalize( worldspace * directional_light_direction ); // TODO - this could be a static passed in from C
	float diffuse = max( 0.0, dot( -light_direction, normal ));
	vec4 total_light_color = light_ambient + directional_light_diffuse * diffuse;
	// Specular
	float spec = max( 0.0, dot( reflect( light_direction, normal ), -view_direction ));
	float shininess = 10.0;
	total_light_color += directional_light_specular * min(1.0, pow( spec, shininess ));

	// reflection
	vec4 refl_bounce = camera_to_world * reflect( view_direction, normalize( normal ));
	vec2 refl_coord = vec2( refl_bounce.x, abs(refl_bounce.y));
	vec4 reflection = texture2D( tex_b, refl_coord ) * material_diffuse.a;

#ifdef FRESNEL
	float fresnel = 0.5 + 0.5 * clamp(1.0 - dot( -view_direction, normal ), 0.0, 1.0);
#else
	float fresnel = 1.0;
#endif

	vec4 fragColor = total_light_color * material_diffuse + reflection * fresnel;
	vec4 local_fog_color = fog_color + (sun_color * sun_fog( camera_space_sun_direction, view_direction ));
	gl_FragColor = vec4( mix( fragColor, local_fog_color, fog ).xyz, 1.0);
}
