// marching.cpp

/*
   A marching cubes implementation for building blocks of Terrain during live
   gameplay.

   Split the terrain world into cube grids. For each grid, sample the terrain
   density function at each corner, then take each cube and run the marching
   algorithm.

   Handle LoDs the same way as the old algorithm - the higher res block can
   sample the edge cubes as if they were at a lower resolution, interpolating
   between spanning corners rather than the true value
   */

struct BlockExtents {
	vector min;
	vector max;
};
struct MarchBlock {
	BlockExtents extents;
	vertex*	verts;
	int*	indices;

};
typedef float (*DensityFunction) (float, float, float); // Sample a density at a coord
template < type T > struct Grid { };

marchBlock* generateBlock(BlockExtents b, DensityFunction d) {
	// sample density function at all grid points
	Grid<float> densities = densitiesFor(b, d);
	// cubes.flatMap(trianglesFor(densities))
	MarchBlock* m = NYI();
	return m;
}

Grid<float> densitiesFor(BlockExtents b, DensityFunction d) {
}

list<triangle> trianglesFor(cube c)
