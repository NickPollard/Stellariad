// gaussian vertex shader
#ifdef GL_ES
precision mediump float;
#endif

attribute vec4 position;

// Need these for shader to work, even though they're unused, as we're currently setting them through macros
attribute vec4 uv;
attribute vec4 normal;
attribute vec4 color;

uniform vec4 screen_size;

varying vec2 texcoord;
varying vec4 ui_color;

varying vec2 tb;
varying vec2 tc;
varying vec2 td;
varying vec2 te;
varying vec2 tf;
varying vec2 tg;

void main() {
	vec2 screen_position = position.xy * vec2( 2.0/screen_size.x, 2.0/screen_size.y ) + vec2( -1.0, -1.0 );
	gl_Position = vec4( screen_position.xy, 0.0, 1.0 );
	texcoord = uv.xy;
	ui_color = color;

	float radius = 2.0;
	float m = 0.30;

	float p1 = 1.46 * radius / screen_size.y;
	float p2 = 3.40 * radius / screen_size.y;
	float p3 = 5.35 * radius / screen_size.y;

#if 1
	tb = texcoord + vec2( 0.0, p1 );
	tc = texcoord + vec2( 0.0, -p1);
	td = texcoord + vec2( 0.0, p2 );
	te = texcoord + vec2( 0.0, -p2);
	tf = texcoord + vec2( 0.0, p3 );
	tg = texcoord + vec2( 0.0, -p3);
#endif
}
