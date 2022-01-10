use {
    std::sync::Arc,
    vulkano::{
        framebuffer::{RenderPassAbstract, Subpass},
        pipeline::{ blend::AttachmentBlend, GraphicsPipeline },
    },
};

use crate::{
    shaders::{vs, fs},
    types::{
        Pipe,
        vertex::{Vertex, VertDef},
    },
};


//struct Texture {}

pub struct Material<P> {
    // Except it's not just one, is it?
    //   e.g. could be: albedo, normal, specular, metalness, parallax etc.
    //texture: Texture,

    // There will be one shader _program_, right?
    // Effectively, a material = one shader plus properties it needs
    vs: vs::Shader,
    fs: fs::Shader,

    //properties: ???

    // Shaders and Draw settings
    // TODO(nickpollard) - we probably want to cache this somehow?
    pipeline: Option<Arc<GraphicsPipeline<VertDef, Pipe, P>>>,

    // Descriptors? Some are likely material properties, some are not?
}

impl<P: RenderPassAbstract> Material<P> {
    pub fn new(vs: vs::Shader, fs: fs::Shader) -> Self {
        Material { fs, vs, pipeline: None }
    }
    /// Create a simple GraphicsPipeline for this material
    /// A GraphicsPipeline is a collection of shaders and settings for executing a draw call
    pub fn pipeline(&mut self, render_pass: P)
        -> Arc<GraphicsPipeline<VertDef, Pipe, P>> {
            match &self.pipeline {
                Some(pipe) => pipe.clone(),
                None => {
                    let device = render_pass.device().clone();
                    let pipe = Arc::new(GraphicsPipeline::start()
                        .vertex_input_single_buffer::<Vertex>()
                        .vertex_shader(self.vs.main_entry_point(), ())
                        // Our vertex buffer is a triangle list
                        .triangle_list()
                        .viewports_dynamic_scissors_irrelevant(1)
                        .fragment_shader(self.fs.main_entry_point(), ())
                        .depth_stencil_simple_depth()
                        .render_pass(Subpass::from(render_pass, 0).unwrap())
                        .blend_collective(AttachmentBlend::alpha_blending())
                        .build(device)
                        .unwrap());
                        self.pipeline = Some(pipe.clone());
                        pipe
                }
            }
    }
}

/*
pub struct CallT<Layout, P> {
    // Vertices and Model Data
    verts: VertexBuffer,
    // Uniform data
    // TODO - this should be Descriptors for `Layout`
    descriptors: Option<Arc<dyn DescriptorSet + Send + Sync>>,
}
*/
