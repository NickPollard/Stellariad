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

#include "common.h"
#include "marching.h"
//-----------------------
#include <forward_list>
#include <tuple>
#include <maths/vector.h>

#define val const auto
#define var auto

template<typename T> using seq = std::forward_list<T>;
using std::tuple;
using std::max;
using std::min;

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
typedef float (*DensityFunction) (vector); // Sample a density at a coord

// Triangle indices for 256 combinations
int triangles[256][8] = {};
static const int cubeSize = 16;

// *** Forward Declarations

Grid<tuple<vector,float>, cubeSize> densitiesFor(BlockExtents b, DensityFunction d);
MarchBlock* march(const BlockExtents b, const Grid<tuple<vector, float>, cubeSize>& densities);

// ***

MarchBlock* generateBlock(const BlockExtents b, const DensityFunction d) {
	// sample density function at all grid points
	// grid = 3dimensional array (don't worry about cache for now)
	auto densities = densitiesFor(b, d);
	//MarchBlock* m = march(b, densities);
	(void)densities;
	MarchBlock* m = nullptr;
	return m;
}

auto coordsFor(BlockExtents b) -> Grid<vector, cubeSize> {
	val delta = (b.max.x - b.min.x) / (float)(b.subdivides);

	auto g = Grid<vector, cubeSize>();
	for ( int i = 0; i < cubeSize; ++i )
		for ( int j = 0; j < cubeSize; ++j )
			for ( int k = 0; k < cubeSize; ++k )
				g.values[i][j][k] = vector_add(Vector( i * delta, j * delta, k * delta, 0.0 ), b.min);
	return g;
}

auto densitiesFor ( BlockExtents b, DensityFunction d ) -> Grid<tuple<vector,float>, cubeSize> {
	auto coords = coordsFor( b );
	return coords.fproduct( function<float(vector)>(d) );
}

struct cube {
	vector corners[8]; // in a particular order - bits for x/y/z?
	float  densities[8]; // in same order
};

/*
	 March a single cube

	 We have the densities at each corner of the cube
	 Each of those corresponds to a bit (== density > 0) of an 8-bit number
	 This indexes into the 256-array of triangle permutations
	 Actual vertex positions are determined by linear interpolating the
	 corner densities
	 */
int triIndex(cube c) {
	// TODO - will need to check and guarantee order here
	auto index = 0;
	for (int i = 0; i < 8; ++i)
		index += (c.densities[i] > 0.0) << i;
	return index;
}

typedef tuple<vector, int> vx;
struct triangle { 
	vx verts[3]; 
};

// Assuming dA is the smallest
vector interpD( vector a, float dA, vector b, float dB ) {
	val f = abs(dA) / (max(dA,dB) - min(dA,dB));
	return veclerp( a, b, f );
}

auto cornersFor(int index) -> tuple<int, int> {
	(void)index; // TODO
	return make_tuple(0,0);
}

// index defines which edge cube it is
auto cubeVert( cube c, int index ) -> vx {
	// given a cube c, and an index indicating an edge-vertex on that cube, calculate the actual
	// vertex position using the corner densities and positions
	val corners = cornersFor(index);
	int first = std::get<0>(corners);
	int second = std::get<1>(corners);
	val a = c.corners[first];
	val b = c.corners[second];
	val dA = c.densities[first];
	val dB = c.densities[second];
	val v = interpD( a, dA, b, dB );
	val hash = index; // Do we even need this? Isn't the index the hash?
	// No because it might be shared with a different cube
	// So we need to know where the cube is in the block
	return make_tuple(v, hash); // Except we need to tuple it with a hash
}

triangle tri(vx a, vx b, vx c) {
	val t = triangle { { a, b, c } };
	return t;
}

//auto cubeVert(cube c, int index) -> vx {
//}

auto trianglesFor(cube c) -> seq<triangle>{
	val index = triIndex(c);
	int indices[8];
	memcpy(indices, triangles[index], sizeof(int)*8);
	// Now we have some kind of index list
	// need to actually sample
	// indices indicate where in the cube they are

	val maxIndexSize = 15;
	// probably we have a list of ints
	auto tris = seq<triangle>();
	for ( int i = 0; i < maxIndexSize; i+=3 ) // TODO Check that verts aren't -1
		tris.push_front(tri( cubeVert(c, indices[i+0]), cubeVert(c, indices[i+1]), cubeVert(c, indices[i+2])));
	return tris;
}
/*

seq<cube> cubesFor(const Grid<tuple<vector,float>, cubeSize> g) {
	// generate a list of cubes for the block extents
	// probably needs to LoD here too?
	seq<cube> cubes;
	for ( int i = 0; i < cubeSize-1; ++i )
		for ( int i = 0; i < cubeSize-1; ++i )
			for ( int i = 0; i < cubeSize-1; ++i ) {
				cube c = cube();
				auto topLeft = g.values[i][j][k]
				c.corners[0] = topLeft.first;
				c.densities[0] = topLeft.second;
				auto idx = g.values[i+1][j][k];
				c.corners[1] = idx.first;
				c.densities[1] = ix.second;
				auto idx = g.values[i][j+1][k];
				c.corners[2] = idx.first;
				c.densities[2] = ix.second;
				auto idx = g.values[i+1][j+1][k];
				c.corners[3] = idx.first;
				c.densities[3] = ix.second;
				auto idx = g.values[i][j][k+1];
				c.corners[4] = idx.first;
				c.densities[4] = ix.second;
				auto idx = g.values[i+1][j][k+1];
				c.corners[5] = idx.first;
				c.densities[5] = ix.second;
				auto idx = g.values[i][j+1][k+1];
				c.corners[6] = idx.first;
				c.densities[6] = ix.second;
				auto idx = g.values[i+1][j+1][k+1];
				c.corners[7] = idx.first;
				c.densities[7] = ix.second;
				cubes.push_back(c);
			}
	return cubes;
}

MarchBlock* march(const BlockExtents b, const Grid<float>& densities) {
	seq<cube> cubes = cubesFor(densitites);
	cubes.flatMap(trianglesFor(densities))
}
*/
