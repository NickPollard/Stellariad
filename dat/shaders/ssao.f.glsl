//#version 110

#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D tex;
uniform vec4 screen_size;

varying vec2 texcoord;
varying vec4 ui_color;

const float pi = 3.14159265358979;

//const float r = 1.0; // Max radius
const float e = 0.5;
const float u = 1.0; // occlusion constant
const float ssaoRadius = 4.0;
const float samples = 8.0;
const float beta = 0.0000;
const float distScale = 5.2;
const float b = 0.0000025;

// TODO - uniforms
const float near = -1.0;
const float far = -1800.0;
const float fov = 0.8;// radians
const float aspect = 3.0/4.0;

vec4 ssao();

/*
float occlusion( vec4 p, vec4 pp ) {
	float d = p.z - pp.z;
	if (d > max) return 0.0;
	return clamp(d / e, 0.0, 1.0) * aoFract;
}
*/

void main() {
	gl_FragColor = ssao();
}
// Unused
float heaviside( float f ) { return f > 0.0 ? 1.0 : 0.0; }

vec4 screenToWorld( vec2 p ) {
	float depth = texture2D( tex, p ).z;
	vec2 ndc;             // Reconstructed NDC-space position
	vec4 eye;             // Reconstructed EYE-space position

	float right = near / atan( fov );
	float left = right;
	float top = right * aspect;
	float bottom = -top;

	eye.z = near * far / ((depth * (far - near)) - far);

	float widthInv = 1.0 / screen_size.x;
	float heightInv = 1.0 / screen_size.y;
	ndc.x = ((p.x ) - 0.5) * 2.0;
	ndc.y = ((p.y ) - 0.5) * 2.0;

	// Can simplify when symmetric perspective
	eye.x = -ndc.x * eye.z * right/near;
	eye.y = -ndc.y * eye.z * top/near;
	eye.w = 0.0;

	return eye;
}

float pointOcclusion( vec2 p, vec4 centre, vec4 centreNormal ) {
	vec4 world = screenToWorld( p );
	vec4 diff = world - centre;
	float d = dot( diff, diff ) * distScale;
	float proj = max(0.0, dot( diff, centreNormal ) + (1000.0 - world.z) * b) + beta * world.z;
	// Don't need heaviside; fall-off effectively covers this
	float occlusion = proj / ( d + e );
	return occlusion;
}

vec4 vecToColor( vec4 v ) {
	return v * 0.5 + vec4( 0.5, 0.5, 0.5, 0.0 );
}

vec4 ssao() {
	vec4 centre = screenToWorld( texcoord );
	float normalDelta = 8.0 / screen_size.x;
	vec4 above = screenToWorld( texcoord + vec2( 0.0, normalDelta ));
	vec4 right = screenToWorld( texcoord + vec2( normalDelta, 0.0 ));
	vec4 v = right - centre;
	vec4 v2 = above - centre;

	float delta = ssaoRadius / screen_size.x;
	vec4 centreNormal = normalize(vec4(cross( v2.xyz, v.xyz ), 0.0));

	// for S pixels
	float a = pointOcclusion( texcoord + vec2( delta, 0.0), centre, centreNormal );
	float b = pointOcclusion( texcoord + vec2(-delta, 0.0), centre, centreNormal );
	float c = pointOcclusion( texcoord + vec2(0.0,  delta), centre, centreNormal );
	float d = pointOcclusion( texcoord + vec2(0.0, -delta), centre, centreNormal );

	float a2 = pointOcclusion( texcoord + vec2( delta,  delta), centre, centreNormal );
	float b2 = pointOcclusion( texcoord + vec2(-delta,  delta), centre, centreNormal );
	float c2 = pointOcclusion( texcoord + vec2(-delta, -delta), centre, centreNormal );
	float d2 = pointOcclusion( texcoord + vec2( delta, -delta), centre, centreNormal );

	float f = 1.0 - (2.0 * pi * u / samples) * (a + b + c + d + a2 + b2 + c2 + d2);

//	f = 1.0;
	return vec4( f, f, f, 1.0 );
}
