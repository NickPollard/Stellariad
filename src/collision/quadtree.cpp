// quadtree.cpp
#include "common.h"
#include "quadtree.h"
//---------------------
#include "string/stringops.h"

void ff(const std::function<int(int)>& f) {
	(void)f;
	return;
}

collisionEvent collide( body* a, body* b ) {
	collisionEvent event = { a, b };
	return event;
}

Vec<collisionEvent> genEvents(const Vec<body*> bodies) {
	// for every body, check every other body
	auto collisions = Vec<collisionEvent>();
	int count = bodies.size();
	for ( int i = 0; i < count; ++i )
		for ( int j = i + 1; j < count; j++ ) {
			vAssert( bodies[i] && bodies[j] )
			if ( !bodies[i]->disabled && !bodies[j]->disabled && body_colliding( bodies[i], bodies[j] ))
				collisions += collide( bodies[i], bodies[j] );
		}
	return collisions;
}

Vec<collisionEvent> leafCollisions(const QuadTree<body*>& q) {
	return genEvents(q.contents);
}

Vec<collisionEvent> branchCollisions(const QuadTree<body*>& q) {
	return q.children.flatmap<collisionEvent>(leafCollisions);
}

Vec<collisionEvent> potentialCollisions(const QuadTree<body*>& q) {
	return (q.leaf) ? leafCollisions(q) : branchCollisions(q);
};
