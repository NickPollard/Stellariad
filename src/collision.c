// collision.c
#include "common.h"
#include "collision.h"
//---------------------
#include "engine.h"
#include "model.h"
#include "test.h"
#include "transform.h"
#include "worker.h"
#include "base/ringqueue.h"
#include "maths/geometry.h"
#include "render/debugdraw.h"

#define kDeadBodyQueueSize 128
#define kNewBodyQueueSize 128

int collision_results_ready = 0;
int collision_results_processed = 0;

collideFunc collide_funcs[kMaxShapeTypes][kMaxShapeTypes];

collisionEvent collision_events[kMaxCollisionEvents];
int event_count;

body* bodies[kMaxCollidingBodies];
int body_count;

ringQueue* collision_dead_body_queue = NULL;
ringQueue* collision_new_body_queue = NULL;

vmutex collision_add_mutex = kMutexInitialiser;

// Forward Declarations
bool body_colliding( body* a, body* b );
bool collisionFunc_SphereHeightfield( shape* sphere_shape, shape* height_shape, matrix matrix_sphere, matrix matrix_heightfield );
bool collisionFunc_HeightfieldSphere( shape* height_shape, shape* sphere_shape, matrix matrix_heightfield, matrix matrix_sphere );
void shape_delete( shape* s );

static inline vector heightField_vertex( heightField* h, int x, int z ) { 
	int index = x + z * h->x_samples;
	vAssert( index >= 0 && index < h->vert_count );
	return h->verts[index];
}

void body_delete( body* b ) {
	mem_free( b );
}

void collision_clearEvents() {
	memset( collision_events, 0, sizeof( collisionEvent ) * kMaxCollisionEvents );
	event_count = 0;
}

void collision_event( body* a, body* b ) {
	vAssert( event_count >= 0 );
	vAssert( event_count < kMaxCollisionEvents );
	collisionEvent* event = &collision_events[event_count++];
	event->a = a;
	event->b = b;
	//printf( "Collision! 0x" xPTRf " 0x" xPTRf "\n", (uintptr_t)a, (uintptr_t)b );
}

void collision_addBody( body* b ) {
	vmutex_lock( &collision_add_mutex );
	{
		ringQueue_push( collision_new_body_queue, b );
	}
	vmutex_unlock( &collision_add_mutex );
}

void collision_addNewBody( body* b ) {
	vAssert( body_count < kMaxCollidingBodies );
	bodies[body_count++] = b;
}

void collision_addNewBodies() {
	while ( ringQueue_count( collision_new_body_queue ) > 0 ) {
		collision_addNewBody( ringQueue_pop( collision_new_body_queue ));
	}
}

#define kMaxDeadBodies 128
int dead_body_count = 0;
body* dead_bodies[kMaxDeadBodies];


void collision_removeBody( body* b ) {
	vAssert( ringQueue_count( collision_dead_body_queue ) < collision_dead_body_queue->size );
	ringQueue_push( collision_dead_body_queue, b );
	b->disabled = true;
}

void collision_removeDeadBody( body* b ) {
	array_remove( (void**)bodies, &body_count, b );
	vAssert( b );
	vAssert( b->shape );
	shape_delete( b->shape );
	body_delete( b );
}

void collision_removeDeadBodies( ) {
	while ( ringQueue_count( collision_dead_body_queue ) > 0 ) {
		collision_removeDeadBody( ringQueue_pop( collision_dead_body_queue ));
	}
}

void collision_callback( body* a, body* b ) {
	if ( a->callback )
		a->callback( a, b, a->callback_data );
}

void collision_runCallbacks() {
	for ( int i = 0; i < event_count; ++i ) {
		collision_callback( collision_events[i].a, collision_events[i].b );
		collision_callback( collision_events[i].b, collision_events[i].a );
	}
}

void collision_generateEvents() {
	// for every body, check every other body
	// TODO - keep separate lists for layers; iterate those instead
	for ( int i = 0; i < body_count; ++i )
		for ( int j = i + 1; j < body_count; j++ ) {
			vAssert( bodies[i] )
			vAssert( bodies[j] )
			if ( !bodies[i]->disabled && !bodies[j]->disabled && body_colliding( bodies[i], bodies[j] ))
				collision_event( bodies[i], bodies[j] );
		}
}

