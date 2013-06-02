#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "maths/maths.h"
#include "maths/matrix.h"
#include "mem/pool.h"

// *** Transform ***
struct transform_s {
	matrix		local;	// Local space matrix; 15th element is 1.f if clean and 2.f if dirty
	matrix		world;
	transform*	parent;
	int			isDirty;
#if DEBUG_STRINGS
	const char* debug_name;
#endif
};

DECLARE_POOL( transform )

// *** Static
void transform_initPool();

// Create a new default transform
transform* transform_create();
transform* transform_createAndAdd( scene* s );

// Delete a transform
void transform_delete( transform* t );

// Create a new default transform with the given parent
transform* transform_create_Parent(scene* s, transform* parent);

// *** Members

// Concatenate the parent world space transforms to produce this world space transform from local
int transform_concatenate(transform* t);

// Mark the transform as dirty (needs concatenation)
void transform_markDirty(transform* t);

// Mark the transform as clean (doesn't need concatenation)
void transform_markClean(transform* t);

// Is the transform dirty? (does it need concatenating?)
int transform_isDirty(transform* t);

// Set the translation of the localspace transformation matrix
void transform_setLocalTranslation(transform* t, vector* v);

void transform_printDebug( transform* t, debugtextframe* f );

void transform_setWorldSpace( transform* t, matrix world );
void transform_setWorldSpacePosition( transform* t, vector* position );
const vector* transform_getWorldPosition( transform* t );
const vector* transform_worldTranslation( transform* t );
quaternion transform_getWorldRotation( transform* t );

void transform_yaw( transform* t, float yaw );
void transform_pitch( transform* t, float pitch );
void transform_roll( transform* t, float roll );

void transform_setWorldRotationMatrix( transform* t, matrix m );

typedef uintptr_t transformHandle;
transformHandle transform_handleOf( transform* t );
transform* transform_fromHandle( transformHandle h );

#endif // __TRANSFORM_H__
