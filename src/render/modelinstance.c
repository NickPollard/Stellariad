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
#include "system/string.h"

#define NULL_INDEX UINTPTR_MAX

IMPLEMENT_POOL( modelInstance )

pool_modelInstance* static_modelInstance_pool = NULL;

void modelInstance_initPool() {
	static_modelInstance_pool = pool_modelInstance_create( 1024 );
}

modelInstance* modelInstance_createEmpty( ) {
	modelInstance* i = pool_modelInstance_allocate( static_modelInstance_pool );
	memset( i, 0, sizeof( modelInstance ));
	i->isStatic = false;
	return i;
}

void modelInstance_deleteSubRibbonEmitters( modelInstance* instance ) {
	for ( int i = 0; i < instance->ribbon_emitter_count; i++ ) {
		ribbonEmitter* e = instance->ribbon_emitters[i];
		ribbonEmitter_destroy( e );
	}
}

void modelInstance_deleteSubParticleEmitters( modelInstance* instance ) { 
	for ( int i = 0; i < instance->emitter_count; i++ ) {
		particleEmitter* e = instance->emitters[i];
		//printf( "Deleting subemitter.\n" );
		particleEmitter_destroy( e );
	}
}

void modelInstance_deleteSubTransforms( modelInstance* instance ) {
	for ( int i = 0; i < instance->transform_count; i++ ) {
		transform* t = instance->transforms[i];
		transform_delete( t );
	}
}

void modelInstance_delete( modelInstance* instance ) {
	// *** Delete sub-elements
	modelInstance_deleteSubRibbonEmitters( instance );
	modelInstance_deleteSubParticleEmitters( instance );
	modelInstance_deleteSubTransforms( instance );

	pool_modelInstance_free( static_modelInstance_pool, instance );
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
		instance->emitters[i] = particleEmitter_create();
#ifdef DEBUG_PARTICLE_SOURCES
		model* m = model_fromInstance( instance );
		char buffer[256];
		sprintf( buffer, "modelInstance subEmitter for \"%s\"", m->filename );
		instance->emitters[i]->debug_creator = string_createCopy( buffer );
#endif // DEBUG

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

void modelInstance_calculateBoundingBox( modelInstance* instance ) {
	model* m = model_fromInstance( instance );
	instance->bb = aabb_calculate( m->_obb, instance->trans->world );
}

void modelInstance_setStatic( modelInstance* m ) {
	m->isStatic = true;
	modelInstance_calculateBoundingBox( m );
}

void modelInstance_draw( modelInstance* instance, camera* cam ) {
	// Bounding box cull
	if (!instance->isStatic)
		modelInstance_calculateBoundingBox( instance );
	//debugdraw_aabb( instance->bb );

	if ( frustum_cull( &instance->bb, cam->frustum ) )
		return;

	render_resetModelView();
	matrix_mulInPlace( modelview, modelview, instance->trans->world );

	model_draw( model_fromInstance( instance ) );
}
