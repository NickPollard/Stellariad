// Bounds.cpp
#include "Bounds.h"

#include "maths/maths.h"

auto Bounds::contains(vec2i point) const -> bool {
  return point.x >= minimum.x && point.x <= maximum.x
      && point.y >= minimum.y && point.y <= maximum.y;
}

auto Bounds::intersect(const Bounds& b) const -> Bounds {
  Bounds result = {{
    min(minimum.x, b.minimum.x),
    min(minimum.y, b.minimum.y)
  }, {
    max(maximum.x, b.maximum.x),
    max(maximum.y, b.maximum.y)
  }};
  return result;
}

auto Bounds::operator ==(const Bounds& b) const -> bool {
  return b.minimum == minimum && b.maximum == maximum;
}

auto Bounds::operator !=(const Bounds& b) const -> bool {
  return !(*this==b);
}
