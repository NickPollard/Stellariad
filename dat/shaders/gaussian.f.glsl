// gaussian fragment shader

#ifdef GL_ES
precision mediump float;
#endif

varying vec2 texcoord;
varying vec4 ui_color;

uniform vec4 screen_size;
uniform sampler2D tex;

void main() {
#if 1
	float radius = 1.0;
	float w = 1.5 * radius / screen_size.x;
	float h = 1.5 * radius / screen_size.y;
	vec4 a = texture2D( tex, texcoord ) * ui_color * 0.7;
	vec4 b = texture2D( tex, texcoord + vec2( w, h)) * ui_color * 0.075;
	vec4 c = texture2D( tex, texcoord + vec2( -w, h )) * ui_color * 0.075;
	vec4 d = texture2D( tex, texcoord + vec2( w, -h )) * ui_color * 0.075;
	vec4 e = texture2D( tex, texcoord + vec2( -w, -h )) * ui_color * 0.075;
	gl_FragColor = a + b + c + d + e;
#else
	gl_FragColor = texture2D( tex, texcoord ) * ui_color;
#endif
}
