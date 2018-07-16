// Block.cpp

namespace terrain {

// TODO - newtype?
//
// #ifdef NEWTYPE_SAFE
// #define NEWTYPE( alias, inner ) \
      struct alias { \
        inner value; \
      } \
   #define unwrap( newtype ) newtype.value \
   #else \
   #define NEWTYPE( alias, inner ) \
      typedef alias = inner; \
   #define unwrap( newtype ) newtype \
   #endif

// Generate Normals
//
// Reads position information from `verts` and produces resulting normals in
// the array allocated `normals`
//
vector* BlockSpec::generateNormals( vector* verts, vector* normals ) {
  const int lod = lodRatio();
  for ( int v = 0; v < samples.v(); ++v ) {
    for ( int u = 0; u < sampels.u(); ++u ) {
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

auto BlockSpec::generatePoints( vector* verts ) -> vector* {
  // u,v coords are relative (to the block)
  for ( int v = -1; v < samples.v()+1; ++v )
    for ( int u = -1; u < samples.u()+1; ++u )
      // generate verts[u][v]
      const int index = indexFromUV(u, v);
      verts[index] = canyon_sampleUV(u,v);
  return verts;
}

auto BlockSpec::generateBlock() -> Future<Block> {
  vector* verts = stackArray( vector, vertCount());
  vector* normals = stackArray( vector, vertCount());

  lodVectors( generatePoints( vertSources, verts ));
  lodVectors( generateNormals( verts, normals ));

  // TODO
  const auto collision = calculateCollision( this, verts );
  const auto aabb = calculateAABB( this, verts );
  auto renderable = generateRenderable( verts, normals );

  auto b = renderable.map( [=](auto r) {
    return Block( r, collision, aabb ));
  });

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
void BlockSpec::lodVectors( vector* vectors ) {
  const int lod = lodRatio();
  for ( int v = 0; v < samples.v(); ++v )
    if ( v % lod != 0 ) {
      vectors[indexFromUV( 0,             v )] = lodV( b, vectors, 0, v, lod );
      vectors[indexFromUV( samples.u()-1, v )] = lodV( b, vectors, samples.u()-1, v, lod );
    }
  for ( int u = 0; u < b->u_samples; ++u )
    if ( u % lod != 0 ) {
      vectors[indexFromUV( u, 0             )] = lodU( b, vectors, u, 0, lod );
      vectors[indexFromUV( u, samples.v()-1 )] = lodU( b, vectors, u, samples.v()-1, lod );
    }
}

auto BlockSpec::positionsFromUV( int u, int v ) -> vec2f {
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

auto BlockSpec::textureUV(vector& position, float v_pos) -> vector {
  return Vector( position.coord.x * texture_scale,
                 position.coord.y * texture_scale,
                 position.coord.z * texture_scale,
                 // TODO - do we need u_min? Can we calculate it?
                 canyon_uvMapped( v_min * texture_scale, v_pos * texture_scale ));
}

auto BlockSpec::generateRenderable( vector* verts, vector* normals ) -> Future<GLRenderable> {
  // We need to allocate these buffers here.
  // They can't be stack allocated as we're going to pass them to the
  // GPU thread to upload (asynchronously) to the GPU, and then free
  // once they're done.
  // Use unique_ptr so ownership is passed to the closure and they 
  // are cleaned up correctly.
  auto vertexBuffer = make_unique<Vector<Vertex>>(); // TODO
  auto elementBuffer = make_unique<Vector<short>>(); // TODO
  vertex* vertices = &vertexBuffer[0];
  short* elements = &elementBuffer[0];
  #if CANYON_TERRAIN_INDEXED
  for ( int v_index = 0; v_index < samples.v(); ++v_index ) {
    for ( int u_index = 0; u_index < samples.u(); ++u_index ) {
      int i = indexFromUV( u_index, v_index );
      float v = positionsFromUV( u_index, v_index ).v();
      if (validIndex( b, u_index, v_index )) {
        int vertIndex = renderIndexFromUV( u_index, v_index );
        vAssert( vertIndex < renderVertCount()); vAssert( vertIndex >= 0 );
        vertices[vertIndex].position = verts[i];
        vector uv = textureUV( b, &verts[i], v );
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
      vert.uv = calcUV( b, &vert.position, vPos );
      canyonTerrainBlock_fillTrianglesForVertex( b, verts, b->vertex_buffer, u, v, &vert );
    }
  }
#endif // CANYON_TERRAIN_INDEXED

  // We've created our buffers; copy them to the GPU for optimized rendering
  // as we know they're static
  // TODO return the actual future
  return copyToStaticGpuBuffers(vertCount, vertexBuffer, elementCount, elementBuffer).map([](auto buffers) {
    auto shader = Shader::byName("dat/shaders/terrain.s");
    return GLRenderable( buffers.get<0>(), buffers.get<1>(), shader, elementCount);
  });
}

// Create GPU vertex buffer objects to hold our data and save transferring to the GPU each frame
// If we've already allocated a buffer at some point, just re-use it
auto copyToStaticGpuBuffers( int vertCount, vertex* verts, int elementCount, unsigned short* elements ) -> Future<Pair<VBO,EBO>> {
  // TODO implement
  Future<VBO> vs = render_requestBuffer( GL_ARRAY_BUFFER,         verts,    sizeof( vertex )   * vertCount    );
  Future<EBO> es = render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, elements, sizeof( GLushort ) * elementCount );
  return fpair(vs, es); // TODO - pair two futures together
}

} // terrain::
