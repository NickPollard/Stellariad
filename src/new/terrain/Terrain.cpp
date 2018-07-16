// Terrain.cpp

namespace terrain {

  // Add all pending blocks that will still be valid with our new bounds calculation
  void Terrain::applyPendingBlocks(Bounds newBounds) {
    const auto approx_count = pendingBlocks.size_approx(); // Estimated size, this is not guaranteed to drain all
    Block* pending = stackArray(Block, approx_count);
    const auto count = pendingBlocks.try_dequeue_bulk(item, approx_count);
    for (int i = 0; i < count; ++i) {
      auto& b = pending[i];
      if ( newBounds.contains(b) )
        blocks[b.coord] = BuiltBlock(b);
    }
  }

  auto Terrain::boundsAt(vector origin) const -> Bounds {
    const auto radius = (blockDimensions - 1) / 2;
    const auto block = blockContaining( spec->canyon, origin );
    return Bounds(block - radius, block + radius);
  }

  auto blockContaining( canyon* c, vector* point ) const -> vec2i {
    float u, v;
    canyonSpaceFromWorld( c, point->coord.x, point->coord.z, &u, &v );
    return vec2i(
      fround( u / uPerBlock(), 1.f ),
      fround( v / vPerBlock(), 1.f )
    );
  }

  void Terrain::updateBlocks() {
    Bounds newBounds = boundsAt( sampleOrigin );

    applyPendingBlocks(newBounds);

    if ( newBounds != bounds ) {
      Bounds intersection = bounds.intersect(newBounds);

      unordered_map<vec2i, MaybeBlock>& oldBlocks = blocks;

      for (b : oldBlocks) {
        if (!intersection.contains(coord(b))) {
          delete b;
          // Don't need to remove from oldBlocks - will be cleared at the end of this function
        }
      }

      // - or -
      //
      // unordered_map<vec2i, MaybeBlock> newBlocks = oldBlocks.filter(newBounds.contains);
      //
      //
      // TODO - could stack array this?
      const auto newSpecs = generateNewSpecs( newBounds );
      for (spec : (*newSpecs)) {
        if (!newBlocks.contains(spec.blockPosition) || newBlocks[spec.blockPosition].lodLevel() < spec.lodLevel())
          spec.generateBlock().foreach( [this&](auto block) { this.setBlock(block); })
        newBlocks[spec.BlockPosition] = PendingBlock(spec);
      }

      blocks = newBlocks;
      bounds = newBounds;
    }
  }

  // From min->max generate all u,v `BlockSpec`s with appropriate lod levels
  auto Terrain::generateNewSpecs( newBounds ) -> unique_ptr<vector<BlockSpec>> {
    size_t expectedSize = 50; // calculate from bounds width/height
    auto specs = make_unique<vector<BlockSpec>>( expectedSize );

    for (int u = newBounds.min.u(); u < newBounds.max.u(); ++u )
      for (int v = newBounds.min.v(); v < newBounds.max.v(); ++v )
        specs.emplace_back<BlockSpec>(vec2i(u, v), blockResolution, this); // TODO - calculate LoD here
  }

  void Terrain::render() {
    for (b : blocks) b.draw();
  }

  void Terrain::setBlock(Block& b) {
    // Append to our concurrent queue
    pendingBlocks.enqueue(b);
  }

} // terrain::
