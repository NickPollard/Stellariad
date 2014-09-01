// scene.h

/*
   A scene object represents a drawable scene of OpenGL models.

   It contains the scene heirarchy graph, specifying models, lights, transforms, etc.
   */
#pragma once

#include "common.fwd.h"
#include "engine.h"
#include "maths/maths.h"
#include "transform.h"

#define MAX_TRANSFORMS 1024
#define MAX_MODELS 2048
#define MAX_LIGHTS 512
#define MAX_EMITTERS 512

#define kSceneDebugTransforms	0x00000001
#define kSceneLightsTransforms	0x00000002

// *** Scene ***
struct scene_s {
	engine*		eng;

	int				model_count;
	modelInstance**	modelInstances;
	//
	int			light_count;
	light**		lights;
	//
	int			transform_count;
	transform**	transforms;
	//
	int					emitter_count;
	particleEmitter**	emitters;
	//
	int					ribbon_emitter_count;
	ribbonEmitter**	ribbon_emitters;
	//
	GLfloat		ambient[4];
	vector		fog_color;
	vector		sky_color;
	vector		sun_color;
	camera*		cam;
	dynamicFog*	fog;

	// Debug
	debugtextframe* debugtext;
	int			debug_flags;
} ;

typedef struct sceneData_s {
	size_t		size;
	//
	int			transform_count;
	transform*	transforms;
	//
	int				model_count;
	modelInstance*	modelInstances;
	//
	int			light_count;
	light*		lights;

	camera*		cam;
} sceneData;

// *** Static functions

void scene_initStatic( );

// *** Scene functions

// Make a Scene
scene* scene_create();

// Traverse the transform graph, updating worldspace transforms
void	scene_concatenateTransforms(scene* s);

// Process input for the scene
void	scene_input( scene* s, input* in );

// Update the scene
void	scene_tick(scene* s, float dt);

// *** Render Methods
void	scene_renderLighting(scene* s);
void	scene_render(scene* s);

//
// *** Accessors
//
void		scene_setAmbient( scene* s, float r, float g, float b, float a );
void		scene_setCamera( scene* s, float x, float y, float z, float w );
vector		scene_skyColor( scene* s, const vector* position );
void		scene_setSkyColor( scene* s, const vector* color );
vector		scene_fogColor( scene* s, const vector* position );
void		scene_setFogColor( scene* s, const vector* color );
vector		scene_sunColor( scene* s, const vector* position );
void		scene_setSunColor( scene* s, const vector* color );

// Transform Access
transform*	scene_transform( scene* s, int i );
void		scene_addTransform( scene* s, transform* t );
void		scene_removeTransform( scene* s, transform* e );
int			scene_transformIndex( scene* s, transform* t );

// ModelInstance Access
modelInstance* scene_model( scene* s, int i );
void		scene_addModel(scene* s, modelInstance* m);
void		scene_removeModel( scene* s, modelInstance* instance );

// Light Access
light*		scene_light( scene* s, int i );
void		scene_addLight( scene* s, light* l );

// Emitter Access
void scene_addEmitter( scene* s, particleEmitter* e );
void scene_addRibbonEmitter( scene* s, ribbonEmitter* e );

// Load & Save
sceneData* scene_save( scene* s );
scene* scene_load( sceneData* data );
void sceneData_free( sceneData* data );

// ### TEST #############################
// Initialise a scene with some test data
scene* test_scene_init( engine* e );
void test_scene_tick(scene* s, float dt);
