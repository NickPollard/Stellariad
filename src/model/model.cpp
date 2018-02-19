#include "common.h"
#include "model.h"
//-----------------------

namespace vitae {
  /*
   * When loading a model, we iterate through children either meshes or transforms
   * Build up a vector of meshes, add them all together
   * Build up a vector of transforms, add them all together
   * Keep an index of current transform, add pairs of (emitter,index)
   * Then add those together
   */

  Model* Model::create( Slice<Mesh*>                _meshes,
                        Slice<transform*>           _transforms,
                        Slice<particleEmitterDef*>  _particles,
                        Slice<ribbonEmitterDef*>    _ribbons     ) :
    meshes( _meshes ),
    transforms( _transforms ) {
  }

}
