// modelinstance.h

#pragma once

/*
	ModelInstance

	A ModelInstance represents a unique instance of a model to be rendered; multiple
	modelInstances can have the same model (and therefore appearance), but can be
	in different positions/situations
   */

struct modelInstance_s {
	model*	model;
	transform* trans;
};

modelInstance* modelInstance_create( model* m );

void modelInstance_draw( modelInstance* instance );
