pub mod call;
pub mod model;
pub mod scene;
pub mod shaders;
pub mod pass;
pub mod system;
pub mod targets;
pub mod types;
pub mod window;

use {
    vulkano::{
        descriptor::{
            descriptor_set::{
                DescriptorSet,
                PersistentDescriptorSet,
                UnsafeDescriptorSetLayout
            },
        },
        device::Device,
        framebuffer::{RenderPassAbstract, Subpass},
        swapchain::{self, AcquireError},
        pipeline::{ blend::AttachmentBlend, GraphicsPipeline },
        sync::{FlushError, GpuFuture},
    },
    std::sync::Arc,
    winit::event_loop::EventLoop,
};

use crate::{
    pass::RenderPass,
    scene::Scene,
    system::{GfxVulkan, GfxDevice},
    targets::RenderTargets,
    types::{
        DynPass,
        Pipe,
        SubBuffer,
        vertex::{Vertex, VertDef},
    },
};

/// The Graphics Subsystem
/// This entity is responsible for initiating and managing rendering
pub struct Gfx {
    /// Required Vulkan bindings
    #[allow(unused)]
    vulkan: GfxVulkan,

    /// Virtual Vulkan device for issuing commands
    render_device: GfxDevice,

    /// Vulkan Render Targets (swapchain, framebuffers)
    render_targets: RenderTargets,

    /// Render Pass
    pass: RenderPass,
}

impl Gfx {
    /// Initialize the Graphics subsystem
    pub fn init() -> (Gfx, EventLoop<()>) {
        // initialize the base vulkan system
        let vulkan = GfxVulkan::init();
        // create a vulkan-backed window
        let (os_window, event_loop) = vulkan.new_window();
        // set up a virtual vulkan device to render to `os_window`
        let render_device = GfxDevice::new(vulkan.physical(), os_window.surface());

        // generate render targets for the surface
        let render_targets = RenderTargets::new(os_window, &render_device).expect("error");
        // create a render pass to define our drawing
        let pass = render_device.create_render_pass(render_targets.swapchain.format());
        (Gfx { vulkan, render_device, render_targets, pass }, event_loop)
    }

    /// Recreate the swapchain and framebuffers - only do this if necessary
    pub fn recreate_swapchain(&mut self) -> bool {
        self.render_targets.recreate_swapchain(&self.pass)
    }

    // Render commands for scene `scene` using renderpass `pass` to the window `window`
    pub fn render<S: Scene<DynPass>>(&mut self, scene: &S) -> Result<(),()> {
        self.render_device.cleanup_finished();

        // 1. Acquire an image from the swapchain to draw
        let (image_num, _is_suboptimal, acquire_future) =
            match swapchain::acquire_next_image(self.render_targets.swapchain.clone(), None) {
            Ok(r) => r,
            Err(AcquireError::OutOfDate) => {
                //should_recreate_swapchain = true;
                return Err(());
            },
            Err(err) => panic!("FATAL: Could not acquire next swapchain image: {:?}", err)
        };
        let frame_buffer = self.render_targets.frame_buffer(image_num);

        // 2. Build the command buffer
        let device = self.render_device.device();
        let pass = self.pass.pass();
        let dyn_state = &self.render_targets.dynamic_state;
        let buffer_pool = &self.pass.uniform_buffers;
        let command_buffer = self.render_device.build_command_buffer(frame_buffer, |builder| {
            scene.draw_all(device, dyn_state, pass, builder, buffer_pool);
        });

        // 3. Execute and wait

        // *** Execute commands
        let future = self.render_device.take_previous_frame_end().join(acquire_future)
            .then_execute(self.render_device.queue(), command_buffer).unwrap()
            .then_swapchain_present(self.render_device.queue(), self.render_targets.swapchain.clone(), image_num)
            .then_signal_fence_and_flush();

        // *** Wait to execute
        match future {
            Ok(future) => self.render_device.wait_to_execute(future),
            Err(FlushError::OutOfDate) => {
                panic!();
                //should_recreate_swapchain = true;
            }
            Err(error) => {
                println!("Error executing GPU future: {:?}", error);
            }
        };

        Ok(())
    }

    pub fn device(&self) -> Arc<Device> {
        self.render_device.device()
    }
}

/// Helper - create a simple GraphicsPipeline for using a given vertex shader and fragment shader
/// A GraphicsPipeline is a collection of shaders and settings for executing a draw call
pub fn create_pipeline<P: RenderPassAbstract>(
    device: Arc<Device>,
    vs: &shaders::vs::Shader,
    fs: &shaders::fs::Shader,
    render_pass: P
  ) -> Arc<GraphicsPipeline<VertDef, Pipe, P>> {

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
             .build(device.clone())
             .unwrap())
}

// TODO - where is this used and why
/// Create a DescriptorSet for a given layout and subbuffer attachment
pub fn descriptors_for<T: Send + Sync + 'static>(
    uniform_buffer_subbuffer: SubBuffer<T>,
    descriptors_layout: Arc<UnsafeDescriptorSetLayout>,
) -> Arc<dyn DescriptorSet + Send + Sync> {
    Arc::new(
        PersistentDescriptorSet::start(descriptors_layout)
        .add_buffer(uniform_buffer_subbuffer).unwrap()
        .build().unwrap()
    )
}

