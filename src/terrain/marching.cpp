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
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/render.h"
#include "render/shader.h"
#include "render/texture.h"
#include "test.h"

#define val const auto
#define var auto

template<typename T> using seq = std::forward_list<T>;
using std::tuple;
using std::max;
using std::min;
using std::get;

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

int triIndex(cube c) {
	// TODO - will need to check and guarantee order here
	auto index = 0;
	val surface = 0.f;
	for (int i = 0; i < 8; ++i) index |= ((c.densities[i] <= surface) ? 1 << i : 0);
	return index;

	/*
  unsigned int cubeindex = 0;
                for(int m=0; m<8; ++m)
                    if(v[m] <= isovalue)
                        cubeindex |= 1<<m;

                v[0] = f(x,y,z); v[1] = f(x_dx,y,z);
                v[2] = f(x_dx,y_dy,z); v[3] = f(x, y_dy, z);
                v[4] = f(x,y,z_dz); v[5] = f(x_dx,y,z_dz);
                v[6] = f(x_dx,y_dy,z_dz); v[7] = f(x, y_dy, z_dz);
												*/
}

typedef int hash;
typedef tuple<vector, hash> vx;
struct triangle { 
	vx verts[3]; 
};

// Assuming dA is the smallest
vector interpD( vector a, float dA, vector b, float dB ) {
	printf( "abs(dA) = %.2f, max = %.2f, min = %.2f.\n", fabsf(dA), fmaxf(dA,dB), fminf(dA,dB));
	val f = fabsf(dA) / (max(dA,dB) - min(dA,dB));
	vector v = veclerp( a, b, f );
	printf( "Interping between " );
	vector_print( &a );
	printf( " with density %.2f and ", dA );
	vector_print( &b );
	printf( " with density %.2f, result: ", dB );
	vector_print( &v );
	printf( "\n" );
	return v;
}

auto cornersFor(int edge) -> tuple<int, int> {
	return make_tuple(edges[edge][0], edges[edge][1]);
}

// index defines which edge cube it is
auto cubeVert( cube c, int index ) -> vx {
	// given a cube c, and an index indicating an edge-vertex on that cube, calculate the actual
	// vertex position using the corner densities and positions
	val corners = cornersFor(index);
	int first = get<0>(corners);
	int second = get<1>(corners);
	val a = c.corners[first];
	val b = c.corners[second];
	val dA = c.densities[first];
	val dB = c.densities[second];
	val v = interpD( a, dA, b, dB );
	// TODO
	val hash = index; // Do we even need this? Isn't the index the hash?
	// No because it might be shared with a different cube
	// So we need to know where the cube is in the block
	return make_tuple(v, hash); // Except we need to tuple it with a hash
}

triangle tri(vx a, vx b, vx c) {
	val t = triangle { { a, b, c } };
	return t;
}

/*
	 March a single cube

	 We have the densities at each corner of the cube
	 Each of those corresponds to a bit (== density > 0) of an 8-bit number
	 This indexes into the 256-array of triangle permutations
	 Actual vertex positions are determined by linear interpolating the
	 corner densities
	 */
auto trianglesFor(cube c) -> seq<triangle>{
	val index = triIndex(c);
	val maxIndexSize = 16;
	int8_t indices[maxIndexSize];
	memcpy(indices, triangles[index], sizeof(int8_t)*maxIndexSize);
	// Now we have some kind of index list
	// need to actually sample
	// indices indicate where in the cube they are
	// probably we have a list of ints
	auto tris = seq<triangle>();
	for ( int i = 0; i < maxIndexSize && indices[i] >= 0; i+=3 )
		tris.push_front(tri( cubeVert(c, indices[i+0]), cubeVert(c, indices[i+1]), cubeVert(c, indices[i+2])));
	return tris;
}

struct Buffers {
	int indexCount;
	int vertCount;
	uint16_t* indices;
	vector* verts;
};

