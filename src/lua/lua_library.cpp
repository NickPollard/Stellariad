// lua_library.c
#include "common.h"
#include "lua_library.h"
//---------------------
#include "canyon.h"
#include "terrain/canyon_terrain.h"
#include "canyon_zone.h"
#include "collision.h"
#include "dynamicfog.h"
#include "engine.h"
#include "input.h"
#include "model.h"
#include "particle.h"
#include "physic.h"
#include "ribbon.h"
#include "scene.h"
#include "transform.h"
#include "vtime.h"
#include "camera/chasecam.h"
#include "camera/flycam.h"
#include "input/keyboard.h"
#include "maths/geometry.h"
#include "maths/quaternion.h"
#include "mem/allocator.h"
#include "render/debugdraw.h"
#include "render/modelinstance.h"
#include "render/texture.h"
#include "script/lisp.h"
#include "system/file.h"
#include "system/string.h"
#include "terrain/marching.h"
#include "ui/panel.h"

#define DEBUG_SANITY_CHECK_POINTERS

// *** Forward Declarations
void lua_completeFuturePtr( lua_State* l, int future_ref, void* ptr );

int LUA_vassert( lua_State* l ) {
	bool b = lua_toboolean( l, 1 );
	luaAssert( l, b );
	return 0;
}

int LUA_print( lua_State* l ) {
	if ( lua_isstring( l, 1 ) ) {
		const char* string = lua_tostring( l, 1 );
		printf( "LUA: %s\n", string );
	} else
		printf( "Error: LUA: Tried to print a non-string object from Lua.\n" );
	return 0;
}

transform* lua_toTransform( lua_State* l, int i ) {
	if (!lua_isnumber( l, i )) {
		return NULL;
	}
	transformHandle h = lua_tonumber( l, i );
	transform* t = transform_fromHandle( h );
	
	/*
	if( (uintptr_t)t <= 0xffff ) {
		printf( "Transform handle: %d.\n", (int)h );
	}
	*/
	//luaAssert( l, (uintptr_t)t > 0xffff );
	return t;
}

int LUA_transform_valid( lua_State* l ) {
	bool b = (lua_toTransform( l, 1 ) != NULL);
	lua_pushboolean( l, b );
	return 1;
}

void lua_optionSome( lua_State* l, const void* ptr ) {
	// TODO - wrapper for accessing paths, e.g. turn option.some into these two commands
	lua_getglobal( l, "option" ); // Get the option table on to the stack
	lua_getfield( l, -1, "some" ); // Look up the 'some' value
	lua_getglobal( l, "option" ); // Get option again (Self pointer)
	lua_pushptr( l, (void*)ptr );
	lua_pcall( l, 2, 1, 0 );
	lua_remove( l, -2 ); // remove the first option table
}

void lua_optionNone( lua_State* l ) {
	// TODO - wrapper for accessing paths, e.g. turn option.some into these two commands
	lua_getglobal( l, "option" ); // Get the option table on to the stack
	lua_getfield( l, -1, "none" ); // Look up the 'some' value
	lua_getglobal( l, "option" ); // Get option again (Self pointer)
	lua_pcall( l, 1, 1, 0 );
	lua_remove( l, -2 ); // remove the first option table
}



#define LUA_CREATE_( type, func ) \
int LUA_create##type( lua_State* l ) { \
	LUA_DEBUG_PRINT( "Create %s\n", #type ); \
	type* ptr = func(); \
	lua_pushptr( l, ptr ); \
	return 1; \
}

//LUA_CREATE_( transform, transform_create )

int LUA_createTransform( lua_State* l ) {
	transform* t = transform_createAndAdd( theScene );
	int arg_count = lua_gettop( l );;
	if ( arg_count == 1 ) {
		transform* parent = lua_toTransform( l, 1 );
		t->parent = parent;
	}
	transformHandle h = transform_handleOf( t );
	lua_pushnumber( l, h );
	return 1;
}

LUA_CREATE_( physic, physic_create )

int LUA_createModelInstance( lua_State* l ) {
	if ( lua_isstring( l, 1 ) ) {
		const char* filename = lua_tostring( l, 1 );
		modelInstance* m = modelInstance_create( model_getHandleFromFilename( filename ) );
		lua_pushptr( l, m );
		return 1;
	} else {
		printf( "Error: LUA: No filename specified for vcreateModelInstance().\n" );
		return 0;
	}
}

int LUA_modelInstanceSetStatic( lua_State* l ) {
	if ( lua_isstring( l, 1 ) ) {
		modelInstance* m = (modelInstance*)lua_toptr( l, 1 );
		modelInstance_setStatic( m );
		lua_pushptr( l, m );
		return 1;
	} else {
		vAssert( 0 );
		return 0;
	}
}

int LUA_modelPreload( lua_State* l ) {
	if ( lua_isstring( l, 1 ) ) {
		const char* filename = lua_tostring( l, 1 );
		model_preload( filename );
		return 0;
	} else {
		printf( "Error: LUA: No filename specified for vmodel_preload().\n" );
		return 0;
	}
}

int LUA_texturePreload( lua_State* l ) {
	if ( lua_isstring( l, 1 ) ) {
		const char* filename = lua_tostring( l, 1 );
		texture_load( filename );
		return 0;
	} else {
		printf( "Error: LUA: No filename specified for vtexture_preload().\n" );
		return 0;
	}
}



typedef struct luaCollisionCallbackData_s {
	lua_State* lua_state;
	int callback_ref;
} luaCollisionCallbackData;

void lua_collisionCallback( body* ths, body* other, void* data ) {
	luaCollisionCallbackData* callback_data = (luaCollisionCallbackData*)data;
	lua_State* l = callback_data->lua_state;
	int ref = callback_data->callback_ref;

	lua_retrieve( l, ref );
	luaAssert( l, ths->intdata );
	lua_retrieve( l, ths->intdata );
	if ( other->intdata ) {
		lua_retrieve( l, other->intdata );
	} else {
		lua_pushnumber( l, 0 );
	}
	lua_pcall( l, 2, 0, 0 );
}

int LUA_createbodySphere( lua_State* l ) {
	int ref = lua_store( l );	// Store top of the stack ( the object )

	body* b = body_create( sphere_create( 3.f ), NULL );
	b->callback = NULL;
	b->intdata = ref;
	collision_addBody( b );
	lua_pushptr( l, b );
	return 1;
}

