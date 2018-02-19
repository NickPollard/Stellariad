// modelinstance.h

#pragma once
#include "mem/pool.h"
#include "maths/maths.h"

#include "frustum.h"
#include "model.h"

/*
	ModelInstance

	A ModelInstance represents a unique instance of a model to be rendered; multiple
	modelInstances can have the same model (and therefore appearance), but can be
	in different positions/situations
   */

struct modelInstance_s {
	modelHandle	model;
	transform*  trans;
	aabb	bb;
	bool	isStatic;

	// Sub Elements
	int		            transform_count;
	int		            emitter_count;
	int		            ribbon_emitter_count;
	transform*			  transforms[kMaxSubTransforms];
	particleEmitter*	emitters[kMaxSubEmitters];
	ribbonEmitter*		ribbon_emitters[kMaxSubEmitters];
};

DECLARE_POOL( modelInstance )

void modelInstance_initPool();

modelInstance* modelInstance_create( modelHandle m );
void           modelInstance_delete( modelInstance* t );

void modelInstance_draw( modelInstance* instance, camera* cam );
void modelInstance_setStatic( modelInstance* m );

namespace vitae {

  /*
   * ModelInstance
   */
  struct ModelInstance {
    public:
      void draw( camera* cam );
      void setStatic();

      ~ModelInstance();

      static create( modelHandle m );

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

      SizedArray<Transform*>        transforms;
      SizedArray<particleEmitter*>  particles;
      SizedArray<ribbonEmitter*>    ribbons;
  };

}
