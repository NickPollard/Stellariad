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
#include "terrain.h"
#include "test.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/render.h"
#include "render/shader.h"
#include "render/texture.h"

#define val const auto
#define var auto

template<typename T> using seq = std::forward_list<T>;
using std::tuple;
using std::max;
using std::min;
using std::get;

texture* marching_texture = NULL;
texture* marching_texture_cliff = NULL;

struct BlockExtents {
	vector min;
	vector max;
	int subdivides;

	BlockExtents( vector mn, vector mx, int sub ) : min(mn), max(mx), subdivides(sub) {}
};

struct Buffers {
	int indexCount;
	int vertCount;
	uint16_t* indices;
	vector* verts;
	vector* normals;
};

struct MarchBlock {
	BlockExtents extents;
	vertex*	verts;
//	int*	indices;
	Buffers buffers;
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
	// grid = 3-dimensional array (don't worry about cache for now)
	return march(b, densitiesFor(b, d));
}

auto coordsFor(BlockExtents b) -> Grid<vector, cubeSize> {
	val delta = (b.max.x - b.min.x) / (float)b.subdivides;

	auto g = Grid<vector, cubeSize>();
	for ( int i = 0; i < cubeSize; ++i )
		for ( int j = 0; j < cubeSize; ++j )
			for ( int k = 0; k < cubeSize; ++k )
				g.values[i][j][k] = vector_add(Vector( i * delta, j * delta, k * delta, 0.0 ), b.min);
	return g;
}

auto densitiesFor ( BlockExtents b, DensityFunction d ) -> Grid<tuple<vector,float>, cubeSize> {
	return coordsFor( b ).fproduct( function<float(vector)>(d) );
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
}

typedef int hash;
typedef tuple<vector, hash> vx;
struct triangle { 
	vx verts[3]; 
};

