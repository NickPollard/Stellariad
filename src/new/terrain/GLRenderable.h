// GLRenderable.h

#include "render/render.h"
#include "frustum.h"

namespace terrain {
  struct VBO { GLuint value; };
  struct EBO { GLuint value; };

  struct GLRenderable {
    shader** _shader;
    GLuint vertexBufferObject;
    GLuint elementBufferObject;
    int   elementCount;
    GLint texture;
    GLint textureB;
    GLint textureC;
    GLint textureD;

    aabb bb;

    bool draw( scene* s );

    GLRenderable( VBO vs, EBO es, shader** s, int elems ) :
      _shader(s),
      vertexBufferObject(vs.value),
      elementBufferObject(es.value),
      elementCount(elems) {}

    // TODO - release buffers once we're done
  };
} // Terrain::
