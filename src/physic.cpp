// physic.c
#include "src/common.h"
#include "src/physic.h"
//---------------------
#include "engine.h"
#include "transform.h"
#include "maths/vector.h"

#ifdef DEBUG_PHYSIC_LIVENESS_TEST
physic* active_physics[1024];
int active_physic_count;
#endif // DEBUG_PHYSIC_LIVENESS_TEST

physic* physic_create()  {
	physic* p = (physic*)mem_alloc( sizeof( physic ));
	memset( p, 0, sizeof( physic ));
	p->velocity = Vector( 0.f, 0.f, 0.f, 0.f );
	p->mass = 0.f;
	p->to_delete = false;

#ifdef DEBUG_PHYSIC_LIVENESS_TEST
	array_add( (void**)active_physics, &active_physic_count, (void*)p );
#endif // DEBUG_PHYSIC_LIVENESS_TEST

	return p;
}

void physic_delete( physic* p ) {
	p->to_delete = true;
}

void physic_tick( void* data, float dt, engine* eng ) {
	physic* p = (physic*)data;
	
#ifdef DEBUG_PHYSIC_LIVENESS_TEST
	physic_assertActive( p );
#endif // DEBUG_PHYSIC_LIVENESS_TEST
	vAssert( p->trans );

	// If requested to delete
	if ( p->to_delete ) {
		mem_free( p );
		stopTick( eng, p, physic_tick );
#ifdef DEBUG_PHYSIC_LIVENESS_TEST
		array_remove( (void**)active_physics, &active_physic_count, (void*)p );
#endif // DEBUG_PHYSIC_LIVENESS_TEST
		return;
	}

	vector delta = vector_scaled( p->velocity, dt );
	vector position = vector_add( delta, *matrix_getTranslation( p->trans->world ));
	transform_setWorldSpacePosition( p->trans, &position );
}

#ifdef DEBUG_PHYSIC_LIVENESS_TEST
void physic_assertActive( physic* p ) {
	vAssert( arrayContains( active_physics, active_physic_count, p ));
}
#endif // DEBUG_PHYSIC_LIVENESS_TEST