auto buffersFor(seq<triangle> tris) -> Buffers {
	/*
		 Naive solution - just map each vert to a new vert, each index becomes just that
		 */
	Buffers bs;
	// TODO - length will need to take into account index compaction?
	int length = std::distance(tris.begin(), tris.end()) * 3;
	bs.indexCount = length;
	bs.vertCount = length;
	bs.indices = (uint16_t*)mem_alloc(sizeof(uint16_t) * length);
	bs.verts = (vector*)mem_alloc(sizeof(vector) * length);

	int i = 0;
	for ( auto t : tris ) {
		bs.verts[i++] = get<0>(t.verts[0]);
		bs.verts[i++] = get<0>(t.verts[1]);
		bs.verts[i++] = get<0>(t.verts[2]);
	}
	for ( int j = 0; j < i; ++j ) bs.indices[j] = j;

	// TODO - a non-naive solution, compacting verts by hash
	// Should just be a group-by (more or less)?

	return bs;
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

//////////////////////////////
float densityFn(vector v) {
	return -v.y + 1.f + fabsf(v.x - 15.f) * 0.54+ fabsf(v.z - 15.f) * 0.54;
}

vertex* vertsFor(const Buffers& bs) {
	vertex* vertices = (vertex*)mem_alloc(sizeof(vertex) * bs.vertCount);
	for ( int i = 0; i < bs.vertCount; ++i ) {
			vertices[i].position = bs.verts[i];
			vertices[i].color = 0;
			vertices[i].uv.x = 0.f;
			vertices[i].uv.y = 0.f;
			vertices[i].normal = Vector(0.f, 1.f, 0.f, 1.f);
	}
	return vertices;
}
void drawMarchedCube(const Buffers& bs, vertex* vertices) {
	drawCall_create( &renderPass_main, *Shader::byName("dat/shaders/reflective.s"), bs.indexCount, bs.indices, vertices, static_texture_default->gl_tex, modelview);
}

cube makeTestCube(vector origin, float size ) {
	cube c;

	/*
    v[0] = f(x,y,z); v[1] = f(x_dx,y,z);
    v[2] = f(x_dx,y_dy,z); v[3] = f(x, y_dy, z);
    v[4] = f(x,y,z_dz); v[5] = f(x_dx,y,z_dz);
    v[6] = f(x_dx,y_dy,z_dz); v[7] = f(x, y_dy, z_dz);
	*/

	//two spirals, on front Z and back Z

	c.corners[0] = origin;
	c.corners[1] = vector_add(origin, Vector(size, 0.0, 0.0, 0.0));
	c.corners[2] = vector_add(origin, Vector(size, size, 0.0, 0.0));
	c.corners[3] = vector_add(origin, Vector(0.0, size, 0.0, 0.0));

	c.corners[4] = vector_add(origin, Vector(0.0, 0.0, size, 0.0));
	c.corners[5] = vector_add(origin, Vector(size, 0.0, size, 0.0));
	c.corners[6] = vector_add(origin, Vector(size, size, size, 0.0));
	c.corners[7] = vector_add(origin, Vector(0.0, size, size, 0.0));

	for (int i = 0; i < 8; ++i) {
			c.densities[i] = densityFn(c.corners[i]);
			vector_print(&c.corners[i]);
			printf( ", density: %.2f\n", c.densities[i] );
	}
	return c;
}

// *** test statics
Buffers static_marching_buffers[9];
vertex* static_marching_verts[9];

static const int drawTestCubes = 9;

void test_marching_draw() {
	for (int i = 0; i < drawTestCubes; ++i)
		drawMarchedCube( static_marching_buffers[i], static_marching_verts[i] );
}

void test_interp() {
	// for all vectors, floats
	vector va = Vector( 10.0, 10.0, 0.0, 1.0 );
	vector vb = Vector( 10.0, 0.0, 0.0, 1.0 );
	float a = 4.1f;
	float b = -5.9f;
	vector r1 = interpD(va, a, vb, b);
   	vector r2 =	interpD(vb, b, va, a);
	test( vector_equal(&r1, &r2), "interpD is commutative", "interpD is NOT commutative" ); 
}

void test_cube() {
	test_interp();
	//vAssert( 0 );
	float size = 10.0;
	//cube c = makeTestCube(Vector(0.0, 0.0, 0.0, 1.0), size);
	//auto tris = trianglesFor(c);
	//auto buffers = buffersFor(tris);
	vector cubePos[9];
	cubePos[0] = Vector(0.0, 0.0, 0.0, 1.0);
	cubePos[1] = Vector(10.0, 0.0, 0.0, 1.0);
	cubePos[2] = Vector(0.0, 0.0, 10.0, 1.0);
	cubePos[3] = Vector(10.0, 0.0, 10.0, 1.0);
	cubePos[4] = Vector(20.0, 0.0, 0.0, 1.0);
	cubePos[5] = Vector(0.0, 0.0, 20.0, 1.0);
	cubePos[6] = Vector(20.0, 0.0, 10.0, 1.0);
	cubePos[7] = Vector(10.0, 0.0, 20.0, 1.0);
	cubePos[8] = Vector(20.0, 0.0, 20.0, 1.0);
	//for ( int i = 0; i < buffers.vertCount; ++i )
			//vector_printf( "Vert: ", &buffers.verts[i]);
	//val verts = vertsFor(buffers);
	//drawMarchedCube(buffers, verts);
	for (int i = 0; i < drawTestCubes; ++i) {
		cube c = makeTestCube(cubePos[i], size);
		auto buffers = buffersFor(trianglesFor(c));
		val verts = vertsFor(buffers);
		static_marching_buffers[i] = buffers;
		static_marching_verts[i] = verts;
	}
}

