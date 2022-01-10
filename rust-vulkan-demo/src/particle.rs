use vulkano::{
    device::Device,
    buffer::CpuBufferPool,
    framebuffer::RenderPassAbstract,
};

use cgmath::{Matrix4, Vector3, Vector4, Rad, PerspectiveFov, ElementWise};
use rand::Rng;
use std::{
    sync::Arc,
    time::Duration,
};
use property::*;
use ringbuffer::*;

use crate::{
    types::Tickable,
};

use gfx::{
    call::{Call, CallT, Drawable},
    model::create_pipeline,
    shaders::{self, vs},
    types::{
        SubBuffer,
        vertex::{Vertex, vertex_buffer},
    },
    descriptors_for,
};

const MAX_PARTICLES: usize = 128;

type Vec4 = Vector4<f32>;

struct Particle {
    position: Vec4,
    age: f32,
    rotation: f32,
}

pub struct EmitterDefinition {
    lifetime: f32,
    spawn_box: Vec4,
    size: Property<f32>,
    color: Property<Vec4>,
    spawn_rate: Property<f32>,
    velocity: Vec4,
    // TODO
    // texture_diffuse
    // flags: u32,
    random_rotation: bool,
    oneshot: bool,
}

impl EmitterDefinition {
    pub fn new(
        lifetime: f32,
        spawn_box: Vec4,
        size: Property<f32>,
        color: Property<Vec4>,
        spawn_rate: Property<f32>,
        velocity: Vec4,
        random_rotation: bool,
        oneshot: bool,
    ) -> EmitterDefinition {
        EmitterDefinition {
            lifetime,
            spawn_box,
            size,
            color,
            spawn_rate,
            velocity,
            random_rotation,
            oneshot,
        }
    }
}

pub struct Emitter {
    // TODO transform
    definition: Arc<EmitterDefinition>,
    next_spawn: f32,
    emitter_age: f32,
    dead: bool, // We've been explicitly killed; just cleaning up
    dying: bool, // We've been asked to die gracefully once we're finished

    particles: RingBuffer<Particle>,
    //vertex_buffer: Vec<Vertex>,
    vs: shaders::vs::Shader,
    fs: shaders::fs::Shader,
    uniform_buffers: CpuBufferPool<vs::ty::Data>,
}

fn frand(_lower: f32, _upper: f32) -> f32 {
    let mut rng = rand::thread_rng();
    rng.gen::<f32>()
}

impl Emitter {
    pub fn new(def: Arc<EmitterDefinition>, device: Arc<Device>,
                uniform_buffers: CpuBufferPool<vs::ty::Data>) -> Emitter {
        Emitter {
            definition: def.clone(),
            next_spawn: 0.0,
            emitter_age: 0.0,
            dead: false,
            dying: false,
            particles: RingBuffer::new(MAX_PARTICLES),
            //vertex_buffer: vec![],
            vs: shaders::vs::Shader::load(device.clone()).unwrap(),
            fs: shaders::fs::Shader::load(device.clone()).unwrap(),
            uniform_buffers,
        }
    }
    fn emit( &mut self ) {
        let random = Vec4::new(
            frand( -1.0, 1.0 ),
            frand( -1.0, 1.0 ),
            frand( -1.0, 1.0 ),
            1.0);
        let pi = std::f64::consts::PI as f32;
        let position = random.mul_element_wise(self.definition.spawn_box);
        let age = 0.0;
        let rotation = if self.definition.random_rotation { frand( 0.0, 2.0*pi ) } else { 0.0 };
        self.particles.add( Particle { position, age, rotation });
    }

