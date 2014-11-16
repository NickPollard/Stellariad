// quadtree.h

#include "collection/vec.h"
#include "collision.h"
#include <stdio.h>

template<typename T> struct Bounded {
	static aabb2d bb2d(const T& t);
};

template<> struct Bounded<body*> {
	static aabb2d bb2d(body* const& b) {
		(void)b;
		aabb2d bb = Aabb2d( -20.f, -5.f, -20.f, -5.f );
		return bb;
	}
};

struct QuadTreeBase {
	float xMax;
	float xMin;
	float zMax;
	float zMin;
	float x;
	float z;
	aabb2d boundingBox;

	bool contains(const aabb2d& bb);
};

template<typename T>
struct QuadTree : public QuadTreeBase {
	Vec<QuadTree<T>*> children;
	Vec<T> contents;
	bool leaf;

	QuadTree(aabb2d bb) : leaf( true ) {
		children.underlying.reserve(4);
		contents.underlying.reserve(20);
		xMin = bb.x_min;
		xMax = bb.x_max;
		zMin = bb.z_min;
		zMax = bb.z_max;
		x = (xMin + xMax) * 0.5f;
		z = (zMin + zMax) * 0.5f;
		boundingBox = Aabb2d( xMin, xMax, zMin, zMax );
	}

	~QuadTree() { for (auto child : children) delete child; }

	bool shouldSplit( const T& t ) {
		// If the item is small enough to fit into only two children or less
		aabb2d bb = Bounded<T>::bb2d(t);
		static const float minSize = 1000.f;
		return (xMax - xMin) > minSize && (bb.x_max < x || bb.z_max < z || bb.x_min > x || bb.z_max < z);
	}

	const QuadTree<T>& operator += ( const T& t ) {
		if (leaf) {
			if (shouldSplit(t)) {
				split();
				*this += t;
			} else
				contents += t;
		}
		else {
			aabb2d bb = Bounded<T>::bb2d(t);
			for (auto child: children)
				if (child->contains(bb))
					*child += t;
		}
		return *this;
	}

	void split() {
//		printf( "Splitting node (size = %.2f).\n", xMax - xMin );
		vAssert( leaf == true && children.size() == 0 );
		leaf = false;
//		printf( "Adding child: %.2f %.2f %.2f %.2f\n", xMin, x, zMin, z );
//		printf( "Adding child: %.2f %.2f %.2f %.2f\n", xMin, x, z, zMax );
//		printf( "Adding child: %.2f %.2f %.2f %.2f\n", x, xMax, zMin, z );
//		printf( "Adding child: %.2f %.2f %.2f %.2f\n", x, xMax, z, zMax );
		children += new QuadTree<T>(Aabb2d( xMin, x, zMin, z ));
		children += new QuadTree<T>(Aabb2d( xMin, x, z, zMax ));
		children += new QuadTree<T>(Aabb2d( x, xMax, zMin, z ));
		children += new QuadTree<T>(Aabb2d( x, xMax, z, zMax ));
		for (auto c: contents) {
			*this += c;
		}
		contents.clear();
	}

	int print(int indent) {
		for ( int i = 0; i < indent; ++i )
			printf( "  " );
		if (leaf)
			printf( "Leaf (%d entries) [ %.2f %.2f %.2f %.2f ] \n", contents.size(), xMin, xMax, zMin, zMax );
		else {
			printf( "Branch:\n" );
			auto lambda =[indent](QuadTree<T>* q){ return q->print(indent + 1); };
			std::function<int(QuadTree<T>*)> f = std::function<int(QuadTree<T>*)>( lambda );
			fmap<QuadTree<T>*, int>(children, f);
		}
		return 0;
	}
};

// TODO - separate collision quadtree from generic quadtree
Vec<collisionEvent> potentialCollisions(const QuadTree<body*>* q);
