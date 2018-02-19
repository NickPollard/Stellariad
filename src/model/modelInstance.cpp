#include "common.h"
#include "modelinstance.h"
//-----------------------
#include "camera.h"
#include "particle.h"
#include "ribbon.h"

namespace vitae {

  modelInstance* ModelInstance::create( modelHandle m ) {
  	modelInstance* instance = new ModelInstance(m); // TODO - pool allocate
    // TODO - implement this
  	return instance;
  }

  ModelInstance::ModelInstance() {
  	createSubTransforms();
  	createParticles();
   	createRibbons();
  }

  ModelInstance::~ModelInstance() {
	  deleteRibbons();
    deleteParticles();
	  deleteSubTransforms();
  }

  void ModelInstance::createSubTransforms() {
  	model* m = model();
  	transformCount = m->transform_count;
  	for ( int i = 0; i < transformCount; ++i )
  		transforms[i] = transform_create( m->transforms[i]->local );
  }

  void ModelInstance::deleteSubTransforms() {
  	for ( int i = 0; i < transformCount; ++i )
      transform_delete( transforms[i] );
  }

  ModelInstance::draw( camera* cam ) {
    // Bounding box cull
	  if (!isStatic) calculateBoundingBox();
	  if ( frustum_cull( &bb, cam->frustum ) ) return;
  
	  render_resetModelView();
	  matrix_mulInPlace( modelview, modelview, trans->world );
  
	  model_draw( model() );
  }

  void ModelInstance::calculateBoundingBox() {
  	bb = aabb_calculate( model()->_obb, instance->trans->world );
  }

  model* ModelInstance::model() {
    return model_getByHandle( _model );
  }
  
  void ModelInstance::createParticles() {
	  model* m = model();
	  emitterCount = m->emitter_count;

	  for ( int i = 0; i < emitterCount; ++i ) {
      transform* t = transforms[m->emitterIndices[i]];
      vAssert( t );
		  particleEmitterDef* def = m->emitterDefs[i];
		  emitters[i] = particleEmitter_create( def, t );
	  }
  }

  void ModelInstance::deleteParticles() { 
	  for ( int i = 0; i < instance->emitter_count; i++ )
		  particleEmitter_destroy( emitters[i] );
  }

  void ModelInstance::createRibbons() {
	  model* m = model();
	  ribbonCount = m->ribbon_emitter_count;

	  for ( int i = 0; i < ribbonCount; ++i ) {
      transform* t = transforms[m->ribbonIndices[i]];
		  ribbons[i] = ribbonEmitter_create( m->ribbon_emitters[i], t );
	  }
  }

  void modelInstance_deleteSubRibbonEmitters( modelInstance* instance ) {
	  for ( int i = 0; i < ribbonCount; ++i )
		  ribbonEmitter_destroy( ribbons[i]; );
  }
}