void collisionMesh_drawWireframe( collisionMesh* m, matrix trans, vector color ) {
	debugdraw_wireframeMesh( m->vert_count, m->verts, m->index_count, m->indices, trans, color );
}

void heightfield_drawWireframe( heightField* h, matrix trans, vector color ) {
	(void)trans;
	//printf( "Drawing heightfield.\n" );
	for ( int u = 0; u < h->x_samples - 1; ++u ) {
		for ( int v = 0; v < h->z_samples - 1; ++v ) {
			vector a = heightField_vertex( h, u, v );
			vector b = heightField_vertex( h, u+1, v );
			vector c = heightField_vertex( h, u, v+1 );
			vector d = heightField_vertex( h, u+1, v+1 );
			debugdraw_line3d( a, b, color );
			debugdraw_line3d( b, d, color );
			debugdraw_line3d( d, c, color );
			debugdraw_line3d( c, a, color );
			(void)c;
		}
	}
}

void body_debugdraw( body* b ) {
	switch ( b->shape->type ) {
		case shapeInvalid:
			break;
		case shapeSphere:
			{
				const vector* position = transform_getWorldPosition( b->trans );
				debugdraw_sphere( *position, b->shape->radius, color_green );
			}
			break;
		case shapeMesh:
			collisionMesh_drawWireframe( b->shape->collision_mesh, b->trans->world, color_green );
			break;
		case shapeHeightField:
			//heightfield_drawWireframe( b->shape->height_field, b->trans->world, color_green );
			break;
	}
}

void collision_debugdraw() {
	for ( int i = 0; i < body_count; ++i ) {
		body_debugdraw( bodies[i] );
	}
}

// Check for any collisions this frame
void collision_tick( int frame_counter, float dt ) {
	(void)dt;

	while ( collision_results_ready > collision_results_processed ) {
		//printf( "tick waiting.\n" );
		vthread_yield();
	}

	collision_clearEvents();
	collision_removeDeadBodies();
	collision_addNewBodies();
	collision_generateEvents();

	// memory write barrier?

	collision_results_ready = frame_counter;
	//collision_results_processed = false;

	//collision_debugdraw();
	//printf( "Total collision bodies: %d.\n", body_count );
}

void collision_processResults( int frame_counter, float dt ) {
	(void)dt;

	if ( frame_counter == 0 )
		return;

	while ( collision_results_ready <= collision_results_processed ) { // Should never actually be less than, should be ==, but I think this is clearer
		//printf( "process waiting.\n" );
		// TODO - Should use a condition variable/wake-up signal here
		vthread_yield();
	}

	collision_runCallbacks();

	// Memory Write Barrier?

	collision_results_processed = frame_counter;
}


typedef struct collisionArgs_s {
	float dt;
	int frame_counter;
} collisionArgs;

collisionArgs collision_args;

void* collision_workerTick( void* args ) {
	//printf( "Worker collision.\n" );
	collisionArgs* a = args;
	collision_tick( a->frame_counter, a->dt );
	//mem_free( args );
	return NULL;
}

void collision_queueWorkerTick( int frame_counter, float dt ) {
	//collisionArgs* args = mem_alloc( sizeof( collisionArgs ));
	collision_args.dt = dt;
	collision_args.frame_counter = frame_counter;
	worker_task collision_task;
	collision_task.func = collision_workerTick;
	collision_task.args = &collision_args;
	worker_addImmediateTask( collision_task );
}

bool collisionFunc_SphereSphere( shape* a, shape* b, matrix matrix_a, matrix matrix_b ) {
	(void)matrix_a; (void)matrix_b;
	vector origin_a = matrix_vecMul( matrix_a, &a->origin );
	vector origin_b = matrix_vecMul( matrix_b, &b->origin );
	return vector_distance( &origin_a, &origin_b ) < ( a->radius + b->radius );
}

bool collisionFunc_MeshSphere( shape* mesh_, shape* sphere_, matrix matrix_mesh, matrix matrix_sphere ) {
	(void)mesh_;
	(void)sphere_;
	(void)matrix_mesh;
	(void)matrix_sphere;

	return false;
}

// Just swap the types around
bool collisionFunc_SphereMesh( shape* sphere_, shape* mesh_, matrix matrix_sphere, matrix matrix_mesh ) {
	return collisionFunc_MeshSphere( mesh_, sphere_, matrix_mesh, matrix_sphere );
}