int LUA_createbodyMesh( lua_State* l ) {
	// Get the mesh but then pop it, so that the object is left on the top for the lua_store command
	modelInstance* render_model = (modelInstance*)lua_toptr( l, 2 );
	lua_pop( l, 1 );
	int ref = lua_store( l );	// Store top of the stack ( the object )

	mesh* render_mesh = model_fromInstance( render_model )->meshes[0];
	body* b = body_create( mesh_createFromRenderMesh( render_mesh ), NULL );
	b->callback = NULL;
	b->intdata = ref;
	collision_addBody( b );
	lua_pushptr( l, b );
	return 1;
}

int LUA_deleteModelInstance( lua_State* l ) {
	modelInstance* m = (modelInstance*)lua_toptr( l, 1 );
	scene_removeModel( theScene, m );
#if DEBUG
	//model* mod = model_fromInstance( m );
	//printf( "Deleting modelinstance %s\n", mod->filename );
#endif // DEBUG
	modelInstance_delete( m );
	return 0;
}

// vscene_addModel( scene, model )
int LUA_scene_addModel( lua_State* l ) {
	scene* s = (scene*)lua_toptr( l, 1 );	
	modelInstance* m = (modelInstance*)lua_toptr( l, 2 );	
	luaAssert( l, m->trans );
	scene_addModel( s, m );
	return 0;
}
int LUA_scene_removeModel( lua_State* l ) {
	scene* s = (scene*)lua_toptr( l, 1 );	
	modelInstance* m = (modelInstance*)lua_toptr( l, 2 );	
	scene_removeModel( s, m );
	return 0;
}

int LUA_scene_setSkyColor( lua_State* l ) {
	scene* s = (scene*)lua_toptr( l, 1 );
	vector* v = (vector*)lua_toptr( l, 2 );
	scene_setSkyColor(s, v);
	return 0;
}

int LUA_scene_setSunColor( lua_State* l ) {
	scene* s = (scene*)lua_toptr( l, 1 );
	vector* v = (vector*)lua_toptr( l, 2 );
	scene_setSunColor(s, v);
	return 0;
}

int LUA_scene_setFogColor( lua_State* l ) {
	scene* s = (scene*)lua_toptr( l, 1 );
	vector* v = (vector*)lua_toptr( l, 2 );
	scene_setFogColor(s, v);
	return 0;
}



int LUA_model_setTransform( lua_State* l ) {
	LUA_DEBUG_PRINT( "lua model set transform\n" );
	lua_assertnumber( l, 1 );
	lua_assertnumber( l, 2 );
	modelInstance* m = (modelInstance*)lua_toptr( l, 1 );
	transform* t = lua_toTransform( l, 2 );
	m->trans = t;
	return 0;
}

int LUA_physic_setTransform( lua_State* l ) {
	LUA_DEBUG_PRINT( "lua physic set transform\n" );
	physic* p = (physic*)lua_toptr( l, 1 );
#ifdef DEBUG_PHYSIC_LIVENESS_TEST
	physic_assertActive( p );
#endif // DEBUG_PHYSIC_LIVENESS_TEST
	transform* t = lua_toTransform( l, 2 );
	p->trans = t;
	return 0;
}

int LUA_body_setTransform( lua_State* l ) {
	LUA_DEBUG_PRINT( "lua body set transform\n" );
	body* b = (body*)lua_toptr( l, 1 );
	transform* t = lua_toTransform( l, 2 );
	b->trans = t;
	return 0;
}
int LUA_body_registerCollisionCallback( lua_State* l ) {
	LUA_DEBUG_PRINT( "Registering lua collision handler.\n" );
	body* b = (body*)lua_toptr( l, 1 );
	// Store the lua func callback in the Lua registry
	// and keep a reference to it so we can resolve it later
	int ref = lua_store( l ); // Must be top of the stack
	b->callback = lua_collisionCallback;
	// Store the Lua ref (which resolves to the function) in the callback_data for the body
	// This allows us to call the correct lua func (or closure) for this body
	// TODO: fix this temp hack
	luaCollisionCallbackData* data = (luaCollisionCallbackData*)mem_alloc( sizeof( luaCollisionCallbackData ));
	data->lua_state = l;
	data->callback_ref = ref;
	b->callback_data = data;
	return 0;
}

int LUA_body_destroy( lua_State* l ) {
	body* b = (body*)lua_toptr( l, 1 );
	luaAssert( l, b );
	collision_removeBody( b );
	return 0;
}

int LUA_physic_destroy( lua_State* l ) {
	physic* p = (physic*)lua_toptr( l, 1 );
	luaAssert( l, p );
#ifdef DEBUG_PHYSIC_LIVENESS_TEST
	physic_assertActive( p );
#endif // DEBUG_PHYSIC_LIVENESS_TEST
	physic_delete( p );
	return 0;
}

int LUA_transform_destroy( lua_State* l ) {
	scene* s = (scene*)lua_toptr( l, 1 );
	transform* t = lua_toTransform( l, 2 );
	luaAssert( l, t );
	scene_removeTransform( s, t );
	transform_delete( t );
	return 0;
}

int LUA_physic_activate( lua_State* l ) {
	LUA_DEBUG_PRINT( "lua physic activate.\n" );
	engine* e = (engine*)lua_toptr( l, 1 );
	physic* p = (physic*)lua_toptr( l, 2 );
#ifdef DEBUG_PHYSIC_LIVENESS_TEST
	physic_assertActive( p );
#endif // DEBUG_PHYSIC_LIVENESS_TEST
	startTick( e, p, physic_tick );
	return 0;
}

int LUA_physic_setVelocity( lua_State* l ) {
	physic* p = (physic*)lua_toptr( l, 1 );
#ifdef DEBUG_PHYSIC_LIVENESS_TEST
	physic_assertActive( p );
#endif // DEBUG_PHYSIC_LIVENESS_TEST
	vector* v = (vector*)lua_toptr( l, 2 );
	p->velocity = *v;
	return 0;
}


int LUA_setWorldSpacePosition( lua_State* l ) {
	if (lua_isnumber( l, 1 ) &&
		   	lua_isnumber( l, 2 ) &&
			lua_isnumber( l, 3 ) &&
			lua_isnumber( l, 4 )) {
		transform* t = lua_toTransform( l, 1 );
		assert( t );
		float x = lua_tonumber( l, 2 );
		float y = lua_tonumber( l, 3 );
		float z = lua_tonumber( l, 4 );
		vector v = Vector( x, y, z, 1.0 );
		matrix m;
		matrix_cpy( m, t->world );
		matrix_setTranslation( m, &v );
		printf( "Transform pointer: " xPTRf ".\n", (uintptr_t)t );
		transform_setWorldSpace( t, m );
	}
	return 0;
}

