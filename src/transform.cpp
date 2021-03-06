#include "common.h"
#include "transform.h"
//-------------------------
#include "scene.h"
#include "maths/quaternion.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "debug/debugtext.h"

short transform_indexOf( transform* t );
short transform_uidOfIndex( short index );

#define kMaxTransforms 1024
IMPLEMENT_POOL( transform );

pool_transform* static_transform_pool = NULL;
short transform_uids[kMaxTransforms];

void transform_initPool() {
	static_transform_pool = pool_transform_create( kMaxTransforms );
	memset( transform_uids, 0, sizeof( short ) * kMaxTransforms );
}

void transform_setWorldSpace( transform* t, matrix world ) {
//	matrix_cpy( t->world, world );
	if ( t->parent ) {
		matrix inv_parent;
	  	matrix_inverse( inv_parent, t->parent->world );
		matrix_mul( t->local, world, inv_parent );
	}
	else
		matrix_cpy( t->local, world );

	transform_markDirty( t );
	transform_concatenate( t );
//	t->local = t->world * inverse( t->parent->world );
}

void transform_setWorldSpacePosition( transform* t, vector* position ) {
	// TODO - test this - Does this actually work in all situations?
	// Concat in case world space rotation is not up to date
	transform_concatenate( t );
	if ( !t->parent ) {
		matrix_setTranslation( t->local, position );
	} else {
		vector position_local;
		Sub( &position_local, position, matrix_getTranslation( t->parent->world ));
		matrix_setTranslation( t->local, &position_local );
	}
	transform_concatenate( t );
}

// Create a new default transform
transform* transform_create() {
	transform* t = pool_transform_allocate( static_transform_pool );
	short index = transform_indexOf( t );
#define SHORT_MAX 32768 // TODO - check this? Should I use unsigned?
	transform_uids[index] = (transform_uids[index] + 1) % SHORT_MAX;
	matrix_setIdentity( t->local );
	matrix_setIdentity( t->world );
	t->parent = NULL;
	t->isDirty = 1; // All transforms are initially dirty, to force initial update
#if DEBUG_STRINGS
	//t->debug_name = debug_string( "Transform" );
#endif
	return t;
}

void transform_delete( transform* t ) {
	pool_transform_free( static_transform_pool, t );
}

transform* transform_createAndAdd( scene* s ) {
	transform* t = transform_create();
	scene_addTransform( s, t );
	return t;
}

// Create a new default transform
transform* transform_createEmpty() {
	transform* t = (transform*)mem_alloc( sizeof( transform ));
	matrix_setIdentity(t->local);
	matrix_setIdentity(t->world);
	t->parent = NULL;
	t->isDirty = 0;
	return t;
}


// Create a new default transform with the given parent
transform* transform_create_Parent(scene* s, transform* parent) {
	(void)s;
	transform* t = transform_create();
	t->parent = parent;
	return t;
}

/*
// Concatenate the parent world space transforms to produce this world space transform from local
void transform_concatenate(transform* t) {
	if (transform_isDirty(t)) {
		transform_markClean(t);
		if (t->parent) {
			transform_concatenate(t->parent);
			t->world = matrix_mul(t->local, t->parent->world);
		}
		else
			t->world = t->local;
	}
}
*/

// Concatenate the parent world space transforms to produce this world space transform from local
int transform_concatenate(transform* t) {
//	printf( "concatenate transform. This = %x, Parent = %x\n", (unsigned int)t, (unsigned int)t->parent );
	vAssert( t );
	if (t->parent)	{
		if (transform_concatenate(t->parent) || transform_isDirty(t)) {
			transform_markDirty(t);
			matrix_mul(t->world, t->parent->world, t->local );
			return true;
		}
	}
	else if (transform_isDirty(t)) {
		matrix_cpy( t->world, t->local );
		return true;
	}
	return false;
}


// Mark the transform as dirty (needs concatenation)
void transform_markDirty(transform* t) {
	t->isDirty = 1;
}

// Mark the transform as clean (doesn't need concatenation)
void transform_markClean(transform* t) {
	t->isDirty = 0;
}

// Is the transform dirty? (does it need concatenating?)
int transform_isDirty(transform* t) {
	return t->isDirty;
}

