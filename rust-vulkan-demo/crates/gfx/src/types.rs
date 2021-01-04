use {
    std::sync::Arc,
    vulkano::{
        descriptor::{
            descriptor_set::{
                PersistentDescriptorSet,
                PersistentDescriptorSetBuf,
            },
            pipeline_layout::PipelineLayoutAbstract,
        },
        framebuffer::{FramebufferAbstract, RenderPassAbstract},
        memory::pool::StdMemoryPool,
        pipeline::GraphicsPipelineAbstract,
    },
};

/// Types for working with Vertices
pub mod vertex;

/// Our standard Pipeline Layout
pub type Pipe = Box<dyn PipelineLayoutAbstract + Send + Sync>;

/// A Standard dynamic Pipeline type
pub type Pipeline = Arc<dyn GraphicsPipelineAbstract + Send + Sync>;

/// Type alias for a common SubBuffer for uniform data
pub type SubBuffer<T> = vulkano::buffer::cpu_pool::CpuBufferPoolSubbuffer<T,Arc<StdMemoryPool>>;

/// A Standard dynamic RenderPass type
pub type DynPass = Arc<dyn RenderPassAbstract + Send + Sync + 'static>;

/// A Standard dynamic DescriptorSet type for a pipeline `P` and data `D`
pub type Descriptors<P,D> = Arc<PersistentDescriptorSet<P,((), PersistentDescriptorSetBuf<SubBuffer<D>>)>>;

/// A collection of framebuffers for drawing operations
pub struct FrameBuffers( pub Vec<Arc<dyn FramebufferAbstract + Sync + Send + 'static>> );

pub enum LoopControl {
    Return,
    Continue
}
