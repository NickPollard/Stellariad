// flycam.c

#include "common.h"
#include "flycam.h"
//---------------------
#include "input.h"
#include "transform.h"
#include "input/keyboard.h"
#include "mem/allocator.h"

// Flycam constructor
flycam* flycam_create() {
	flycam* f = (flycam*)mem_alloc( sizeof( flycam ));

	f->pan_sensitivity = Vector(2.f, 2.f, 2.f, 0.f);
	f->track_sensitivity = Vector(2.f, 2.f, 2.f, 0.f);

	matrix_setIdentity( f->transform );
	f->translation = Vector( 0.f, 0.f, 0.f, 1.f );
	f->euler = Vector( 0.f, 0.f, 0.f, 0.f );

	camera_init( &f->camera_target );

	return f;
}

void flycam_process( flycam* cam, flycamInput* in );

void flycam_input( void* data, input* in  ) {
	flycam* cam = (flycam*)data;
	flycamInput fly_in;
	fly_in.pan = Vector( 0.f, 0.f, 0.f, 0.f );
	fly_in.track = Vector( 0.f, 0.f, 0.f, 0.f );
	float delta = 0.1f;

	if ( input_keyHeld( in, KEY_SHIFT ) )
		delta = 10.f;
	if ( input_keyHeld( in, KEY_CTRL ) )
		delta = 0.01f;

	delta *= 3.f;

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
/*	printf( "Flycam input: track: %.2f, %.2f, %.2f, %.2f\n", fly_in.track.coord.x, 
															fly_in.track.coord.y,
															fly_in.track.coord.z,
															fly_in.track.coord.w );*/

	float mouseScale = 0.005f;

	int x = 0, y = 0;
	// Mouse drag coord is from TopLeft, ie. +x = right, +y = down
#ifndef ANDROID
	input_mouseDrag( in, BUTTON_LEFT, &x, &y );
#else
	input_getTouchDrag( in, 0, &x, &y );
#endif
	// We cross assign x and y, as an x pan is a pan around the x axis, 
	// aka a pitch with is from vertical movement
	fly_in.pan.coord.y = (float)x * mouseScale;
	fly_in.pan.coord.x = -(float)y * mouseScale;

	flycam_process( cam, &fly_in );
}

// Read in an input structure
void flycam_process( flycam* cam, flycamInput* in ) {
	// *** Absolute
	Add( &cam->euler, &cam->euler, &in->pan );
	matrix_fromEuler( cam->transform, &cam->euler );
	// We have a translation in camera space
	// Want to go to world space, so use normal (not inverse) cam transform
	vector translation_delta = matrix_vecMul( cam->transform, &in->track );
	Add( &cam->translation, &cam->translation, &translation_delta );
	matrix_setTranslation( cam->transform, &cam->translation );
}

/*
// Set the camera target to output frame data to
void flycam_setTarget( flycam* f, camera* c ) {
	f->camera_target = c;
}
*/

// Update the flycam, setting the target data to latest
void flycam_tick( void* data, float dt, engine* eng ) {
	(void)eng;
	(void)dt;
	flycam* f = (flycam*)data;
	transform_setWorldSpace( f->camera_target.trans, f->transform );
}
