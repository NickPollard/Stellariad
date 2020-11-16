use cgmath::{Vector3, Vector4};
use vulkano::{
    command_buffer::DynamicState,
    descriptor::descriptor_set::PersistentDescriptorSet,
    descriptor::descriptor_set::PersistentDescriptorSetBuf,
    device::{Device, DeviceExtensions},
    framebuffer::{Framebuffer, FramebufferAbstract, RenderPassAbstract},
    image::SwapchainImage,
    instance::{Instance, PhysicalDevice},
    memory::pool::StdMemoryPool,
    pipeline::{
        GraphicsPipelineAbstract,
        viewport::Viewport,
    },
    swapchain::{PresentMode, SurfaceTransform, Swapchain},
};
use vulkano_win::VkSurfaceBuild;
use gfx::call::Drawable;

use std::{
    sync::Arc,
    time::Duration,
};

pub trait Tickable {
    fn tick(&mut self, dt: Duration);
}

pub fn vec3(x: f32, y: f32, z: f32) -> Vector3<f32> {
    Vector3::new(x, y, z)
}

pub fn vec4(x: f32, y: f32, z: f32, w: f32) -> Vector4<f32> {
    Vector4::new(x, y, z, w)
}

pub trait DrawAndTick<P> : Drawable<P> + Tickable {}

impl<P, T : Drawable<P> + Tickable> DrawAndTick<P> for T {}
