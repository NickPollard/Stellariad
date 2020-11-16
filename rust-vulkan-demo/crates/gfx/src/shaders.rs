use vulkano::device::Device;
use std::sync::Arc;

// mod shaders
pub mod vs {
    vulkano_shaders::shader!{
        ty: "vertex",
        src: "
#version 450

layout(set = 0, binding = 0) uniform Data {
    mat4 world;
} uniforms;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

layout(location = 0) out float depth;
layout(location = 1) out vec4 frag_position;
layout(location = 2) out vec4 frag_normal;
layout(location = 3) out vec4 frag_color;

void main() {
    vec4 pos = uniforms.world * vec4(position, 1.0);
    gl_Position = vec4(pos.x, pos.y, pos.z, pos.w);
    depth = gl_Position.z;
    frag_color = color;
    frag_normal = uniforms.world * vec4( normal.xyz, 0.0 );
    frag_position = vec4( pos.xyz, 1.0 );
}",
    }
}

pub mod fs {
    vulkano_shaders::shader!{
        ty: "fragment",
        src: "
#version 450

layout(location = 0) in float depth;
layout(location = 1) in vec4 frag_position;
layout(location = 2) in vec4 frag_normal;
layout(location = 3) in vec4 frag_color;

layout(location = 0) out vec4 f_color;

const vec4 directional_light_direction = vec4( 1.0, 1.0, 1.0, 1.0 );
const vec4 directional_light_diffuse = vec4( 0.5, 0.5, 0.3, 1.0 );
const vec4 directional_light_specular = vec4( 0.5, 0.5, 0.3, 1.0 );
const vec4 ambient_color = vec4( 0.2, 0.2, 0.2, 1.0 );

void main() {

    vec4 view_dir = normalize( frag_position );
    // TODO mul by modelview?
    vec4 light_dir = normalize(directional_light_direction);
    float diffuse = max( 0.0, dot( -light_dir, frag_normal) );
    vec4 diffuse_color = directional_light_diffuse * diffuse;

    vec4 spec_bounce = reflect( light_dir, frag_normal );
    float spec = max( 0.0, dot( spec_bounce, -view_dir ));
    float shininess = 10.0;
    float specular = pow( spec, shininess );
    vec4 specular_color = directional_light_specular * specular;

    vec4 lit_color = frag_color * (diffuse_color + specular_color + ambient_color);

    float near = 20.0;
    float far = 80.0;
    float fog = clamp((depth - near) / (far - near), 0.0, 1.0);

    const vec4 fog_color = vec4(0.0, 0.0, 1.0, 1.0);
    f_color = mix(lit_color, fog_color, fog);

    ////f_color = vec4(fog, fog, 0.5, 1.0);
    //f_color = vec4(frag_normal.xyz, 1.0);
    ////f_color = frag_color;
}
",
    }
}

pub fn load_shaders(device: Arc<Device>) -> (vs::Shader,fs::Shader) {
    let vs = vs::Shader::load(device.clone()).unwrap();
    let fs = fs::Shader::load(device.clone()).unwrap();
    (vs,fs)
}
