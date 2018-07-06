// Terrain.h

namespace Terrain {

  struct Terrain {
    const TerrainSpec spec;
    Bounds renderBounds;

    auto updateBlocks() -> void;
    auto render() -> void;
  };

  struct vec2i {
    int x;
    int y;

    auto u() -> int { return x; }
    auto v() -> int { return y; }
  }

  struct vec2f {
    float x;
    float y;

    auto u() -> float { return x; }
    auto v() -> float { return y; }
  }

  struct Bounds {
    vec2i min;
    vec2i max;

    auto operator==(const Bounds& b) const -> Bool
    auto operator!=(const Bounds& b) const -> Bool

    auto intersect(const Bounds& b) const -> Bounds
    auto contains(vec2i c) const -> Bool
  }

  // Constant terrain properties
  struct TerrainSpec {
    const vec2f dimensions;
    const vec2i blockDimensions;

    const vec2i samplesPerBlock;
    const vec2i lodIntervals;

    const canyon& canyon;

    auto totalBlocks() const -> int { return blockDimensions.x * blockDimensions.y; }
    auto boundsAt(vector origin) const -> Bounds;
  }

} // Terrain::
