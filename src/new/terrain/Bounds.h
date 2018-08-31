// Bounds.h
#pragma once

struct vec2i {
  int x;
  int y;

  auto u() const -> int { return x; }
  auto v() const -> int { return y; }

  vec2i( int _x, int _y ) : x(_x), y(_y) {}

  // Scalar group operations
  auto operator +(int s) const -> vec2i { return vec2i( x + s, y + s); }
  auto operator -(int s) const -> vec2i { return vec2i( x - s, y - s); }
  auto operator /(int s) const -> vec2i { return vec2i( x / s, y / s); }
  auto operator *(int s) const -> vec2i { return vec2i( x * s, y * s); }

  // Point-wise vector operations
  auto operator +(vec2i v) const -> vec2i { return vec2i( x + v.x, y + v.y); }
  auto operator -(vec2i v) const -> vec2i { return vec2i( x - v.x, y - v.y); }
  auto operator /(vec2i v) const -> vec2i { return vec2i( x / v.x, y / v.y); }
  auto operator *(vec2i v) const -> vec2i { return vec2i( x * v.x, y * v.y); }

  auto operator ==(const vec2i& v) const -> bool { return v.x == x && v.y == y; }
  auto operator !=(const vec2i& v) const -> bool { return v.x != x || v.y != y; }
};

struct vec2f {
  float x;
  float y;

  auto u() const -> float { return x; }
  auto v() const -> float { return y; }
};

struct Bounds {
  vec2i minimum;
  vec2i maximum;

  Bounds(vec2i _min, vec2i _max) : minimum(_min), maximum(_max) {}

  auto operator ==(const Bounds& b) const -> bool;
  auto operator !=(const Bounds& b) const -> bool;

  auto intersect(const Bounds& b) const -> Bounds;
  auto contains(vec2i p) const -> bool;
};
