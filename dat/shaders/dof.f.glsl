// UI fragment shader

#ifdef GL_ES
precision mediump float;
#endif

varying vec2 texcoord;

uniform sampler2D tex;
uniform sampler2D tex_b;

uniform vec4 screen_size;

const float aoFract = 0.250;
const float e = 0.5;
const float max = 0.001;

const float r = 200.0; // Max radius
const float u = 1.5; // occlusion constant
const float samples = 8.0;
const float pi = 3.14159265358979;

// TODO - uniforms
const float near = -1.0;
const float far = -1800.0;
const float fov = 0.8;// radians
const float aspect = 3.0/4.0;

vec4 screenToWorld( vec2 p ) {
	float depth = texture2D( tex_b, p ).z;
	vec2 ndc;             // Reconstructed NDC-space position
	vec4 eye;             // Reconstructed EYE-space position

	float right = near / atan( fov );
	float left = right;
	float top = right * aspect;
	float bottom = -top;

	eye.z = near * far / ((depth * (far - near)) - far);
//	eye.z = (2.0 * near) / (far + near - depth * (far - near));	

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

void main() {
	//float depth = screenToWorld( texcoord ).z / 1000.f;
	//float depth = 0.0;
	//gl_FragColor = texture2D( tex_b, texcoord ) * vec4(0.5, 0.5, 0.5, 0.5);
	float near = 0.05;
	float far = 0.7;
	float maxBlur = 0.99;
	//float bl = clamp((depth - near) / ( far - near ), 0.0, 1.0) * maxBlur;
	//float mask = depth > near ? 1.0 : 0.0;
	//gl_FragColor = vec4(bl, 0.0, 0.0, 1.0);
	vec4 color = texture2D( tex, texcoord );
	float bloomFactor = 4.0;
	float minBloom = 0.9;
	float bloomAmount = clamp(((( color.x + color.y + color.z ) / 3.0) - minBloom) / (1.0 - minBloom), 0.0, 1.0);
	float bloom = pow( bloomAmount, bloomFactor);
	float blur = 0.3;
	gl_FragColor = color * 
		vec4(blur + bloom,
					blur + bloom,
		 			blur + bloom,
		 			blur);
}