bool triangle_intersectingMesh( vector a, vector b, vector c, collisionMesh* m ) {
	(void)a;
	(void)b;
	(void)c;
	(void)m;
	//vAssert( 0 ); //NYI
	return false;
}


vector line_normal( vector a, vector b ) {
	// TODO
	float x = b.coord.x - a.coord.x;
	float z = b.coord.z - a.coord.z;
	vector normal = Vector( z, 0.f, -x, 0.f );
	Normalize( &normal, &normal );
	return normal;
}

// Using just X and Y
bool line_intersect2d( vector point, vector dir, vector a, vector b ) {
	/*
	printf( "line_intersect2d: point: (" );
	vector_print( &point );
	printf( " ), edge ( " );
	vector_print( &a );
	printf( " ) --> ( " );
	vector_print( &b );
	printf( " )\n" );
	*/

	if ( vector_equal( &a, &b )) {
		// Degenerate triangle, so no intersection
		return false;
	}
	vector normal = line_normal( a, b );
	vector offset;
	Sub( &offset, &a, &point );
	float dir_dot_normal = Dot( &dir, &normal );
	if ( f_eq( dir_dot_normal, 0.f )) {
		// If it's parellel to the edge, it will never cross
		return false;
	}
	float d = Dot( &offset, &normal ) / dir_dot_normal;
	vector intersection;
	vector_scale( &intersection, &dir, d ); 
	Add( &intersection, &intersection, &point );
	//printf( "dir_dot_normal: %.2f, intersection.y %.2f\n", dir_dot_normal, intersection.coord.y );
	vAssert( f_eq( intersection.coord.y, 0.f ));

	vector line = normalized( vector_sub( b, a ));
	float d_a = Dot( &a, &line );
	float d_b = Dot( &b, &line );
	float d_int = Dot( &intersection, &line );

	// Line must be in front of us in the DIR we are tracing
	float point_d = Dot( &point, &dir );
	float line_d = Dot( &intersection, &dir );
	//printf( "point_d: %.2f, line_d: %.2f\n", point_d, line_d );

	/*
	if ( d_int >= d_a && d_int <= d_b && line_d >= point_d ) {
		printf( "Intersects.\n" );
	}
	else {
		printf( "No intersect.\n" );
	}
	*/
	return ( d_int >= d_a && d_int <= d_b && line_d >= point_d );
}

// Is a point inside the triangle
bool point_insideTriangle( vector point, vector a, vector b, vector c ) {
	// Extend a line from the point in one direction, count how many edges it crosses
	// Odd number: inside
	// Event number: outside
	int intersections = 0;

	//project onto the plane
	point.coord.y = 0.f;
	a.coord.y = 0.f;
	b.coord.y = 0.f;
	c.coord.y = 0.f;

	vector ab, bc, ca;
	Sub( &ab, &b, &b );
	Sub( &bc, &c, &a );
	Sub( &ca, &a, &c );

	intersections += line_intersect2d( point, z_axis, a, b ) ? 1 : 0;
	intersections += line_intersect2d( point, z_axis, b, c ) ? 1 : 0;
	intersections += line_intersect2d( point, z_axis, c, a ) ? 1 : 0;

	//printf( "Intersections: %d.\n", intersections );
	return (intersections % 2 ) == 1;
}

int line_intersectsTriangle( vector point, vector line, vector a, vector b, vector c ) {
	vAssert( isNormalized( &line ));

	// Calculate the plane of the 3 points
	vector plane_normal;
	float plane_d;
	plane( a, b, c, &plane_normal, &plane_d );

	float diff = plane_d - Dot( &point, &plane_normal );
	float line_dot_normal = Dot( &line, &plane_normal );
	// If the plane is parallel to the line, then return true if the point is on the plane
	if ( f_eq( line_dot_normal, 0.f )) {
		return f_eq( diff, 0.f );
	}
	vector intersection;
	vector_scale( &intersection, &line, diff / line_dot_normal );
	Add( &intersection, &intersection, &point );
	
	float d = Dot( &point, &line );
	float d_intersect = Dot( &intersection, &line );

	//printf( "d: %.2f, d_intersect: %.2f\n", d, d_intersect );

	if ( d_intersect < d ) {
		return false;
	}
	else {
		// Test that the intersection is inside the triangle
		if ( point_insideTriangle( point, a, b, c ) ) {
			//printf( "d: %.2f, d_intersect: %.2f\n", d, d_intersect );
			return true;
		}
		else {
			return false;
		}
	}
}

