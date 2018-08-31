// Block.h
#pragma once

#include "new/terrain/Bounds.h"
#include "new/terrain/GLRenderable.h"
#include "new/terrain/TerrainSpec.h"
#include "collision/collision.h"
#include "concurrent/future.h"

//using namespace brando;
using brando::concurrent::Future;

namespace terrain {
  struct Block;

  struct BlockSpec {
    // Construct a new Block from a given BlockSpec
    // TODO
    auto generateBlock() const -> Future<Block*>; // An owned block; not unique_ptr due to move constraints

    const vec2i blockPosition;  // Where this block is in `block-uv` space
    const vec2i samples;        // The resolution of the heightmap
    const TerrainSpec& terrain; // The constant terrain properties

    BlockSpec( const vec2i pos, const vec2i _samples, const TerrainSpec& _terrain) :
      blockPosition(pos),
      samples(_samples),
      terrain(_terrain)
    {}

    // TODO - store lod level
    auto lodLevel() const -> int { return 1; }

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
        return elementCount();
      #endif
    }
    auto positionsFromUV( int u, int v ) const -> vec2f;

    // Extents in `world-uv` space
    auto u_min() const -> float { return (static_cast<float>(blockPosition.u()) - 0.5f) * terrain.uPerBlock(); }
    auto v_min() const -> float { return (static_cast<float>(blockPosition.v()) - 0.5f) * terrain.vPerBlock(); }
    auto u_max() const -> float { return u_min() + terrain.uPerBlock(); }
    auto v_max() const -> float { return v_min() + terrain.vPerBlock(); }

    // TODO - add element-wise vec2 multiplication and addition
    //Extents in `block-uv` space
    auto blockSpaceMinU() const -> int { return blockPosition.u() * terrain.blockResolution.u() - (terrain.blockResolution.u() / 2); }
    auto blockSpaceMinV() const -> int { return blockPosition.v() * terrain.blockResolution.v() - (terrain.blockResolution.v() / 2); }

    auto lodRatio() const -> int { return 1; } // TODO
    auto lodU( vector* verts, int u, int v, int lodStep ) const -> vector;
    auto lodV( vector* verts, int u, int v, int lodStep ) const -> vector;


    private:
      auto generatePoints( vector* verts ) const -> vector*;
      auto generateNormals( vector* verts, vector* normals ) const -> vector*;
      void lodVectors( vector* vectors ) const;
      auto generateRenderable( vector* verts, vector* normals ) const -> Future<GLRenderable>;

      auto textureUV(vector& position, float v_pos) const -> vector;
  };

  struct Block {
    BlockSpec spec;
    GLRenderable renderable;
    const body& collision;

    Block( BlockSpec s, GLRenderable r, const body& b ) : spec(s), renderable(r), collision(b) {}

    auto draw( scene* s ) -> bool { return renderable.draw( s ); }
    ~Block() { delete &collision; } // TODO - how do we actually delete?
  };
}
