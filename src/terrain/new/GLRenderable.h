// GLRenderable.h

namespace Terrain {
  struct GLRenderable {
    shader** shader;
    GLuint vertexBufferObject;
    GLuint elementBufferObject;
    int   elementCount;
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
} // Terrain::
