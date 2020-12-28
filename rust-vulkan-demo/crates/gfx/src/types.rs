//use cgmath::{Vector3, Vector4};
use vulkano::{
    impl_vertex,
    buffer::{CpuAccessibleBuffer, BufferUsage},
    command_buffer::DynamicState,
    descriptor::{
        descriptor_set::{
            PersistentDescriptorSet,
            PersistentDescriptorSetBuf,
        },
        pipeline_layout::PipelineLayoutAbstract,
    },
    device::{Device, DeviceExtensions},
    format::Format,
    framebuffer::{Framebuffer, FramebufferAbstract, RenderPassAbstract},
    image::{SwapchainImage, AttachmentImage},
    instance::{Instance, PhysicalDevice},
    memory::pool::StdMemoryPool,
    pipeline::{
        //blend::AttachmentBlend,
        //GraphicsPipeline,
        GraphicsPipelineAbstract,
        vertex::SingleBufferDefinition,
        viewport::Viewport,
    },
    swapchain::{PresentMode, SurfaceTransform, Swapchain, ColorSpace, FullscreenExclusive},
};
use vulkano_win::VkSurfaceBuild;
use winit::{
    event_loop::EventLoop,
    window::{Window, WindowBuilder},
};

use std::sync::Arc;

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

/// Our standard Pipeline Layout
pub type Pipe = Box<dyn PipelineLayoutAbstract + Send + Sync>;

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

/// A Standard dynamic Pipeline type
pub type Pipeline = Arc<dyn GraphicsPipelineAbstract + Send + Sync>;

/// Type alias for a common SubBuffer for uniform data
pub type SubBuffer<T> = vulkano::buffer::cpu_pool::CpuBufferPoolSubbuffer<T,Arc<StdMemoryPool>>;

/// A Standard dynamic RenderPass type
pub type DynPass = Arc<dyn RenderPassAbstract + Send + Sync + 'static>;

/// A Standard dynamic DescriptorSet type for a pipeline `P` and data `D`
pub type Descriptors<P,D> = Arc<PersistentDescriptorSet<P,((), PersistentDescriptorSetBuf<SubBuffer<D>>)>>;

pub struct FrameBuffers( pub Vec<Arc<dyn FramebufferAbstract + Sync + Send + 'static>> );

pub struct VulkanSystem {
    pub instance: Arc<Instance>,
    pub physical_index: usize,
    pub device: Arc<Device>,
    pub queue: Arc<vulkano::device::Queue>,
    pub surface: Arc<vulkano::swapchain::Surface<Window>>,
    // TODO(nickpollard) This probably doesn't belong here
    pub events_loop: EventLoop<()>,
}

impl VulkanSystem {
    pub fn new() -> VulkanSystem {
        // Create a Vulkan instance, we need extensions to be able to create windows
        let extensions = vulkano_win::required_extensions();
        let instance = Instance::new(None, &extensions, None).expect("Could not initialise Vulkan");

        // Choose a physical device (i.e. GPU), we'll just assume the first one is good
        let physical = PhysicalDevice::enumerate(&instance).next().expect("No physical devices found");
        println!("Using device: {} (type: {:?})", physical.name(), physical.ty());

        let events_loop = EventLoop::new();
        let surface = WindowBuilder::new()
            .with_inner_size(winit::dpi::LogicalSize::new(320, 240))
            .build_vk_surface(&events_loop, instance.clone()).expect("Could not create surface");

        // Get a queue for submitting draw commands (we'll just use one, for simplicity)
        let queue_family = physical.queue_families().find(|&q|
            q.supports_graphics() && surface.is_supported(q).unwrap_or(false)
        ).expect("Could not find valid drawing queue");

        let device_extensions = DeviceExtensions { khr_swapchain: true, .. DeviceExtensions::none() };
        let (device, mut queues) = Device::new(physical, physical.supported_features(), &device_extensions, [(queue_family, 0.5)].iter().cloned()).expect("Could not initialize logical device");
        // We just need the first queue
        let queue = queues.next().expect("Could not create a queue");
        let physical_index = physical.index();
        VulkanSystem{ instance, physical_index, device, queue, surface, events_loop }
    }

    fn physical<'a>(&'a self) -> Result<PhysicalDevice<'a>, String> {
        PhysicalDevice::from_index( &self.instance, self.physical_index ).ok_or(String::from("Invalid device index"))
    }
}

/// Required Vulkan data defining the window we will render to
pub struct RenderWindow {
    pub swapchain: Arc<Swapchain<Window>>,
    pub images: Vec<Arc<SwapchainImage<Window>>>,
    pub depth_image: Arc<AttachmentImage>,
    pub dynamic_state: DynamicState,
    device: Arc<Device>,
}

impl RenderWindow {
    pub fn new(system: &VulkanSystem) -> Result<RenderWindow,String> {
        // Create a swapchain
        let (swapchain, images) = {
            let caps = system.surface.capabilities(system.physical()?).unwrap();
            let window = system.surface.window();
            let usage = caps.supported_usage_flags;
            // take the first alpha blend mode
            let alpha = caps.supported_composite_alpha.iter().next().unwrap();
            let format = caps.supported_formats[0].0;
            let dimensions = window.inner_size();
            let dimensions: (u32, u32) = (dimensions.width, dimensions.height);
            let initial_dimensions = [dimensions.0, dimensions.1];
            Swapchain::new(
                system.device.clone(),
                system.surface.clone(),
                caps.min_image_count,
                format,
                initial_dimensions,
                1,
                usage,
                &system.queue,
                SurfaceTransform::Identity,
                alpha,
                PresentMode::Fifo,
                FullscreenExclusive::Default,
                true,
                ColorSpace::SrgbNonLinear,
            ).expect("Could not create swapchain")
        };
        let dynamic_state = DynamicState {
            line_width: None,
            viewports: None,
            scissors: None,
            compare_mask: None,
            write_mask: None,
            reference: None,
        };
        let depth_image = AttachmentImage::new(system.device.clone(), [1,1], Format::D16Unorm).unwrap();
        let device = system.device.clone();
        Ok(RenderWindow{ swapchain, images, depth_image, dynamic_state, device })
    }

    pub fn refresh_framebuffers(&mut self, render_pass: Arc<dyn RenderPassAbstract + Send + Sync + 'static>) -> FrameBuffers {
        self.recalculate_dimensions();

        let dims = self.images[0].dimensions();
        self.depth_image = AttachmentImage::new(self.device.clone(), dims, Format::D16Unorm).unwrap();

        // Generate new framebuffers
        FrameBuffers(self.images.iter().map(|image| {
            Arc::new(
                Framebuffer::start(render_pass.clone())
                .add(image.clone()).unwrap()
                .add(self.depth_image.clone()).unwrap()
                .build().unwrap()
            ) as Arc<dyn FramebufferAbstract + Send + Sync>
        }).collect::<Vec<_>>())
    }

    /// This method is called once during initialization, then again whenever the window is resized
    fn recalculate_dimensions(&mut self) {
        let dimensions = self.images[0].dimensions();

        let viewport = Viewport {
            origin: [0.0, 0.0],
            dimensions: [dimensions[0] as f32, dimensions[1] as f32],
            depth_range: 0.0 .. 1.0,
        };

        self.dynamic_state.viewports = Some(vec!(viewport));
    }
}

pub enum LoopControl {
    Return,
    Continue
}