// Assuming dA is the smallest
vector interpD( vector a, float dA, vector b, float dB ) {
	val f = fabsf(dA) / (max(dA,dB) - min(dA,dB));
	return veclerp( a, b, f );
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

auto normalFor(vector a, vector b, vector c) -> vector {
		vector ab = vector_sub(a, b);
		vector bc = vector_sub(b, c);
		return normalized(vector_cross(bc, ab));
}

auto buffersFor(const seq<triangle>& tris) -> Buffers {
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
	bs.normals = (vector*)mem_alloc(sizeof(vector) * length);

	int i = 0;
	for ( auto t : tris ) {
		bs.verts[i++] = get<0>(t.verts[0]);
		bs.verts[i++] = get<0>(t.verts[1]);
		bs.verts[i++] = get<0>(t.verts[2]);
		vector normal = normalFor(bs.verts[i-1], bs.verts[i-2], bs.verts[i-3]);
		bs.normals[i-1] = normal;
		bs.normals[i-2] = normal;
		bs.normals[i-3] = normal;
	}
	for ( int j = 0; j < i; ++j ) bs.indices[j] = j;

	// TODO - a non-naive solution, compacting verts by hash
	// Should just be a group-by (more or less)?

	return bs;
}
typedef Grid<tuple<vector,float>, cubeSize> densityGrid;

void setCorner(cube& c, int i, tuple<vector,float> t) {
	c.corners[i] = get<0>(t);
	c.densities[i] = get<1>(t);
}

seq<cube> cubesFor(const densityGrid& g) {
	// generate a list of cubes for the block extents
	// probably needs to LoD here too?
	seq<cube> cubes;
	for ( int i = 0; i < cubeSize-1; ++i )
		for ( int j = 0; j < cubeSize-1; ++j )
			for ( int k = 0; k < cubeSize-1; ++k ) {
				cube c = cube();

				setCorner(c, 0, g.values[i][j][k] );
				setCorner(c, 1, g.values[i+1][j][k] );
				setCorner(c, 2, g.values[i+1][j+1][k] );
				setCorner(c, 3, g.values[i][j+1][k] );
				setCorner(c, 4, g.values[i][j][k+1] );
				setCorner(c, 5, g.values[i+1][j][k+1] );
				setCorner(c, 6, g.values[i+1][j+1][k+1] );
				setCorner(c, 7, g.values[i][j+1][k+1] );
				cubes.push_front(c);
			}
	return cubes;
}

vertex* vertsFor(const Buffers& bs) {
	vertex* vertices = (vertex*)mem_alloc(sizeof(vertex) * bs.vertCount);
	const float scale = 1.f;
	for ( int i = 0; i < bs.vertCount; ++i ) {
		vertices[i].position = bs.verts[i];
		vertices[i].color = 0;
		vertices[i].uv.x = vertices[i].position.x * scale;
		vertices[i].uv.y = vertices[i].position.y * scale;
		//vertices[i].normal = Vector(0.f, 1.f, 0.f, 1.f);
		vertices[i].normal = bs.normals[i];
		vertices[i].normal.w = vertices[i].position.z * scale;
	}
	return vertices;
}

MarchBlock* march(const BlockExtents b, const densityGrid& densities) {
	(void)b;
	seq<cube> cubes = cubesFor(densities);

	//
	val triangles = new seq<triangle>();
	for( auto c : cubes ) {
		val tris = trianglesFor(c);
		for( auto t : tris) triangles->push_front(t);
	}
	//val triangles = cubes.flatMap(trianglesFor(densities)); // TODO - triangles from densities
	//


	val buffers = buffersFor(*triangles);
	val verts = vertsFor(buffers);
	MarchBlock* m = (MarchBlock*)mem_alloc(sizeof( MarchBlock ));
	m->extents = b;
	m->verts = verts;
	m->buffers = buffers;
	return m;
}

//////////////////////////////
float fromHeightField(float height, float y) {
	return height - (y - 60.f);
}

float densityFn(vector v) {
	float height = canyonTerrain_sampleUV( v.x, v.z );
	return fromHeightField(height, v.y);
}

void drawMarchedCube(const Buffers& bs, vertex* vertices) {
	render_resetModelView( );
	if (bs.indexCount > 0 && marching_texture->gl_tex && marching_texture->gl_tex ) {
		val draw = drawCall::create( &renderPass_main, *Shader::byName("dat/shaders/terrain.s"), bs.indexCount, bs.indices, vertices, marching_texture->gl_tex, modelview);
		draw->texture_b = marching_texture_cliff->gl_tex;		
		draw->texture_c = marching_texture->gl_tex;		
		draw->texture_d = marching_texture_cliff->gl_tex;		
	}
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

	for (int i = 0; i < 8; ++i)
		c.densities[i] = densityFn(c.corners[i]);
	return c;
}

// *** test statics
static const int cubesPerWidth = 24;
static const int drawTestCubes = cubesPerWidth * cubesPerWidth * cubesPerWidth;
Buffers static_marching_buffers[drawTestCubes];
vertex* static_marching_verts[drawTestCubes]; 
MarchBlock* static_cube = NULL;

void test_marching_draw() {
	//for (int i = 0; i < drawTestCubes; ++i)
	//	drawMarchedCube( static_marching_buffers[i], static_marching_verts[i] );
	if (static_cube)
		drawMarchedCube( static_cube->buffers, static_cube->verts );
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

	if ( !marching_texture ) 			{ marching_texture		= texture_load( "dat/img/terrain/grass.tga" ); }
	if ( !marching_texture_cliff )	{ marching_texture_cliff = texture_load( "dat/img/terrain/cliff_grass.tga" ); }

	//vector cubePos[drawTestCubes];
	// TODO - for (auto i : 1 to 10)
	float w = 5.0;
	/*
	int i = 0;
	val origin = Vector(-20.f, -100.f, -20.f, 0.f);
	for ( int x = 0; x < cubesPerWidth; ++x ) {
		for ( int y = 0; y < cubesPerWidth; ++y ) {
			for ( int z = 0; z < cubesPerWidth; ++z ) {
				cubePos[i] = vector_add(Vector((float)x * w, (float)y * w, (float)z * w, 1.0 ), origin);
				cube c = makeTestCube(cubePos[i], w);
				val buffers = buffersFor(trianglesFor(c));
				val verts = vertsFor(buffers);
				static_marching_buffers[i] = buffers;
				static_marching_verts[i] = verts;
				++i;
			}
		}
	}
	*/
	
	BlockExtents b = BlockExtents( origin, vector_add(origin, Vector(cubesPerWidth * w, cubesPerWidth * w, cubesPerWidth * w, 0.f)), cubesPerWidth );
	static_cube = generateBlock(b, densityFn);
}

