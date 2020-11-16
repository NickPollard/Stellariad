# TODO

* *DONE* Load a .obj file into a model
* *DONE* Perspective transform (e.g. so rotation isn't broken)
* GFX
  * *DONE* Shade it based on depth (e.g. fog)
  * Load normals into vertex data (e.g. colour on normals)
  * Light from a single point or directional light
* CAMERA
  * Pass in separate world->screen space matrix, from perspective matrix
* Do a depth pass

## Scene

We need to store a camera somewhere - under the scene?
Will need to be updated from the engine, but read from the renderer

struct Transform {
  transformation: Mat4
}

struct Camera {
  position: Transform
}

struct Transformation<To,From> {
  trans: Mat4,
  to: PhantomData<To>,
  from PhantomData<From>,
}

impl<To, From> Transformation<To, From> {
  fn compose(contra: Transformation<From,Prev>) -> Transformation<To,Prev>;
}

trait Camera {
  fn camera_from_world() -> Mat4 {
  }
}

## Input