int LUA_keyPressed( lua_State* l ) {
	if ( lua_isnumber( l, 2 ) ) {
		input* in = (input*)lua_toptr( l, 1 );
		int key = (int)lua_tonumber( l, 2 );
		bool pressed = input_keyPressed( in, (enum key)key );
		lua_pushboolean( l, pressed );
		return 1;
	}
	printf( "Error: Lua: keyPressed called without key specified.\n" );
	return 0;
}
int LUA_keyHeld( lua_State* l ) {
	if ( lua_isnumber( l, 2 ) ) {
		input* in = (input*)lua_toptr( l, 1 );
		int key = (int)lua_tonumber( l, 2 );
		bool held = input_keyHeld( in, (enum key)key );
		lua_pushboolean( l, held );
		return 1;
	}
	printf( "Error: Lua: keyHeld called without key specified.\n" );
	return 0;
}

#ifdef TOUCH
int LUA_touchPressed( lua_State* l ) {
	if ( lua_isnumber( l, 2 ) && 
	 	lua_isnumber( l, 3 ) &&
	 	lua_isnumber( l, 4 ) &&
	 	lua_isnumber( l, 5 ) )
	{
		input* in = (input*)lua_toptr( l, 1 );
		int x_min = lua_tonumber( l, 2 );
		int y_min = lua_tonumber( l, 3 );
		int x_max = lua_tonumber( l, 4 );
		int y_max = lua_tonumber( l, 5 );
		bool pressed = input_touchPressed( in, x_min, y_min, x_max, y_max );
		lua_pushboolean( l, pressed );
		return 1;
	}
	printf( "Error: Lua: Invalid touch bounds specified" );
	return 0;
}

int LUA_touchHeld( lua_State* l ) {
	if ( lua_isnumber( l, 2 ) && 
	 	lua_isnumber( l, 3 ) &&
	 	lua_isnumber( l, 4 ) &&
	 	lua_isnumber( l, 5 ) )
	{
		input* in = (input*)lua_toptr( l, 1 );
		int x_min = lua_tonumber( l, 2 );
		int y_min = lua_tonumber( l, 3 );
		int x_max = lua_tonumber( l, 4 );
		int y_max = lua_tonumber( l, 5 );
		bool pressed = input_touchHeld( in, x_min, y_min, x_max, y_max );
		lua_pushboolean( l, pressed );
		return 1;
	}
	printf( "Error: Lua: Invalid touch bounds specified" );
	return 0;
}

int LUA_createTouchPad( lua_State* l ) {
	input* in = (input*)lua_toptr( l, 1 );
	int x = lua_tonumber( l, 2 );
	int y = lua_tonumber( l, 3 );
	int w = lua_tonumber( l, 4 );
	int h = lua_tonumber( l, 5 );
	touchPad* pad = touchPanel_addTouchPad( &in->touch, touchPad_create( x, y, w, h ));
	lua_pushptr( l, pad );
	return 1;
}

int LUA_touchPadDragged( lua_State* l ) {
	touchPad* pad = (touchPad*)lua_toptr( l, 1 );
	int x, y;
	bool dragged = touchPad_dragged( pad, &x, &y );
	lua_pushboolean( l, dragged );
	lua_pushnumber( l, x );
	lua_pushnumber( l, y );
	return 3;
}

int LUA_touchPadTouched( lua_State* l ) {
	touchPad* pad = (touchPad*)lua_toptr( l, 1 );
	int x, y;
	bool touched = touchPad_touched( pad, &x, &y );
	lua_pushboolean( l, touched );
	lua_pushnumber( l, x );
	lua_pushnumber( l, y );
	return 3;
}

int LUA_createGesture( lua_State* l ) {
	float distance = lua_tonumber( l, 1 );
	float duration = lua_tonumber( l, 2 );
	vector* direction = (vector*)lua_toptr( l, 3 );
	float angle_tolerance = lua_tonumber( l, 4 );
	gesture* g = gesture_create( distance, duration, *direction, angle_tolerance );
	lua_pushptr( l, g );
	return 1;
}

int LUA_gesture_performed( lua_State* l ) {
	touchPad* pad = (touchPad*)lua_toptr( l, 1 );
	gesture* g = (gesture*)lua_toptr( l, 2 );
	bool performed = input_gesturePerformed( pad, g );
	lua_pushboolean( l, performed );
	return 1;
}

#else
int LUA_touchHeld( lua_State* l ) {
	lua_pushboolean( l, false );
	return 1;
}
int LUA_touchPressed( lua_State* l ) {
	lua_pushboolean( l, false );
	return 1;
}
int LUA_createTouchPad( lua_State* l ) {
	lua_pushnumber( l, 0 );
	return 1;
}
int LUA_touchPadDragged( lua_State* l ) {
	lua_pushboolean( l, false );
	lua_pushnumber( l, -1 );
	lua_pushnumber( l, -1 );
	return 3;
}
int LUA_touchPadTouched( lua_State* l ) {
	lua_pushboolean( l, false );
	lua_pushnumber( l, -1 );
	lua_pushnumber( l, -1 );
	return 3;
}

int LUA_createGesture( lua_State* l ) {
	(void)l;
	return 0;
}

int LUA_gesture_performed( lua_State* l ) {
	lua_pushboolean( l, false );
	return 1;
}
#endif // TOUCH


int LUA_transform_yaw( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	float yaw = lua_tonumber( l, 2 );
	transform_yaw( t, yaw );
	return 0;
}

int LUA_transform_roll( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	float roll = lua_tonumber( l, 2 );
	transform_roll( t, roll );
	return 0;
}

int LUA_transform_pitch( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	float pitch = lua_tonumber( l, 2 );
	transform_pitch( t, pitch );
	return 0;
}

// Make a vector in Lua
// This is using light userdata?
int LUA_vector( lua_State* l ) {
	float x = lua_tonumber( l, 1 );
	float y = lua_tonumber( l, 2 );
	float z = lua_tonumber( l, 3 );
	float w = lua_tonumber( l, 4 );
	vector* v = lua_createVector( );
	*v = Vector( x, y, z, w );
	lua_pushptr( l, v );
	return 1;
}

int LUA_transformVector( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	vector* v = (vector*)lua_toptr( l, 2 );
	vector* _v = lua_createVector();
	*_v = matrix_vecMul( t->world, v );
	lua_pushptr( l, _v );
	return 1;
}

int LUA_transform_setLocalPosition( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	vector* v = (vector*)lua_toptr( l, 2 );
	transform_setLocalTranslation( t, v );
	return 0;
}

int LUA_transform_setWorldPosition( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	vector* v = (vector*)lua_toptr( l, 2 );
	transform_setWorldSpacePosition( t, v );
	return 0;
}

