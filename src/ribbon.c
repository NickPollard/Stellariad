// ribbon.c
#include "src/common.h"
#include "src/ribbon.h"
//---------------------
#include "particle.h"
#include "transform.h"
#include "maths/matrix.h"
#include "mem/allocator.h"
#include "render/debugdraw.h"
#include "render/render.h"
#include "render/texture.h"
#include "script/sexpr.h"

GLushort*	ribbon_element_buffer;

ribbonEmitter* ribbonEmitter_create() {
	ribbonEmitter* r = mem_alloc( sizeof( ribbonEmitter ));
	memset( r, 0, sizeof( ribbonEmitter ));

	r->vertex_buffer = mem_alloc( sizeof( vertex ) * kMaxRibbonPairs * 2 );
	r->diffuse = texture_load( "dat/img/star_rgba64.tga" );

	r->begin	= Vector( 0.5f, 0.f, 0.f, 1.f );
	r->end		= Vector( -0.5f, 0.f, 0.f, 1.f );

	// Test
	r->color = property_create( 5 );
	property_addfv( r->color, 0.f, (float*)&color_green );
	property_addfv( r->color, 1.f, (float*)&color_red );

	r->billboard = true;
	r->radius = 0.5f;

	return r;
}

void ribbonEmitter_tick( void* emitter, float dt, engine* eng ) {
	(void)dt; (void)eng; (void)emitter;
	ribbonEmitter* r = emitter;

	// Emit a new ribbon vertex pair
	int vertex_last = ( r->pair_first + r->pair_count ) % kMaxRibbonPairs;
	vAssert( vertex_last < kMaxRibbonPairs && vertex_last >= 0 );

	if ( r->billboard ) {
		//r->vertex_array[vertex_last][0] = *transform_worldTranslation( r->trans );
		//r->vertex_array[vertex_last][1]	= *transform_worldTranslation( r->trans );
		vector v = Vector( 0.f, 0.f, 0.f, 1.f );
		r->vertex_array[vertex_last][0] = matrix_vecMul( r->trans->world, &v );
		r->vertex_array[vertex_last][1]	= matrix_vecMul( r->trans->world, &v );
	}
	else {
		r->vertex_array[vertex_last][0] = matrix_vecMul( r->trans->world, &r->begin );
		r->vertex_array[vertex_last][1]	= matrix_vecMul( r->trans->world, &r->end );
	}
	r->vertex_ages[vertex_last] = 0.f;

	if ( r->pair_count < kMaxRibbonPairs ) {
		++r->pair_count;
	}
	else {
		++r->pair_first;
	}
}

void ribbonEmitter_staticInit() {
	ribbon_element_buffer = mem_alloc( sizeof( GLushort ) * ( kMaxRibbonPairs - 1 ) * 12 );
	for ( int i = 0; i < kMaxRibbonPairs; ++i ) {
		if ( i > 0 ) {
			ribbon_element_buffer[i*12-12] = i * 2 - 2;
			ribbon_element_buffer[i*12-11] = i * 2 - 1;
			ribbon_element_buffer[i*12-10] = i * 2 + 0;
			ribbon_element_buffer[i*12-9] = i * 2 + 1;
			ribbon_element_buffer[i*12-8] = i * 2 + 0;
			ribbon_element_buffer[i*12-7] = i * 2 - 1;

			/*
			ribbon_element_buffer[i*12-6] = i * 2 - 1;
			ribbon_element_buffer[i*12-5] = i * 2 - 2;
			ribbon_element_buffer[i*12-4] = i * 2 + 0;
			ribbon_element_buffer[i*12-3] = i * 2 + 0;
			ribbon_element_buffer[i*12-2] = i * 2 + 1;
			ribbon_element_buffer[i*12-1] = i * 2 - 1;
			*/
		}
	}
}

