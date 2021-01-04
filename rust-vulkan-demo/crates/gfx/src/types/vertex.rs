use {
    vulkano::{
        buffer::{CpuAccessibleBuffer, BufferUsage},
        device::Device,
        impl_vertex,
        pipeline::vertex::SingleBufferDefinition,
    },
    std::sync::Arc,
};


/// Our standard Vertex definition
#[derive(Debug, Clone)]
pub struct Vertex {
    pub position: [f32; 3],
    pub normal: [f32; 3],
    pub color: [f32; 4],
}

impl Default for Vertex {
    fn default() -> Vertex {
        Vertex {
            position: [0.0, 0.0, 0.0],
            normal: [0.0, 0.0, 0.0],
            color: [0.0, 0.0, 0.0, 0.0],
        }
    }
}

impl_vertex!(Vertex, position, normal, color); // What does this do?

/// Our standard Vertex Buffer Definition
pub type VertDef = SingleBufferDefinition<Vertex>;

/// Our standard Vertex Buffer
#[derive(Clone)]
pub struct VertexBuffer( pub Arc<CpuAccessibleBuffer<[Vertex]>> );

impl VertexBuffer {
    pub fn new(inner: Arc<CpuAccessibleBuffer<[Vertex]>>) -> VertexBuffer {
        VertexBuffer(inner)
    }
}

pub fn vertex_buffer(device: Arc<Device>, verts: &Vec<Vertex>) -> VertexBuffer {
    // Host caching is usually used when the CPU often needs to read from the GPU
    let host_cached = false;
    VertexBuffer(
        CpuAccessibleBuffer::from_iter(device.clone(), BufferUsage::all(), host_cached, verts.iter().cloned())
        .unwrap())
}
