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

	BlockExtents( vector mn, vector size, int sub ) : min(mn), max(vector_add(mn, size)), subdivides(sub) {}
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
static const int cubeSize = 48;

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
	val delta = (b.max.x - b.min.x) / (float)(cubeSize - 1);

	auto g = Grid<vector, cubeSize>();
	for ( int i = 0; i < cubeSize; ++i )
		for ( int j = 0; j < cubeSize; ++j )
			for ( int k = 0; k < cubeSize; ++k )
				g.values[i][j][k] = vector_add(Vector( i * delta, j * delta, k * delta, 0.0 ), b.min);
	return g;
}

auto densitiesFor( BlockExtents b, DensityFunction d ) -> Grid<tuple<vector,float>, cubeSize> {
	return coordsFor( b ).fproduct( function<float(vector)>(d) );
}

struct cube {
	vector corners[8]; // in a particular order - bits for x/y/z?
	float  densities[8]; // in same order
	int origin;
};

int triIndex(cube c) {
	int index = 0;
	val surface = 0.f;
	for (int i = 0; i < 8; ++i) index |= ((c.densities[i] <= surface) ? 1 << i : 0);
	return index;
}

auto normalFor(vector a, vector b, vector c) -> vector;

typedef int hash;
typedef tuple<vector, hash> vx;
struct triangle { 
	vx verts[3]; 
	auto pos(int i) const -> vector { return get<0>(verts[i]); }

	auto hash(int i) const -> int { return get<1>(verts[i]); }

	auto normal() const -> vector { return normalFor(pos(0), pos(2), pos(1)); }
};

// Assuming dA is the smallest
vector interpD( vector a, float dA, vector b, float dB ) {
	val f = fabsf(dA) / (max(dA,dB) - min(dA,dB));
	return veclerp( a, b, f );
}

auto cornersFor(int edge) -> tuple<int, int> {
	return make_tuple(edges[edge][0], edges[edge][1]);
}

int offsetOf( int index, int w ) {
	val offset = edgeOffset[index];
	// TODO - could compute these offline if w is static?
	return (offset.x * (w*w) + offset.y * w + offset.z) * 3 + offset.dir;
}

// index defines which edge cube it is
auto cubeVert( cube c, int index, int origin ) -> vx {
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
	val width = cubeSize;
	auto hash = origin * 3 + offsetOf(index, width);
	vAssert( hash >= 0 );
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
	int8_t* indices = triangles[index];
	// Now we have the index list for the triangle, to pull out correct verts
	auto tris = seq<triangle>();
	vx cVerts[3];
	for ( int i = 0; i + 2 < maxIndexSize && indices[i] >= 0; i+=3 ) {
		for ( int j = 0; j < 3; ++j ) cVerts[j] = cubeVert(c, indices[i+j], c.origin);
		tris.push_front(tri( cVerts[0], cVerts[1], cVerts[2]));
	}

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

		 TODO - this is our next feature
		 What we actually want to do
		 - groupByHash
		 - for shared verts (ie same hash) average the nornals (sum and normalize)
		 - for every triangle, calculate normal and accumulate that to all verts
		 - normalize every vert normal
		 - store normals in buffers
		 
		 we know for a marchblock the total possible number of verts
		 can have an array to store the verts, indexed by hash
		 store each vertex there
		 store (and accumulate normals)
		 then put in buffer

		 */

	static const int MaxVerts = cubeSize * cubeSize * cubeSize * 3; // TODO fix
	vertex* verts = (vertex*)mem_alloc( sizeof(vertex) * MaxVerts );
	for (int i = 0; i < MaxVerts; ++i)
		verts[i].normal = Vector(0.0, 0.0, 0.0, 0.0);
	for (val t: tris) {
		val normal = t.normal();
		for (int i = 0; i < 3; ++i ) {
			var hash = t.hash(i);
			vAssert( hash >= 0 && hash < MaxVerts );
			verts[hash].position = t.pos(i);
			verts[hash].normal = vector_add(verts[hash].normal, normal);
		}
	}
	for (int i = 0; i < MaxVerts; ++i)
		verts[i].normal = normalized(verts[i].normal);

	Buffers bs;
	// TODO - length will need to take into account index compaction?
	int length = std::distance(tris.begin(), tris.end()) * 3;
	bs.indexCount = length;
	bs.vertCount = length;
	bs.indices = (uint16_t*)mem_alloc(sizeof(uint16_t) * length);
	bs.verts = (vector*)mem_alloc(sizeof(vector) * length);
	bs.normals = (vector*)mem_alloc(sizeof(vector) * length);

	int i = 0;
	for ( val t : tris ) {
		for (int j = 0; j < 3; j++) {
			val hash = t.hash(j);
			vAssert( i < length );
			vAssert( hash >= 0 && hash < MaxVerts );
			bs.verts[i] = verts[hash].position;
			bs.normals[i] = verts[hash].normal;
			++i;
		}
	}
	for ( int j = 0; j < i; ++j ) bs.indices[j] = j;

	mem_free( verts );

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
	//int origin = 0;
	for ( int x = 0; x < cubeSize-1; ++x )
		for ( int y = 0; y < cubeSize-1; ++y )
			for ( int z = 0; z < cubeSize-1; ++z ) {
				cube c = cube();

				setCorner(c, 0, g.values[x][y][z] );
				setCorner(c, 1, g.values[x+1][y][z] );
				setCorner(c, 2, g.values[x+1][y+1][z] );
				setCorner(c, 3, g.values[x][y+1][z] );
				setCorner(c, 4, g.values[x][y][z+1] );
				setCorner(c, 5, g.values[x+1][y][z+1] );
				setCorner(c, 6, g.values[x+1][y+1][z+1] );
				setCorner(c, 7, g.values[x][y+1][z+1] );
				c.origin = x * (cubeSize*cubeSize) +
							y * cubeSize +
							z;
				cubes.push_front(c);
			}
	return cubes;
}

