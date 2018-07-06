// Terrain.cpp

namespace Terrain {

  Terrain::updateBlocks ()
  {
    /* We have a set of current blocks, B, and a set of projected blocks based on the new position, B'
       All blocks ( b | b is in B n B' ) we keep, shifting their pointers to the correct position
       All other blocks fill up the empty spaces, then are recalculated */

    Bounds bounds = t.boundsAt( sampleOrigin );

    if ( bounds != t->bounds ) {
      Bounds intersection = t->bounds.intersect(newBounds);

      for (b : allBlocks) {
        if (!intersection.contains(coord(b))) {
          // newBlock
        }
        else {
          // dropBlock
        }
      }

      for (b : allBlocks) {
        if (b.lodLevel() < b.currentLodLevel || !intersection.contains(block)) {
          // regenerate()
        }
      }

      t.blocks = blocks;
      t.bounds = bounds;
    }
  }

  Terrain::render() {
    for (b : allBlocks) {
      // TODO - test if renderable is nonEmpty?
      b.renderable.draw();
    }
  }

} // Terrain::
