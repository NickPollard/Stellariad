// ribbon.c
#include "src/common.h"
#include "src/ribbon.h"
//---------------------
#include "particle.h"
#include "transform.h"
#include "maths/matrix.h"
#include "mem/allocator.h"
#include "render/drawcall.h"
#include "render/debugdraw.h"
#include "render/render.h"
#include "render/shader.h"
#include "render/texture.h"
#include "script/sexpr.h"
#include "system/file.h"
#include "system/hash.h"

#define kMaxRibbonEmitters 1024

map* ribbonEmitterAssets = NULL;
#define kMaxRibbonAssets 256

GLushort*	ribbon_element_buffer = NULL;

// *** Object pool
DECLARE_POOL( ribbonEmitter );
IMPLEMENT_POOL( ribbonEmitter );
pool_ribbonEmitter* static_ribbon_pool = NULL;

void ribbon_initPool() {
	static_ribbon_pool = pool_ribbonEmitter_create( kMaxRibbonEmitters );
}

ribbonEmitterDef* ribbonEmitterDef_create() {
	ribbonEmitterDef* r = (ribbonEmitterDef*)mem_alloc( sizeof( ribbonEmitterDef ));
	memset( r, 0, sizeof( ribbonEmitterDef ));
	r->billboard = true;
	r->radius = 0.5f;
	r->lifetime = 1.f;
	r->begin	= Vector( 0.5f, 0.f, 0.f, 1.f );
	r->end		= Vector( -0.5f, 0.f, 0.f, 1.f );
	r->diffuse	= static_texture_default;
	r->static_texture = false;	

	// Test
	r->color = property_create( 5 );
	property_addfv( r->color, 0.f, (float*)&color_green );
	property_addfv( r->color, 1.f, (float*)&color_red );
	return r;
}

void ribbonEmitterDef_deInit( ribbonEmitterDef* r ) {
	property_delete( r->color );
}

ribbonEmitter* ribbonEmitter_create( ribbonEmitterDef* def ) {
	ribbonEmitter* r = pool_ribbonEmitter_allocate( static_ribbon_pool );
	memset( r, 0, sizeof( ribbonEmitter ));
	for ( int i = 0; i < kMaxRibbonPairs * 2; i+=2 ) {
		r->vertex_buffer[i].uv = Vec2( 0.f, 0.f );
		r->vertex_buffer[i+1].uv = Vec2( 1.f, 0.f );
	}
	r->next_tex_v = 0.f;

	// Definition
	r->definition = def;
	return r;
}

void ribbonEmitter_tickTimes( ribbonEmitter* r, float dt ) {
	for ( int i = 0; i < r->pair_count; ++i ) {
		int real_index = ( i + r->pair_first ) % kMaxRibbonPairs;
		r->vertex_ages[real_index] += dt;
	}
}

void ribbonEmitter_tick( void* emitter, float dt, engine* eng ) {
	ribbonEmitter* r = (ribbonEmitter*)emitter;
	if ( !eng->paused ) {
		// Emit a new ribbon vertex pair
		int vertex_last = ( r->pair_first + r->pair_count ) % kMaxRibbonPairs;
		vAssert( vertex_last < kMaxRibbonPairs && vertex_last >= 0 );

		if ( r->definition->billboard ) {
			r->vertex_array[vertex_last][0] = *transform_worldTranslation( r->trans );
			r->vertex_array[vertex_last][1]	= *transform_worldTranslation( r->trans );
		}
		else {
			r->vertex_array[vertex_last][0] = matrix_vecMul( r->trans->world, &r->definition->begin );
			r->vertex_array[vertex_last][1]	= matrix_vecMul( r->trans->world, &r->definition->end );
		}
		r->vertex_ages[vertex_last] = 0.f;
		r->next_tex_v += ( dt / r->definition->lifetime );
		r->tex_v[vertex_last] = r->next_tex_v;

		if ( r->pair_count < kMaxRibbonPairs ) {
			++r->pair_count;
		}
		else {
			++r->pair_first;
		}

		ribbonEmitter_tickTimes( r, dt );
	}
}

void ribbonEmitter_staticInit() {
	ribbon_initPool();
	ribbon_element_buffer = (GLushort*)mem_alloc( sizeof( GLushort ) * ( kMaxRibbonPairs - 1 ) * 6 );
	for ( int i = 1; i < kMaxRibbonPairs; ++i ) {
		ribbon_element_buffer[i*6-6] = i * 2 - 2;
		ribbon_element_buffer[i*6-5] = i * 2 - 1;
		ribbon_element_buffer[i*6-4] = i * 2 + 0;
		ribbon_element_buffer[i*6-3] = i * 2 + 1;
		ribbon_element_buffer[i*6-2] = i * 2 + 0;
		ribbon_element_buffer[i*6-1] = i * 2 - 1;
	}

	ribbonEmitterAssets = map_create( kMaxRibbonAssets, sizeof( ribbonEmitterDef* ));
}

