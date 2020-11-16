use {
    cgmath::Matrix4,
    std::marker::PhantomData,
};

/// A projection from `A` space to `B` space
/// e.g. a Projection<Camera, World> maps from World space to Camera space
pub struct Projection<B, A> {
    matrix: Matrix4<f64>,
    _phantom: PhantomData<*const (B,A)>
}

impl<B,C> Projection<C,B> {
    pub fn from_matrix(m: Matrix4<f64>) -> Projection<C,B> {
        Projection{ matrix: m, _phantom: PhantomData }
    }

    /// Compose two projections, `self` after `before`
    pub fn compose<A>(&self, before: &Projection<B,A>) -> Projection<C,A> {
        Projection{ matrix: self.matrix * before.matrix, _phantom: PhantomData }
    }
}

/// Object Space
pub struct Object {}
/// World Space
pub struct World {}
/// Camera Space
pub struct Camera {}
/// Screen Space
pub struct Screen {}
