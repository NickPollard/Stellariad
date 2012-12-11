// modelinstance.c

#include "common.h"
#include "modelinstance.h"
//-----------------------
#include "camera.h"
#include "particle.h"
#include "ribbon.h"
#include "transform.h"
#include "maths/vector.h"
#include "render/debugdraw.h"
#include "render/render.h"

#define NULL_INDEX UINTPTR_MAX

IMPLEMENT_POOL( modelInstance )

pool_modelInstance* static_modelInstance_pool = NULL;

void modelInstance_initPool() {
	static_modelInstance_pool = pool_modelInstance_create( 1024 );
}

modelInstance* modelInstance_createEmpty( ) {
	modelInstance* i = pool_modelInstance_allocate( static_modelInstance_pool );
	memset( i, 0, sizeof( modelInstance ));
	return i;
}

void modelInstance_delete( modelInstance* t ) {
	pool_modelInstance_free( static_modelInstance_pool, t );
}


void modelInstance_createSubTransforms( modelInstance* instance ) {
	model* m = model_fromInstance( instance );
	for ( int i = 0; i < m->transform_count; i++ ) {
		//printf( "Adding model sub transform.\n" );
		instance->transforms[i] = transform_create();
		matrix_cpy( instance->transforms[i]->local, m->transforms[i]->local );
	}
	instance->transform_count = m->transform_count;
}

void modelInstance_createSubParticleEmitters( modelInstance* instance ) {
	model* m = model_fromInstance( instance );
	for ( int i = 0; i < m->emitter_count; i++ ) {
		//printf( "Adding model particle emitter.\n" );
		instance->emitters[i] = particleEmitter_create();

		// TEST setup particle stuff
		// TODO cleanup particle/particleDef split (inc. creation)
		vAssert( m->emitters[i]->definition );
		instance->emitters[i]->definition = m->emitters[i]->definition;

		// Take parent transform if in model
		// This is stored as an index rather than a pointer
		uintptr_t transform_index = (uintptr_t)m->emitters[i]->trans;
		//printf( "transform_index = %d.\n", (int)transform_index );
		if ( transform_index == NULL_INDEX )
			instance->emitters[i]->trans = NULL;
		else
			instance->emitters[i]->trans = instance->transforms[transform_index];
	}
	instance->emitter_count = m->emitter_count;
}

void modelInstance_createSubRibbonEmitters( modelInstance* instance ) {
	model* m = model_fromInstance( instance );
	for ( int i = 0; i < m->ribbon_emitter_count; i++ ) {
		instance->ribbon_emitters[i] = ribbonEmitter_copy( m->ribbon_emitters[i]);

		/*
		instance->ribbon_emitters[i] = ribbonEmitter_create();
		ribbonEmitter_setColor( instance->ribbon_emitters[i], m->ribbon_emitters[i]->color );
		instance->ribbon_emitters[i]->diffuse = m->ribbon_emitters[i]->diffuse;
		*/

		// Take parent transform if in model
		// This is stored as an index rather than a pointer
		uintptr_t transform_index = (uintptr_t)m->ribbon_emitters[i]->trans;
		if ( transform_index == NULL_INDEX )
			instance->ribbon_emitters[i]->trans = NULL;
		else
			instance->ribbon_emitters[i]->trans = instance->transforms[transform_index];
	}
	instance->ribbon_emitter_count = m->ribbon_emitter_count;
}

modelInstance* modelInstance_create( modelHandle m ) {
	modelInstance* instance = modelInstance_createEmpty();
	instance->model = m;

	modelInstance_createSubTransforms( instance );
	modelInstance_createSubParticleEmitters( instance );
	modelInstance_createSubRibbonEmitters( instance );
	
	vAssert( !instance->trans );

	return instance;
}

aabb aabb_calculate( obb bb, matrix m ) {
	vector points[8];
	points[0] = bb.min;
	points[1] = bb.max;
	points[2] = Vector( bb.max.coord.x, bb.min.coord.y, bb.min.coord.z, 1.f );
	points[3] = Vector( bb.min.coord.x, bb.max.coord.y, bb.min.coord.z, 1.f );
	points[4] = Vector( bb.min.coord.x, bb.min.coord.y, bb.max.coord.z, 1.f );
	points[5] = Vector( bb.max.coord.x, bb.max.coord.y, bb.min.coord.z, 1.f );
	points[6] = Vector( bb.min.coord.x, bb.max.coord.y, bb.max.coord.z, 1.f );
	points[7] = Vector( bb.max.coord.x, bb.min.coord.y, bb.max.coord.z, 1.f );
	
	aabb aligned_bb;

	vector vert = points[0];
	if ( m )
		vert = matrix_vecMul( m, &points[0] );
	aligned_bb.min = vert;
	aligned_bb.max = vert;

	for ( int i = 1; i < 8; ++i ) {
		vert = points[i];
		if ( m )
			vert = matrix_vecMul( m, &points[i] );
		aligned_bb.min.coord.x = fminf( aligned_bb.min.coord.x, vert.coord.x );
		aligned_bb.min.coord.y = fminf( aligned_bb.min.coord.y, vert.coord.y );
		aligned_bb.min.coord.z = fminf( aligned_bb.min.coord.z, vert.coord.z );

		aligned_bb.max.coord.x = fmaxf( aligned_bb.max.coord.x, vert.coord.x );
		aligned_bb.max.coord.y = fmaxf( aligned_bb.max.coord.y, vert.coord.y );
		aligned_bb.max.coord.z = fmaxf( aligned_bb.max.coord.z, vert.coord.z );
	}
	return aligned_bb;
}
	/*
aabb aabb_calculate( int vert_count, vector* verts, matrix m ) {
	vAssert( vert_count > 1 );
	aabb bb;
	vector vert = verts[0];
	if ( m )
		vert = matrix_vecMul( m, &verts[0] );
	bb.min = vert;
	bb.max = vert;
	for ( int i = 1; i < vert_count; i++ ) {
		vert = verts[i];
		if ( m )
			vert = matrix_vecMul( m, &verts[i] );

   	//INLINED:
		//bb.min = vector_min( &bb.min, &vert );
		//bb.max = vector_max( &bb.max, &vert );
		
		bb.min.coord.x = fminf( bb.min.coord.x, vert.coord.x );
		bb.min.coord.y = fminf( bb.min.coord.y, vert.coord.y );
		bb.min.coord.z = fminf( bb.min.coord.z, vert.coord.z );
		
		bb.max.coord.x = fmaxf( bb.max.coord.x, vert.coord.x );
		bb.max.coord.y = fmaxf( bb.max.coord.y, vert.coord.y );
		bb.max.coord.z = fmaxf( bb.max.coord.z, vert.coord.z );
	}
	return bb;
}
	*/

