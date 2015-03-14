//#version 110
// Reflective Fragment Shader

#ifdef GL_ES
precision highp float;
#endif

varying vec4 frag_position;
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
uniform	mat4 projection;

// Test Light values
const vec4 ambient = vec4( 0.05, 0.05, 0.1, 1.0 );
// Directional Light
const vec4 directional_light_diffuse = vec4( 1.0, 1.0, 0.8, 1.0 );
const vec4 directional_light_specular = vec4( 1.0, 1.0, 0.8, 1.0 );

const vec4 sunwhite = vec4(0.5, 0.5, 0.5, 1.0);

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










const vec4 incomingLight = vec4(0.9, 0.9, 0.8, 1.0);

/* Fresnel - Schlick's approximation
	 R = R0 + (1 - R0)(1 - v.h)^5
	 */
float fresnel(float R0, vec4 v, vec4 h) { return R0 + (1.0 - R0) * pow((1.0 - max(0.0, dot(-v, h))), 5.0); }

void main() {
	// Normalization HAS to happen at the FRAGMENT level; is considerably non-linear
	vec4 c = projection * frag_position;
	vec2 coord = (c.xy / c.w) * 0.5 + vec2(0.5, 0.5);

	//vec4 texture_normal = texture2D( tex_normal, texcoord ) * 2.0 - vec4(1.0, 1.0, 1.0, 0.0);
	vec4 texture_normal = vec4(0.0, 0.0, 1.0, 0.0);

	vec4 image_normal = vec4( texture_normal.xyz, 0.0 );

	vec4 binormal = vec4( 0.0, 1.0, 0.0, 0.0 );
	vec4 tangent = vec4( cross( binormal.xyz, frag_normal.xyz ), 0.0 );
	mat4 tangent_space = mat4( tangent, binormal, frag_normal, vec4( 0.0, 0.0, 0.0, 1.0 ) );

	// *** ambient light
	vec4 ssaoVec = texture2D( ssao_tex, coord );
	float ssao = ssaoVec.x;

	/*
		 Cook-Torrance specular micro-facet model

		 f(l,v) = D(h)F(v,h)G(v,h,l) / 4(n.l)(n.v)

		 where
		 D = Normal _D_istribution Function
		 F = _F_resnel term
		 V = _V_isibility term
		 (l = light vector, v = view vector, h = ?)
		 */

	vec4 v = vec4( normalize( frag_position ).xyz, 0.0 );
	vec4 l = normalize( worldspace * directional_light_direction ); // TODO - this could be a static passed in from C
	vec4 h = normalize(-v + -l);
	vec4 n = normalize(modelview * tangent_space * image_normal);

	/*
		 Energy conservation - energy is either reflected (specular), or absorbed and reflected (diffuse)
		 Specular energy can be spread narrow (glossy) or wide (rough) - but total light energy is constant
		 */

	// *** Control parameters (Should be in texture? Could go in normal texture)
	float roughness = 0.3; // microfacet distribution, 0.0 = perfectly smooth, 1.0 = uniformly distributed?
	float metalicity = 0.9; // How much energy is specularly reflected, vs diffuse. 1.0 = full metal, 0.0 = full insulator

	vec4 material = texture2D( tex, texcoord );
	vec4 albedo = material * vec4(1.0, 0.8, 0.5, 1.0);
	//vec4 albedo = vec4(0.5, 0.5, 0.5, 1.0);
	float R0 = metalicity * 0.8; // base fresnel reflectance (head-on)

	float f = fresnel(R0, v, h); // Should be used for spec; unused as it seems to make little difference?

	// *** reflection
	vec4 bounce = camera_to_world * reflect( v, n );
	vec4 reflection = texture2D( tex_b, vec2( bounce.x, abs(bounce.y)) ) * material.a;

	// *** Specular
	vec4 specularLight = metalicity * incomingLight;

	// *** Beckman distribution function
	float alpha = acos(dot(n, h));
	float m = roughness; // Root-Mean-Square of microfacets
	float cosA_2 = cos(alpha) * cos(alpha);
	float t = (1.0 - cosA_2) /
									(cosA_2 * (m * m));

	float pi = 3.14159;
	float kSpec = exp(-t) / (pi * m * m * cosA_2 * cosA_2);
	vec4 specular = specularLight * kSpec;

	float diffuseK = max( 0.0, dot( -l, n ));
	vec4 diffuseLight = (1.0 - metalicity) * incomingLight;

	float reflF = fresnel(R0, v, n);
	vec4 fragColor = 
		ambient * ssao +
		(diffuseLight * albedo * diffuseK) + 
		specular * f +			// Direct lighting
		reflection * reflF; // Indirect lighting

	// TODO - additive fog? (in-scattering)
	float sun = sun( camera_space_sun_direction, v );
	float fogSun = sun_fog( camera_space_sun_direction, v );
	vec4 local_fog_color = fog_color + (sun_color * fogSun) + (sunwhite * sun);

	gl_FragColor = vec4( mix( fragColor, local_fog_color, fog ).xyz, 1.0);
}

