//#version 110

#ifdef GL_ES
precision mediump float;
#endif

// Phong Vertex Shader
// Attributes
attribute vec4 position;
attribute vec4 normal;
attribute vec4 uv;
attribute vec4 color;

// Varying
varying vec4 frag_color;

// Uniform
uniform vec4 screen_size; // = vec2( 1280.0, 720.0 );

void main() {
	vec2 screen_position = position.xy * vec2( 2.0/screen_size.x, 2.0/screen_size.y ) + vec2( -1.0, -1.0 );
	gl_Position = vec4( screen_position.xy, 0.0, 1.0 );
	frag_color = color;
}
