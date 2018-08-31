// Terrain.cpp

#include "common.h"
#include "new/terrain/Terrain.h"
#include "canyon.h" // for canyonSpaceFromWorld

#include <map>

using std::unordered_map;

namespace terrain {

  // Add all pending blocks that will still be valid with our new bounds calculation
  void Terrain::applyPendingBlocks(Bounds newBounds) {
    (void)newBounds;
    /*
    const auto approx_count = pendingBlocks.size_approx(); // Estimated size, this is not guaranteed to drain all
    Block* pending = stackArray(Block, approx_count);
    const auto count = pendingBlocks.try_dequeue_bulk(item, approx_count);
    for (int i = 0; i < count; ++i) {
      auto& b = pending[i];
      if ( newBounds.contains(b) )
        blocks[b.coord] = BuiltBlock(b);
    }
    */
  }

  auto TerrainSpec::boundsAt(vector origin) const -> Bounds {
    const auto radius = (blockDimensions - 1) / 2;
    const auto block = blockContaining( _canyon, origin );
    return Bounds(block - radius, block + radius);
  }

  auto TerrainSpec::blockContaining( const canyon& c, vector& point ) const -> vec2i {
    float u, v;
    // TODO - no const_cast
    canyonSpaceFromWorld( const_cast<canyon*>(&c), point.coord.x, point.coord.z, &u, &v );
    return vec2i(
      fround( u / uPerBlock(), 1.f ),
      fround( v / vPerBlock(), 1.f )
    );
  }

  void Terrain::updateBlocks() {
    vector sampleOrigin = Vector(0.f, 0.f, 0.f, 1.f); // TODO
    Bounds newBounds = spec.boundsAt( sampleOrigin );

    applyPendingBlocks(newBounds);

    if ( newBounds != bounds ) {
      Bounds intersection = bounds.intersect(newBounds);

      //unordered_map<vec2i, MaybeBlock>& oldBlocks = blocks;

      for (auto b : blocks) {
        if (!intersection.contains( b.blockPosition() )) {
          // TODO - remove this here
          // delete b;
          // Don't need to remove from oldBlocks - will be cleared at the end of this function
        }
      }

      // - or -
      //
      // unordered_map<vec2i, MaybeBlock> newBlocks = oldBlocks.filter(newBounds.contains);
      //
      //
      // TODO - could stack array this?

      /*
      const auto newSpecs = generateNewSpecs( newBounds );
      for (auto spec : (*newSpecs)) {
        if (!newBlocks.contains(spec.blockPosition) || newBlocks[spec.blockPosition].lodLevel() < spec.lodLevel())
          spec.generateBlock().foreach( [&](auto block) { this->setBlock(block); });
        newBlocks[spec.blockPosition] = PendingBlock(spec);
      }

      blocks = newBlocks;
      bounds = newBounds;
      */
    }
  }

  // From min->max generate all u,v `BlockSpec`s with appropriate lod levels
  auto Terrain::generateNewSpecs( Bounds newBounds ) -> unique_ptr<std::vector<BlockSpec>> {
    size_t expectedSize = 50; // calculate from bounds width/height
    auto specs = make_unique<std::vector<BlockSpec>>( expectedSize );

    for (int u = newBounds.minimum.u(); u < newBounds.maximum.u(); ++u )
      for (int v = newBounds.minimum.v(); v < newBounds.maximum.v(); ++v ) {
        specs->emplace_back(vec2i(u, v), spec.blockResolution, spec); // TODO - calculate LoD here
      }

    return specs;
  }

  void Terrain::render() {
    for (auto b : blocks) b.draw();
  }

  void Terrain::setBlock(Block& b) {
    // Append to our concurrent queue
    pendingBlocks.enqueue(b);
  }

} // terrain::
