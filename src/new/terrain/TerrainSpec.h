// TerrainSpec.h
#pragma once

namespace terrain {
  // Constant terrain properties
  struct TerrainSpec {
    const vec2f radius;             // How wide and deep (in world space) the terrain is
    const vec2i blockDimensions;    // How many block subdivisions in u and v space
    const vec2i blockResolution;    // Vertex resolution of blocks
    const vec2i lodIntervals;       // Distances at which we adjust LoD of blocks

    const canyon& _canyon;

    auto totalBlocks() const -> int { return blockDimensions.x * blockDimensions.y; }
    auto boundsAt(vector origin) const -> Bounds;
    auto uPerBlock() const -> float { return radius.u() * 2 / static_cast<float>(blockDimensions.u()); }
    auto vPerBlock() const -> float { return radius.v() * 2 / static_cast<float>(blockDimensions.v()); }
    auto uScale() const -> float { return uPerBlock() / static_cast<float>(blockResolution.u()); }
    auto vScale() const -> float { return vPerBlock() / static_cast<float>(blockResolution.v()); }
    auto blockContaining( const canyon& c, vector& point ) const -> vec2i;
  };

} // terrain::
