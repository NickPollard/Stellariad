use {
    std::{
        sync::Arc,
        path::Path,
    },
    tobj,
    vulkano::{
        buffer::CpuBufferPool,
        device::Device,
        framebuffer::{RenderPassAbstract, Subpass},
        pipeline::{ blend::AttachmentBlend, GraphicsPipeline },
    },
};

use crate::{
    call::{Drawable, Call, CallT},
    material::Material,
    shaders::{vs, fs},
    types::{
        DynPass,
        Pipe,
        vertex::{Vertex, vertex_buffer, VertDef},
    },
};

pub struct Mesh {
    pub verts: Vec<Vertex>,
    pub vs: vs::Shader,
    pub fs: fs::Shader,
    material: Material<DynPass>,
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
        let material = Material::new(vs, fs);
        let vs = vs::Shader::load(device.clone()).unwrap();
        let fs = fs::Shader::load(device.clone()).unwrap();
        let mesh = Mesh{ verts, fs, vs, material };
        Ok(Model{ mesh })
    }
}

impl<P: RenderPassAbstract + Send + Sync + 'static + Clone> Drawable<P> for Model {
    fn draw_call(&self, pass: P, _buffer_pool: &CpuBufferPool<vs::ty::Data>) -> Box<dyn Call<P>> {
        let verts = vertex_buffer(pass.device().clone(), &self.mesh.verts);
        //let pipeline = create_pipeline(&self.mesh.vs, &self.mesh.fs, pass.clone());
        let pipeline = self.mesh.material.pipeline(pass);
        // TODO bind the descriptor
        Box::new(CallT::new(pipeline, verts, None))
    }
}

/// Create a simple GraphicsPipeline for using a given vertex shader and fragment shader
/// A GraphicsPipeline is a collection of shaders and settings for executing a draw call
pub fn create_pipeline<P: RenderPassAbstract>(
    vs: &vs::Shader,
    fs: &fs::Shader,
    render_pass: P
  ) -> Arc<GraphicsPipeline<VertDef, Pipe, P>> {
    let device = render_pass.device().clone();
    Arc::new(GraphicsPipeline::start()
             .vertex_input_single_buffer::<Vertex>()
             .vertex_shader(vs.main_entry_point(), ())
             // Our vertex buffer is a triangle list
             .triangle_list()
             .viewports_dynamic_scissors_irrelevant(1)
             .fragment_shader(fs.main_entry_point(), ())
             .depth_stencil_simple_depth()
             .render_pass(Subpass::from(render_pass, 0).unwrap())
             .blend_collective(AttachmentBlend::alpha_blending())
             .build(device)
             .unwrap())
}
