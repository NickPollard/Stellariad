use {
    vulkano::{
        buffer::CpuBufferPool,
        device::DeviceOwned,
        command_buffer::{AutoCommandBufferBuilder, DynamicState},
    },
};

use crate::shaders::vs;

// Static scene camera definition
pub mod camera;
// Handling of different spaces, e.g. world-space, object-space, camera-space
pub mod space;

// A Gfx scene is some collection of Renderables
pub trait Scene<P: DeviceOwned> {
    // TODO - don't pass in `device`, as the renderpass will be `DeviceOwned` and therefore provide
    // `.device()`
    fn draw_all(&self,
        dynamic_state: &DynamicState,
        pass: P,
        cmd_buffer: &mut AutoCommandBufferBuilder,
        buffer_pool: &CpuBufferPool<vs::ty::Data>,
    );
}

/*
pub struct Scene<P> {
    pub camera: Camera,
    pub drawables: Vec<Box<dyn DrawAndTick<P>>>,
}

impl<P: Clone> Scene<P> {
  pub fn draw_all(&self, device: Arc<Device>, dynamic_state: &DynamicState, pass: P, mut cmd_buffer: AutoCommandBufferBuilder) -> AutoCommandBufferBuilder {
      for drawable in self.drawables.iter() {
          cmd_buffer = drawable.draw_call(device.clone(), pass.clone()).draw(dynamic_state, cmd_buffer);
      }
      cmd_buffer
  }

  pub fn tick_all(&mut self, dt: Duration) {
    for tickable in self.drawables.iter_mut() {
        tickable.tick(dt);
    }
  }
}
*/