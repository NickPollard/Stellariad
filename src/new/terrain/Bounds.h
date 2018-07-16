// Bounds.h

struct vec2i {
  int x;
  int y;

  auto u() -> int { return x; }
  auto v() -> int { return y; }

  vec2i( int _x, int _y ) : x(_x), y(_y) {}

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

  auto operator ==(const vec2i& v) const -> bool { return v.x == x && v.y == y; }
  auto operator !=(const vec2i& v) const -> bool { return v.x != x || v.y != y; }
};

struct vec2f {
  float x;
  float y;

  auto u() -> float { return x; }
  auto v() -> float { return y; }
};

struct Bounds {
  vec2i minimum;
  vec2i maximum;

  auto operator ==(const Bounds& b) const -> bool;
  auto operator !=(const Bounds& b) const -> bool;

  auto intersect(const Bounds& b) const -> Bounds;
  auto contains(vec2i p) const -> bool;
};
