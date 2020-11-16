#![allow(unused)]

use std::collections::HashMap;

pub struct InputDevices {}

#[derive(Clone)]
pub struct Inputs {}

pub fn tick(_devices: &mut InputDevices, _inputs: Inputs) -> Inputs {
    Inputs{}
    // Tick all input Devices, e.g.
    // Tick Mouse
    // Tick Keyboard
    // Tick Touch
}

#[derive(PartialEq, Eq)]
pub enum KeyState {
    /// Pressed this frame
    Pressed,
    /// Still held from a previous frame
    Held,
    /// Raised this frame
    Raised,
    /// Still unheld from a previous frame
    Unheld,
}

pub struct InputData {
    keys: HashMap<Key, KeyState>,
    //mouse: Mouse,
    //touch: Panel,
}

/// Identifies a unique physical key on the keyboard
// TODO: or a combination of keys?
#[derive(PartialEq, Eq, Hash)]
struct Key;

struct Mouse;
struct Panel;

/// Identifies an input bind we've defined. These are semantic actions, eg
/// 'quit key', 'new game key', 'weapon key' etc.
type Keybind = i64;

pub struct Input {
    frames: Vec<InputData>,
    active_frame: usize,
    keybinds: HashMap<Keybind, Key>,
}

impl Input {
    fn active_frame(&self) -> &InputData { &self.frames[self.active_frame] }

    fn key_bound(&self, bind: &Keybind) -> Option<&Key> {
        self.keybinds.get(bind)
    }

    fn bind_pressed(&self, bind: &Keybind) -> bool {
        let key = self.key_bound(bind);
        let state = key.and_then(|key| self.active_frame().keys.get(key));
        match state {
            Some(s) => *s == KeyState::Pressed,
            None => false,
        }
    }
}
