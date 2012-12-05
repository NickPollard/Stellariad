// ribbon.c
#include "src/common.h"
#include "src/ribbon.h"
//---------------------
#include "transform.h"
#include "maths/matrix.h"
#include "mem/allocator.h"
#include "render/render.h"
#include "render/texture.h"

ribbonEmitter* ribbonEmitter_create() {
	ribbonEmitter* r = mem_alloc( sizeof( ribbonEmitter ));
	memset( r, 0, sizeof( ribbonEmitter ));

	r->vertex_buffer = mem_alloc( sizeof( vertex ) * kMaxRibbonPairs * 2 );
	r->element_buffer = mem_alloc( sizeof( GLushort ) * ( kMaxRibbonPairs - 1 ) * 12 );

	r->diffuse = texture_load( "dat/img/star_rgba64.tga" );
	//r->diffuse = texture_load( "dat/img/vitae_sky2_export_flattened.tga" );

	r->pair_count = 0;

	r->begin	= Vector( 0.5f, 0.f, 0.f, 1.f );
	r->end		= Vector( -0.5f, 0.f, 0.f, 1.f );

	return r;
}

void ribbonEmitter_tick( void* emitter_void, float dt, engine* eng ) {
	(void)dt; (void)eng;
	ribbonEmitter* r = emitter_void;

	// Emit a new ribbon vertex pair
	int vertex_last = ( r->pair_first + r->pair_count ) % kMaxRibbonPairs;
	vAssert( vertex_last < kMaxRibbonPairs && vertex_last >= 0 );

	r->vertex_array[vertex_last][0] = matrix_vecMul( r->trans->world, &r->begin );
	r->vertex_array[vertex_last][1]	= matrix_vecMul( r->trans->world, &r->end );

	if ( r->pair_count < kMaxRibbonPairs ) {
		++r->pair_count;
	}
	else {
		++r->pair_first;
	}

	// Build render arrays
	float v_delta = 1.f / (float)( r->pair_count - 1 );
	for ( int i = 0; i < r->pair_count; ++i ) {
		int real_index = ( i + r->pair_first ) % kMaxRibbonPairs;
		r->vertex_buffer[i*2+0].position = r->vertex_array[real_index][0];
		r->vertex_buffer[i*2+0].uv = Vector( 0.f, (float)i * v_delta, 0.f, 1.f );
		//r->vertex_buffer[i*2+0].uv = Vector( 0.f, (float)real_index * v_delta, 0.f, 1.f );
		r->vertex_buffer[i*2+0].color = Vector( 1.f, 1.f, 1.f, 1.f );
		r->vertex_buffer[i*2+0].normal = Vector( 1.f, 1.f, 1.f, 1.f ); // Should be cross product
		r->vertex_buffer[i*2+1].position = r->vertex_array[real_index][1];
		r->vertex_buffer[i*2+1].uv = Vector( 1.f, (float)i * v_delta, 0.f, 1.f );
		//r->vertex_buffer[i*2+1].uv = Vector( 1.f, (float)real_index * v_delta, 0.f, 1.f );
		r->vertex_buffer[i*2+1].color = Vector( 1.f, 1.f, 1.f, 1.f );
		r->vertex_buffer[i*2+1].normal = Vector( 1.f, 1.f, 1.f, 1.f ); // Should be cross product
		if ( i > 0 ) {
			r->element_buffer[i*12-12] = i * 2 - 2;
			r->element_buffer[i*12-11] = i * 2 - 1;
			r->element_buffer[i*12-10] = i * 2 + 0;
			r->element_buffer[i*12-9] = i * 2 + 1;
			r->element_buffer[i*12-8] = i * 2 + 0;
			r->element_buffer[i*12-7] = i * 2 - 1;

			r->element_buffer[i*12-6] = i * 2 - 1;
			r->element_buffer[i*12-5] = i * 2 - 2;
			r->element_buffer[i*12-4] = i * 2 + 0;
			r->element_buffer[i*12-3] = i * 2 + 0;
			r->element_buffer[i*12-2] = i * 2 + 1;
			r->element_buffer[i*12-1] = i * 2 - 1;
		}
	}
}

void ribbonEmitter_render( void* emitter_void ) {
	ribbonEmitter* r = emitter_void;
	(void)r;

	// Reset modelview; our positions are in world space
	render_resetModelView();
	int index_count = ( r->pair_count - 1 ) * 12; // 6 if single-sided
	if ( r->diffuse->gl_tex && r->pair_count > 1 ) {
		drawCall* draw = drawCall_create( &renderPass_alpha, resources.shader_particle, index_count, r->element_buffer, r->vertex_buffer, 
				r->diffuse->gl_tex, modelview );
		draw->depth_mask = GL_FALSE;
	}
}
