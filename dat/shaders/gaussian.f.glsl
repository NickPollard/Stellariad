// gaussian fragment shader

#ifdef GL_ES
precision mediump float;
#endif

varying vec2 texcoord;
varying vec4 ui_color;

uniform vec4 screen_size;
uniform sampler2D tex;

const float g1 = 0.55;
const float g2 = 0.32;
const float g3 = 0.12;

varying vec2 tb;
varying vec2 tc;
varying vec2 td;
varying vec2 te;
varying vec2 tf;
varying vec2 tg;

void main() {
#if 1
	float radius = 8.0;
	//float w = 1.5 * radius / screen_size.x;
	//float h = 1.5 * radius / screen_size.y;
	float m = 0.30;

	float p1 = 1.46 * radius / screen_size.x;
	float p2 = 3.40 * radius / screen_size.x;
	float p3 = 5.35 * radius / screen_size.x;

#if 1
	vec4 a = texture2D( tex, texcoord) * ui_color * (1.0 - (m*2.0));
	vec4 b = texture2D( tex, tb ) * ui_color * m * g1;
	vec4 c = texture2D( tex, tc ) * ui_color * m * g1;
	vec4 d = texture2D( tex, td ) * ui_color * m * g2;
	vec4 e = texture2D( tex, te ) * ui_color * m * g2;
	vec4 f = texture2D( tex, tf ) * ui_color * m * g3;
	vec4 g = texture2D( tex, tg ) * ui_color * m * g3;
#else
	vec4 a = texture2D( tex, texcoord ) * ui_color * (1.0 - (m*2.0));
	vec4 b = texture2D( tex, texcoord + vec2( p1, 0.0 )) * ui_color * m * g1;
	vec4 c = texture2D( tex, texcoord + vec2( -p1, 0 )) * ui_color * m * g1;
	vec4 d = texture2D( tex, texcoord + vec2( p2, 0.0 )) * ui_color * m * g2;
	vec4 e = texture2D( tex, texcoord + vec2( -p2, 0.0 )) * ui_color * m * g2;
	vec4 f = texture2D( tex, texcoord + vec2( p3, 0.0)) * ui_color * m * g3;
	vec4 g = texture2D( tex, texcoord + vec2( -p3, 0.0 )) * ui_color * m * g3;
#endif
	//vec4 d = texture2D( tex, texcoord + vec2( w, -h )) * ui_color * 0.075;
	//vec4 e = texture2D( tex, texcoord + vec2( -w, -h )) * ui_color * 0.075;
	gl_FragColor = a + b + c + d + e + f + g;
	//gl_FragColor = texture2D( tex, texcoord ) * ui_color;
#else
	gl_FragColor = texture2D( tex, texcoord ) * ui_color;
#endif
}