int LUA_transform_getWorldPosition( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	//printf( "lua_transform_getWorldPosition - transform = " xPTRf "\n", (uintptr_t)t );
#ifdef DEBUG_SANITY_CHECK_POINTERS
	luaAssert( l, t );
	luaAssert( l, (uintptr_t)t > 0xffff );
#endif // DEBUG_SANITY_CHECK_POINTERS
	if ( t ) {
		const vector* v = transform_getWorldPosition( t );
		lua_optionSome( l, v );
	} else {
		lua_optionNone( l );
	}
	return 1;
}

int LUA_vector_values( lua_State* l ) {
	const vector* v = (const vector*)lua_toptr( l, 1 );
	lua_pushnumber( l, v->coord.x );
	lua_pushnumber( l, v->coord.y );
	lua_pushnumber( l, v->coord.z );
	lua_pushnumber( l, v->coord.w );
	return 4;
}

int LUA_chasecam_follow( lua_State* l ) {
	LUA_DEBUG_PRINT( "LUA chasecam_follow\n" );
	engine* e = (engine*)lua_toptr( l, 1 );
	transform* t = lua_toTransform( l, 2 );
	chasecam* c = chasecam_create();
	startTick( e, (void*)c, chasecam_tick );	
	c->cam.trans = transform_createAndAdd( theScene );
	theScene->cam = &c->cam;
	chasecam_setTarget( c, t );
	lua_pushptr( l, c );
	return 1;
}

int LUA_flycam( lua_State* l ) {
	LUA_DEBUG_PRINT( "LUA flycam\n" );
	engine* e = (engine*)lua_toptr( l, 1 );
	flycam* c = flycam_create();
	startTick( e, (void*)c, flycam_tick );	
	startInput( e, (void*)c, flycam_input );	
	c->camera_target.trans = transform_createAndAdd( theScene );
	theScene->cam = &c->camera_target;
	lua_pushptr( l, c );
	return 1;
}

int LUA_setCamera( lua_State* l ) {
	theScene->cam = (camera*)lua_toptr( l, 1 );
	return 0;
}

int LUA_cameraPosition( lua_State* l ) {
	camera* c = (camera*)lua_toptr( l, 1 );
	vector* v = lua_createVector();
	*v = *transform_getWorldPosition( c->trans );
	lua_pushptr( l, v );
	return 1;
}

int LUA_transform_setWorldSpaceByTransform( lua_State* l ) {
	transform* dst = lua_toTransform( l, 1 );
	transform* src = lua_toTransform( l, 2 );
	transform_setWorldSpace( dst, src->world );
	return 0;
}
int LUA_transform_eulerAngles( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	float yaw = lua_tonumber( l, 2 );
	float pitch = lua_tonumber( l, 3 );
	float roll = lua_tonumber( l, 4 );
	vector angles = eulerAngles( yaw, pitch, roll );
	matrix m;
	matrix_fromEuler( m, &angles );
	matrix_copyRotation( t->local, m );
	transform_concatenate( t );
	return 0;
}

int LUA_transform_distance( lua_State* l ) {
	transform* a = lua_toTransform( l, 1 );
	transform* b = lua_toTransform( l, 2 );
	float distance = vector_distance( transform_getWorldPosition( a ), transform_getWorldPosition( b ));
	lua_pushnumber( l, (double)distance );
	return 1;
}

int LUA_transform_setRotation( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	quaternion* q = (quaternion*)lua_toptr( l, 2 );
	matrix m;
	matrix_fromRotationTranslation( m, *q, Vector( 0.f, 0.f, 0.f, 1.f ));
	transform_setWorldRotationMatrix( t, m );
	return 0;
}

int LUA_transform_setWorldSpaceByMatrix( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	matrix* m = (matrix*)lua_toptr( l, 2 );
	transform_setWorldSpace( t, *m );
	return 0;
}

int LUA_transform_facingWorld( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	const vector* look_at = (const vector*)lua_toptr( l, 2 );
	const vector* position = transform_getWorldPosition( t );
	vector displacement;
	Sub( &displacement, look_at, position );
	Normalize( &displacement, &displacement );
	matrix m;
	matrix_look( m, displacement );
	transform_setWorldRotationMatrix( t, m );
	return 0;
}

particleEmitter* createEmitter( const char* particle_file, transform* t, engine* e ) {
	particleEmitterDef* def = particle_loadAsset( particle_file );
	particleEmitter* emitter = particle_newEmitter( def );

#ifdef DEBUG_PARTICLE_SOURCES
	char buffer[256];
	sprintf( buffer, "Lua particle create: %s.", particle_file );
	emitter->debug_creator = string_createCopy( buffer );
#endif // DEBUG

	emitter->trans = t;

	engine_addRender( e, emitter, particleEmitter_render );
	startTick( e, emitter, particleEmitter_tick );
	return emitter;
}

int LUA_particle_oneShot( lua_State* l ) {
	engine* e = (engine*)lua_toptr( l, 1 );
	transform* t = lua_toTransform( l, 2 );
	const char* particle_file = lua_tostring( l, 3 );
	particleEmitter* emitter = createEmitter( particle_file, t, e );
	emitter->oneshot = true;
	lua_pushptr( l, emitter );
	return 1;
}

int LUA_particle_create( lua_State* l ) {
	engine* e = (engine*)lua_toptr( l, 1 );
	transform* t = lua_toTransform( l, 2 );
	const char* particle_file = lua_tostring( l, 3 );
	particleEmitter* emitter = createEmitter( particle_file, t, e );
	lua_pushptr( l, emitter );
	return 1;
}


int LUA_particle_destroy( lua_State* l ) {
	particleEmitter* emitter = (particleEmitter*)lua_toptr( l, 1 );
	luaAssert( l, emitter );
#ifdef DEBUG_PARTICLE_LIVENESS_TEST
	particleEmitter_assertActive( emitter );
#endif // DEBUG_PARTICLE_LIVENESS_TEST
	particleEmitter_destroy( emitter );
	return 0;
}

int LUA_ribbon_create( lua_State* l ) {
	printf( "Create ribbon emitter!\n" );
	engine* e = (engine*)lua_toptr( l, 1 );
	transform* t = lua_toTransform( l, 2 );
	const char* filename = lua_tostring( l, 3 );

	ribbonEmitter* emitter = ribbonEmitter_create( ribbon_loadAsset( filename ));
	emitter->trans = t;

	engine_addRender( e, emitter, ribbonEmitter_render );
	startTick( e, emitter, ribbonEmitter_tick );
	
	lua_pushptr( l, emitter );
	return 1;
}

