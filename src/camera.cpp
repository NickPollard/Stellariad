// camera.c

#include "common.h"
#include "camera.h"
//---------------------
#include "maths/maths.h"
#include "transform.h"
#include "mem/allocator.h"

static const float default_z_near = 1.f;
static const float default_z_far = 1800.f;
static const float default_fov = 0.8f; // In Radians

camera* camera_create() {
	camera* c = (camera*)mem_alloc( sizeof( camera ));
	c->trans = NULL;
	camera_init( c );
	return c;
}

void camera_init( camera* c ) {
	c->z_near = default_z_near;
	c->z_far = default_z_far;
	c->fov = default_fov;
}

camera* camera_createWithTransform( scene* s ) {
	camera* c = camera_create();
	c->trans = transform_createAndAdd( s );
	return c;
}

const vector* camera_getTranslation( camera* c ) {
	return matrix_getTranslation( c->trans->local );
}

void camera_setTranslation( camera* c, const vector* v ) {
	matrix_setTranslation( c->trans->local, v );
}

// Calculate the planes of the view frustum defined by the camera *c*
void camera_calculateFrustum( camera* c, vector* frustum ) {
	vAssert( frustum );
	// 6 planes: Front, Back, Top, Bottom, Left, Right
	// We can define a plane in a standard 4-element vector:
	// First 3 elements are the normal, last is the D value

	vector v = Vector( 0.0, 0.0, 1.0, 0.0 ); // 0.0 w coordinate since vector not point
	vector view = matrix_vecMul( c->trans->world, &v );
	vector p = *matrix_getTranslation( c->trans->world );

	vector front = view;
	front.coord.w = Dot( &p, &view ) + c->z_near;
	vector back = vector_scaled(view, -1.f);
	back.coord.w = 1.f;
	back.coord.w = Dot( &p, &back ) - c->z_far;

	frustum[0] = front;
	frustum[1] = back;

	// Calculate side-planes
	// D is still based on the camera position
	// Vector is based on field of view and aspect ratio

	// Left vector is view, rotated around up, by fov/2

	vector up = matrix_vecMul( c->trans->world, &y_axis );

	vector left = vector_rotateAxisAngle(view, up, c->fov * 0.5f);
	vector right = vector_rotateAxisAngle(view, up, c->fov * -0.5f);
	left.coord.w = Dot( &p, &left );
	right.coord.w = Dot( &p, &right );
	
	frustum[2] = left;
	frustum[3] = right;
}
