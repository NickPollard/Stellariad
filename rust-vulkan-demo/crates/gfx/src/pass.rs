use {
    std::sync::Arc,
    vulkano::{
        single_pass_renderpass,
        buffer::{BufferUsage, CpuBufferPool},
        device::Device,
        format::Format,
        framebuffer::{
            Framebuffer,
            FramebufferAbstract,
        },
        image::attachment::AttachmentImage,
        image::swapchain::SwapchainImage,
    },
};

use crate::{
    types::{
        DynPass,
        FrameBuffers,
    },
    shaders::vs,
};

/// A RenderPass to draw to a particular window
pub struct RenderPass {
    /// Vulkan Render Pass
    pass: DynPass,

    /// Uniform Buffers
    pub uniform_buffers: CpuBufferPool<vs::ty::Data>,

    /// Device
    device: Arc<Device>,
}

impl RenderPass {
    /// Create a default new render pass
    pub fn new(device: Arc<Device>, color_format: Format) -> RenderPass {
        // Create the pass
        let pass = create_renderpass(device.clone(), color_format);
        // Allocate a Bufferpool for our vertex data
        let uniform_buffers = CpuBufferPool::<vs::ty::Data>::new(device.clone(), BufferUsage::all());
        RenderPass { pass, uniform_buffers, device }
    }

    /// Generate the framebuffers for our rendering - both color and depth buffers
    /// Do this if e.g. the window size has changed
    /// We create one color image per screen in the swapchain
    pub fn create_framebuffers(&self, images: &[Arc<SwapchainImage<winit::window::Window>>]) -> FrameBuffers {
        let dimensions = images[0].dimensions();

        // Create a shared Depth Image buffer
        let depth_image = AttachmentImage::new(self.device.clone(), dimensions, Format::D16Unorm).unwrap();

        // Generate new Framebuffers
        //  - Each swapchain image has its own color buffer, and shares the depth buffer
        FrameBuffers(images.iter().map(|image| {
            Arc::new(
                Framebuffer::start(self.pass.clone())
                .add(image.clone()).unwrap()
                .add(depth_image.clone()).unwrap()
                .build().unwrap()
            ) as Arc<dyn FramebufferAbstract + Send + Sync>
        }).collect::<Vec<_>>())
    }

    pub fn pass(&self) -> DynPass {
        self.pass.clone()
    }
}

fn create_renderpass(device: Arc<Device>, color_format: Format) -> DynPass {
    Arc::new(single_pass_renderpass!(
        device,
        attachments: {
            // `color` is a custom name we give to the main color output
            color: {
                // Gpu should clear this before drawing
                load: Clear,
                // We want to store (not discard) the output
                store: Store,
                // image format
                format: color_format,
                samples: 1,
            },
            // `depth` is a custom name we give to the depth attachment (second)
            depth: {
                // Gpu should clear this before drawing
                load: Clear,
                // We dont need to store the output
                store: DontCare,
                // image format
                format: Format::D16Unorm,
                samples: 1,
            }
        },
        pass: {
            // use the `color` attachment as the only one
            color: [color],
            // depth stencil
            depth_stencil: {depth}
        }
    ).unwrap())
}

