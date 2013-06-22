// camera.h
#pragma once

struct camera_s {
	transform* trans;
	// z-clipping
	float z_near;
	float z_far;

	float aspect; // aspect ratio of the render window
	float fov; // in Radians
//	float focalLength;
//	float aperture;
};

camera* camera_create();

void camera_init( camera* c );

camera* camera_createWithTransform(scene* s);

const vector* camera_getTranslation(camera* c);

void camera_setTranslation(camera* c, const vector* v);

// Calculate the planes of the view frustum defined by the camera *c*
void camera_calculateFrustum( camera* c, vector* frustum );
