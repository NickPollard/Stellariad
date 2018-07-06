// Block.h

namespace Terrain {
  struct BlockSpec {
    // Construct a new Block from a given BlockSpec
    auto generateBlock() const -> Future<Block>;

    const vec2i position; // Where this block is
    const vec2i samples;  // The resolution of the heightmap

    auto vertCount() const -> int { return ( samples.u() +2 ) * ( samples.v() +2); } // +2 because we extend 1 step further on each edge of the block
    auto triangleCount() const -> int { return ( samples.u() - 1 ) * ( samples.v() - 1 ) * 2; }
    auto elementCount() const -> int { return triangleCount() * 3; }
    auto indexFromUV( int u, int v ) const -> int { return u + 1 + ( v + 1 ) * ( samples.u() + 2 ); }

    private:
      auto generatePoints( vector* verts ) const -> vector*;
      auto generateNormals( vector* verts, vector* normals ) const -> vector*;
      void lodVectors( vector* vectors ) const;
  };

  struct Block {
    BlockSpec spec;
    Option<GLRenderable> renderable;
    // collision collision;
    // aabb aabb; TODO is this just in the renderable?

    auto draw() -> bool { return renderable.draw(); }
  };

  struct GLRenderable {
    shader** shader;
    GLuint vertexBufferObject;
    GLuint elementBufferObject;
    int elementCount;
    GLint texture;
    GLint textureB;
    GLint textureC;
    GLint textureD;

    aabb bb;

    bool draw();

    GLRenderable( VBO vs, EBO es, shader** s, int elems ) :
      shader(s),
      vertexBufferObject(vs.value),
      elementBufferObject(es.value),
      elementCount(elems) {}

    // TODO - release buffers once we're done
  };
}
