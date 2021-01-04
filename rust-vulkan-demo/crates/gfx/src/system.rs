use {
    vulkano::{
        command_buffer::{AutoCommandBufferBuilder, AutoCommandBuffer, SubpassContents},
        device::{Device, DeviceExtensions, Queue},
        format::Format,
        framebuffer::FramebufferAbstract,
        instance::{Instance, PhysicalDevice},
        swapchain::Surface,
        sync::{self, GpuFuture},
    },
    std::sync::Arc,
};

use crate::{
    pass::RenderPass,
    window::SysWindow,
};

/// The base Vulkan system and physical GPU
pub struct GfxVulkan {
    /// The Vulkan Instance we are using
    pub instance: Arc<Instance>,

    /// Index of the physical device
    pub physical_index: usize,
}

impl GfxVulkan {
    pub fn init() -> Self {
        // Create a Vulkan instance, we need extensions to be able to create windows
        let extensions = vulkano_win::required_extensions();
        let instance = Instance::new(None, &extensions, None).expect("Could not initialise Vulkan");

        // Choose a physical device (i.e. GPU), we'll just assume the first one is good
        let physical = PhysicalDevice::enumerate(&instance).next().expect("No physical devices found");
        let physical_index = physical.index();
        GfxVulkan { instance, physical_index }
    }

    pub fn new_window(&self) -> SysWindow {
        SysWindow::new(self.instance.clone(), self.physical())
    }

    pub fn physical<'a>(&'a self) -> PhysicalDevice<'a> {
        PhysicalDevice::from_index( &self.instance, self.physical_index ).expect("Invalid device index")
    }
}

/// Virtual Device interface for sending draw commands
pub struct GfxDevice {
    /// Virtual Vulkan Device for interfacing with the GPU
    device: Arc<Device>,

    /// Command Queue for the device for issuing Draw Commands
    queue: Arc<Queue>,

    /// Future that completes when the GPU has finished rendering the previous frame
    previous_frame_end: Box<dyn GpuFuture>,
}

impl GfxDevice {
    /// Create a new GfxDevice capable of rendering to `surface` using `physical`
    pub fn new<'a, W>(physical: PhysicalDevice<'a>, surface: Arc<Surface<W>>) -> Self {
        // Get a queue for submitting draw commands (we'll just use one, for simplicity)
        let queue_family = physical.queue_families().find(|&q|
            q.supports_graphics() && surface.is_supported(q).unwrap_or(false)
        ).expect("Could not find valid drawing queue");

        // Required vulkan extensions
        let device_extensions = DeviceExtensions { khr_swapchain: true, .. DeviceExtensions::none() };

        let (device, mut queues) =
            Device::new(
                physical,
                physical.supported_features(),
                &device_extensions,
                [(queue_family, 0.5)].iter().cloned()
            ).expect("Could not initialize logical device");

        // We only need the first queue
        let queue = queues.next().expect("Could not create a queue");

        // Create a synchronization future for the end of drawing the last frame
        let previous_frame_end = Box::new(sync::now(device.clone())) as Box<dyn GpuFuture>;

        GfxDevice{ device, queue, previous_frame_end }
    }

    pub fn device(&self) -> Arc<Device> {
        self.device.clone()
    }

    pub fn queue(&self) -> Arc<Queue> {
        self.queue.clone()
    }

    pub fn create_render_pass(&self, color_format: Format) -> RenderPass {
        RenderPass::new(self.device(), color_format)
    }

    pub fn build_command_buffer<F>(&mut self, frame_buffer: Arc<dyn FramebufferAbstract + Sync + Send + 'static>, draw_fn: F) -> AutoCommandBuffer
    where F: FnOnce(&mut AutoCommandBufferBuilder)
    {
        let black = [0.0, 0.0, 0.0, 1.0].into();
        let clear_values = vec![black, 1.0f32.into()];

        // *** Build command buffer
        // TODO(nickpollard) - handle depth buffering
        //   - create the depth image and store it
        //   - update the size of the depth image
        //   - bind the depth image here
        let mut buffer_builder = AutoCommandBufferBuilder::primary_one_time_submit(self.device.clone(), self.queue.family()).unwrap();
        buffer_builder.begin_render_pass(frame_buffer, SubpassContents::Inline, clear_values).unwrap();

        draw_fn(&mut buffer_builder);

        let _ = buffer_builder.end_render_pass().unwrap();
        let command_buffer = buffer_builder.build().unwrap();
        command_buffer
    }

    pub(crate) fn take_previous_frame_end(&mut self) -> Box<dyn GpuFuture> {
        let mut previous_frame = Box::new(sync::now(self.device())) as Box<_>;
        std::mem::swap(&mut previous_frame, &mut self.previous_frame_end);
        previous_frame
    }

    pub(crate) fn wait_to_execute<F: GpuFuture + 'static>(&mut self, future: F) {
        self.previous_frame_end = Box::new(future) as Box<_>;
    }

    pub(crate) fn cleanup_finished(&mut self) {
        self.previous_frame_end.cleanup_finished();
    }
}
