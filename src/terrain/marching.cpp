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
typedef std::vector seq;

struct BlockExtents {
	vector min;
	vector max;
	int subdivides;
};
struct MarchBlock {
	BlockExtents extents;
	vertex*	verts;
	int*	indices;

};
typedef float (*DensityFunction) (float, float, float); // Sample a density at a coord
template < type T > struct Grid { };

// Triangle indices for 256 combinations
int triangles[256][8] = {};

marchBlock* generateBlock(const BlockExtents b, const DensityFunction d) {
	// sample density function at all grid points
	Grid<float> densities = densitiesFor(b, d);
	MarchBlock* m = march(b, densities);
	return m;
}

Grid<vector> coordsFor(BlockExtents b) {
	// TODO - write a range class
	float delta = (max.x - min.x) / (float)subdivides;
	auto indx = from(1).to(10) * delta;
	// Range 3d? -> i.e. grid

}

Grid<float> densitiesFor(BlockExtents b, DensityFunction d) {
	Grid<vector> coords = coordsFor(b);
	return coords.map(d);
}

seq<triangle> trianglesFor(cube c)

seq<cube> cubesFor(const BlockExtents b) {
	// generate a list of cubes for the block extents
	// probably needs to LoD here too?
}

MarchBlock* march(const BlockExtents b, const Grid<float>& densities) {
	seq<cube> cubes = cubesFor(b);
	cubes.flatMap(trianglesFor(densities))
}