// Set the translation of the localspace transformation matrix
void transform_setLocalTranslation(transform* t, vector* v) {
	matrix_setTranslation(t->local, v);
	transform_markDirty(t);
}

void transform_printDebug( transform* t, debugtextframe* f ) {
	(void)f;
	char string[128];
#if DEBUG_STRINGS
	sprintf( string, "Transform: Name: %s, Translation %.2f, %.2f, %.2f", 
			t->debug_name,
			t->world[3][0], 
			t->world[3][1], 
			t->world[3][2] );
#else
	sprintf( string, "Transform: Translation %.2f, %.2f, %.2f", 
			t->world[3][0], 
			t->world[3][1], 
			t->world[3][2] );
#endif
#ifndef ANDROID	
//	PrintDebugText( f, string );
#endif
}

void transform_yaw( transform* t, float yaw ) {
	matrix m;
	matrix_rotY( m, yaw );
	matrix_mul( t->local, t->local, m );
}

void transform_pitch( transform* t, float pitch ) {
	matrix m;
	matrix_rotX( m, pitch );
	matrix_mul( t->local, t->local, m );
}

void transform_roll( transform* t, float roll ) {
	matrix m;
	matrix_rotZ( m, roll );
	matrix_mul( t->local, t->local, m );
}

const vector* transform_getWorldPosition( transform* t ) {
	vAssert( (uintptr_t)t > 0xffff );
	return matrix_getTranslation( t->world );
}

const vector* transform_worldTranslation( transform* t ) {
	vAssert( (uintptr_t)t > 0xffff );
	return matrix_getTranslation( t->world );
}

quaternion transform_getWorldRotation( transform* t ) {
	vAssert( (uintptr_t)t > 0xffff );
	return matrix_getRotation( t->world );
}

void transform_setWorldRotationMatrix( transform* t, matrix world_m ) {
	transform_concatenate( t );
	// parent * local_m = world_m;
	if ( t->parent ) {
		matrix inv_parent, local_m;
		matrix_inverse( inv_parent, t->parent->world );
		matrix_mul( local_m, inv_parent, world_m );
		matrix_copyRotation( t->local, local_m );
	}
	else {
		matrix_copyRotation( t->local, world_m );
	}
}

#define kTransformHandleIndexMask	0x0000ffff
#define kTransformHandleUidMask		0xffff0000
#define kTransformHandleUidShift	16

transformHandle transform_handleOf( transform* t ) {
	short index = transform_indexOf( t );
	short uid = transform_uidOfIndex( index );
	transformHandle h = index | (uid << kTransformHandleUidShift);
	return h;
}

short transformHandle_index( transformHandle h ) {
	return (short)(h & kTransformHandleIndexMask);
}

short transformHandle_uid( transformHandle h ) {
	return (short)((h & kTransformHandleUidMask) >> kTransformHandleUidShift );
}

short transform_uidOfIndex( short index ) {
	return transform_uids[index];
}

transform* transform_atIndex( short index ) {
	vAssert( 0 <= index );
	vAssert( index < kMaxTransforms );
	transform* t = &static_transform_pool->pool[index];
	vAssert( (uintptr_t)t > 0xffff );
	return t;
}

short transform_indexOf( transform* t ) {
	return (short)(t - static_transform_pool->pool);
}

transform* transform_fromHandle( transformHandle h ) {
	// Lookup Handle as an index in the transformPool
	// Check the id value to the id of the handle
	// If identical, return it, otherwise null
	short uid = transformHandle_uid( h );
	short index = transformHandle_index( h );
	vAssert( 0 <= index );
	vAssert( index < kMaxTransforms );
	transform* t = NULL;
	if ( !static_transform_pool->free[index] && transform_uidOfIndex( index ) == uid ) {
		t = transform_atIndex( index );
	}
	else {
		t = NULL;
	}
	if( (uintptr_t)t <= 0xffff ) {
		/*
		printf( "Transform uid: %d.\n", (int)uid );
		printf( "Transform index: %d.\n", (int)index );
		printf( "Transform handle: 0x%x.\n", (int)h );
		printf( "Transform t: 0x" xPTRf ".\n", (uintptr_t)t );
		*/
	}
	return t;
}
