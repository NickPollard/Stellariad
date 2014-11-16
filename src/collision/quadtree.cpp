// quadtree.cpp
#include "common.h"
#include "quadtree.h"
//---------------------
#include "maths/geometry.h"
#include "string/stringops.h"

bool QuadTreeBase::contains(const aabb2d& bb) {
	return overlap(boundingBox, bb);
}

collisionEvent collide( body* a, body* b ) {
	collisionEvent event = { a, b };
	return event;
}

Vec<collisionEvent> genEvents(const Vec<body*>& bodies) {
	// for every body, check every other body
	auto collisions = Vec<collisionEvent>();
	int count = bodies.size();
	// NOTE: Cast down to C-Array for performance (Probably a C++ safe way of doing this, but cant get it to work right now)
	body*const * bodiesArray = &bodies[0];
	for ( int i = 0; i < count; ++i )
		for ( int j = i + 1; j < count; j++ ) {
			vAssert( bodiesArray[i] && bodiesArray[j] )
			if ( !bodiesArray[i]->disabled && !bodiesArray[j]->disabled && body_colliding( bodiesArray[i], bodiesArray[j] ))
				collisions += collide( bodiesArray[i], bodiesArray[j] );
		}
	return collisions;
}

Vec<collisionEvent> leafCollisions(const QuadTree<body*>* q) {
//	printf( "leaf collisions (%d entries)\n", q->contents.size());
	return genEvents(q->contents);
}

Vec<collisionEvent> branchCollisions(const QuadTree<body*>* q) {
//	printf( "branch collisions (%d entries)\n", q->contents.size());
	return q->children.flatmap<collisionEvent>(potentialCollisions);
}

Vec<collisionEvent> potentialCollisions(const QuadTree<body*>* q) {
	return (q->leaf) ? leafCollisions(q) : branchCollisions(q);
};
