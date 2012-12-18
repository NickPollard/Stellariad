// collision.h

#define kMaxCollisionEvents 256
#define kMaxShapeTypes 4
#define kMaxCollidingBodies 1024

#include "maths/geometry.h"
#include "maths/maths.h"
#include "maths/matrix.h"
#include "maths/vector.h"

#define kCollisionLayerPlayer	1
#define kCollisionLayerEnemy	2
#define kCollisionLayerBullet	4
#define kCollisionLayerTerrain	8

enum shapeType {
	shapeInvalid,
	shapeSphere,
	shapeMesh,
	shapeHeightField
};

// A full arbitrary collision mesh
typedef struct collisionMesh_s {
	vector* verts;	
	int vert_count;
	uint16_t* indices;
	int index_count;
} collisionMesh;

// A mesh-shape defined by a heighfield - so underneath is always colliding
// No re-entrant shapes - any given X,Z pair maps to exactly one Y
typedef struct heightField_s {
	int x_samples;		// How many verts wide the field is
	int z_samples;		// How many verts long the field is
	float width;		// How wide (in game units) - X - the field is
	float length;		// How long (in game units) - Z - the field is
	vector *verts;
	aabb2d	aabb;
} heightField;

typedef struct shape_s {
	enum shapeType type;
	union {
		float radius;
		collisionMesh* collision_mesh;
		heightField* height_field;
	};
	vector origin;
} shape;

typedef void (*collisionCallback)( body* this, body* collided_width, void* data );
typedef uint8_t collision_layers_t;

struct body_s {
	transform* trans;
	shape* shape;
	union {
		void* data;	// Pointer for storing arbitrary data - e.g. the owner 
		int intdata;
	};
	collision_layers_t layers;
	collision_layers_t collide_with;
	collisionCallback callback;
	void* callback_data;
	bool disabled;
};

typedef struct collisionEvent_s {
	body* a;
	body* b;
} collisionEvent;

typedef bool (*collideFunc)( shape* a, shape* b, matrix matrix_a, matrix matrix_b );

// Initialize the collision system
void collision_init();

// Add a body to the collision system
void collision_addBody( body* b );
void collision_removeBody( body* b );

// Check for any collisions this frame
void collision_tick( int frame_counter, float dt );
// Tick but on worker thread
void collision_queueWorkerTick( int frame_counter, float dt );
// Tick just generates events, this calls callbacks and removes dead bodies
void collision_processResults( int frame_counter, float dt );

// Did the body hit anything this frame
bool body_collided( body* b );

// Did two bodies hit each other this frame
bool body_collidedBody( body* a, body* b );

body* body_create( shape* s, transform* t );
shape* sphere_create( float radius );
shape* mesh_createFromRenderMesh( mesh* render_mesh );
void shape_delete( shape* s );
void heightField_delete( heightField* h );
heightField* heightField_create( float width, float length, int x_samples, int z_samples);
shape* shape_heightField_create( heightField* h );
void heightField_calculateAABB( heightField* h );

// Unit tests
void test_collision();
