#pragma once

/*
   A Model contains many meshes, each of which use a given shader
   */
namespace vitae {
  struct Model {
    public:
      void draw();

      static Model*      create(  Slice<Mesh*>                 _meshes,
                                  Slice<transform*>            _transforms,
                                  Slice<particleEmitterDef*>   _particles,
                                  Slice<ribbonEmitterDef*>     _ribbons     );
      static modelHandle load( const char* filename );
      static void        preload( const char* filename );

    private:
	    obb boundingBox;

      Slice<Mesh*>               meshes;
      Slice<transform*>          transforms;
      Slice<pair<int, particleEmitterDef*>> particles;
      Slice<pair<int, ribbonEmitterDef*>>   ribbons;
  };
}