// Vert and Mesh must be in the same coordinate space now
bool vertex_insideMesh( vector v, collisionMesh* m ) {
	// Extend a line from the point in one direction, count how many polygons it crosses
	// Odd number: inside
	// Event number: outside
	int intersections = 0;
	for ( int i = 0; i < m->index_count; i = i+3 ) {
		vector a = m->verts[m->indices[i+0]];
		vector b = m->verts[m->indices[i+1]];
		vector c = m->verts[m->indices[i+2]];
		intersections += line_intersectsTriangle( v, y_axis, a, b, c );
	}

	return ( intersections % 2 ) == 1;
}

bool mesh_insideMesh( collisionMesh* mesh_a, collisionMesh* mesh_b, matrix matrix_a, matrix matrix_b ) {
	// We build a combined matrix that translates verts from a-space into b-space
	// So we can then do the collision tests purely in b-space
	matrix a_space_to_b_space;
	matrix inv_b;
	matrix_inverse( inv_b, matrix_b );
	matrix_mul( a_space_to_b_space, inv_b, matrix_a );

	// Then test each vertex of MESH_A for whether it's wholly inside the other mesh
	bool inside = false;
	for ( int i = 0; !inside && ( i < mesh_a->vert_count ); ++i ) {
		vector vert = matrix_vecMul( a_space_to_b_space, &mesh_a->verts[i] );
		inside |= vertex_insideMesh( vert, mesh_b );
	}
	return inside;
}

bool mesh_intersectingMesh( collisionMesh* mesh_a, collisionMesh* mesh_b, matrix matrix_a, matrix matrix_b ) {
	// We build a combined matrix that translates verts from a-space into b-space
	// So we can then do the collision tests purely in b-space
	matrix a_space_to_b_space;
	matrix inv_b;
	matrix_inverse( inv_b, matrix_b );
	matrix_mul( a_space_to_b_space, inv_b, matrix_a );

	bool intersecting = false;
	for ( int i = 0; !intersecting && i < mesh_a->index_count; i = i+3 ) {
		vector a = matrix_vecMul( a_space_to_b_space, &mesh_a->verts[mesh_a->indices[i+0]]);
		vector b = matrix_vecMul( a_space_to_b_space, &mesh_a->verts[mesh_a->indices[i+1]]);
		vector c = matrix_vecMul( a_space_to_b_space, &mesh_a->verts[mesh_a->indices[i+2]]);
		intersecting |= triangle_intersectingMesh( a, b, c, mesh_b );
	}
	return intersecting;
}

bool collisionFunc_MeshMesh( shape* mesh_a, shape* mesh_b, matrix matrix_a, matrix matrix_b ) {
	// There are 3 possible situations here
	// 1. The meshes are not colliding at all
	// 2. At least one mesh has at least one vertex inside the other
	// 3. No vertices are inside the other mesh, but there are intersecting edges/tris
	printf( "MeshMesh! First mesh has %d verts, second has %d\n", mesh_a->collision_mesh->vert_count, mesh_b->collision_mesh->vert_count );
	
	bool colliding = false;
	// Test each vertex to see if it's wholly inside the other mesh
	// and vica versa
	colliding |= mesh_insideMesh( mesh_a->collision_mesh, mesh_b->collision_mesh, matrix_a, matrix_b );
	if ( !colliding ) {
		colliding |= mesh_insideMesh( mesh_b->collision_mesh, mesh_a->collision_mesh, matrix_b, matrix_a );
		if ( !colliding ) {
			// Test for intersections of triangles (without intersecting vertices)
			colliding |= mesh_intersectingMesh( mesh_b->collision_mesh, mesh_a->collision_mesh, matrix_b, matrix_a );
		}
	}
	
	return colliding;
}

bool collisionFunc_invalid( shape* a, shape* b, matrix matrix_a, matrix matrix_b ) {
	printf( "Error, attempting to use invalid shape collision func. Shape tpes %d and %d.\n", a->type, b->type );
	vAssert( false );
	(void)matrix_a;
	(void)matrix_b;
}