vertex* vertsFor(const Buffers& bs) {
	vertex* vertices = (vertex*)mem_alloc(sizeof(vertex) * bs.vertCount);
	const float scale = 0.0325f;
	for ( int i = 0; i < bs.vertCount; ++i ) {
		vertices[i].position = bs.verts[i];
		vertices[i].color = 0;
		vertices[i].uv.x = vertices[i].position.x * scale;
		vertices[i].uv.y = vertices[i].position.y * scale;
		vertices[i].normal = bs.normals[i];
		vertices[i].normal.w = vertices[i].position.z * scale;
	}
	return vertices;
}

MarchBlock* march(const BlockExtents b, const densityGrid& densities) {
	(void)b;
	seq<cube> cubes = cubesFor(densities);

	val triangles = new seq<triangle>();
	for( auto c : cubes ) {
		val tris = trianglesFor(c);
		for( auto t : tris) triangles->push_front(t);
	}

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
	(void)bs;
	(void)vertices;
	/*
	render_resetModelView( );
	if (bs.indexCount > 0 && marching_texture->gl_tex && marching_texture->gl_tex ) {
		val draw = drawCall::create( &renderPass_main, *Shader::byName("dat/shaders/terrain.s"), bs.indexCount, bs.indices, vertices, marching_texture->gl_tex, modelview);
		draw->texture_b = marching_texture_cliff->gl_tex;		
		draw->texture_c = marching_texture->gl_tex;		
		draw->texture_d = marching_texture_cliff->gl_tex;		
	}
	*/
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
#define TestCubes 8
MarchBlock* static_cube[TestCubes] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void test_marching_draw() {
	for ( int i = 0; i < TestCubes; ++i )
		if (static_cube[i])
			drawMarchedCube( static_cube[i]->buffers, static_cube[i]->verts );
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

	if ( !marching_texture )       { marching_texture       = texture_load( "dat/img/terrain/grass.tga" ); }
	if ( !marching_texture_cliff ) { marching_texture_cliff = texture_load( "dat/img/terrain/cliff_grass.tga" ); }

	const float w = 5.0;
	auto blockDimensions = Vector(cubesPerWidth * w, cubesPerWidth * w, cubesPerWidth * w, 0.f);
	
	vector origins[8];
	origins[0] = origin;
	origins[1] = vector_add(origin, Vector(-cubesPerWidth * w, 0.f, 0.f, 0.f));
	origins[2] = vector_add(origin, Vector(0.f, 0.f, cubesPerWidth * w, 0.f));
	origins[3] = vector_add(origin, Vector(-cubesPerWidth * w, 0.f, cubesPerWidth * w, 0.f));
	origins[4] = vector_add(origin, Vector(0.f, -cubesPerWidth * w, 0.f, 0.f));
	origins[5] = vector_add(origin, Vector(-cubesPerWidth * w, -cubesPerWidth * w, 0.f, 0.f));
	origins[6] = vector_add(origin, Vector(0.f, -cubesPerWidth * w, cubesPerWidth * w, 0.f));
	origins[7] = vector_add(origin, Vector(-cubesPerWidth * w, -cubesPerWidth * w, cubesPerWidth * w, 0.f));

	(void)blockDimensions;
	for (int i = 0; i < TestCubes; ++i)
		static_cube[i] = generateBlock(BlockExtents(origins[i], blockDimensions, cubesPerWidth), densityFn);
}
