// Terrain.h
#pragma once

#include "concurrentqueue/concurrentqueue.h"
#include "new/terrain/Block.h"
#include "new/terrain/Bounds.h"
#include "new/terrain/TerrainSpec.h"
#include <vector>

using moodycamel::ConcurrentQueue;

namespace terrain {
  
  struct Block;

  struct MaybeBlock {
    virtual auto lodLevel() const -> int;
    virtual void draw() const;
    virtual auto blockPosition() const -> vec2i;
  };

  struct BuiltBlock : public MaybeBlock {
    unique_ptr<Block> block;

    virtual void draw(); // TODO { block->draw(); }
    virtual auto blockPosition() const -> vec2i { return block->spec.blockPosition; }
  };

  struct PendingBlock : public MaybeBlock {
    //unique_ptr<BlockSpec> spec;
    BlockSpec spec;

    virtual void draw() {}
    virtual auto blockPosition() const -> vec2i { return spec.blockPosition; }
  };

  struct Terrain {
    const TerrainSpec spec;
    Bounds bounds;
    std::vector<MaybeBlock> blocks;
    ConcurrentQueue<Block> pendingBlocks;

    void updateBlocks();
    void render();

    private:
      auto generateNewSpecs( Bounds newBounds ) -> unique_ptr<std::vector<BlockSpec>>;
      void applyPendingBlocks(Bounds newBounds);
      void setBlock(Block& b);
  };

} // terrain::