void collision_initCollisionFuncs() {
	for ( int i = 0; i < kMaxShapeTypes; ++i ) {
		for ( int j = 0; j < kMaxShapeTypes; ++j ) {
			collide_funcs[i][j] = collisionFunc_invalid;
		}
	}
	collide_funcs[shapeSphere][shapeSphere] = collisionFunc_SphereSphere;
	collide_funcs[shapeMesh][shapeSphere] = collisionFunc_MeshSphere;
	collide_funcs[shapeSphere][shapeMesh] = collisionFunc_SphereMesh;
	collide_funcs[shapeMesh][shapeMesh] = collisionFunc_MeshMesh;

	collide_funcs[shapeSphere][shapeHeightField] = collisionFunc_SphereHeightfield;
	collide_funcs[shapeHeightField][shapeSphere] = collisionFunc_HeightfieldSphere;
}

collideFunc collision_func( enum shapeType type_a, enum shapeType type_b ) {
	return collide_funcs[type_a][type_b];
}

bool shape_colliding( shape* a, shape* b, matrix matrix_a, matrix matrix_b ) {
	vAssert( a );
	vAssert( b );
	collideFunc collide_func = collision_func( a->type, b->type );
	return collide_func( a, b, matrix_a, matrix_b );
}

bool body_colliding( body* a, body* b ) {
	//vAssert( a->trans );
	//vAssert( b->trans );
	bool test_collision =	( a->collide_with & b->layers ) |
	   						( a->layers & b->collide_with );
	return test_collision && shape_colliding( a->shape, b->shape, a->trans->world, b->trans->world );
}

bool body_collided( body* b ) {
	// Look for any event with this body
	for ( int i = 0; i < event_count; ++i )
		if ( collision_events[i].a == b || collision_events[i].b == b )
			return true;
	return false;
}

bool body_collidedBody( body* a, body* b ) {
	(void)a; (void)b;
	return false;
}

shape* sphere_create( float radius ) {
	shape* s = mem_alloc( sizeof( shape ));
	s->type = shapeSphere;
	s->radius = radius;
	s->origin = Vector( 0.f, 0.f, 0.f, 1.f );
	return s;
}

/*
void vertexBuffer_asPositionsOnly( vector* verts, vertex* src_verts, int vert_count ) {
	for ( int i = 0; i < vert_count; ++i ) {
		verts[i] = src_verts[i].position;
	}
}
*/

collisionMesh* collisionMesh_fromRenderMesh( mesh* render_mesh ) {
	collisionMesh* m = mem_alloc( sizeof( collisionMesh ));
	m->vert_count = render_mesh->vert_count;
	m->index_count = render_mesh->index_count;

	// Allocate our buffers
	m->verts = mem_alloc( sizeof( vector ) * m->vert_count );
	m->indices = mem_alloc( sizeof( m->indices[0] ) * m->index_count );
	
	// Now fill them
	memcpy( m->indices, render_mesh->indices, sizeof( m->indices[0] ) * m->index_count );
	memcpy( m->verts, render_mesh->verts, sizeof( m->verts[0] ) * m->vert_count );
	return m;
}


shape* mesh_createFromRenderMesh( mesh* render_mesh ) {
	shape* s = mem_alloc( sizeof( shape ));
	s->type = shapeMesh;
	s->collision_mesh = collisionMesh_fromRenderMesh( render_mesh );
	return s;
}


//
//
//
//




int heightField_xPosition( heightField* h, float x_sample ) {
	float x_per_sample = h->width / ((float)h->x_samples - 1.f );
	float x_min = h->width * -0.5f;
	return floorf(( x_sample - x_min ) / x_per_sample );
}

int heightField_zPosition( heightField* h, float z_sample ) {
	float z_per_sample = h->length / ((float)h->z_samples - 1.f );
	float z_min = h->length * -0.5f;
	return floorf(( z_sample - z_min ) / z_per_sample );
}

// Sample a single plane on a heightfield at X and Y
// We assume that we already know we are above this plane
float heightField_plane_sample( vector a, vector b, vector c, float x, float z ) {
	vector normal;
	float d;
	plane( a, b, c, &normal, &d );
	float y = ( d - normal.coord.x * x - normal.coord.z * z ) / normal.coord.y; 
	return y;	
}

