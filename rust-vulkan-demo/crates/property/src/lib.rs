use cgmath::Vector4;
use std::cmp::min;

/// An interpolated property value of type `T`
pub struct Property<T> where
    T: LinearInterpolate {
    count: usize,
    data: Vec<(f32,T)>,
}

// This is really a VectorSpace?
pub trait LinearInterpolate {
    fn lerp(a: &Self, b: &Self, factor: f32) -> Self;
}

impl LinearInterpolate for f32 {
    fn lerp(a: &f32, b: &f32, factor: f32) -> f32 {
        assert!( factor <= 1.0 && factor >= 0.0 );
        (b - a) * factor + a
    }
}

impl<S: LinearInterpolate> LinearInterpolate for Vector4<S> {
    fn lerp(a: &Vector4<S>, b: &Vector4<S>, factor: f32) -> Vector4<S> {
        Vector4::new(
            LinearInterpolate::lerp(&a.x, &b.x, factor),
            LinearInterpolate::lerp(&a.y, &b.y, factor),
            LinearInterpolate::lerp(&a.z, &b.z, factor),
            LinearInterpolate::lerp(&a.w, &b.w, factor),
        )
    }
}

fn clamp<T: PartialOrd>( value: T, bottom: T, top: T ) -> T {
    //min( max(value, bottom), top)
    if value < bottom {
        bottom
    } else if value > top {
        top
    } else {
        value
    }
}


fn map_range( p: f32, begin: f32, end: f32 ) -> f32 { (p - begin) / (end - begin) }

impl<T: LinearInterpolate + Clone> Property<T> {
    pub fn sample( &self, time: f32 ) -> T {
        assert!(self.count > 0);
        if self.count == 1 {
            return self.data[0].1.clone();
        }
        assert!(self.count > 1);
        let mut time_before = 0.0;
        let mut before = 0;
        let mut after = 0;
        while after < self.count && self.data[after].0 < time {
            time_before = self.data[before].0;
            before = after;
            after += 1;
        }
        after = min( after, self.count - 1);
        let time_after = self.data[after].0;
        let factor = if after == before { 0.0 } else {
            assert!( after > before );
            map_range( time, time_before, time_after )
        };
        let factor = clamp( factor, 0.0, 1.0 );
        LinearInterpolate::lerp( &self.data[before].1, &self.data[after].1, factor )
    }

    pub fn duration( self ) -> f32 { self.data[self.count - 1].0 }
}

impl<T: LinearInterpolate> From<Vec<(f32,T)>> for Property<T> {
    fn from(data: Vec<(f32,T)>) -> Property<T> {
        let count = data.len();
        Property{ count, data }
    }
}

#[cfg(test)]
mod test {
    use super::*;

    fn test_value() -> Property<f32> {
        Property::from(vec![(0.0, 0.0), (5.0, 1.0)])
    }

    #[test]
    fn initial() {
        assert_eq!(test_value().sample(0.0), 0.0)
    }

    #[test]
    fn terminal() {
        assert_eq!(test_value().sample(5.0), 1.0)
    }

    #[test]
    fn midway() {
        assert_eq!(test_value().sample(2.5), 0.5)
    }

    #[test]
    fn after() {
        assert_eq!(test_value().sample(7.5), 1.0)
    }

    // TODO
    // * test before first frame
    // * test after last frame
    // * test rate of linear interpolation
}
