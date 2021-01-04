use vulkano::{
    buffer::CpuBufferPool,
    command_buffer::{AutoCommandBufferBuilder, DynamicState},
    descriptor::{
        descriptor_set::{DescriptorSet},
    },
    device::Device,
    pipeline::{
        GraphicsPipeline,
        GraphicsPipelineAbstract,
    }
};

use std::sync::Arc;

use crate::{
    shaders::vs,
    types::vertex::{VertDef, VertexBuffer},
};

/// An object which is drawable in a given pass
pub trait Drawable<P> {
    fn draw_call(&self, device: Arc<Device>, pass: P, buffer_pool: &CpuBufferPool<vs::ty::Data>) -> Box<dyn Call<P>>;
}

pub struct CallT<Layout, P> {
    // Shaders and Draw settings
    pipeline: Arc<GraphicsPipeline<VertDef, Layout, P>>,
    // Vertices and Model Data
    verts: VertexBuffer,
    // Uniform data
    // TODO - this should be Descriptors for `Layout`
    descriptors: Option<Arc<dyn DescriptorSet + Send + Sync>>,
}

impl<Layout, P> CallT<Layout, P> {
  pub fn new(
    pipeline: Arc<GraphicsPipeline<VertDef, Layout, P>>,
    verts: VertexBuffer,
    descriptors: Option<Arc<dyn DescriptorSet + Send + Sync>>
  ) -> CallT<Layout, P> {
    CallT{ pipeline, verts, descriptors }
  }
}

pub trait Call<P> where {
  /// Draw this call
  fn draw(&self, dynamic_state: &DynamicState, cmd_buffer: &mut AutoCommandBufferBuilder);
  // TODO - needs some way of identifying its pipeline to look up correct batch
}

impl<Layout, P> Call<P> for CallT<Layout, P>
  where
    P: Send + Sync + 'static,
    Layout: Send + Sync + 'static,
    GraphicsPipeline<VertDef, Layout, P>: GraphicsPipelineAbstract //+ VertexSource<Verts> + Clone
{
    fn draw(&self, dynamic_state: &DynamicState, cmd_buffer: &mut AutoCommandBufferBuilder) {
        match self.descriptors.as_ref().cloned() {
            Some(descriptors) => {
                let r = cmd_buffer.draw(
                    self.pipeline.clone(),
                    dynamic_state,
                    vec!(self.verts.0.clone()),
                    descriptors,
                    () /*constants*/,
                    );
                let _ =r.unwrap(); // TODO
            }
            None => {
                let r = cmd_buffer.draw(
                    self.pipeline.clone(),
                    dynamic_state,
                    vec!(self.verts.0.clone()),
                    () /*descriptor sets*/,
                    () /*constants*/,
                    );
                let _ = r.unwrap(); // TODO
            }
        }
    }
}