    fn tick_internal( &mut self, dt: f32 ) {
        if self.dead || (self.dying && self.particles.empty()) {
            // TODO - kill (Remove from iteration)
            return;
        }

        let delta = self.definition.velocity * dt;

        {
            for p in self.particles.iter_mut() {
                p.age += dt;
                p.position += delta;
            }
        }

        //let definition = &self.definition;
        // TODO - fix this and add test
        //self.particles.drop_while(|p| p.age > definition.lifetime );

        // TODO - make this a state enum
        if !(self.dead || self.dying) {
            // TODO - burst mode
            self.next_spawn += dt;
            let spawn_rate = self.definition.spawn_rate.sample(self.emitter_age);
            let spawn_interval = 1.0 / spawn_rate;
            while self.next_spawn > spawn_interval {
                self.next_spawn = ( self.next_spawn - spawn_interval ).max(0.0);
                self.emit();
            }
        }
        self.emitter_age += dt;
    }

    fn render( &self ) -> Vec<Vertex> {
        let mut vertex_buffer = Vec::new();
        vertex_buffer.clear();

        if !self.dead {
            // Set up modelview matrix?
            for p in self.particles.iter() {
                let (a,b,c,d) = Emitter::quad(p, &self.definition);
                vertex_buffer.push(a.clone());
                vertex_buffer.push(b.clone());
                vertex_buffer.push(c);
                vertex_buffer.push(b);
                vertex_buffer.push(a);
                vertex_buffer.push(d);
            }
        }

        vertex_buffer
    }

    fn quad(p: &Particle, definition: &EmitterDefinition) -> (Vertex,Vertex,Vertex,Vertex) {
        let size = definition.size.sample(p.age);
        let sample_color = definition.color.sample(p.age);

        let rotation_matrix = Matrix4::from_angle_z( Rad(p.rotation) );

        let color = [ sample_color[0], sample_color[1], sample_color[2], sample_color[3] ];
        let normal = [1.0, 0.0, 0.0];

        let offset = rotation_matrix * Vector4::new( size, size, 0.0, 0.0 );
        let point = p.position + offset;
        let a = Vertex{ position: [point.x, point.y, point.z], normal, color };
        let offset = rotation_matrix * Vector4::new( -size, -size, 0.0, 0.0 );
        let point = p.position + offset;
        let b = Vertex{ position: [point.x, point.y, point.z], normal, color };
        let offset = rotation_matrix * Vector4::new( size, -size, 0.0, 0.0 );
        let point = p.position + offset;
        let c = Vertex{ position: [point.x, point.y, point.z], normal, color };
        let offset = rotation_matrix * Vector4::new( -size, size, 0.0, 0.0 );
        let point = p.position + offset;
        let d = Vertex{ position: [point.x, point.y, point.z], normal, color };
        (a,b,c,d)
    }
}

impl<P: RenderPassAbstract + Send + Sync + 'static + Clone>
  Drawable<P> for Emitter {
    fn draw_call(&self, pass: P, buffer_pool: &CpuBufferPool<vs::ty::Data>) -> Box<dyn Call<P>> {
        let verts = self.render();
        let vbuffer = vertex_buffer(pass.device().clone(), &verts);
        let pipeline = create_pipeline(&self.vs, &self.fs, pass);
        let subbuffer = uniform_buffer(&self.uniform_buffers);
        let descriptors = descriptors_for(subbuffer, pipeline.layout().descriptor_set_layout(0).unwrap().clone());
        Box::new(CallT::new(pipeline, vbuffer, Some(descriptors)))
    }
}

impl Tickable for Emitter {
    fn tick(&mut self, dt: Duration) {
        self.tick_internal(dt.as_secs_f64() as f32)
    }
}

// TODO
fn uniform_buffer(
    buffer_pool: &CpuBufferPool<vs::ty::Data>,
  )
-> SubBuffer<vs::ty::Data> {
    let near = 1.0;
    let far = 1800.0;
    let fovy : Rad<f32> = Rad(0.8);
    let width = 16.0;
    let height = 9.0;
    let aspect = width / height;
    let perspective: Matrix4<f32> = (PerspectiveFov{ fovy, aspect, near, far }).into();

    let translate = Matrix4::from_translation(Vector3::new(0.0, 0.0, -30.0));

    let uniform_data = vs::ty::Data {
        world: (perspective * translate).into(),
    };

    buffer_pool.next(uniform_data).unwrap()
}