// Sample the given heightfield at the given coordinate X, Z, returning the height Y
float heightField_sample( heightField* h, float x, float z ) {
	// We need to find which polygon we're over
	int x_pos = heightField_xPosition( h, x );
	int z_pos = heightField_zPosition( h, z );

	// At this point we know it lies in the square x_pos -> x_pos+1, z_pos -> z_pos + 1
	// We need to find out which triangle it lies over out of the two possible
	// Triangles always go the same way on a height field
	vector xz = heightField_vertex( h, x_pos, z_pos );
	vector xz_ = heightField_vertex( h, x_pos, z_pos + 1 );
	vector x_z = heightField_vertex( h, x_pos + 1, z_pos );
	vector x_z_ = heightField_vertex( h, x_pos + 1, z_pos + 1 );

	float x_per_sample = h->width / ((float)h->x_samples - 1.f );
	float z_per_sample = h->length / ((float)h->z_samples - 1.f );
	float x_offset = ( x - xz.coord.x ) / x_per_sample;
	float z_offset = ( z - xz.coord.z ) / z_per_sample;

	if ( x_offset + z_offset > 1.f ) {
		// Far triangle
		return heightField_plane_sample( x_z_, x_z, xz_, x, z );
	} else {
		// Near triangle
		return heightField_plane_sample( xz, xz_, x_z, x, z );
	}
}


bool polygon2d_contains( vector a, vector b, vector c, vector d, vector point ) {
	vector line = normal2d( vector_sub( a, b ) );
	bool outside = Dot( &line, &point ) > Dot( &line, &a );
	if ( outside )
		return false;
	
	line = normal2d( vector_sub( b, c ) );
	outside = Dot( &line, &point ) > Dot( &line, &b );
	if ( outside )
		return false;

	line = normal2d( vector_sub( c, d ) );
	outside = Dot( &line, &point ) > Dot( &line, &c );
	if ( outside )
		return false;
	
	line = normal2d( vector_sub( d, a ) );
	outside = Dot( &line, &point ) > Dot( &line, &d );
	if ( outside )
		return false;

	return true;
}

void heightField_calculateAABB( heightField* h ) {
	h->aabb.x_max = -FLT_MAX;
	h->aabb.x_min = FLT_MAX;
	h->aabb.z_max = -FLT_MAX;
	h->aabb.z_min = FLT_MAX;
	for ( int i = 0; i < h->x_samples; i++ ) {
		for ( int j = 0; j < h->z_samples; j++ ) {
			vector v = heightField_vertex( h, i, j );
			h->aabb.x_max = fmaxf( h->aabb.x_max, v.coord.x );
			h->aabb.x_min = fminf( h->aabb.x_min, v.coord.x );
			h->aabb.z_max = fmaxf( h->aabb.z_max, v.coord.z );
			h->aabb.z_min = fminf( h->aabb.z_min, v.coord.z );
		}
	}
}

bool AABBcontains( aabb2d aabb, float x, float z ) {
	return x <= aabb.x_max &&
			x >= aabb.x_min &&
			z <= aabb.z_max &&
			z >= aabb.z_min;
}

bool heightField_contains( heightField* h, float x, float z ) {
	if ( !AABBcontains( h->aabb, x, z ) ) { 
		//printf( "AABB does not contain point.\n" );
		return false;
	}
	
	return true;
}

