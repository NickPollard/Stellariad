// geometry.h

/*
 * Renderable tri-based geometry
 */

namespace render {
  using VertexBuffer = GLuint*;
  using ElementBuffer = GLuint*;

  struct Geometry {
    VertexBuffer  verts;
    ElementBuffer elems;
    int           indexCount;
  };
}
