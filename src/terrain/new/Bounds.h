// Bounds.h

struct vec2i {
  int x;
  int y;

  auto u() -> int { return x; }
  auto v() -> int { return y; }

  // Scalar group operations
  auto operator +(int s) -> vec2i { return vec2i( x + s, y + s); }
  auto operator -(int s) -> vec2i { return vec2i( x - s, y - s); }
  auto operator /(int s) -> vec2i { return vec2i( x / s, y / s); }
  auto operator *(int s) -> vec2i { return vec2i( x * s, y * s); }

  // Point-wise vector operations
  auto operator +(vec2i v) -> vec2i { return vec2i( x + v.x, y + v.y); }
  auto operator -(vec2i v) -> vec2i { return vec2i( x - v.x, y - v.y); }
  auto operator /(vec2i v) -> vec2i { return vec2i( x / v.x, y / v.y); }
  auto operator *(vec2i v) -> vec2i { return vec2i( x * v.x, y * v.y); }
};

struct vec2f {
  float x;
  float y;

  auto u() -> float { return x; }
  auto v() -> float { return y; }
};

struct Bounds {
  vec2i min;
  vec2i max;

  auto operator==(const Bounds& b) const -> Bool
  auto operator!=(const Bounds& b) const -> Bool

  auto intersect(const Bounds& b) const -> Bounds
  auto contains(vec2i p) const -> Bool
};
