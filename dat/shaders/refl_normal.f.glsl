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
varying vec2 screenCoord;

uniform mat4 worldspace;
uniform mat4 camera_to_world;
uniform vec4 directional_light_direction;
uniform vec4 sun_color;
uniform sampler2D ssao_tex;

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

float sun( vec4 sunDir, vec4 fragPosition ) {
	vec4 sun = normalize( sunDir );
	vec4 dir = normalize( fragPosition );
	float g = max(0.1, smoothstep( 0.50, 1.0, abs(sun.z)));
	float f = smoothstep( 1.0 - 0.33 * g, 1.0, clamp( dot( sun, dir ), 0.0, 1.0 ));
	vec3 v = cross(dir.xyz, sun.xyz);
	return pow(f, 4.0) + (1.0 - pow(f, 4.0)) * 0.25 * max(0.0, 0.5 + 0.5 * cos(atan(v.x / v.y) * 20.0));//cos(angle);
}

void main() {
	// light-invariant calculations
	vec4 view_direction = vec4( normalize( frag_position ).xyz, 0.0 );
	vec4 material_diffuse = texture2D( tex, texcoord );
	vec4 ssao = texture2D( ssao_tex, screenCoord * 0.5 + vec2( 0.5, 0.5 ) );

	vec4 texture_normal = texture2D( tex_normal, texcoord );
	float x = texture_normal.x - 0.5 * 2.0; 
	float z = texture_normal.z - 0.5 * 2.0; 
	float y = 1.0 - ( x*x + z*z );
	vec4 image_normal = vec4( x, y, z, 0.0 );

	vec4 binormal = vec4( 0.0, 1.0, 0.0, 0.0 );
	vec4 tangent = vec4( cross( binormal.xyz, frag_normal.xyz ), 0.0 );
	mat4 tangent_space = mat4( tangent, binormal, frag_normal, vec4( 0.0, 0.0, 0.0, 1.0 ) );
	vec4 normal = modelview * tangent_space * image_normal;

	// *** Directional light
	// Ambient + Diffuse
	vec4 light_direction = normalize( worldspace * directional_light_direction ); // TODO - this could be a static passed in from C
	float diffuse = max( 0.0, dot( -light_direction, normal ));
	vec4 total_light_color = light_ambient * ssao.x + directional_light_diffuse * diffuse;
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

	material_diffuse = vec4( 0.5, 0.5, 0.5, 1.0 );
	vec4 fragColor = total_light_color * material_diffuse;// + reflection * fresnel;
	vec4 sunwhite = vec4(0.5, 0.5, 0.5, 1.0);
	float sun = sun( camera_space_sun_direction, view_direction );
	float fogSun = sun_fog( camera_space_sun_direction, view_direction );
	vec4 local_fog_color = fog_color + (sun_color * fogSun) + (sunwhite * sun);
	gl_FragColor = vec4( mix( fragColor, local_fog_color, fog ).xyz, 1.0);
}
