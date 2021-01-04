use tobj;
use vulkano::{
    buffer::CpuBufferPool,
    device::Device,
    framebuffer::RenderPassAbstract,
};

use std::{
    sync::Arc,
    path::Path,
};

use crate::{
    call::{Drawable, Call, CallT},
    create_pipeline,
    shaders::{vs, fs},
    types::vertex::{Vertex, vertex_buffer},
};

pub struct Mesh {
    pub verts: Vec<Vertex>,
    pub vs: vs::Shader,
    pub fs: fs::Shader,
}

pub struct Model {
    // For now a single mesh
    pub mesh: Mesh,
}

impl Model {
    pub fn from_obj(device: Arc<Device>, filename: &str) -> Result<Model, String> {
        let (mut models, _materials) = tobj::load_obj(&Path::new(filename)).map_err(|_| "Error")?;

        if models.len() != 1 {
            return Err(String::from("Must be 1 single model"));
        };

        let mesh = &mut models[0].mesh;
        let (positions, normals, indices) = (&mut mesh.positions, &mut mesh.normals, &mut mesh.indices);
        let default_color = [ 0.7, 0.7, 0.7, 1.0 ];
        let verts: Vec<_> = indices.iter().map(|&i| {
            let i = (i as usize) * 3;
            // TODO - coord system is upside down?
            Vertex{ position: [positions[i], -positions[i+1], positions[i+2]],
            normal: [normals[i], normals[i+1], normals[i+2]],
            color: default_color }
        }).collect();
        let vs = vs::Shader::load(device.clone()).unwrap();
        let fs = fs::Shader::load(device.clone()).unwrap();
        let mesh = Mesh{ verts, fs, vs };
        Ok(Model{ mesh })
    }
}

impl<P: RenderPassAbstract + Send + Sync + 'static + Clone> Drawable<P> for Model {
    fn draw_call(&self, device: Arc<Device>, pass: P, _buffer_pool: &CpuBufferPool<vs::ty::Data>) -> Box<dyn Call<P>> {
        let verts = vertex_buffer(device.clone(), &self.mesh.verts);
        let pipeline = create_pipeline(device, &self.mesh.vs, &self.mesh.fs, pass.clone());
        // TODO bind the descriptor
        //let descriptors =
        //Arc::new(PersistentDescriptorSet::start(pipeline.clone(), 0).build().unwrap());
        Box::new(CallT::new(pipeline, verts, None))
    }
}
