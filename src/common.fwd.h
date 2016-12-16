#ifndef __COMMON_FWD_H__
#define __COMMON_FWD_H__

struct CanyonTerrainBlock;
struct CanyonTerrain;

#ifndef __cplusplus
struct actor_s;
struct actorSystem_s;
struct body_s;
struct cacheBlock_s;
struct camera_s;
struct canyon_s;
struct canyonData_s;
struct canyonZone_s;
struct debugtextframe_s;
struct dynamicFog_s;
struct future_s;
struct heapAllocator_s;
struct input_s;
struct light_s;
struct map_s;
struct model_s;
struct modelInstance_s;
struct mesh_s;
struct pair_s;
struct particleEmitter_s;
struct property_s;
struct ribbonEmitter_s;
struct ribbonEmitterDef_s;
struct quad_s;
struct quaternion_s;
struct scene_s;
struct shader_s;
struct terrainRenderable_s;
struct texture_s;
struct transform_s;
struct triple_s;
struct vertex_s;
struct vertPositions;
struct window_s;
struct window_buffer_s;
struct worker_task_s;
struct xwindow_s;
union vector_u;
#endif

typedef struct actor_s actor;
typedef struct actorSystem_s actorSystem;
typedef struct body_s body;
typedef struct cacheBlock_s cacheBlock;
typedef struct camera_s camera;
typedef struct canyon_s canyon;
typedef struct canyonData_s canyonData;
typedef struct canyonZone_s canyonZone;
typedef struct debugtextframe_s debugtextframe;
typedef struct dynamicFog_s dynamicFog;
struct engine;
typedef struct future_s future;
typedef struct heapAllocator_s heapAllocator;
typedef struct input_s input;
typedef struct light_s light;
typedef struct map_s map;
typedef struct model_s model;
typedef struct modelInstance_s modelInstance;
typedef struct mesh_s mesh;
typedef struct pair_s pair;
typedef struct particleEmitter_s particleEmitter;
typedef struct property_s property;
typedef struct ribbonEmitter_s ribbonEmitter;
typedef struct ribbonEmitterDef_s ribbonEmitterDef;
typedef struct quad_s quad;
typedef struct quaternion_s quaternion;
typedef struct scene_s scene;
typedef struct shader_s shader;
typedef struct texture_s texture;
struct TerrainCache;
typedef struct terrainRenderable_s terrainRenderable;
typedef struct transform_s transform;
typedef struct triple_s triple;
typedef struct vertex_s vertex;
typedef struct vertPositions_s vertPositions;
typedef struct window_s window;
typedef struct window_buffer_s window_buffer;
typedef struct worker_task_s worker_task;
typedef struct xwindow_s xwindow;

typedef union vector_u vector;
typedef union vector_u color;

typedef int modelHandle; // A Handle into the model array

typedef actor* ActorRef;
typedef worker_task Msg;

typedef void* (*taskFunc)( void* );

#endif // __COMMON_FWD_H__