void ribbonEmitter_render( void* emitter, scene* s ) {
	(void)s;
	ribbonEmitter* r = (ribbonEmitter*)emitter;
	
	// Build render arrays
	int render_pair_count = 0;

	for ( int i = 0; i < r->pair_count; ++i ) {
		for ( ; i < r->pair_count && r->vertex_ages[(i+r->pair_first) % kMaxRibbonPairs] >= r->definition->lifetime; ++i ) { }
		const int ths = ( i + r->pair_first ) % kMaxRibbonPairs;
		if ( r->vertex_ages[ths] < r->definition->lifetime ) ++render_pair_count; else break;
	}

	const vector* camera = matrix_getTranslation( camera_mtx );

	int j = 0;
	float v = 0.f;
	float v_delta = 1.f / (float)( render_pair_count - 1 );
	for ( int i = 0; i < r->pair_count; ++i ) {
		for ( ; i < r->pair_count && r->vertex_ages[(i+r->pair_first) % kMaxRibbonPairs] >= r->definition->lifetime; ++i ) { }
		int ths = ( i + r->pair_first ) % kMaxRibbonPairs;
		const int left = j * 2;
		const int right = left + 1;

		if ( r->vertex_ages[ths] < r->definition->lifetime ) {
			r->vertex_buffer[left].uv.y = ( r->definition->static_texture ? r->tex_v[ths] : v );
			r->vertex_buffer[left].color = intFromVector(property_samplev( r->definition->color, v ));
			r->vertex_buffer[right].uv.y = r->vertex_buffer[left].uv.y;
			r->vertex_buffer[right].color = r->vertex_buffer[j*2+0].color;

			if ( !r->definition->billboard ) {
				r->vertex_buffer[left].position = r->vertex_array[ths][0];
				r->vertex_buffer[right].position = r->vertex_array[ths][1];
			} else {
				vector last_pos, current_pos;
				if ( i > 0 ) {
					int last = ( ths + kMaxRibbonPairs - 1 ) % kMaxRibbonPairs;
					last_pos = r->vertex_array[last][0];
					current_pos = r->vertex_array[ths][0];
				}
				else {
					int next = ( ths + 1 ) % kMaxRibbonPairs;
					last_pos = r->vertex_array[ths][0];
					current_pos = r->vertex_array[next][0];
				}

				// TODO - could do one for the whole ribbon
				vector view_dir = vector_sub( current_pos, *camera);
				// TODO - could do this in tick and cache?
				vector ribbon_dir = vector_sub( current_pos, last_pos );
				// Provide a sensible default in case the ribbon_dir is 0
				if ( vector_lengthSq( &ribbon_dir ) <= 0.f ) ribbon_dir = z_axis;
				const vector normal = vector_cross( view_dir, ribbon_dir );
				float normalization = 1.f / vector_length( &normal );
				const vector offset = vector_scaled( normal, normalization * r->definition->radius );
				r->vertex_buffer[j*2+0].position = vector_sub( r->vertex_array[ths][0], offset );
				r->vertex_buffer[j*2+1].position = vector_add( r->vertex_array[ths][1], offset );
			}

			v += v_delta;
			++j;
		}
		else break;
	}
	vAssert( j == render_pair_count );
	if ( r->definition->diffuse->gl_tex && render_pair_count > 1 ) {
		// Reset modelview; our positions are in world space
		render_resetModelView();
		int index_count = ( render_pair_count - 1 ) * 6; // 12 if double-sided
		drawCall::create( &renderPass_alpha, *Shader::byName( "dat/shaders/particle.s" ), index_count, ribbon_element_buffer, r->vertex_buffer, 
				r->definition->diffuse->gl_tex, modelview )->depth(false);
	}
}

void ribbonEmitterDef_setColor( ribbonEmitterDef* r, property* color ) {
	if ( r->color ) {
		property_delete( r->color );
	}
	r->color = color;
}

ribbonEmitterDef* ribbon_loadAsset( const char* filename ) {
	int key = mhash( filename );
	// try to find it if it's already loaded
	void** result = (void**)map_find( ribbonEmitterAssets, key );
	if ( result ) {
		ribbonEmitterDef* def = *((ribbonEmitterDef**)result);
		// If the file has changed, we want to update ths (live-reloading)
		if ( vfile_modifiedSinceLast( filename )) {
			// Load the new file
			// Save over the old
			ribbonEmitterDef* neo = (ribbonEmitterDef*)sexpr_loadFile( filename );
			ribbonEmitterDef_deInit( def );
			*def = *neo;
		}
		return def;
	}
	
	// otherwise load it and add it
	ribbonEmitterDef* def = (ribbonEmitterDef*)sexpr_loadFile( filename );
	map_add( ribbonEmitterAssets, key, &def );
	return def;
}

ribbonEmitter* ribbonEmitter_copy( ribbonEmitter* src ) {
	return ribbonEmitter_create( src->definition );
}

void ribbonEmitter_destroy( ribbonEmitter* r ) {
	vAssert( r );
	pool_ribbonEmitter_free( static_ribbon_pool, r );
}
