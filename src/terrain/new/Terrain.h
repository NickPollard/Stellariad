// Terrain.h
#include "concurrentqueue/concurrentqueue.h"

using moodycamel::ConcurrentQueue;

namespace terrain {

  struct Terrain {
    const TerrainSpec spec;
    Bounds bounds;
    vector<MaybeBlock> blocks;
    ConcurrentQueue<Block> pendingBlocks;

    void updateBlocks();
    void render();
    auto blockContaining( canyon* c, vector* point ) const -> vec2i

    private:
      auto generateNewSpecs( newBounds ) -> unique_ptr<vector<BlockSpec>>
      void applyPendingBlocks();
  };

  // Constant terrain properties
  struct TerrainSpec {
    const vec2f radius;             // How wide and deep (in world space) the terrain is
    const vec2i blockDimensions;    // How many block subdivisions in u and v space
    const vec2i blockResolution;    // Vertex resolution of blocks
    const vec2i lodIntervals;       // Distances at which we adjust LoD of blocks

    const canyon& canyon;

    auto totalBlocks() const -> int { return blockDimensions.x * blockDimensions.y; }
    auto boundsAt(vector origin) const -> Bounds;
    auto uPerBlock() const -> float { return radius.u() * 2 / static_cast<float>(blockDimensions.u()); }
    auto vPerBlock() const -> float { return radius.v() * 2 / static_cast<float>(blockDimensions.v()); }
    auto uScale() const -> float { uPerBlock() / static_cast<float>(blockResolution.u()); }
    auto vScale() const -> float { vPerBlock() / static_cast<float>(blockResolution.v()); }
  };

  class MaybeBlock {
    virtual auto lodLevel() const -> int
    virtual void draw() const
  };

  struct BuiltBlock : public MaybeBlock {
    unique_ptr<Block> block;

    virtual void draw() {
      block->draw();
    }
  }

  struct PendingBlock : public MaybeBlock {
    //unique_ptr<BlockSpec> spec;
    BlockSpec spec;

    virtual void draw() {}
  }

} // terrain::
