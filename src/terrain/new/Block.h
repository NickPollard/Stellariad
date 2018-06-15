// Block.h

namespace Terrain {
  struct Block {
    Option<GLRenderable> renderable;

    vertex*         vertexBuffer;
    unsigned short* elementBuffer;

    auto triangleCount() -> int { return ( uSamples - 1 ) * ( vSamples - 1 ) * 2; }
    auto elementCount() -> int { return triangleCount() * 3; }

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
