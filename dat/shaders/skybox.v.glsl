//#version 110
// Phong Vertex Shader
// Attributes
attribute vec4 position;
attribute vec4 normal;
attribute vec4 uv;

// Varying
varying vec4 frag_position;
varying vec2 texcoord;

// Uniform
uniform	mat4 projection;
uniform	mat4 modelview;

void main() {
	gl_Position = projection * modelview * position;
	frag_position = position;
	texcoord = uv.xy;
}