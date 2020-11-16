use std::any::{Any, TypeId};

use std::collections::HashMap;
use std::time::Duration;

/*
struct AnyMap {
    inner: HashMap<TypeId, ()>
}

impl AnyMap {
    fn get<T: Any>() -> Option<T> {
        None
    }

    fn register<T: Any + Tickable>() {
        let f: fn(&mut T, Duration) = T::tick;
        ();
    }
}
*/

pub trait Tickable {
    fn tick(&mut self, dt: Duration);
}

//----

pub struct Ticklist<T> {
    ts: Vec<T>,
    tick: fn(&mut T, Duration),
}

impl<T> Ticklist<T> {
    pub fn tick_all(&mut self, dt: Duration) {
        for t in self.ts.iter_mut() {
            (self.tick)(t, dt)
        }
    }
}

impl<T: Tickable> Ticklist<T> {
    pub fn from(t: T) -> Ticklist<T> {
        Ticklist {
            ts: vec![t],
            tick: T::tick,
        }
    }

    pub fn add(&mut self, t: T) {
        self.ts.push(t)
    }
}

pub struct Tickmap {
    map: HashMap<TypeId, Box<dyn Any>>,
}

impl Tickmap {
    pub fn get<T: Any>(&mut self) -> Option<&mut Ticklist<T>> {
        self.map.get_mut(&TypeId::of::<T>()).and_then(|a| a.downcast_mut())
    }

    pub fn register<T: Any + Tickable>(&mut self, t: T) {
        //let f: fn(&mut T, Duration) = T::tick;
        match self.get::<T>() {
            Some(tickers) => {
                tickers.add(t);
            }
            None => {
                let id: TypeId = T::type_id(&t);
                let tickers = Box::new(Ticklist::from(t));
                self.map.insert(id, tickers);
            }
        }
    }
}
