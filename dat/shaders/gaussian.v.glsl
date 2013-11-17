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

void main() {
	vec2 screen_position = position.xy * vec2( 2.0/screen_size.x, 2.0/screen_size.y ) + vec2( -1.0, -1.0 );
	gl_Position = vec4( screen_position.xy, 0.0, 1.0 );
	texcoord = uv.xy;
	ui_color = color;
}
