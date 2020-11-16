#![allow(dead_code, unused)]

#[macro_use]
extern crate vulkano;

mod engine;
mod particle;
mod physics;
mod rotating_obj;
//mod system;
mod types;

use {
    argh::FromArgs,
    gfx::{
        Gfx,
        model::Model,
        scene::camera::Camera,
    },
    vulkano::framebuffer::RenderPassAbstract,
};


use crate::{
    engine::{
        Engine, Scene,
    },
    rotating_obj::{RotatingObj},
};

#[derive(FromArgs)]
/// test configuration
struct Config {
    /// model to display
    #[argh(option)]
    model: String
}

fn main() {
    let config: Config = argh::from_env();

    println!("Starting up");

    let gfx = Gfx::init();
    let scene = load_model_demo_scene(&gfx, &config.model);
    //let scene = load_model_demo_scene(&gfx, "assets/ship_hd_2.obj");
    let mut engine = Engine::init(gfx, scene);
    let mut cont = true;
    while cont {
        cont = engine.tick();
    }
}

/// Create our example test scene
fn init_test_scene<RPass: RenderPassAbstract + Send + Sync + 'static + Clone>(gfx: &Gfx) -> Scene<RPass> {
    /*
       let particle_def = Arc::new(particle::EmitterDefinition::new(
       0.0,
       vec4(5.0, 0.0, 5.0, 0.0),
       Property::from(vec![(0.0,0.5), (5.0, 3.0)]),
       Property::from(vec![
       (0.0, vec4(1.0, 1.0, 0.0, 0.0)),
       (0.5, vec4(1.0, 1.0, 0.0, 1.0)),
       (1.5, vec4(1.0, 0.0, 0.0, 1.0)),
       (2.0, vec4(1.0, 0.0, 0.0, 0.0)),
       ]),
       Property::from(vec![(0.0,10.0)]),
       vec4(0.0, -5.0, 0.0, 0.0),
       false,
       false,
       ));
       let particle = particle::Emitter::new(particle_def, gfx.system.device.clone(), gfx.uniform_buffers.clone());
       let scene: Scene<RPass> = Scene{ drawables: vec![ Box::new(particle) ] };
       */
    let model = RotatingObj::new(Model::from_obj(gfx.system.device.clone(), "assets/ship_hd_2.obj").unwrap(), gfx.uniform_buffers.clone());
    let camera = Camera::origin();
    let scene: Scene<RPass> = Scene{ camera, drawables: vec![ Box::new(model) ] };
    scene
}

/// Create our test scene showcasing the given model
fn load_model_demo_scene<RPass: RenderPassAbstract + Send + Sync + 'static + Clone>(gfx: &Gfx, model: &str) -> Scene<RPass> {
    let model = RotatingObj::new(Model::from_obj(gfx.system.device.clone(), model).unwrap(), gfx.uniform_buffers.clone());
    Scene{ camera: Camera::origin(), drawables: vec![ Box::new(model) ] }
}