void test_aabb_calculate( ) {
/*
	vector verts[] = {
		{{ 1.f, 0.f, -1.f, 1.f }},
		{{ -1.f, 1.f, 0.f, 1.f }},
		{{ 0.f, -1.f, 1.f, 1.f }}
	};
	aabb bb = aabb_calculate( 3, verts, NULL );
#if 0
	printf( "AABB: ( " );
	vector_print( &bb.min );
	printf( " ) to ( " );
	vector_print( &bb.max );
	printf( ")\n" );
#endif
	vector v_min = Vector( -1.f, -1.f, -1.f, 1.f );
	vector v_max = Vector( 1.f, 1.f, 1.f, 1.f );
	vAssert( vector_equal( &bb.min, &v_min ));
	vAssert( vector_equal( &bb.max, &v_max ));
*/
}

void modelInstance_calculateBoundingBox( modelInstance* instance ) {
	model* m = model_fromInstance( instance );
	instance->bb = aabb_calculate( m->obb, instance->trans->world );
#if 0
	printf( "AABB: ( " );
	vector_print( &bb.min );
	printf( " ) to ( " );
	vector_print( &bb.max );
	printf( ")\n" );
#endif
}

void aabb_expand( aabb* bb, vector* points ) {
	points[0] = bb->min;
	points[1] = bb->max;
	points[2] = Vector( bb->max.coord.x, bb->min.coord.y, bb->min.coord.z, 1.f );
	points[3] = Vector( bb->min.coord.x, bb->max.coord.y, bb->min.coord.z, 1.f );
	points[4] = Vector( bb->min.coord.x, bb->min.coord.y, bb->max.coord.z, 1.f );
	points[5] = Vector( bb->max.coord.x, bb->max.coord.y, bb->min.coord.z, 1.f );
	points[6] = Vector( bb->min.coord.x, bb->max.coord.y, bb->max.coord.z, 1.f );
	points[7] = Vector( bb->max.coord.x, bb->min.coord.y, bb->max.coord.z, 1.f );
}

bool plane_cull( aabb* bb, vector* plane ) {
	vector points[8];
	aabb_expand( bb, points );
	for ( int i = 0; i < 6; i++ ) {
		if ( Dot( &points[i], plane ) > plane->coord.w )
			return false;
	}
	return true;
}

// Cull the AABB *bb* against the frustum defined as 6 planes in *frustum*
bool frustum_cull( aabb* bb, vector* frustum ) {
	if ( plane_cull( bb, &frustum[0] )) {
		return true;
	}
	/*
	if ( plane_cull( bb, &frustum[1] ))
		return true;
		*/
	return false;
}

void debugdraw_aabb( aabb bb ) {
	vector points[8];
	aabb_expand( &bb, points );
	debugdraw_line3d( points[0], points[3], color_green );
	debugdraw_line3d( points[0], points[4], color_green );
	debugdraw_line3d( points[3], points[6], color_green );
	debugdraw_line3d( points[4], points[6], color_green );

	debugdraw_line3d( points[1], points[5], color_green );
	debugdraw_line3d( points[1], points[7], color_green );
	debugdraw_line3d( points[5], points[2], color_green );
	debugdraw_line3d( points[7], points[2], color_green );

	debugdraw_line3d( points[0], points[2], color_green );
	debugdraw_line3d( points[1], points[6], color_green );
	debugdraw_line3d( points[3], points[5], color_green );
	debugdraw_line3d( points[4], points[7], color_green );
}

void modelInstance_draw( modelInstance* instance, camera* cam ) {
	// Bounding box cull
	modelInstance_calculateBoundingBox( instance );
	//debugdraw_aabb( instance->bb );

	vector frustum[6];
	camera_calculateFrustum( cam, frustum );
	if ( frustum_cull( &instance->bb, frustum ) )
		return;

	render_resetModelView();
	matrix_mul( modelview, modelview, instance->trans->world );

	model_draw( model_fromInstance( instance ) );
}
