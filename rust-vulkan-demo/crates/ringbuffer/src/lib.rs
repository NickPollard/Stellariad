pub struct RingBuffer<T> {
    capacity: usize,
    data: Vec<T>,
    front: usize,
    size: usize
}

impl<T> RingBuffer<T> {
    pub fn new(capacity: usize) -> RingBuffer<T> {
        RingBuffer::<T> { capacity, data: Vec::with_capacity(capacity), front: 0, size: 0 }
    }

    // Always succeeds, might remove back
    pub fn add(&mut self, t: T) {
        self.front = self.advance(self.front);
        if self.size < self.capacity {
            self.size += 1;
            self.data.push(t);
        } else {
            self.data[self.front] = t;
        }
    }

    // Advance an index to the next index in the ring buffer
    pub fn advance(&self, index: usize) -> usize { (index + 1) % self.capacity }

    pub fn get(&self, index: usize) -> &T {
        &self.data[self.raw_index(index)]
    }

    pub fn get_mut<'a>(&'a mut self, index: usize) -> &'a mut T {
        let i = self.raw_index(index);
        &mut self.data[i]
    }

    pub fn raw_index(&self, index: usize) -> usize {
        (self.front - 1 - index) % self.capacity
    }

    pub fn iter(&self) -> RingBufferIter<T> {
        RingBufferIter{ buffer: self, index: 0 }
    }

    pub fn iter_mut(&mut self) -> RingBufferIterMut<T> {
        RingBufferIterMut{ buffer: self, index: 0 }
    }

    pub fn count(&self) -> usize { self.size }

    pub fn empty(&self) -> bool { self.size <= 0 }

    pub fn drop_while<F>(&mut self, f: F)
        where F: Fn(&T) -> bool {
            while self.size > 0 && f(self.get(0)) {
                self.front = self.advance(self.front);
                self.size -= 1;
            }

        }
}

pub struct RingBufferIter<'a, T> {
    buffer: &'a RingBuffer<T>,
    index: usize,
}

pub struct RingBufferIterMut<'a, T> {
    buffer: &'a mut RingBuffer<T>,
    index: usize,
}

impl<'a, T> Iterator for RingBufferIterMut<'a, T> {
    type Item = &'a mut T;
    fn next(&mut self) -> Option<Self::Item> {
        unsafe {
            if self.index < self.buffer.size {
                let elem: *mut T = self.buffer.get_mut(self.index);
                self.index += 1;
                Some(&mut *elem)
            } else {
                None
            }
        }
    }
}

impl<'a, T> Iterator for RingBufferIter<'a, T> {
    type Item = &'a T;
    fn next(&mut self) -> Option<Self::Item> {
        unsafe {
            if self.index < self.buffer.size {
                let elem: *const T = self.buffer.get(self.index);
                self.index += 1;
                Some(& *elem)
            } else {
                None
            }
        }
    }
}

#[cfg(test)]
mod test {
    use super::RingBuffer;
    #[test]
    fn empty_ring_iter() {
        let buffer = RingBuffer::<u64>::new(10);
        for i in buffer.iter() {
            println!("{}", i);
        }
        assert!(true);
    }
    #[test]
    fn empty_ring_iter_mut() {
        let mut buffer = RingBuffer::<u64>::new(10);
        for i in buffer.iter_mut() {
            println!("{}", i);
        }
        assert!(true);
    }
    #[test]
    fn non_empty_ring_iter() {
        let mut buffer = RingBuffer::<u64>::new(10);
        buffer.add(2);
        buffer.add(1);
        let mut out = Vec::<u64>::new();
        for i in buffer.iter() {
            out.push(*i);
        }
        assert!(out == vec![1,2]);
    }
}

