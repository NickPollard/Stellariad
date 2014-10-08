// light.c

#include "common.h"
#include "light.h"
//---------------------
#include "transform.h"
#include "mem/allocator.h"
#include "render/debugdraw.h"
#include "render/render.h"
#include <assert.h>

light* light_create() {
	light* l = (light*)mem_alloc(sizeof(light));
	light_setDiffuse(l, 1.f, 1.f, 1.f, 1.f);
	light_setSpecular(l, 0.f, 0.f, 0.f, 0.f);
	light_setAttenuation(l, 1.f, 1.f, 0.f);
	return l;
}

light* light_createWithTransform(scene* s) {
	light* l = light_create();
	transform* t = transform_createAndAdd( s );
	l->trans = t;
	return l;
}

void light_setAttenuation(light* l, float constant, float linear, float quadratic) {
	l->attenuationConstant = constant;
	l->attenuationLinear = linear;
	l->attenuationQuadratic = quadratic;
}

// Set the diffuse component for the light
void light_setDiffuse(light* l, float r, float g, float b, float a) {
	l->diffuse_color.val[0] = r;
	l->diffuse_color.val[1] = g;
	l->diffuse_color.val[2] = b;
	l->diffuse_color.val[3] = a;
	// TEST for now set specular as well
	light_setSpecular( l, r, g, b, a );
}

// Set the specular component for the light
void light_setSpecular( light* l, float r, float g, float b, float a ) {
	l->specular_color.val[0] = r;
	l->specular_color.val[1] = g;
	l->specular_color.val[2] = b;
	l->specular_color.val[3] = a;
}

void light_setPosition(light* l, vector* pos) {
	assert(l->trans != NULL);
	matrix_setTranslation(l->trans->local, pos);
}

// Render a batch of lights to the shader
// This sets up the uniform parameters for the lights in the shader
// Will setup <count> lights from the array pointed to by <lights>
void light_renderLights( int count, light** lights ) {
	int light_count = max( count, MAX_RENDER_LIGHTS );
	(void)light_count;
	(void)lights;
/*
	// the shader wants a struct of arrays, so we need to concatenate our
	// data into that form.
	// Extract the positions from each light into an array of positions
	vector positions[MAX_RENDER_LIGHTS];
	vector diffuses[MAX_RENDER_LIGHTS];
	vector speculars[MAX_RENDER_LIGHTS];
#if DEBUG_RENDER_LIGHTS
	printf( "Lighting - rendering %d lights to the shader, with positions:\n", light_count );
#endif
	for ( int i = 0; i < light_count; i++ ) {
		positions[i] = *matrix_getTranslation( lights[i]->trans->world );
		diffuses[i] = lights[i]->diffuse_color;
		speculars[i] = lights[i]->specular_color;
#if DEBUG_RENDER_LIGHTS
		printf( "\t" );
		vector_print( &positions[i] );
		printf( "\n" );
#endif
	}

	glUniform4fv( *resources.uniforms.light_position, light_count, (GLfloat*)positions );
	glUniform4fv( *resources.uniforms.light_diffuse, light_count, (GLfloat*)diffuses );
	glUniform4fv( *resources.uniforms.light_specular, light_count, (GLfloat*)speculars );
	*/
}
