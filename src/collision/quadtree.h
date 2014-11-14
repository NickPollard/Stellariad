// quadtree.h

#include "collection/vec.h"
#include "collision.h"

template<typename T>
struct QuadTree {
	Vec<QuadTree<T>> children;
	Vec<T> contents;
	bool leaf;

	QuadTree() {
			leaf = true;
			if (!leaf) {
					children += QuadTree<T>();
					children += QuadTree<T>();
					children += QuadTree<T>();
					children += QuadTree<T>();
			}
	}

	const QuadTree<T>& operator +=( const T& t ) {
			if (leaf) contents += t;
			else {
					// Add to (all possible) children
			}
			return *this;
	}

	float x;
	float y;
};

// TODO - separate collision quadtree from generic quadtree
Vec<collisionEvent> potentialCollisions(const QuadTree<body*>& q);
