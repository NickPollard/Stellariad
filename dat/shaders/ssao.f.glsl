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

float occlusion( vec4 p, vec4 pp ) {
	float d = p.z - pp.z;
	if (d > max) return 0.0;
	//if (p.z - e < pp.z ) 
	return clamp(d / e, 0.0, 1.0) * aoFract;
		/*
	if ( d - e > 0.0 ) // if neighbouring pixel is closer
		return aoFract;
	else 
		return 0.0;
		*/
}

void main() {
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
}
