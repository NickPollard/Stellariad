// TODO remove after updating vulkano
#![allow(deprecated)]

pub mod call;
pub mod model;
pub mod scene;
pub mod shaders;
pub mod types;
pub mod window;

use vulkano::{
    buffer::{BufferUsage, cpu_pool::CpuBufferPool},
    command_buffer::{AutoCommandBufferBuilder, SubpassContents},
    descriptor::{
        descriptor_set::{
            DescriptorSet,
            PersistentDescriptorSet,
            UnsafeDescriptorSetLayout
        },
    },
    device::Device,
    format::Format,
    framebuffer::{RenderPassAbstract, Subpass},
    single_pass_renderpass,
    swapchain::{self, AcquireError, SwapchainCreationError, Swapchain},
    pipeline::{ blend::AttachmentBlend, GraphicsPipeline },
    sync::{self, GpuFuture, FlushError},
};
use std::{
    sync::Arc,
    //time::Instant,
};
use winit;

use crate::{
    scene::Scene,
    shaders::vs,
    types::{LoopControl, RenderWindow, DynPass, VulkanSystem, FrameBuffers, Pipe, SubBuffer, Vertex, VertDef},
    window::get_dimensions,
};

/// The Graphics Subsystem
/// This entity is responsible for initiating and managing rendering
pub struct Gfx {
    /// Required Vulkan bindings
    pub system: VulkanSystem,

    /// The Window to Render to
    window: RenderWindow,

    /// Render Pass
    pass: DynPass,

    /// Uniform Buffers
    pub uniform_buffers: CpuBufferPool<vs::ty::Data>,

    /// Future that completes when the GPU has finished rendering the previous frame
    previous_frame_end: Box<dyn GpuFuture>,
}

impl Gfx {
    pub fn render<S: Scene<DynPass>>(&mut self, scene: &S, should_recreate_swapchain: &mut bool) -> Result<(),()> {
        self.previous_frame_end.cleanup_finished();

        let mut framebuffers = self.window.refresh_framebuffers(self.pass.clone());

        if *should_recreate_swapchain {
            let result = recreate_swapchain(
                &self.system.surface.window(),
                &mut self.window,
                &mut framebuffers,
                should_recreate_swapchain,
                self.pass.clone());
            match result {
                Ok(()) => (),
                Err(LoopControl::Continue) => return Ok(()),
                Err(LoopControl::Return) => return Err(()),
            }
        }

        // Acquire an image from the swapchain to draw
        let (image_num, _is_suboptimal, acquire_future) = match swapchain::acquire_next_image(self.window.swapchain.clone(), None) {
            Ok(r) => r,
            Err(AcquireError::OutOfDate) => {
                *should_recreate_swapchain = true;
                return Err(());
            },
            Err(err) => panic!("{:?}", err)
        };

        let black = [0.0, 0.0, 0.0, 1.0].into();
        let clear_values = vec![black, 1.0f32.into()];

        // *** Build command buffer
        // TODO(nickpollard) - handle depth buffering
        //   - create the depth image and store it
        //   - update the size of the depth image
        //   - bind the depth image here
        let mut buffer_builder = AutoCommandBufferBuilder::primary_one_time_submit(self.system.device.clone(), self.system.queue.family()).unwrap();
        buffer_builder.begin_render_pass(framebuffers.0[image_num].clone(), SubpassContents::Inline, clear_values).unwrap();
        scene.draw_all(self.system.device.clone(), &self.window.dynamic_state, self.pass.clone(), &mut buffer_builder);
        let _ = buffer_builder.end_render_pass().unwrap();
        let command_buffer = buffer_builder.build().unwrap();

        let mut previous_frame = Box::new(sync::now(self.system.device.clone())) as Box<_>;

        std::mem::swap(&mut previous_frame, &mut self.previous_frame_end);

        // *** Execute commands
        let future = previous_frame.join(acquire_future)
            .then_execute(self.system.queue.clone(), command_buffer).unwrap()
            .then_swapchain_present(self.system.queue.clone(), self.window.swapchain.clone(), image_num)
            .then_signal_fence_and_flush();

        // *** Wait to execute
        match future {
            Ok(future) => {
                self.previous_frame_end = Box::new(future) as Box<_>;
            },
            Err(FlushError::OutOfDate) => {
                *should_recreate_swapchain = true;
            }
            Err(e) => {
                println!("{:?}", e);
            }
        }
        Ok(())
    }

    pub fn init() -> Gfx {
        let system = VulkanSystem::new();
        let window = RenderWindow::new(&system).expect("error");
        let pass = create_renderpass(system.device.clone(), window.swapchain.clone());
        let uniform_buffers = CpuBufferPool::<vs::ty::Data>::new(system.device.clone(), BufferUsage::all());

        //let framebuffers = render_window.refresh_framebuffers(render_pass.clone());
        let previous_frame_end = Box::new(sync::now(system.device.clone())) as Box<dyn GpuFuture>;
        Gfx { system, window, pass, uniform_buffers, previous_frame_end }
    }
}

fn recreate_swapchain(
    os_window: &winit::window::Window,
    render_window: &mut RenderWindow,
    framebuffers: &mut FrameBuffers,
    should_recreate_swapchain: &mut bool,
    render_pass: DynPass,
) -> Result<(),LoopControl> {

    let dimensions = get_dimensions(os_window);

    let (new_swapchain, _new_images) = match render_window.swapchain.recreate_with_dimensions([dimensions.0, dimensions.1]) {
        Ok(r) => r,
        // This error tends to happen when the user is manually resizing the window.
        // Simply restarting the loop is the easiest way to fix this issue.
        Err(SwapchainCreationError::UnsupportedDimensions) => return Err(LoopControl::Continue),
        Err(err) => panic!("{:?}", err)
    };

    render_window.swapchain = new_swapchain;
    // Framebuffers contains an Arc on the old swapchain, so recreate them as well.
    *framebuffers = render_window.refresh_framebuffers(render_pass.clone());

    *should_recreate_swapchain = false;
    Ok(())
}

pub fn create_pipeline<P: RenderPassAbstract + Clone>(
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
             .render_pass(Subpass::from(render_pass.clone(), 0).unwrap())
             .blend_collective(AttachmentBlend::alpha_blending())
             .build(device.clone())
             .unwrap())
}

pub fn descriptors_for<T: Send + Sync + 'static>(
    uniform_buffer_subbuffer: SubBuffer<T>,
    //pipeline: Pipeline
    layout: Arc<UnsafeDescriptorSetLayout>,
) -> Arc<dyn DescriptorSet + Send + Sync> {
    Arc::new(
        PersistentDescriptorSet::start(layout)
        .add_buffer(uniform_buffer_subbuffer).unwrap()
        .build().unwrap()
    )
}

pub fn create_renderpass(device: Arc<Device>, swapchain: Arc<Swapchain<winit::window::Window>>) -> DynPass {
    Arc::new(single_pass_renderpass!(
        device.clone(),
        attachments: {
            // `color` is a custom name we give to the main color output
            color: {
                // Gpu should clear this before drawing
                load: Clear,
                // We want to store (not discard) the output
                store: Store,
                // image format
                format: swapchain.format(),
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

