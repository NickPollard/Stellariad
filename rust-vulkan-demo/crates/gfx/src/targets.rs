use {
    vulkano::{
        command_buffer::DynamicState,
        device::Device,
        format::Format,
        framebuffer::FramebufferAbstract,
        pipeline::viewport::Viewport,
        image::{
            attachment::AttachmentImage,
            swapchain::SwapchainImage,
        },
        swapchain::{PresentMode, SurfaceTransform, Swapchain, SwapchainCreationError, ColorSpace, FullscreenExclusive},
    },
    std::sync::Arc,
};

use crate::{
    types::FrameBuffers,
    system::GfxDevice,
    pass::RenderPass,
    window::SysWindow,
};

/// Required Vulkan data defining a window we will render to
pub struct RenderTargets {
    /// Swapchain definition
    pub swapchain: Arc<Swapchain<winit::window::Window>>,
    /// Drawable images
    pub images: Vec<Arc<SwapchainImage<winit::window::Window>>>,
    /// Depth image
    pub depth_image: Arc<AttachmentImage>,
    /// Per-draw(?) state
    pub dynamic_state: DynamicState,
    /// Vulkan Device
    #[allow(unused)]
    device: Arc<Device>,

    framebuffers: FrameBuffers,

    /// The OS window we're interacting with
    os_window: SysWindow,
}

impl RenderTargets {
    // TODO(nickpollard)
    // Needs from system:
    //  .surface - SysWindow
    //  .physical - GfxVulkan
    //  .queue - ???
    //  .device
    pub fn new(os_window: SysWindow, gfx: &GfxDevice) -> Result<RenderTargets,String> {
        let surface = os_window.surface();
        // Create a swapchain
        let (swapchain, images) = {
            let caps = os_window.capabilities();
            let window = surface.window();
            let usage = caps.supported_usage_flags;
            // take the first alpha blend mode
            let alpha = caps.supported_composite_alpha.iter().next().unwrap();
            let format = caps.supported_formats[0].0;
            let dimensions = window.inner_size();
            let dimensions: (u32, u32) = (dimensions.width, dimensions.height);
            let initial_dimensions = [dimensions.0, dimensions.1];
            Swapchain::new(
                gfx.device(),
                surface,
                caps.min_image_count,
                format,
                initial_dimensions,
                1,
                usage,
                &gfx.queue(),
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
        let depth_image = AttachmentImage::new(gfx.device(), [1,1], Format::D16Unorm).unwrap();
        let device = gfx.device();
        // TODO: framebuffers::empty()
        let framebuffers = FrameBuffers( vec![] );
        Ok(RenderTargets{ swapchain, images, depth_image, dynamic_state, device, framebuffers, os_window })
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

    /// Recreate our swapchain, such as for a newly resized window
    /// This also generates new framebuffers
    /// returns true if successful,
    /// or false if the swapchain could not be recreated
    /// PANIC: this will panic if a fatal error is encountered when recreating the swapchain
    pub fn recreate_swapchain(&mut self, render_pass: &RenderPass) -> bool {
        let dimensions = self.os_window.dimensions();

        // 1. init a new swapchain
        let (new_swapchain, new_images) =
            match self.swapchain.recreate_with_dimensions([dimensions.0, dimensions.1]) {
                Ok(r) => r,
                // TODO(nickpollard) - decide how to handle this
                //  * Possibly push out to caller?
                //
                // This error tends to happen when the user is manually resizing the window.
                // Simply restarting the loop is the easiest way to fix this issue.
                Err(SwapchainCreationError::UnsupportedDimensions) => return false,
                Err(err) => panic!("FATAL - could not recreate swapchain: {:?}", err)
            };
        self.swapchain = new_swapchain;
        self.images = new_images;

        self.recalculate_dimensions();

        // 2. recreate framebuffers
        //
        // Framebuffers contains an Arc on the old swapchain, so recreate them as well.
        self.framebuffers = render_pass.create_framebuffers(&self.images);
        true
    }

    pub fn frame_buffer(&self, image_num: usize) -> Arc<dyn FramebufferAbstract + Sync + Send + 'static> {
        self.framebuffers.0[image_num].clone()
    }
}