bool heightField_collides( heightField* h, vector point ) {
	//vector_printf( "Testing heightfield against point: ", &point );
	// Check every possible polygon?
	for ( int i = 0; i < h->x_samples - 1; i++ ) {
		for ( int j = 0; j < h->z_samples - 1; j++ ) {
			// polygon = i -> i+1, j -> j+1
			vector a = heightField_vertex( h, i, j );
			vector b = heightField_vertex( h, i+1, j );
			vector c = heightField_vertex( h, i, j+1 );
			vector d = heightField_vertex( h, i+1, j+1 );

			// Temp AABB check to weed out triangles for debugging
			aabb2d aabb;
			/*
			aabb.x_max = fmaxf( a.coord.x, fmaxf( b.coord.x, fmaxf( c.coord.x, d.coord.x )));
			aabb.x_min = fminf( a.coord.x, fminf( b.coord.x, fminf( c.coord.x, d.coord.x )));
			aabb.z_max = fmaxf( a.coord.z, fmaxf( b.coord.z, fmaxf( c.coord.z, d.coord.z )));
			aabb.z_min = fminf( a.coord.z, fminf( b.coord.z, fminf( c.coord.z, d.coord.z )));
			*/
			// We know that +ve i correlates with +ve x, and same for j and z
			//aabb.x_max = fmaxf( a.coord.x, fmaxf( b.coord.x, fmaxf( c.coord.x, d.coord.x )));
			//aabb.x_min = fminf( a.coord.x, fminf( b.coord.x, fminf( c.coord.x, d.coord.x )));
			aabb.x_max = fmaxf( a.coord.x, c.coord.x );
			aabb.x_min = fminf( b.coord.x, d.coord.x );
			aabb.z_max = fmaxf( c.coord.z, d.coord.z );
			aabb.z_min = fminf( a.coord.z, b.coord.z );
			//printf( "aabb: %.2f -> %.2f, %.2f -> %.2f\n", aabb.x_min, aabb.x_max, aabb.z_min, aabb.z_max );
			if ( !AABBcontains( aabb, point.coord.x, point.coord.z ))
				 continue;

			if ( line_intersectsTriangle( point, y_axis, a, b, c ) ||
					line_intersectsTriangle( point, y_axis, d, c, b )) {
				/*
				debugdraw_line3d( a, b, color_blue );
				debugdraw_line3d( b, d, color_blue );
				debugdraw_line3d( d, c, color_blue );
				debugdraw_line3d( c, a, color_blue );
				*/
				return true;
			}
		}
	}
	return false;
}

// Do a collision test between a sphere and a heightfield
bool collisionFunc_SphereHeightfield( shape* sphere_shape, shape* height_shape, matrix matrix_sphere, matrix matrix_heightfield ) {
	// For now, assuming that the sphere is actually a point
	// TODO - take into account the radius

	// Translate the sphere into heightfield space
	/*
	matrix sphere_to_height;
	matrix inv_b;
	matrix_inverse( inv_b, matrix_heightfield );
	matrix_mul( sphere_to_height, inv_b, matrix_sphere );
	vector sphere_position = matrix_vecMul( sphere_to_height, &sphere_shape->origin );
	*/
	(void)sphere_shape;
	(void)matrix_heightfield;
	vector sphere_position = *matrix_getTranslation( matrix_sphere );


	// Check that the sphere is actually over the heightfield
	if ( !heightField_contains( height_shape->height_field, sphere_position.coord.x, sphere_position.coord.z )) {
		return false;
	}

	if ( heightField_collides( height_shape->height_field, sphere_position )) {
		return true;
	}

	return false;
}

bool collisionFunc_HeightfieldSphere( shape* height_shape, shape* sphere_shape, matrix matrix_heightfield, matrix matrix_sphere ) {
	return collisionFunc_SphereHeightfield( sphere_shape, height_shape, matrix_sphere, matrix_heightfield );
}

shape* shape_heightField_create( heightField* h ) {
	shape* s = mem_alloc( sizeof( shape ));
	s->type = shapeHeightField;
	s->height_field = h;
	return s;
}

heightField* heightField_create( float width, float length, int x_samples, int z_samples) {
	heightField* h = mem_alloc( sizeof( heightField ));
	memset( h, 0, sizeof( heightField ));
	h->width = width;
	h->length = length;
	h->x_samples = x_samples;
	h->z_samples = z_samples;
	h->vert_count = x_samples * z_samples;
	h->verts =  mem_alloc( sizeof( h->verts[0] ) * h->vert_count );
	return h;
}

void heightField_delete( heightField* h ) {
	vAssert( h );
	vAssert( h->verts );
	mem_free( h->verts );
	mem_free( h );
}

void shape_delete( shape* s ) {
	if ( s->type == shapeHeightField ) {
		vAssert( s->height_field );
		mem_free( s->height_field );
	}
	mem_free( s );
}