canyon* theCanyon; // TODO

// Get the world X,Y,Z position of a point a given DISTANCE down the canyon
int LUA_canyonPosition( lua_State* l ) {
	canyon* c = (canyon*)lua_toptr( l, 1 );
	float u = lua_tonumber( l, 2 );
	float v = lua_tonumber( l, 3 );
	float x, y, z;
	terrain_worldSpaceFromCanyon( c, u, v, &x, &z );
	y = canyonTerrain_sampleUV( u, v );
	lua_pushnumber( l, x );
	lua_pushnumber( l, y );
	lua_pushnumber( l, z );
	return 3;
}

// Get the Canyon U,V position of a point from a given world position
int LUA_canyon_fromWorld( lua_State* l ) {
	canyon* c = (canyon*)lua_toptr( l, 1 );
	vector* world_position = (vector*)lua_toptr( l, 2 );
#ifdef DEBUG_SANITY_CHECK_POINTERS
	luaAssert( l, (uintptr_t)world_position > 0xffff );
#endif // DEBUG_SANITY_CHECK_POINTERS
	float u, v;
	canyonSpaceFromWorld( c, world_position->coord.x, world_position->coord.z, &u, &v );
	lua_pushnumber( l, u );
	lua_pushnumber( l, v );
	return 2;
}
int LUA_canyonU_atWorld( lua_State* l ) {
	canyon* c = (canyon*)lua_toptr( l, 1 );
	vector* world_position = (vector*)lua_toptr( l, 2 );
#ifdef DEBUG_SANITY_CHECK_POINTERS
	luaAssert( l, (uintptr_t)world_position > 0xffff );
#endif // DEBUG_SANITY_CHECK_POINTERS
	float u, v;
	canyonSpaceFromWorld( c, world_position->coord.x, world_position->coord.z, &u, &v );
	lua_pushnumber( l, u );
	(void)v;
	return 1;
}
int LUA_canyonV_atWorld( lua_State* l ) {
	canyon* c = (canyon*)lua_toptr( l, 1 );
	vector* world_position = (vector*)lua_toptr( l, 2 );
#ifdef DEBUG_SANITY_CHECK_POINTERS
	luaAssert( l, (uintptr_t)world_position > 0xffff );
#endif // DEBUG_SANITY_CHECK_POINTERS
	float u, v;
	canyonSpaceFromWorld( c, world_position->coord.x, world_position->coord.z, &u, &v );
	lua_pushnumber( l, v );
	(void)u;
	return 1;
}

int LUA_canyonzone_fromV( lua_State* l ) {
	float v = lua_tonumber( l, 1 );
	int zone = canyon_zone( v );
	lua_pushnumber( l, zone );
	return 1;
}

int LUA_canyonzoneType_fromV( lua_State* l ) {
	canyon* c = (canyon*)lua_toptr( l, 1 );
	float v = lua_tonumber( l, 2 );
	int zone = canyon_zoneType( c, v );
	lua_pushnumber( l, zone );
	return 1;
}


int LUA_dynamicSky_blend( lua_State* l ) {
	float v = lua_tonumber( l, 1 );
	(void)v;
	float blend = canyonZone_blend( v );
	int previous = canyon_zone( v );
	int next = previous + 1;
	dynamicFog_blend( theScene->fog, previous, next, blend );
	return 0;
}

int LUA_fx_toggleBloom( lua_State* l ) {
	(void)l;
	render_bloom_enabled = !render_bloom_enabled;
	printf( "Toggling bloom: %s\n", render_bloom_enabled ? "on" : "off" );
	return 0;
}

int LUA_createUIPanel( lua_State* l ) {
	engine* e = (engine*)lua_toptr( l, 1 );
	const char* texture_path = lua_tostring( l, 2 );
	vector* ui_color = (vector*)lua_toptr( l, 3 );
	float x = lua_tonumber( l, 4 );
	float y = lua_tonumber( l, 5 );
	float w = lua_tonumber( l, 6 );
	float h = lua_tonumber( l, 7 );
	panel* p = panel_create( e );
	p->x = x;
	p->y = y;
	p->width = w;
	p->height = h;
	p->color = *ui_color;
	textureProperties* properties = (textureProperties*)mem_alloc( sizeof( textureProperties ));
	properties->wrap_s = GL_CLAMP_TO_EDGE;
	properties->wrap_t = GL_CLAMP_TO_EDGE;
	p->_texture = texture_loadWithProperties( texture_path, properties );
	engine_addRender( e, p, panel_render );
	lua_pushptr( l, p );
	return 1;
}

int LUA_createUIPanel_future( lua_State* l ) {
	engine* e = (engine*)lua_toptr( l, 1 );
	const char* texture_path = lua_tostring( l, 2 );
	vector* ui_color = (vector*)lua_toptr( l, 3 );
	float x = lua_tonumber( l, 4 );
	float y = lua_tonumber( l, 5 );
	float w = lua_tonumber( l, 6 );
	float h = lua_tonumber( l, 7 );
	
	int future_ref = lua_store( l ); // Must be top of the stack

	panel* p = panel_create( e );
	p->x = x;
	p->y = y;
	p->width = w;
	p->height = h;
	p->color = *ui_color;
	textureProperties* properties = (textureProperties*)mem_alloc( sizeof( textureProperties ));
	properties->wrap_s = GL_CLAMP_TO_EDGE;
	properties->wrap_t = GL_CLAMP_TO_EDGE;
	p->_texture = texture_loadWithProperties( texture_path, properties );
	engine_addRender( e, p, panel_render );

	lua_completeFuturePtr( l, future_ref, p );

	lua_pushptr( l, p );
	return 1;
}

int LUA_showUIPanel( lua_State* l ) {
	engine* e = (engine*)lua_toptr( l, 1 );
	panel* p = (panel*)lua_toptr( l, 2 );
	panel_show( e, p );
	return 0;
}

int LUA_hideUIPanel( lua_State* l ) {
	engine* e = (engine*)lua_toptr( l, 1 );
	panel* p = (panel*)lua_toptr( l, 2 );
	panel_hide( e, p );
	return 0;
}

int LUA_setPanelAlpha( lua_State* l ) {
	panel* p = (panel*)lua_toptr( l, 1 );
	float a = lua_tonumber( l, 2 );
	panel_setAlpha( p, a );
	return 0;
}

int LUA_panelFadeOut( lua_State* l ) {
	panel* p = (panel*)lua_toptr( l, 1 );
	float t = lua_tonumber( l, 2 );
	panel_fadeOut( p, t );	
	return 0;
}

