use {
    vulkano::{
        instance::{PhysicalDevice},
        swapchain::{Capabilities, Surface},
    },
    vulkano_win::VkSurfaceBuild,
    std::sync::Arc,
    winit::{
        event_loop::EventLoop,
        window::WindowBuilder,
    }
};

//use crate::types::VulkanSystem;

// TODO(nickpollard) - should this be something like a `SystemWindow`?
// i.e. this is our interaction with the OS's windowing system
pub struct SysWindow {
    /// Where we're drawing to
    surface: Arc<Surface<winit::window::Window>>,

    /// Capabilities for drawing to this surface via our physical device
    /// (The GFX library only supports a single physical device currently)
    capabilities: Capabilities,
}

impl SysWindow {
    pub fn new<'a>(instance: Arc<vulkano::instance::Instance>, physical: PhysicalDevice<'a>) -> (Self, EventLoop<()>) {
        // Create a new event loop to handle window events
        let events_loop = EventLoop::new();
        // Build the Vulkan surface from our window
        //TODO(nickpollard) - take size as params
        let surface = WindowBuilder::new()
            .with_inner_size(winit::dpi::LogicalSize::new(320, 240))
            .build_vk_surface(&events_loop, instance).expect("Could not create surface");

        let capabilities = surface.capabilities(physical)
            .expect("No capabilities retrieved for new window surface");

        (SysWindow { surface, capabilities }, events_loop)
    }

    pub fn surface(&self) -> Arc<Surface<winit::window::Window>> {
        self.surface.clone()
    }

    /// Return the width and height of the window viewport
    pub fn dimensions(&self) -> (u32,u32) {
        let dimensions = self.surface.window().inner_size();
        (dimensions.width, dimensions.height)
    }

    pub fn capabilities(&self) -> Capabilities {
        self.capabilities.clone()
    }
}
