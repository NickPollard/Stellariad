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
varying vec2 texcoord;
varying vec4 frag_position;
varying float height;

// Uniform
uniform	mat4 projection;
uniform	mat4 modelview;

void main() {
	gl_Position = projection * modelview * position;
	frag_position = modelview * position;
	texcoord = uv.xy;
	// height is relative to the world, not to the camera
	height = position.y;
}
