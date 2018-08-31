// Block.cpp

#include "common.h"
#include "Block.h"
//-
//#include <memory> // for std::make_unique
#include <utility> // for std::pair
#include "canyon.h"                 // for terrain_worldSpaceFromCanyon
#include "render/shader.h"
#include "terrain/terrain.h"        // for canyon_sampleUV
//#include "terrain/terrain_collision.h" // for terrainBlock_calculateCollision

using namespace brando;
using brando::concurrent::Future;
using std::move;

const float block_texture_repeat = 10.f;

float _uvMapped( float block_minimum, float f ) { return f - ( block_texture_repeat * floorf( block_minimum / block_texture_repeat )); }

namespace terrain {

// Generate Normals
//
// Reads position information from `verts` and produces resulting normals in
// the array allocated `normals`
//
vector* BlockSpec::generateNormals( vector* verts, vector* normals ) const {
  const int lod = lodRatio();
  for ( int v = 0; v < samples.v(); ++v ) {
    for ( int u = 0; u < samples.u(); ++u ) {
      const vector left   = verts[indexFromUV( u, (v == samples.v() - 1) ? v - lod : v - 1 )];
      const vector right  = verts[indexFromUV( u, (v == 0) ? v + lod : v + 1)];
      const vector top    = verts[indexFromUV( (u == samples.u() - 1) ? u - lod : u - 1, v )];
      const vector bottom = verts[indexFromUV( (u == 0) ? u + lod : u + 1, v )];

      const int thisIndex = indexFromUV( u, v );

      const vector a = vector_sub(bottom, top);
      const vector b = vector_sub(right, left);

      vAssert( thisIndex < vertCount() );

      normals[thisIndex] = normalized(vector_cross(a, b));
    }
  }
  return normals;
}

auto BlockSpec::generatePoints( vector* verts ) const -> vector* {
  // u,v coords are relative (to the block)
  for ( int v = -1; v < samples.v()+1; ++v )
    for ( int u = -1; u < samples.u()+1; ++u ) {
      // generate verts[u][v]
      const int index = indexFromUV(u, v);
      const vec2f canyonSpace = positionsFromUV( u, v );
      float x, z = 0.f;
      terrain_worldSpaceFromCanyon( &terrain._canyon, canyonSpace.u(), canyonSpace.v(), &x, &z );
      verts[index] = Vector( x, canyon_sampleUV(u,v), z, 1.f );
    }
  return verts;
}

auto BlockSpec::generateBlock() const -> Future<Block*> {
  vector* verts = stackArray( vector, vertCount());
  vector* normals = stackArray( vector, vertCount());

  lodVectors( generatePoints( verts ));
  lodVectors( generateNormals( verts, normals ));

  // TODO
  //const auto collision = calculateCollision( this, verts );
  //const auto aabb = calculateAABB( this, verts );
  const body* collision = nullptr;
  auto renderable = generateRenderable( verts, normals );

  auto b = renderable.map( function<Block*(GLRenderable)>([=](auto r) {
    // TODO - pass in AABB as well
    Block* _b = new Block( *this, r, *collision );
    return _b;
  }));

// TODO
//   Where does this happen now?
// terrain->setBlock( position.u(), position.v(), Block( r, collision, aabb ) );

  return b;
}

// lodVectors
//
// Take an input array of `vectors`, and modify (in-place) each edge (ie. coord 0 or max)
// input to a LoD-adjusted output e.g. a bilinear filtering of the lower-LoD grid
//
void BlockSpec::lodVectors( vector* vectors ) const {
  const int lod = lodRatio();
  for ( int v = 0; v < samples.v(); ++v )
    if ( v % lod != 0 ) {
      vectors[indexFromUV( 0,             v )] = lodV( vectors, 0, v, lod );
      vectors[indexFromUV( samples.u()-1, v )] = lodV( vectors, samples.u()-1, v, lod );
    }
  for ( int u = 0; u < samples.u(); ++u )
    if ( u % lod != 0 ) {
      vectors[indexFromUV( u, 0             )] = lodU( vectors, u, 0, lod );
      vectors[indexFromUV( u, samples.v()-1 )] = lodU( vectors, u, samples.v()-1, lod );
    }
}

auto BlockSpec::lodU( vector* verts, int u, int v, int lodStep ) const -> vector {
  const int prev = indexFromUV( u-(u%lodStep),           v );
  const int next = indexFromUV( u-(u%lodStep) + lodStep, v );
  return vector_lerp(
      &verts[prev],
      &verts[next],
      static_cast<float>( u % lodStep ) / static_cast<float>(lodStep)
  );
}

auto BlockSpec::lodV( vector* verts, int u, int v, int lodStep ) const -> vector {
  const int prev = indexFromUV( u, v-(v%lodStep)           );
  const int next = indexFromUV( u, v-(v%lodStep) + lodStep );
  return vector_lerp(
      &verts[prev],
      &verts[next],
      static_cast<float>( v % lodStep ) / static_cast<float>(lodStep)
  );
}

auto BlockSpec::positionsFromUV( int u, int v ) const -> vec2f {
  const int lod = lodRatio();
  if ( u == -1          ) u = -lod;
  if ( u == samples.u() ) u = samples.u() -1 + lod;
  if ( v == -1          ) v = -lod;
  if ( v == samples.v() ) v = samples.v() -1 + lod;

  const float step = 4 / lod;
  const vec2f out = {
    static_cast<float>(blockSpaceMinU() + u * step) * terrain.uScale(),
    static_cast<float>(blockSpaceMinV() + v * step) * terrain.vScale()
  };
  return out;
}

auto BlockSpec::textureUV(vector& position, float v_pos) const -> vector {
  float texture_scale = 1.f; // TODO - from where?
  return Vector( position.coord.x * texture_scale,
                 position.coord.y * texture_scale,
                 position.coord.z * texture_scale,
                 // TODO - do we need u_min? Can we calculate it?
                 _uvMapped( v_min() * texture_scale, v_pos * texture_scale ));
}

// Create GPU vertex buffer objects to hold our data and save transferring to the GPU each frame
// If we've already allocated a buffer at some point, just re-use it
auto copyToStaticGpuBuffers(int vertCount, unique_ptr<std::vector<vertex>> vertexBuffer, int elementCount, unique_ptr<std::vector<short>> elementBuffer) -> Future<std::pair<VBO,EBO>> {
  (void)vertCount, (void)elementCount, (void)vertexBuffer, (void)elementBuffer;
  // TODO - asynchronously copy to the GPU, in the render thread
  concurrent::Executor& ex = *(concurrent::Executor*)(nullptr);
  return concurrent::Promise<std::pair<VBO,EBO>>(ex).future();

/*
  // TODO implement asynchronous requestBuffer with futures
  Future<VBO> vs = render_requestBuffer( GL_ARRAY_BUFFER,         verts,    sizeof( vertex )   * vertCount    );
  Future<EBO> es = render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, elements, sizeof( GLushort ) * elementCount );
  // TODO - pair two futures together
  return fpair(vs, es); 
*/
}

auto BlockSpec::generateRenderable( vector* verts, vector* normals ) const -> Future<GLRenderable> {
  // We need to allocate these buffers here.
  // They can't be stack allocated as we're going to pass them to the
  // GPU thread to upload (asynchronously) to the GPU, and then free
  // once they're done.
  // Use unique_ptr so ownership is passed to the closure and they 
  // are cleaned up correctly.
  auto vertexBuffer = make_unique<std::vector<vertex>>(); // TODO
  auto elementBuffer = make_unique<std::vector<short>>(); // TODO
  #if CANYON_TERRAIN_INDEXED
  vertex* vertices = &vertexBuffer[0];
  short* elements = &elementBuffer[0];
  for ( int v_index = 0; v_index < samples.v(); ++v_index ) {
    for ( int u_index = 0; u_index < samples.u(); ++u_index ) {
      int i = indexFromUV( u_index, v_index );
      float v = positionsFromUV( u_index, v_index ).v();
      if (validIndex( u_index, v_index )) {
        int vertIndex = renderIndexFromUV( u_index, v_index );
        vAssert( vertIndex < renderVertCount()); vAssert( vertIndex >= 0 );
        vertices[vertIndex].position = verts[i];
        vector uv = textureUV( verts[i], v );
        vertices[vertIndex].uv = Vec2( uv.coord.x, uv.coord.y );
        vertices[vertIndex].color = intFromVector(Vector( canyonZone_terrainBlend( v ), 0.f, 0.f, 1.f ));
        vertices[vertIndex].normal = normals[i];
        vertices[vertIndex].normal.coord.w = uv.coord.z;
      }
    }
  }
  // TODO couldn't this be one static buffer we re-use, like non-indexed?
  int i = 0;
  for ( int v = 0; v + 1 < samples.v(); ++v ) {
    for ( int u = 0; u + 1 < samples.u(); ++u ) {
      vAssert( i * 3 + 5 < r->element_count ); // TODO lift out of the loop
      vAssert( renderIndexFromUV( u + 1, v + 1 ) < renderVertCount() );
      elements[ i * 3 + 0 ] = renderIndexFromUV( u,     v     );
      elements[ i * 3 + 1 ] = renderIndexFromUV( u + 1, v     );
      elements[ i * 3 + 2 ] = renderIndexFromUV( u,     v + 1 );
      elements[ i * 3 + 3 ] = renderIndexFromUV( u + 1, v     );
      elements[ i * 3 + 4 ] = renderIndexFromUV( u + 1, v + 1 );
      elements[ i * 3 + 5 ] = renderIndexFromUV( u,     v + 1 );
      i+=2;
    }
  }
  vAssert( i == triangleCount() );
#else
  for ( int v = 0; v < samples.v(); v ++ ) {
    for ( int u = 0; u < samples.u(); u ++  ) {
      int i = indexFromUV( u, v );
      vertex vert;
      vert.position = verts[i];
      vert.normal = normals[i];
      float vPos = positionsFromUV( u, v ).v();
      vector uv = textureUV( vert.position, vPos );
      vert.uv = Vec2( uv.coord.x, uv.coord.y );
      // TODO
      //canyonTerrainBlock_fillTrianglesForVertex( verts, vertexBuffer, u, v, &vert );
    }
  }
#endif // CANYON_TERRAIN_INDEXED

  // We've created our buffers; copy them to the GPU for optimized rendering
  // as we know they're static
  // TODO return the actual future
  // TODO extend brando .map to allow lambda types
  int elems = elementCount();
  return copyToStaticGpuBuffers(renderVertCount(), move(vertexBuffer), elems, move(elementBuffer)).map(std::function<GLRenderable(std::pair<VBO,EBO>)>([elems](auto buffers) {
    auto shader = Shader::byName("dat/shaders/terrain.s");
    return GLRenderable( buffers.first, buffers.second, shader, elems);
  }));
}

} // terrain::
