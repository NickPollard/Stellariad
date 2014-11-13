// quadtree.cpp

template<typename T>
Vec<collisions> QuadTree::potentialCollisions() {
	if (leaf) leafCollisions(); else children >>= potentialCollisions;
};

template<typename T>
Vec<collision> QuadTree::leafCollisions() {
	// This creates a new Vec object, return that by value, but it stores values on the heap?
	return Vec<collision>();
};
