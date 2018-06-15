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

} // Terrain::

updateBlocks ()
{
  /* We have a set of current blocks, B, and a set of projected blocks based on the new position, B'
     All blocks ( b | b is in B n B' ) we keep, shifting their pointers to the correct position
     All other blocks fill up the empty spaces, then are recalculated */
  int bounds[2][2];
  int intersection[2][2];

  bounds = t.newBounds( sampleOrigin );

  // t.cache.update(bounds);

  if (!boundsEqual( bounds, t->bounds )) {
    intersection = t->bounds.intersect(newBounds);

    for (b : allBlocks) {
      if (!intersection.contains(coord(b))) {
        // newBlock
      }
      else {
        // dropBlock
      }
    }

    for (b : allBlocks) {
      if (b.lodLevel() < b.currentLodLevel || !intersection.contains(block)) {
        // regenerate()
      }
    }

    t.blocks = blocks;
    t.bounds = bounds;
  }
}
