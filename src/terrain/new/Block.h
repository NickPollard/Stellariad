// Block.h

namespace terrain {
  struct BlockSpec {
    // Construct a new Block from a given BlockSpec
    auto generateBlock() const -> Future<Block>;

    const vec2i blockPosition;  // Where this block is in `block-uv` space
    const vec2i samples;        // The resolution of the heightmap
    const TerrainSpec& terrain; // The constant terrain properties

    // TODO - store lod level

    auto vertCount() const -> int { return ( samples.u() +2 ) * ( samples.v() +2); } // +2 because we extend 1 step further on each edge of the block
    auto triangleCount() const -> int { return ( samples.u() - 1 ) * ( samples.v() - 1 ) * 2; }
    auto elementCount() const -> int { return triangleCount() * 3; }
    auto indexFromUV( int u, int v ) const -> int { return u + 1 + ( v + 1 ) * ( samples.u() + 2 ); }
    auto renderIndexFromUV( int u, int v ) const -> int {
      vAssert( u >= 0 && u < samples.u() ); vAssert( v >= 0 && v < samples.v() ); return u + v * samples.u();
    }
    // As this is just renderable verts, we dont have the extra buffer space for normal generation
    auto renderVertCount() const -> int {
      #if CANYON_TERRAIN_INDEXED
        return samples.u() * samples.v();
      #else
        return element_count;
      #endif
    auto positionsFromUV( int u_index, int v_index, float& u, float& v ) const -> vec2f;

    // Extents in `world-uv` space
    auto u_min() const -> float { return (static_cast<float>(blockPosition.u()) - 0.5f) * terrain.uf(); }
    auto v_min() const -> float { return (static_cast<float>(blockPosition.v()) - 0.5f) * terrain.vf(); }
    auto u_max() const -> float { u_min() + terrain.uf(); }
    auto v_max() const -> float { v_min() + terrain.vf(); }

    // TODO - add element-wise vec2 multiplication and addition
    //Extents in `block-uv` space
    auto blockSpaceMinU() const -> int { return blockPosition.u() * terrain.blockResolution.u() - (terrain.blockResolution.u() / 2); }
    auto blockSpaceMinV() const -> int { return blockPosition.v() * terrain.blockResolution.v() - (terrain.blockResolution.v() / 2); }

    private:
      auto generatePoints( vector* verts ) const -> vector*;
      auto generateNormals( vector* verts, vector* normals ) const -> vector*;
      void lodVectors( vector* vectors ) const;
  };

  struct Block {
    BlockSpec spec;
    GLRenderable renderable;
    body& collision;

    auto draw() -> bool { return renderable.draw(); }
    ~Block() { delete collision; } // TODO - how do we actually delete?
  };
}
