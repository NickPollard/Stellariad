//#version 110

#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D tex;
uniform vec4 screen_size;

varying vec2 texcoord;
varying vec4 ui_color;

const float aoFract = 0.250;
const float e = 0.00005;
const float max = 0.001;

vec4 ssao();

float occlusion( vec4 p, vec4 pp ) {
	float d = p.z - pp.z;
	if (d > max) return 0.0;
	return clamp(d / e, 0.0, 1.0) * aoFract;
}

void main() {
#if 0
	float delta = 4.0 / screen_size.x;
	vec4 here = texture2D( tex, texcoord );
	vec4 right = texture2D( tex, texcoord + vec2(delta, 0.0));
	vec4 left = texture2D( tex, texcoord - vec2(delta, 0.0));
	vec4 top = texture2D( tex, texcoord + vec2(0.0, delta));
	vec4 bottom = texture2D( tex, texcoord - vec2(0.0, delta));
	float ao = 0.0;
	ao += occlusion(here, right);
	ao += occlusion(here, left);
	ao += occlusion(here, top);
	ao += occlusion(here, bottom);
	//if (here.z - e < right.z ) a += 0.250;
	//if (here.z - e < left.z) a += 0.250;
	//if (here.z - e < top.z ) a += 0.250;
	//if (here.z - e < bottom.z) a += 0.250;

	//gl_FragColor = vec4(0.0, 0.0, 0.0, ao);
	gl_FragColor = vec4(1.0 - ao, 1.0 - ao, 1.0 - ao, 1.0 );
	//gl_FragColor = vec4(here.z - 0.5, 0.0, 0.0, 1.0);
	//gl_FragColor = vec4(0.5, 0.0, 0.0, 1.0);
	
#else
	gl_FragColor = ssao();
#endif
}

const float r = 0.5; // Max radius
const float u = 0.5; // occlusion constant
const float u2 = u * u;
const float samples = 4.0;
const float pi = 3.14159265358979;
// TODO - uniforms
const float near = -1.0;
const float far = -1800.0;

float heaviside( float f ) { return f > 0.0 ? 1.0 : 0.0; }

vec4 screenToWorld( vec2 p ) {
	float depth = texture2D( tex, p ).z;
	vec2 ndc;             // Reconstructed NDC-space position
	vec4 eye;             // Reconstructed EYE-space position

	float right = 0.0;
	float left = 0.0;
	float top = 0.0;
	float bottom = 0.0;

	eye.z = near * far / ((depth * (far - near)) - far);

	float widthInv = 1.0 / screen_size.x;
	float heightInv = 1.0 / screen_size.y;
	ndc.x = ((texcoord.x * widthInv) - 0.5) * 2.0;
	ndc.y = ((texcoord.y * heightInv) - 0.5) * 2.0;

	eye.x = ( (-ndc.x * eye.z) * (right-left)/(2.0*near) - eye.z * (right+left)/(2.0*near) );
	eye.y = ( (-ndc.y * eye.z) * (top-bottom)/(2.0*near) - eye.z * (top+bottom)/(2.0*near) );
	eye.w = 0.0;

	return eye;
}

float pointOcclusion( vec2 p, vec4 centre, vec4 centreNormal ) {
	vec4 world = screenToWorld( p );
	vec4 diff = world - centre;
	float d = dot( diff, diff );
	float d_ = min( sqrt(d), r );
	float proj = max(0.0, dot( diff, centreNormal ));
	float occlusion = heaviside(r - d) * proj / max( u2, d );
	return occlusion;
}

vec4 ssao() {
	vec4 centre = screenToWorld( texcoord );
	vec4 centreNormal = vec4( 0.0, 0.0, 1.0, 0.0 );
	float delta = 4.0 / screen_size.x;
	// for S pixels
	float a = pointOcclusion( texcoord + vec2( delta, 0.0), centre, centreNormal );
	float b = pointOcclusion( texcoord + vec2(-delta, 0.0), centre, centreNormal );
	float c = pointOcclusion( texcoord + vec2(0.0,  delta), centre, centreNormal );
	float d = pointOcclusion( texcoord + vec2(0.0, -delta), centre, centreNormal );
	//float f = 1.0 - (2.0 * pi * u / samples) * (a + b + c + d);
	float depth = texture2D( tex, texcoord ).z;
	float n = 1.0;
	float fa = 1800.0;
	float eyed = n * fa / ((depth * (fa - n)) - fa);
	float f = ((eyed / fa) * -1.0);
	return vec4(f, f, f, 1.0);
}
