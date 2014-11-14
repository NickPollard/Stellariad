// chasecam.c

#include "common.h"
#include "chasecam.h"
//---------------------
#include "transform.h"
#include "maths/quaternion.h"
#include "maths/vector.h"

#include "render/render.h" // TODO remove

chasecam* chasecam_create() {
	chasecam* c = (chasecam*)mem_alloc( sizeof( chasecam ));
	memset( c, 0, sizeof( chasecam ));
	c->position = Vector( 0.f, 0.f, 0.f, 1.f );
	c->rotation = Quaternion( 0.f, 0.f, 0.f, 1.f );
	c->lerp_speed = kDefaultChasecamLerpSpeed;
	c->slerp_speed = kDefaultChasecamSlerpSpeed;
	camera_init( &c->cam );
	return c;
}

// Calculate the position as the offset from the chase target
vector chasecam_targetPosition( chasecam* c ) {
	vector offset = Vector( 0.f, 8.f, -35.f, 1.f );
	//vector position = matrix_vecMul( c->target->world, &offset );
	matrix m;
	matrix_fromRotationTranslation( m, c->rotation, *matrix_getTranslation( c->target->world ));
	vector position = matrix_vecMul( m, &offset );
	return position;
}

quaternion chasecam_targetRotation( chasecam* c ) {
	quaternion rotation = transform_getWorldRotation( c->target );
	return rotation;
}

void chasecam_tick( void* data, float dt, engine* eng ) {
	(void)eng;
	chasecam* c = (chasecam*) data;

	vAssert( c->target );
	transform_concatenate( c->target );

	vector position = chasecam_targetPosition( c );
	float lerp = fclamp( c->lerp_speed * dt, 0.f, 1.f );
	c->position = vector_lerp( &c->position, &position, lerp );

	quaternion target_rotation = chasecam_targetRotation( c );
	float slerp_factor = fclamp( c->slerp_speed * dt, 0.f, 1.f );
	c->rotation = quaternion_slerp( c->rotation, target_rotation, slerp_factor );
	
	matrix world_space;
	matrix_fromRotationTranslation( world_space, c->rotation, c->position );
	render_validateMatrix( world_space );
	transform_setWorldSpace( c->cam.trans, world_space );	
}

void chasecam_setTarget( chasecam* c, transform* t ) {
	c->target = t;
	// When setting a target, jump straight to position and rotation
	// Concatenate the target transform to make sure it's up-to-date
	transform_concatenate( c->target );
	c->position = chasecam_targetPosition( c );
	c->rotation = chasecam_targetRotation( c );
}