void ribbonEmitter_render( void* emitter ) {
	ribbonEmitter* r = emitter;
	
	// Build render arrays
	r->render_pair_count = 0;

	/*
	float max_time = 1.f;
	for ( int i = 0; i < r->pair_count; ++i ) {
		int real_index = ( i + r->pair_first ) % kMaxRibbonPairs;
		r->vertex_ages[real_index] += dt;
		if ( r->vertex_ages[real_index] < max_time ) {
			++r->render_pair_count;
		}
	}
	*/
#if 1
	r->render_pair_count = r->pair_count;

	int j = 0;
	float v = 0.f;
	float v_delta = 1.f / (float)( r->render_pair_count - 1 );
	for ( int i = 0; i < r->pair_count; ++i ) {
		int real_index = ( i + r->pair_first ) % kMaxRibbonPairs;
		j = i;

		//if ( r->vertex_ages[real_index] < max_time ) {
		if ( !r->billboard ) {
			r->vertex_buffer[j*2+0].position = r->vertex_array[real_index][0];
		}
		r->vertex_buffer[j*2+0].uv = Vector( 0.f, v, 0.f, 1.f );
		r->vertex_buffer[j*2+0].color = property_samplev( r->color, v );
		//r->vertex_buffer[j*2+0].color = color_green;
		r->vertex_buffer[j*2+0].normal = Vector( 1.f, 1.f, 1.f, 1.f ); // Should be cross product

		if ( !r->billboard ) {
			r->vertex_buffer[j*2+1].position = r->vertex_array[real_index][1];
		}
		r->vertex_buffer[j*2+1].uv = Vector( 1.f, v, 0.f, 1.f );
		r->vertex_buffer[j*2+1].color = property_samplev( r->color, v );
		//r->vertex_buffer[j*2+1].color = color_green;
		r->vertex_buffer[j*2+1].normal = Vector( 1.f, 1.f, 1.f, 1.f ); // Should be cross product

		v += v_delta;
		//++j;
		/*
		   }
		   else {
		   continue;
		   }
		   */
	}
#endif

	if ( r->billboard ) {
		int j = 0;
		for ( int i = 0; i < r->pair_count; ++i ) {
			int this = ( i + r->pair_first ) % kMaxRibbonPairs;
			vector last_pos, current_pos;
			if ( i > 0 ) {
				int last = ( this + kMaxRibbonPairs - 1 ) % kMaxRibbonPairs;
				last_pos = r->vertex_array[last][0];
				current_pos = r->vertex_array[this][0];
			}
			else {
				int next = ( this + 1 ) % kMaxRibbonPairs;
				last_pos = r->vertex_array[this][0];
				current_pos = r->vertex_array[next][0];
			}

			vector view_dir = vector_sub( current_pos, *matrix_getTranslation( camera_mtx ));
			vector ribbon_dir = vector_sub( current_pos, last_pos );
			vector normal = normalized( vector_cross( view_dir, ribbon_dir ));
			r->vertex_buffer[i*2+0].position = vector_add( r->vertex_array[this][0], vector_scaled( normal, -r->radius ));
			r->vertex_buffer[i*2+1].position = vector_add( r->vertex_array[this][1], vector_scaled( normal, r->radius ));

			/*
			vector a = r->vertex_buffer[j*2+0].position;
			vector b = vector_add ( a, vector_scaled( normal, -r->radius ));
			debugdraw_line3d( a, b, color_blue );

			a = r->vertex_buffer[j*2+1].position;
			b = vector_add ( a, vector_scaled( normal, r->radius ));
			debugdraw_line3d( a, b, color_green );
			++j;
			*/
		}

		/*
		for ( int i = 1; i < kMaxRibbonPairs; ++i ) {
			//int real_index = ( i + r->pair_first ) % kMaxRibbonPairs;
			int j = i;
			vector a = r->vertex_buffer[j*2-2].position;
			vector b = r->vertex_buffer[j*2+0].position;
			debugdraw_line3d( a, b, color_red );
			a = r->vertex_buffer[j*2-1].position;
			b = r->vertex_buffer[j*2+1].position;
			debugdraw_line3d( a, b, color_red );
		}
		*/
	}

	// Reset modelview; our positions are in world space
	render_resetModelView();
	int index_count = ( r->render_pair_count - 1 ) * 12; // 6 if single-sided
	if ( r->diffuse->gl_tex && r->render_pair_count > 1 ) {
		drawCall* draw = drawCall_create( &renderPass_alpha, resources.shader_particle, index_count, ribbon_element_buffer, r->vertex_buffer, 
				r->diffuse->gl_tex, modelview );
		draw->depth_mask = GL_FALSE;
	}
}

void ribbonEmitter_setColor( ribbonEmitter* r, property* color ) {
	if ( r->color ) {
		property_delete( r->color );
	}
	r->color = color;
}

ribbonEmitter* ribbon_loadAsset( const char* filename ) {
	ribbonEmitter* emitter = sexpr_loadFile( filename );
	return emitter;
	/*
	int key = mhash( filename );
	// try to find it if it's already loaded
	void** result = map_find( ribbonEmitterAssets, key );
	if ( result ) {
		ribbonEmitter* emitter = *((ribbonEmitter**)results);
		// If the file has changed, we want to update this (live-reloading)
		if ( vfile_modifiedSinceLast( filename )) {
			// Load the new file
			// Save over the old
		}
		return emitter;
	}
	
	// otherwise load it and add it
	ribbonEmitter* emitter = sexpr_loadRibbonEmitter( filename );
	map_add( ribbonEmitterAssets, key, &emitter );
	return emitter;
	*/
}
