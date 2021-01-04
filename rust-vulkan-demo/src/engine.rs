use {
    gfx::{
        Gfx,
        scene::camera::Camera,
        shaders::vs,
        types::DynPass,
    },
    input::{self, InputDevices, Inputs},
    property::Property,
    std::{
        sync::Arc,
        time::{Duration, Instant},
    },
    vulkano::{
        buffer::CpuBufferPool,
        device::Device,
        command_buffer::{AutoCommandBufferBuilder, DynamicState},
    },
    winit::{
        event::{Event, WindowEvent, WindowEvent::KeyboardInput},
        event_loop::EventLoop,
    },
};

use crate::{
    physics::collision,
    particle,
    rotating_obj::RotatingObj,
    types::{vec4, DrawAndTick},
};


const KEY_ESCAPE: u32 = 1;

/// An `Engine` is a core struct for processing game updates and renders
pub struct Engine<P> {
    /// When was the last frame processed
    last_frame: Instant,

    /// The scene of entities
    scene: Scene<P>,

    /// The Graphics subsystem
    gfx: Gfx,

    /// Input processiong
    input_devices: InputDevices,
    inputs: Inputs,

    /// The window event loop
    event_loop: EventLoop<()>,
}

impl<P: Clone> Engine<P> {
    /// Do we need to quit yet?
    fn process_window_events(&mut self) -> (bool, bool) {
        let mut done = false;
        let mut recreate_swapchain = false;
        /*
        self.event_loop.poll_events(|ev| {
            match ev {
                Event::WindowEvent { event: WindowEvent::CloseRequested, .. } => done = true,
                Event::WindowEvent { event: WindowEvent::Resized(_), .. } => recreate_swapchain = true,
                Event::WindowEvent { event: KeyboardInput { device_id, input, is_synthetic }, .. } => {
                    // TODO -> forward these to an input handler
                    // u32, hardware dependent
                    let scancode = input.scancode;
                    println!("Key pressed with scancode: {}", scancode);
                    if scancode == KEY_ESCAPE {
                        done = true;
                    }
                }
                _ => ()
            }
        });
        (done, recreate_swapchain)
        */
        (false, false)
    }
}

impl Engine<DynPass> {
    // Returns true if program should continue running
    pub fn tick(&mut self) -> bool {
        let time_delta = self.last_frame.elapsed();
        self.last_frame = Instant::now();

        // *** Process Inputs
        self.inputs = input::tick(&mut self.input_devices, self.inputs.clone());

        // *** Process Collisions
        let _ = collision::tick(vec![]);

        // *** Process Game scripts
        // lua.tick();

        // *** Update all Game Objects
        self.scene.tick_all(time_delta);

        // *** Handle window events
        let (quit, window_was_resized) = self.process_window_events();

        // TODO - properly handle recreating swapchain
        if window_was_resized {
            assert!(self.gfx.recreate_swapchain());
        }
        // *** Render all Gfx Objects
        let result = self.gfx.render(&mut self.scene);

        if let Err(e) = result {
            println!("{:?}", e);
            return false;
        }

        !quit
    }

    // Run the engine, using the windows event loop to do the driving
    pub fn run(self) {
    }

    pub fn init(gfx: Gfx, scene: Scene<DynPass>) -> Engine<DynPass> {
        println!("Starting Engine");
        let last_frame = Instant::now();
        let event_loop = EventLoop::new();

        let input_devices = InputDevices {};
        let inputs = Inputs {};

        Engine { last_frame, scene, gfx, input_devices, inputs, event_loop }
    }
}

pub struct Scene<P> {
    pub camera: Camera,
    pub drawables: Vec<Box<dyn DrawAndTick<P>>>,
}

impl<P: Clone> Scene<P> {
  pub fn tick_all(&mut self, dt: Duration) {
    for tickable in self.drawables.iter_mut() {
        tickable.tick(dt);
    }
  }
}

impl<P: Clone> gfx::scene::Scene<P> for Scene<P> {
  fn draw_all(
      &self,
      device: Arc<Device>,
      dynamic_state: &DynamicState,
      pass: P,
      cmd_buffer: &mut AutoCommandBufferBuilder,
      buffer_pool: &CpuBufferPool<vs::ty::Data>,
  ) {
      for drawable in self.drawables.iter() {
          drawable.draw_call(device.clone(), pass.clone(), buffer_pool).draw(dynamic_state, cmd_buffer);
      }
  }
}