#if UNIT_TEST
void test_heightField() {
	heightField* h = heightField_create( 10.f, 8.f, 2, 2 );
	(void)h;
	h->verts[0].coord.y = 2.f; // Near corner
	h->verts[1].coord.y = 1.f;
	h->verts[2].coord.y = 0.f;
	h->verts[3].coord.y = -2.f; // Far corner

	{
		float y;
		y = heightField_sample( h, -5.f, -4.f );
		test( ( y == h->verts[0].coord.y ), "heightField Sample near corner success", "heightField Sample near corner failure" );
		y = heightField_sample( h, 4.99999f, 3.99999f );
		test( ( f_eq( y, h->verts[3].coord.y ) ), "heightField Sample far corner success", "heightField Sample far corner failure" );
	}

	{
		shape* sphere_shape = sphere_create( 0.f ); // No radius for now
		shape* height_shape = shape_heightField_create( h );
		matrix matrix_sphere;
		matrix_setIdentity( matrix_sphere );
		bool collision = collisionFunc_SphereHeightfield( sphere_shape, height_shape, matrix_sphere, matrix_identity );
		test( ( collision == true ), "heightField collision test success", "heightField collision test failure" );
	
		vector position = Vector( 0.f, 2.f, 0.f, 1.f );
		matrix_setTranslation( matrix_sphere, &position );
		collision = collisionFunc_SphereHeightfield( sphere_shape, height_shape, matrix_sphere, matrix_identity );
		test( ( collision == false ), "heightField no collision test success", "heightField no collision test failure" );
	}

	heightField_delete( h );
}
#endif // UNIT_TEST

//
//
//
//




// TODO - make a body pool to save on mem_alloc
body* body_create( shape* s, transform* t ) {
	body* b = mem_alloc( sizeof( body ));
	memset( b, 0, sizeof( body ));
	b->shape = s;
	b->trans = t;
	b->disabled = false;
	return b;
}

void collision_init() {
	body_count = 0;
	collision_clearEvents();
	collision_initCollisionFuncs();
	collision_dead_body_queue = ringQueue_create( kDeadBodyQueueSize );
	collision_new_body_queue = ringQueue_create( kNewBodyQueueSize );
}

#if UNIT_TEST
void test_collision() {
	/*
	printf( "--- Beginning Unit Test: Collision ---\n" );
	shape sphere_a;
   	sphere_a.type = shapeSphere;
	sphere_a.radius = 1.f;
	sphere_a.origin = Vector( 0.f, 0.f, 0.f, 0.f );
	shape sphere_b;
   	sphere_b.type = shapeSphere;
	sphere_b.radius = 1.f;
	sphere_b.origin = Vector( 0.f, 0.f, 0.f, 0.f );
	vAssert( shape_colliding( &sphere_a, &sphere_b, matrix_identity, matrix_identity ));

	shape sphere_c;
   	sphere_c.type = shapeSphere;
	sphere_c.radius = 1.f;
	sphere_c.origin = Vector( 0.f, 2.f, 0.f, 0.f );

	vAssert( !shape_colliding( &sphere_a, &sphere_c, matrix_identity, matrix_identity ));

	transform* t_a = transform_create();
	transform* t_b = transform_create();
	transform* t_c = transform_create();
	vector position = Vector( 0.f, 2.f, 0.f, 1.f );
	transform_setWorldSpacePosition( t_c, &position );

	body* body_a = body_create( sphere_create( 1.f ), t_a );
	body* body_b = body_create( sphere_create( 1.f ), t_b );
	body* body_c = body_create( sphere_create( 1.f ), t_c );
	body_a->layers |= 0x1;
	body_b->layers |= 0x1;
	body_c->layers |= 0x1;
	body_a->collide_with |= 0x1;
	body_b->collide_with |= 0x1;
	body_c->collide_with |= 0x1;

	collision_init();
	collision_addBody( body_a );
	collision_addBody( body_b );
	collision_addBody( body_c );
	collision_tick( 0, 0.f );

	vAssert( body_collided( body_a ));
	vAssert( body_collided( body_b ));
	vAssert( !body_collided( body_c ));

	printf( "Bodies collided this frame: A: %d, B: %d, c:%d\n",
			body_collided( body_a ),
			body_collided( body_b ),
			body_collided( body_c ));
	vAssert( 0 );

	collision_removeBody( body_a );
	collision_removeBody( body_b );
	collision_removeBody( body_c );
	// Force remove them immediately
	collision_removeDeadBodies();

	test_heightField();
	*/
}
#endif // UNIT_TEST
