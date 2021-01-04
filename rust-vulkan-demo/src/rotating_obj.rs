use {
    cgmath::{Matrix3, Matrix4, Vector3, Rad, PerspectiveFov},
    gfx::{
        create_pipeline,
        descriptors_for,
        call::{Drawable, Call, CallT},
        model::Model,
        shaders::{self, vs},
        types::{
            SubBuffer,
            vertex::{Vertex, vertex_buffer},
        },
    },
    std::{
        sync::Arc,
        time::{Instant, Duration},
        path::Path,
    },
    vulkano::{
        device::Device,
        buffer::cpu_pool::CpuBufferPool,
        framebuffer::RenderPassAbstract,
    },
};

use crate::{
    types::Tickable,
};

pub struct Mesh {
    verts: Vec<Vertex>,
    vs: shaders::vs::Shader,
    fs: shaders::fs::Shader,
}

pub struct RotatingObj {
    model: Model,
    rotation_start: Instant,
}

impl RotatingObj {
    pub fn new(model: Model) -> RotatingObj {
        let rotation_start = Instant::now();
        RotatingObj{ model, rotation_start }
    }
}

impl<P: RenderPassAbstract + Send + Sync + 'static + Clone>
  Drawable<P> for RotatingObj {
    // TODO(nickpollard) - this uniform buffer is shader specific
    fn draw_call(&self, device: Arc<Device>, pass: P, buffer_pool: &CpuBufferPool<vs::ty::Data>) -> Box<dyn Call<P>> {
        let verts = vertex_buffer(device.clone(), &self.model.mesh.verts);
        let pipeline = create_pipeline(device, &self.model.mesh.vs, &self.model.mesh.fs, pass);
        let desc_layout = pipeline.layout().descriptor_set_layout(0).unwrap();

        let uniform_buffer_subbuffer = rotation_buffer(buffer_pool, self.rotation_start);
        let descriptors = descriptors_for(uniform_buffer_subbuffer, desc_layout.clone());

        Box::new(CallT::new(pipeline, verts, Some(descriptors)))
    }
}

impl Tickable for RotatingObj {
    fn tick(&mut self, _dt: Duration) {
    }
}

fn rotation_buffer(
    buffer_pool: &CpuBufferPool<vs::ty::Data>,
    rotation_start: Instant)
-> SubBuffer<vs::ty::Data> {
    let elapsed = rotation_start.elapsed();
    let rotation = elapsed.as_secs() as f64 + elapsed.subsec_nanos() as f64 / 1_000_000_000.0;
    let rotation = Matrix3::from_angle_y(Rad(rotation as f32));

    let near = 1.0;
    let far = 1800.0;
    let fovy : Rad<f32> = Rad(0.8);
    let width = 16.0;
    let height = 9.0;
    let aspect = width / height;
    let perspective: Matrix4<f32> = (PerspectiveFov{ fovy, aspect, near, far }).into();

    let translate = Matrix4::from_translation(Vector3::new(0.0, 0.0, -30.0));

    let uniform_data = vs::ty::Data {
        world: (perspective * translate * Matrix4::from(rotation)).into(),
    };

    buffer_pool.next(uniform_data).unwrap()
}
