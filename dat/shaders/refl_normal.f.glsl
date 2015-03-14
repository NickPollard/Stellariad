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

// Test Light values
const vec4 light_ambient = vec4( 0.1, 0.1, 0.2, 1.0 );
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










const vec4 incomingLight = vec4(1.0, 1.0, 1.0, 1.0);

/* Fresnel - Schlick's approximation
	 R = R0 + (1 - R0)(1 - v.h)^5
	 */
float fresnel(float R0, vec4 v, vec4 h) {
	float R = R0 + (1.0 - R0) * pow((1.0 - max(0.0, dot(-v, h))), 5.0);
	//float R = max(0.0, dot(normalize(v), normalize(h)));
	return R;
}

void main() {
	// light-invariant calculations
	vec4 view_direction = vec4( normalize( frag_position ).xyz, 0.0 );

	// Normalization HAS to happen at the FRAGMENT level; is considerably non-linear
	//vec4 c = projection * frag_position;
	//vec2 coord = (c.xy / c.w) * 0.5 + vec2(0.5, 0.5);

	//vec4 texture_normal = texture2D( tex_normal, texcoord ) * 2.0 - vec4(1.0, 1.0, 1.0, 0.0);
	vec4 texture_normal = vec4(0.0, 0.0, 1.0, 0.0);

	vec4 image_normal = vec4( texture_normal.xyz, 0.0 );

	vec4 binormal = vec4( 0.0, 1.0, 0.0, 0.0 );
	vec4 tangent = vec4( cross( binormal.xyz, frag_normal.xyz ), 0.0 );
	mat4 tangent_space = mat4( tangent, binormal, frag_normal, vec4( 0.0, 0.0, 0.0, 1.0 ) );
	vec4 normal = modelview * tangent_space * image_normal;
	//vec4 normal = frag_normal;

	// *** ssao
	vec4 ssaoVec = texture2D( ssao_tex, screenCoord );
	float ssao = ssaoVec.x;

	// *** Directional light
	// Ambient + Diffuse
	vec4 ambient = light_ambient;
	vec4 light_direction = normalize( worldspace * directional_light_direction ); // TODO - this could be a static passed in from C
	//float diffuseK = max( 0.0, dot( -light_direction, normal ));
//	vec4 diffuse = directional_light_diffuse * diffuseK;
//	vec4 total_light_color = (ambient + diffuse) * ssao;




	/*
		 Cook-Torrance specular micro-facet model

		 f(l,v) = D(h)F(v,h)G(v,h,l) / 4(n.l)(n.v)

		 where
		 D = Normal _D_istribution Function
		 F = _F_resnel term
		 V = _V_isibility term
		 (l = light vector, v = view vector, h = ?)
		 */

	vec4 v = normalize(view_direction);
	vec4 l = normalize(light_direction);
	vec4 h = normalize(-v + -l);
	vec4 n = normal;

	/*
		 Energy conservation - energy is either reflected (specular), or absorbed and reflected (diffuse)
		 Specular energy can be spread narrow (glossy) or wide (rough) - but total light energy is constant
		 */

	// ***Control parameters
	float roughness = 0.5; // microfacet distribution, 0.0 = perfectly smooth, 1.0 = uniformly distributed?
	float metalicity = 0.5; // How much energy is specularly reflected, vs diffuse. 1.0 = full metal, 0.0 = full insulator

	vec4 material = texture2D( tex, texcoord );
	//vec4 albedo = material * 0.2;
	vec4 albedo = vec4(0.5, 0.5, 0.5, 1.0);
	float R0 = metalicity * 0.5; // base fresnel reflectance (head-on)

	float f = fresnel(R0, v, h);

	// *** reflection
	vec4 bounce = camera_to_world * reflect( view_direction, normalize( normal ));
	vec4 reflection = texture2D( tex_b, vec2( bounce.x, abs(bounce.y)) ) * 1.0;//material.a;

	// *** Specular
	vec4 specularLight = metalicity * incomingLight;
//	float spec = max( 0.0, dot( reflect( light_direction, normal ), -view_direction ));
//	float specPower = 16.0 / (roughness + 1.0);

	// *** Beckman distribution function
	float alpha = acos(dot(n, h));
	float m = roughness; // Root-Mean-Square of microfacets
	float cosA_2 = cos(alpha) * cos(alpha);
	float t = (1.0 - cosA_2) /
									(cosA_2 * (m * m));

	float pi = 3.14159;
	float kSpec = exp(-t) / (pi * m * m * cosA_2 * cosA_2);
	vec4 specular = specularLight * kSpec;

	float diffuseK = max( 0.0, dot( -light_direction, normal ));
	vec4 diffuseLight = (1.0 - metalicity) * incomingLight;

	float reflF = fresnel(R0, v, n);
	//float ff = 1.0 - max(0.0, dot(-v, n));
	//float reflF = ff * ff * ff * ff * ff * ff;
	vec4 fragColor = 
		(diffuseLight * albedo * diffuseK) + 
		(specular) +
		 reflection * reflF;

	// TODO - additive fog? (in-scattering)
	float sun = sun( camera_space_sun_direction, view_direction );
	float fogSun = sun_fog( camera_space_sun_direction, view_direction );

	vec4 local_fog_color = fog_color + (sun_color * fogSun) + (sunwhite * sun);
	gl_FragColor = vec4( mix( fragColor, local_fog_color, fog ).xyz, 1.0);
}

