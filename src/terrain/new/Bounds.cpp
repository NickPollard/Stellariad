// Bounds.cpp
#include "Bounds.h"

auto Bounds::contains(vec2i point) const -> bool {
  return point.x >= min.x && point.x =< max.x
      && point.y >= min.y && point.y =< max.y;
}

auto Bounds::intersect(const Bounds& b) const -> Bounds {
  Bounds b = {{
    min(min.x, b.min.x),
    min(min.y, b.min.y),
  }, {
    max(max.x, b.max.x),
    max(max.y, b.max.y),
  }};
  return b;
}

auto Bounds::operator==(const Bounds& b) const -> Bool {
  return b.min == min && b.max == max;
}

auto Bounds::operator!=(const Bounds& b) const -> Bool {
  return !(this==b);
}