int LUA_panelFadeIn( lua_State* l ) {
	panel* p = (panel*)lua_toptr( l, 1 );
	float t = lua_tonumber( l, 2 );
	panel_fadeIn( p, t );	
	return 0;
}

int LUA_body_setLayers( lua_State* l ) {
	body* b = (body*)lua_toptr( l, 1 );
	collision_layers_t layers = (collision_layers_t)lua_tonumber( l, 2 );
	b->layers = layers;
	return 0;
}

int LUA_body_setCollidableLayers( lua_State* l ) {
	body* b = (body*)lua_toptr( l, 1 );
	collision_layers_t layers = (collision_layers_t)lua_tonumber( l, 2 );
	b->collide_with = layers;
	return 0;
}

int LUA_createCanyon(lua_State* l) {
	engine* e = (engine*)lua_toptr( l, 1 );
	scene* s = (scene*)lua_toptr( l, 2 );
	canyon* c = canyon_create( s, "dat/script/lisp/canyon_zones.s" );
	canyon_generateInitialPoints( c );
	canyonBuffer_seek( c, 0 );
	theCanyon = c;
	startTick( e, c, canyon_tick );
	
	const int uBlocks = 9;
	const int vBlocks = 17;
	const int uSamples = 64;
	const int vSamples = 48;
	CanyonTerrain* t = canyonTerrain_create( c, uBlocks, vBlocks, uSamples, vSamples, 640.f, 960.f );

	// *** Marching cube terrain
	buildMarchingCubes( c );
	
	//CanyonTerrain* t = canyonTerrain_create( c, 7, 9, 80, 80, 640.f, 960.f );
	t->setLodIntervals( 1, 3 );
	startTick( e, (void*)t, canyonTerrain_tick );
	//engine_addRender( e, (void*)t, canyonTerrain_render );
	lua_pushptr( l, c );
	return 1;
}

int LUA_debugdraw_cross( lua_State* l ) {
	vector* center = (vector*)lua_toptr( l, 1 );
	float radius = lua_tonumber( l, 2 );
	debugdraw_cross( *center, radius, color_green );
	return 0;
}

int LUA_vector_distance( lua_State* l ) {
	const vector* a = (const vector*)lua_toptr( l, 1 );
	const vector* b = (const vector*)lua_toptr( l, 2 );
	float distance = vector_distance( a, b );
	lua_pushnumber( l, (double)distance );
	return 1;
}
int LUA_vector_add( lua_State* l ) {
	const vector* a = (const vector*)lua_toptr( l, 1 );
	const vector* b = (const vector*)lua_toptr( l, 2 );
	vector* v = lua_createVector();
	*v = vector_add( *a, *b );
	lua_pushptr( l, v );
	return 1;
}

int LUA_vector_subtract( lua_State* l ) {
	const vector* a = (const vector*)lua_toptr( l, 1 );
	const vector* b = (const vector*)lua_toptr( l, 2 );
	vector* v = lua_createVector();
	*v = vector_sub( *a, *b );
	lua_pushptr( l, v );
	return 1;
}

int LUA_vector_normalize( lua_State* l ) {
	const vector* v = (const vector*)lua_toptr( l, 1 );
	vector* v_normalized = lua_createVector();
	*v_normalized = normalized( *v );
	lua_pushptr( l, v_normalized );
	return 1;
}

int LUA_vector_dot( lua_State* l ) {
	const vector* a = (const vector*)lua_toptr( l, 1 );
	const vector* b = (const vector*)lua_toptr( l, 2 );
	lua_pushnumber( l, Dot( a, b ));
	return 1;
}

int LUA_vector_scale( lua_State* l ) {
	const vector* v = (const vector*)lua_toptr( l, 1 );
	float scale = lua_tonumber( l, 2 );
	vector* v_scaled = lua_createVector();
	*v_scaled = vector_scaled( *v, scale );
	lua_pushptr( l, v_scaled );
	return 1;
}

// *** Matrix

int LUA_transformWorldMatrix( lua_State* l ) {
	matrix* m = lua_createMatrix();
	transform* t = lua_toTransform( l, 1 );
	matrix_cpy( *m, t->world );
	lua_pushptr( l, m );
	return 1;
}

int LUA_matrix_setTranslation( lua_State* l ) {
	matrix* m = (matrix*)lua_toptr( l, 1 );
	vector* v = (vector*)lua_toptr( l, 2 );
	matrix_setTranslation( *m, v );
	return 0;
}

int LUA_matrix_facing( lua_State* l ) {
	vector* facing = (vector*)lua_toptr( l, 1 );
	vector* from = (vector*)lua_toptr( l, 2 );
	vector displacement = normalized( vector_sub( *facing, *from ));
	
	matrix* m = lua_createMatrix();
	matrix_look( *m, displacement );
	matrix_setTranslation( *m, from );
	lua_pushptr( l, m );
	return 1;
}

int LUA_matrix_toEulerAngles( lua_State* l ) {
	matrix *m = (matrix*)lua_toptr( l, 1 );
	vector ypr = matrix_toEuler( *m );
	lua_pushnumber( l, ypr.coord.x );
	lua_pushnumber( l, ypr.coord.y );
	lua_pushnumber( l, ypr.coord.z );
	return 3;
}

// *** Quaternions

int LUA_quaternion_fromTransform( lua_State* l ) {
	transform* t = lua_toTransform( l, 1 );
	quaternion* q = lua_createQuaternion();
	*q = matrix_getRotation( t->world );
	lua_pushptr( l, q );
	return 1;
}

int LUA_quaternion_look( lua_State* l ) {
	const vector* v = (const vector*)lua_toptr( l, 1 );
	matrix m;
	matrix_look( m, *v );
	quaternion* q = lua_createQuaternion();
	*q = matrix_getRotation( m );
	lua_pushptr( l, q );
	return 1;
}

int LUA_quaternion_rotation( lua_State* l ) {
	const quaternion* q = (const quaternion*)lua_toptr( l, 1 );
	const vector* v = (const vector*)lua_toptr( l, 2 );
	vector* result = lua_createVector();
	*result = quaternion_rotation( *q, *v );
	lua_pushptr( l, result );
	return 1;
}

int LUA_quaternion_slerp( lua_State* l ) {
	const quaternion* from = (const quaternion*)lua_toptr( l, 1 );
	const quaternion* to = (const quaternion*)lua_toptr( l, 2 );
	float f = lua_tonumber( l, 3 );
	quaternion* result = lua_createQuaternion();
	*result = quaternion_slerp( *from, *to, f );
	lua_pushptr( l, result );
	return 1;
}

