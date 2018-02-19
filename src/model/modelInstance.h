#pragma once

namespace vitae {

  /*
   * ModelInstance
   */
  struct ModelInstance {
    public:
      void draw( camera* cam );
      void setStatic();

      ~ModelInstance();

      /*
       * Create a new modelInstance for the given model
       * The instance is allocated from efficient storage
       */
      static create( modelHandle m );

      /*
       * Delete the instance and free up its storage
       * You should call this on any modelInstance that is no longer needed
       */
      remove();

    private:
      ModelInstance( modelHandle m ); // Private constructor - use `create`
      void calculateBoundingBox();

      void createSubTransforms()
      void deleteSubTransforms()

      void createParticles();
      void deleteParticles();

      void createRibbons();
      void deleteRibbons();

      modelHandle _model;
      transform*  trans;
      aabb        bb;
      bool        isStatic;

      Slice<Transform*>        transforms;
      Slice<particleEmitter*>  particles;
      Slice<ribbonEmitter*>    ribbons;
  };

}
