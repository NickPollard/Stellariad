// Block.cpp

namespace Terrain {

bool GLRenderable::draw( scene* s ) {
  if ( frustum_cull( &bb, s->cam->frustum ) )
    return false;

  // TODO - use a cached drawcall
  drawCall* draw = drawCall::create(
    &renderPass_main,
    *shader,
    elementCount,
    null, // use VBO
    null, // use EBO
    texture,
    modelview );
  draw->texture_b = textureB;
  draw->texture_c = textureC;
  draw->texture_d = textureD;
  draw->texture_normal = terrain_texture->gl_tex;
  draw->texture_b_normal = terrain_texture->gl_tex;
  draw->vertex_VBO = vertexBufferObject;
  draw->element_VBO = elementBufferObject;

  drawCall* drawDepth = drawCall::create(
    &renderPass_depth,
    *Shader::depth(),
    elementCount,
    null, // use VBO
    null, // use EBO
    texture,
    modelview );
  drawDepth->vertex_VBO = vertexBufferObject;
  drawDepth->element_VBO = elementBufferObject;
  return true;
}

Future<GLRenderable> createRenderable(Block& block) {
  auto elementCount = block.elementCount();
  auto buffers = createBuffers( block.vertexBuffer, elementCount, block.elementBuffer );
  buffers.map([](bs){
    return new GLRenderable(bs.get<0>(), bs.get<1>(), block.terrainShader, elementCount);
  })
}

struct VBO {
  GLuint value;
}
struct EBO {
  GLuint value;
}

// Create GPU vertex buffer objects to hold our data and save transferring to the GPU each frame
// If we've already allocated a buffer at some point, just re-use it
Future<Pair<VBO,EBO>> createBuffers( vertCount, vertex* verts, int elementCount, unsigned short* elements ) {
  // TODO implement
  Future<VBO> vs = render_requestBuffer( GL_ARRAY_BUFFER,         verts,    sizeof( vertex )   * vertCount    );
  Future<EBO> es = render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, elements, sizeof( GLushort ) * elementCount );
  return fpair(vs, es); // TODO - pair two futures together
}

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
  vector* verts = (vector*)stackArray( vector, vertCount());
  vector* normals = (vector*)stackArray( vector, vertCount());

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

auto BlockSpec::generateRenderable( vector* verts, vector* normals ) -> Future<GLRenderable> {
  #if CANYON_TERRAIN_INDEXED
  for ( int v_index = 0; v_index < samples.v(); ++v_index ) {
    for ( int u_index = 0; u_index < samples.u(); ++u_index ) {
      int i = indexFromUV( u_index, v_index );
      float u,v;
      canyonTerrainBlock_positionsFromUV( b, u_index, v_index, &u, &v );
      if (validIndex( b, u_index, v_index )) {
        int buffer_index = canyonTerrainBlock_renderIndexFromUV( b, u_index, v_index );
        vAssert( buffer_index < canyonTerrainBlock_renderVertCount( b ));
        vAssert( buffer_index >= 0 );
        r->vertex_buffer[buffer_index].position = verts[i];
        vector uv = calcUV( b, &verts[i], v );
        r->vertex_buffer[buffer_index].uv = Vec2( uv.coord.x, uv.coord.y );
        r->vertex_buffer[buffer_index].color = intFromVector(Vector( canyonZone_terrainBlend( v ), 0.f, 0.f, 1.f ));
        r->vertex_buffer[buffer_index].normal = normals[i];
        r->vertex_buffer[buffer_index].normal.coord.w = uv.coord.z;
      }
    }
  }
  int triangleCount = canyonTerrainBlock_triangleCount( b );
  int i = 0;
  for ( int v = 0; v + 1 < samples.v(); ++v ) {
    for ( int u = 0; u + 1 < samples.u(); ++u ) {
      vAssert( i * 3 + 5 < r->element_count );
      vAssert( canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 ) < canyonTerrainBlock_renderVertCount( b ) );
      r->element_buffer[ i * 3 + 0 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v );
      r->element_buffer[ i * 3 + 1 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
      r->element_buffer[ i * 3 + 2 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
      r->element_buffer[ i * 3 + 3 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
      r->element_buffer[ i * 3 + 4 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 );
      r->element_buffer[ i * 3 + 5 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
      i+=2;
    }
  }
  vAssert( i == triangleCount );
#else
  for ( int v = 0; v < samples.v(); v ++ ) {
    for ( int u = 0; u < samples.u(); u ++  ) {
      int i = indexFromUV( u, v );
      vertex vert;
      vert.position = verts[i];
      vert.normal = normals[i];
      float u_pos, v_pos;
      canyonTerrainBlock_positionsFromUV( b, u, v, &u_pos, &v_pos );
      vert.uv = calcUV( b, &vert.position, v_pos );
      canyonTerrainBlock_fillTrianglesForVertex( b, verts, b->vertex_buffer, u, v, &vert );
    }
  }
#endif // CANYON_TERRAIN_INDEXED

  // We've created our buffers; copy them to the GPU for optimized rendering
  // as we know they're static
  // TODO return the actual future
  return copyToStaticGpuBuffers(vertex_buffer, element_buffer);
}

} // Terrain::
