use winit::window::Window;

pub fn get_dimensions(window: &Window) -> (u32,u32) {
    let dimensions = window.inner_size();
    (dimensions.width, dimensions.height)
}
