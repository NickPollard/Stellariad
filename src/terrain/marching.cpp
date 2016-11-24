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
#include "canyon.h"
#include "terrain.h"
#include "test.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/render.h"
#include "render/shader.h"
#include "render/texture.h"

//#define val const auto
//#define var auto

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
//typedef float (*DensityFunction) (vector); // Sample a density at a coord
typedef function<float(vector)> DensityFunction; // Sample a density at a coord

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
	// b.min -> world space origin for the block
	// calculate world-space positions and sample as such
	const auto delta = (b.max.x - b.min.x) / (float)(cubeSize - 1);

	auto g = Grid<vector, cubeSize>();
	for ( int i = 0; i < cubeSize; ++i )
		for ( int j = 0; j < cubeSize; ++j )
			for ( int k = 0; k < cubeSize; ++k )
				g.values[i][j][k] = vector_add(Vector( i * delta, j * delta, k * delta, 0.0 ), b.min);
	return g;
}

auto densitiesFor( BlockExtents b, DensityFunction d ) -> Grid<tuple<vector,float>, cubeSize> {
	return coordsFor( b ).fproduct( d );
}

struct cube {
	vector corners[8]; // in a particular order - bits for x/y/z?
	float  densities[8]; // in same order
	int origin;
};

int triIndex(cube c) {
	int index = 0;
	const auto surface = 0.f;
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
	const auto f = fabsf(dA) / (max(dA,dB) - min(dA,dB));
	return veclerp( a, b, f );
}

auto cornersFor(int edge) -> tuple<int, int> {
	return make_tuple(edges[edge][0], edges[edge][1]);
}

int offsetOf( int index, int w ) {
	const auto offset = edgeOffset[index];
	// TODO - could compute these offline if w is static?
	return (offset.x * (w*w) + offset.y * w + offset.z) * 3 + offset.dir;
}

// index defines which edge cube it is
auto cubeVert( cube c, int index, int origin ) -> vx {
	// given a cube c, and an index indicating an edge-vertex on that cube, calculate the actual
	// vertex position using the corner densities and positions
	const auto corners = cornersFor(index);
	int first = get<0>(corners);
	int second = get<1>(corners);
	const auto a = c.corners[first];
	const auto b = c.corners[second];
	const auto dA = c.densities[first];
	const auto dB = c.densities[second];
	const auto v = interpD( a, dA, b, dB );
	const auto width = cubeSize;
	auto hash = origin * 3 + offsetOf(index, width);
	vAssert( hash >= 0 );
	return make_tuple(v, hash); // Except we need to tuple it with a hash
}

triangle tri(vx a, vx b, vx c) {
	const auto t = triangle { { a, b, c } };
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
	const auto index = triIndex(c);
	const auto maxIndexSize = 16;
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
	for (const auto t: tris) {
		const auto normal = t.normal();
		for (int i = 0; i < 3; ++i ) {
			auto hash = t.hash(i);
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
	for ( const auto t : tris ) {
		for (int j = 0; j < 3; j++) {
			const auto hash = t.hash(j);
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
	seq<cube> cubes = cubesFor(densities);

	const auto triangles = new seq<triangle>();
	for( auto c : cubes ) {
		const auto tris = trianglesFor(c);
		for( auto t : tris) triangles->push_front(t);
	}

	const auto buffers = buffersFor(*triangles);
	const auto verts = vertsFor(buffers);
	MarchBlock* m = (MarchBlock*)mem_alloc(sizeof( MarchBlock ));
	m->extents = b;
	m->verts = verts;
	m->buffers = buffers;
	return m;
}

//////////////////////////////
//float fromHeightField(float height, float y) {
//	return height - (y - 0.f);
//}

// TODO - cache this so that we don't do a hugely high number of terrain samples
/*
   We need to cache these in some kind of struct beforehand
   do we
     a) on demand cache them
	 b) ahead of time cache them
   */
struct CachedHeights {
	CachedHeights() {
			for (int i = 0; i < width; ++i)
				for (int j = 0; j < height; ++j)
						positions[i][j] = FLT_MAX;
	}
	auto getOrElseUpdate(vector v, function<float(vector)> sample) -> float {
		static const int wrap = 1024;
		const auto i = (wrap - (int)floorf(v.x)) % wrap;
		const auto j = (wrap - (int)floorf(v.z)) % wrap;
		if (positions[i][j] == FLT_MAX)
			positions[i][j] = sample(v);
		return positions[i][j];
	}

	static const int width = 1024;
	static const int height = 1024;
	float positions[width][height];
};

auto fromHeightFieldWithCache(function<float(vector)> heightfield) -> function<float(vector)> {
	CachedHeights* cached = new CachedHeights();
	return function<float(vector)>([cached, heightfield](vector v) { 
		const auto height = cached->getOrElseUpdate(v, heightfield);
		return height - v.y;
	});
}

float densityFn(canyon* c, vector pos) {
	float u, v;
	canyonSpaceFromWorld( c, pos.x, pos.z, &u, &v );
	float height = canyonTerrain_sampleUV( u, v );
	return height - pos.y;
}

void drawMarchedCube(const Buffers& bs, vertex* vertices) {
	(void)bs;
	(void)vertices;
	render_resetModelView( );
	if (bs.indexCount > 0 && marching_texture->gl_tex && marching_texture->gl_tex ) {
		const auto draw = drawCall::create( &renderPass_main, *Shader::byName("dat/shaders/terrain.s"), bs.indexCount, bs.indices, vertices, marching_texture->gl_tex, modelview);
		draw->texture_b = marching_texture_cliff->gl_tex;		
		draw->texture_c = marching_texture->gl_tex;		
		draw->texture_d = marching_texture_cliff->gl_tex;		
	}
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

void buildMarchingCubes(canyon* c) {
	test_interp();

	if ( !marching_texture )       { marching_texture       = texture_load( "dat/img/terrain/grass.tga" ); }
	if ( !marching_texture_cliff ) { marching_texture_cliff = texture_load( "dat/img/terrain/cliff_grass.tga" ); }

	const float w = 5.0;
	const float width = w * cubesPerWidth;
	auto blockDimensions = Vector(width, width, width, 0.f);
	
	vector origins[8];
	origins[0] = origin;
	origins[1] = vector_add(origin, Vector(-width, 0.f, 0.f, 0.f));
	origins[2] = vector_add(origin, Vector(0.f, 0.f, width, 0.f));
	origins[3] = vector_add(origin, Vector(-width, 0.f, width, 0.f));
	origins[4] = vector_add(origin, Vector(0.f, -width, 0.f, 0.f));
	origins[5] = vector_add(origin, Vector(-width, -width, 0.f, 0.f));
	origins[6] = vector_add(origin, Vector(0.f, -width, width, 0.f));
	origins[7] = vector_add(origin, Vector(-width, -width, width, 0.f));

	const auto fn = function<float(vector)>([c](vector v) { return densityFn(c, v); });
	const auto dnFn = fromHeightFieldWithCache(fn);

	for (int i = 0; i < TestCubes; ++i)
		//static_cube[i] = generateBlock(BlockExtents(origins[i], blockDimensions, cubesPerWidth), function<float(vector)>([c](vector v) { return densityFn(c, v); }));
		static_cube[i] = generateBlock(BlockExtents(origins[i], blockDimensions, cubesPerWidth), dnFn );
}