int LUA_quaternion_slerpAngle( lua_State* l ) {
	const quaternion* from = (const quaternion*)lua_toptr( l, 1 );
	const quaternion* to = (const quaternion*)lua_toptr( l, 2 );
	float angle = lua_tonumber( l, 3 );
	quaternion* result = lua_createQuaternion();
	*result = quaternion_slerpAngle( *from, *to, angle );
	lua_pushptr( l, result );
	return 1;
}

// C = A | B
int LUA_bit_OR( lua_State* l ) {
	int a = lua_tonumber( l, 1 );
	int b = lua_tonumber( l, 2 );
	int c = a | b;
	lua_pushnumber( l, c );
	return 1;
}

// C = A @ B
int LUA_bit_AND( lua_State* l ) {
	int a = lua_tonumber( l, 1 );
	int b = lua_tonumber( l, 2 );
	int c = a & b;
	lua_pushnumber( l, c );
	return 1;
}

int LUA_rand_newSeq( lua_State* l ) {
	randSeq* random_sequence = (randSeq*)mem_alloc( sizeof( randSeq ));
	random_sequence->buffer[0] = 0x1;
	random_sequence->buffer[1] = 0x2;
	random_sequence->buffer[2] = 0x3;
	deterministic_seedRandSeq( 0x0, random_sequence );
	lua_pushptr( l, random_sequence );
	return 1;
}

int LUA_rand( lua_State* l ) {
	randSeq* random_sequence = (randSeq*)lua_toptr( l, 1 );
	float floor = lua_tonumber( l, 2 );
	float ceiling = lua_tonumber( l, 3 );
	float rand = deterministic_frand( random_sequence, floor, ceiling );
	lua_pushnumber( l, rand );
	return 1;
}

int LUA_pause( lua_State* l ) {
	engine* e = (engine*)lua_toptr( l, 1 );
	e->paused = true;
	return 0;
}

int LUA_unpause( lua_State* l ) {
	engine* e = (engine*)lua_toptr( l, 1 );
	e->paused = false;
	return 0;
}

void lua_completeFuturePtr( lua_State* l, int future_ref, void* ptr ) {
	// we want to call f:complete( ptr )	
	// Get table(future, "complete")
	lua_getglobal( l, "future" );
	lua_getfield( l, -1, "complete" );
	// push the future
	lua_retrieve( l, future_ref );
	// push the ptr
	lua_pushptr( l, ptr );
	// execute
	lua_pcall( l, 2, 0, 0 );
}

// ***


void lua_keycodes( lua_State* l ) {
	lua_newtable( l ); // Create a table
	char capital_offset = 'A' - 'a';
	for ( char i = 'a'; i <= 'z'; i++ ) {
		char string[2];
		string[0] = i;
		string[1] = '\0';
		lua_setfieldi( l, string, i + capital_offset );
	}
	lua_setfieldi( l, "w", KEY_W );
	lua_setfieldi( l, "a", KEY_A );
	lua_setfieldi( l, "s", KEY_S );
	lua_setfieldi( l, "d", KEY_D );
	lua_setfieldi( l, "q", KEY_Q );
	lua_setfieldi( l, "c", KEY_C );
	lua_setfieldi( l, "p", KEY_P );
	lua_setfieldi( l, "up", KEY_UP );
	lua_setfieldi( l, "down", KEY_DOWN );
	lua_setfieldi( l, "left", KEY_LEFT );
	lua_setfieldi( l, "right", KEY_RIGHT );
	lua_setfieldi( l, "space", KEY_SPACE );
	lua_setglobal( l, "key" ); // store the table in the 'key' global variable
}


