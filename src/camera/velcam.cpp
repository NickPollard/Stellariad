// velcam.c
// A velocity-based flycam

#include "common.h"
#include "velcam.h"
//---------------------
#include "camera.h"
#include "input.h"
#include "transform.h"
#include "input/keyboard.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/render.h" // TODO: remove

// velcam constructor
velcam* velcam_create( engine* e ) {
	velcam* f = (velcam*)mem_alloc( sizeof( velcam ));
	f->pan_sensitivity = Vector(1.f, 1.f, 1.f, 0.f);
	f->track_sensitivity = Vector(1.f, 1.f, 1.f, 0.f);
//	matrix_setIdentity( f->transform );
	f->translation = Vector( 0.f, 0.f, 0.f, 1.f );
	f->euler = Vector( 0.f, 0.f, 0.f, 0.f );
	f->trans = transform_create();
	f->phys = physic_create();
	f->phys->trans = f->trans;

	startTick( e, (void*)f->phys, physic_tick );
	return f;
}

void velcam_process( velcam* cam, velcamInput* in );

void velcam_input( velcam* cam, input* in  ) {
	velcamInput fly_in;
	fly_in.pan = Vector( 0.f, 0.f, 0.f, 0.f );
	fly_in.track = Vector( 0.f, 0.f, 0.f, 0.f );
	float delta = 0.01f;

	if ( input_keyHeld( in, KEY_SHIFT ) )
		delta = 0.1f;

	// Cam now looks down the positive Z axis
	if ( input_keyHeld( in, KEY_W ))
		fly_in.track.coord.z = delta;
	if ( input_keyHeld( in, KEY_S ))
		fly_in.track.coord.z = -delta;
	if ( input_keyHeld( in, KEY_Q ))
		fly_in.track.coord.y = delta;
	if ( input_keyHeld( in, KEY_E ))
		fly_in.track.coord.y = -delta;
	if ( input_keyHeld( in, KEY_LEFT ) || input_keyHeld( in, KEY_A ))
		fly_in.track.coord.x = -delta;
	if ( input_keyHeld( in, KEY_RIGHT ) || input_keyHeld( in, KEY_D ))
		fly_in.track.coord.x = delta;
/*	printf( "velcam input: track: %.2f, %.2f, %.2f, %.2f\n", fly_in.track.coord.x, 
															fly_in.track.coord.y,
															fly_in.track.coord.z,
															fly_in.track.coord.w );*/

	float mouseScale = 0.005f;

	int x = 0, y = 0;
	// Mouse drag coord is from TopLeft, ie. +x = right, +y = down
	input_mouseDrag( in, BUTTON_LEFT, &x, &y );
	// We cross assign x and y, as an x pan is a pan around the x axis, 
	// aka a pitch with is from vertical movement
	fly_in.pan.coord.y = (float)x * mouseScale;
	fly_in.pan.coord.x = -(float)y * mouseScale;


	velcam_process( cam, &fly_in );
}

// Read in an input structure
void velcam_process( velcam* cam, velcamInput* in ) {
	// *** Absolute
	Add( &cam->euler, &cam->euler, &in->pan );
	matrix m;
	matrix_fromEuler( m, &cam->euler );
	render_validateMatrix( m );
	// We have a translation in camera space
	// Want to go to world space, so use normal (not inverse) cam transform
	vector translation_delta = matrix_vecMul( m, &in->track );
	Add( &cam->translation, matrix_getTranslation( cam->trans->world ), &translation_delta );
	matrix_setTranslation( m, &cam->translation );
	transform_setWorldSpace( cam->trans, m );
}

// Set the camera target to output frame data to
void velcam_setTarget( velcam* f, camera* c ) {
	f->camera_target = c;
}

// Update the velcam, setting the target data to latest
void velcam_tick( velcam* f, float dt ) {
	(void)dt;
	assert( f->camera_target );
	vector vel;
	vector forward = Vector( 0.f, 0.f, 1.f, 0.f );
	vel = matrix_vecMul( f->trans->world, &forward );
	float speed = 0.f;
	vector_scale( &vel, &vel, speed );
	f->phys->velocity = vel;
	transform_setWorldSpace( f->camera_target->trans, f->trans->world );
}
