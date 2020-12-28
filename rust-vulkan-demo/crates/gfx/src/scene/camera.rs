use crate::scene::space::{self, Projection};

/// A Camera is a type which supports a mapping from World space to Camera Space, and a mapping
/// from Camera Space to Screen Space
pub struct Camera {
    #[allow(unused)]
    camera_space: Projection<space::Camera, space::World>,
    #[allow(unused)]
    projection: Projection<space::Screen, space::Camera>,
}

impl Camera {
    /// Return the transformation matrix
    pub fn camera_from_world() -> Projection<space::Camera, space::World> {
        // TODO
        panic!()
    }

    /// Return the projection matrix
    pub fn projection() -> Projection<space::Screen, space::Camera> {
        // TODO
        panic!()
    }

    pub fn origin() -> Camera {
        let identity = cgmath::Matrix4::from_scale(1.0);
        Camera { camera_space: Projection::from_matrix(identity), projection: Projection::from_matrix(identity) }
    }
}