void luaLibrary_import( lua_State* l ) {

	/////////////// Functions /////////////////

	// *** Bitwise
	lua_registerFunction( l, LUA_bit_OR, "bitwiseOR" );
	lua_registerFunction( l, LUA_bit_AND, "bitwiseAND" );

	// *** General
	lua_registerFunction( l, LUA_vassert, "vassert" );
	lua_registerFunction( l, LUA_registerCallback, "registerEventHandler" );
	lua_registerFunction( l, LUA_print, "vprint" );
	lua_registerFunction( l, LUA_debugdraw_cross, "vdebugdraw_cross" );
	lua_registerFunction( l, LUA_rand, "vrand" );
	lua_registerFunction( l, LUA_rand_newSeq, "vrand_newSeq" );
	lua_registerFunction( l, LUA_pause, "vpause" );
	lua_registerFunction( l, LUA_unpause, "vunpause" );

	// *** Vector
	lua_registerFunction( l, LUA_vector, "Vector" );
	lua_registerFunction( l, LUA_vector_values, "vvector_values" );
	lua_registerFunction( l, LUA_vector_distance, "vvector_distance" );
	lua_registerFunction( l, LUA_vector_subtract, "vvector_subtract" );
	lua_registerFunction( l, LUA_vector_add, "vvector_add" );
	lua_registerFunction( l, LUA_vector_normalize, "vvector_normalize" );
	lua_registerFunction( l, LUA_vector_scale, "vvector_scale" );
	lua_registerFunction( l, LUA_vector_dot, "vvector_dot" );

	// *** Matrix
	lua_registerFunction( l, LUA_transformWorldMatrix, "vtransformWorldMatrix" );
	lua_registerFunction( l, LUA_matrix_setTranslation, "vmatrix_setTranslation" );
	lua_registerFunction( l, LUA_matrix_toEulerAngles, "vmatrix_toEulerAngles" );
	lua_registerFunction( l, LUA_matrix_facing, "vmatrix_facing" );

	// *** Quaternion
	lua_registerFunction( l, LUA_quaternion_fromTransform, "vquaternion_fromTransform" );
	lua_registerFunction( l, LUA_quaternion_look, "vquaternion_look" );
	lua_registerFunction( l, LUA_quaternion_rotation, "vquaternion_rotation" );
	lua_registerFunction( l, LUA_quaternion_slerp, "vquaternion_slerp" );
	lua_registerFunction( l, LUA_quaternion_slerpAngle, "vquaternion_slerpAngle" );

	// *** Input
	lua_registerFunction( l, LUA_keyPressed, "vkeyPressed" );
	lua_registerFunction( l, LUA_keyHeld, "vkeyHeld" );
	lua_registerFunction( l, LUA_touchPressed, "vtouchPressed" );
	lua_registerFunction( l, LUA_touchHeld, "vtouchHeld" );
	lua_registerFunction( l, LUA_createTouchPad, "vcreateTouchPad" );
	lua_registerFunction( l, LUA_touchPadTouched, "vtouchPadTouched" );
	lua_registerFunction( l, LUA_touchPadDragged, "vtouchPadDragged" );
	lua_registerFunction( l, LUA_createGesture, "vgesture_create" );
	lua_registerFunction( l, LUA_gesture_performed, "vgesture_performed" );

	// *** Scene
	lua_registerFunction( l, LUA_createModelInstance, "vcreateModelInstance" );
	lua_registerFunction( l, LUA_modelInstanceSetStatic, "vmodelInstanceSetStatic" );
	lua_registerFunction( l, LUA_modelPreload, "vmodel_preload" );
	lua_registerFunction( l, LUA_deleteModelInstance, "vdeleteModelInstance" );
	lua_registerFunction( l, LUA_model_setTransform, "vmodel_setTransform" );
	lua_registerFunction( l, LUA_setWorldSpacePosition, "vsetWorldSpacePosition" );
	lua_registerFunction( l, LUA_scene_addModel, "vscene_addModel" );
	lua_registerFunction( l, LUA_scene_removeModel, "vscene_removeModel" );
	lua_registerFunction( l, LUA_scene_setSkyColor, "vscene_setSkyColor" );
	lua_registerFunction( l, LUA_scene_setFogColor, "vscene_setFogColor" );
	lua_registerFunction( l, LUA_scene_setSunColor, "vscene_setSunColor" );
	lua_registerFunction( l, LUA_texturePreload, "vtexture_preload" );

	// *** Terrain
	lua_registerFunction( l, LUA_createCanyon, "vcanyon_create" );

	// *** Physic
	lua_registerFunction( l, LUA_createphysic, "vcreatePhysic" );
	lua_registerFunction( l, LUA_physic_setTransform,	"vphysic_setTransform" );
	lua_registerFunction( l, LUA_physic_activate,		"vphysic_activate" );
	lua_registerFunction( l, LUA_physic_setVelocity,	"vphysic_setVelocity" );
	lua_registerFunction( l, LUA_physic_destroy,		"vphysic_destroy" );

	// *** Transform
	lua_registerFunction( l, LUA_createTransform, "vcreateTransform" );
	lua_registerFunction( l, LUA_transform_yaw, "vtransform_yaw" );
	lua_registerFunction( l, LUA_transform_pitch, "vtransform_pitch" );
	lua_registerFunction( l, LUA_transform_roll, "vtransform_roll" );
	lua_registerFunction( l, LUA_transformVector, "vtransformVector" );
	lua_registerFunction( l, LUA_transform_setLocalPosition, "vtransform_setLocalPosition" );
	lua_registerFunction( l, LUA_transform_setWorldPosition, "vtransform_setWorldPosition" );
	lua_registerFunction( l, LUA_transform_getWorldPosition, "vtransform_getWorldPosition" );
	lua_registerFunction( l, LUA_transform_setWorldSpaceByTransform, "vtransform_setWorldSpaceByTransform" );
	lua_registerFunction( l, LUA_transform_destroy, "vdestroyTransform" );
	lua_registerFunction( l, LUA_transform_facingWorld, "vtransform_facingWorld" );
	lua_registerFunction( l, LUA_transform_eulerAngles, "vtransform_eulerAngles" );
	lua_registerFunction( l, LUA_transform_distance, "vtransform_distance" );
	lua_registerFunction( l, LUA_transform_setRotation, "vtransform_setRotation" );
	lua_registerFunction( l, LUA_transform_setWorldSpaceByMatrix, "vtransform_setWorldSpaceByMatrix" );
	lua_registerFunction( l, LUA_transform_valid, "vtransform_valid" );

	// *** Particles
	lua_registerFunction( l, LUA_particle_create, "vparticle_create" );
	lua_registerFunction( l, LUA_particle_oneShot, "vparticle_oneShot" );
	lua_registerFunction( l, LUA_particle_destroy, "vparticle_destroy" );
	
	// *** Ribbons
	lua_registerFunction( l, LUA_ribbon_create, "vribbon_create" );

	// *** Collision
	lua_registerFunction( l, LUA_createbodySphere, "vcreateBodySphere" );
	lua_registerFunction( l, LUA_createbodyMesh, "vcreateBodyMesh" );
	lua_registerFunction( l, LUA_body_setTransform, "vbody_setTransform" );
	lua_registerFunction( l, LUA_body_registerCollisionCallback, "vbody_registerCollisionCallback" );
	lua_registerFunction( l, LUA_body_destroy, "vdestroyBody" );
	lua_registerFunction( l, LUA_body_setCollidableLayers, "vbody_setCollidableLayers" );
	lua_registerFunction( l, LUA_body_setLayers, "vbody_setLayers" );

	// *** Camera
	lua_registerFunction( l, LUA_chasecam_follow, "vchasecam_follow" );
	lua_registerFunction( l, LUA_flycam, "vflycam" );
	lua_registerFunction( l, LUA_setCamera, "vscene_setCamera" );
	lua_registerFunction( l, LUA_cameraPosition, "vcamera_position" );

	// *** UI
	lua_registerFunction( l, LUA_createUIPanel, "vuiPanel_create" );
	lua_registerFunction( l, LUA_createUIPanel_future, "vuiPanel_create_future" );
	lua_registerFunction( l, LUA_hideUIPanel, "vuiPanel_hide" );
	lua_registerFunction( l, LUA_showUIPanel, "vuiPanel_show" );
	lua_registerFunction( l, LUA_setPanelAlpha, "vuiPanel_setAlpha" );
	lua_registerFunction( l, LUA_panelFadeIn, "vuiPanel_fadeIn" );
	lua_registerFunction( l, LUA_panelFadeOut, "vuiPanel_fadeOut" );
	

	// *** Game
	lua_registerFunction( l, LUA_canyonPosition, "vcanyon_position" );
	lua_registerFunction( l, LUA_canyon_fromWorld, "vcanyon_fromWorld" );
	lua_registerFunction( l, LUA_canyonU_atWorld, "vcanyonU_atWorld" );
	lua_registerFunction( l, LUA_canyonV_atWorld, "vcanyonV_atWorld" );
	lua_registerFunction( l, LUA_canyonzone_fromV, "vcanyonzone_fromV" );
	lua_registerFunction( l, LUA_canyonzoneType_fromV, "vcanyonzoneType_fromV" );
	lua_registerFunction( l, LUA_dynamicSky_blend, "vdynamicSky_blend" );

	// *** fx
	lua_registerFunction( l, LUA_fx_toggleBloom, "vfx_toggleBloom" );
	/////////////// Constants /////////////////

	lua_keycodes( l );

#ifdef TOUCH
	lua_setConstant_bool( l, "touch_enabled", true );
#else
	lua_setConstant_bool( l, "touch_enabled", false );
#endif
}
