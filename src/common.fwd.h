#ifndef __COMMON_FWD_H__
#define __COMMON_FWD_H__

struct body_s;
struct camera_s;
struct canyon_s;
struct canyonTerrain_s;
struct canyonZone_s;
struct debugtextframe_s;
struct dynamicFog_s;
struct engine_s;
struct heapAllocator_s;
struct input_s;
struct light_s;
struct map_s;
struct model_s;
struct modelInstance_s;
struct mesh_s;
struct particleEmitter_s;
struct property_s;
struct ribbonEmitter_s;
struct quaternion_s;
struct scene_s;
struct shader_s;
struct texture_s;
struct transform_s;
struct vertex_s;
struct window_s;
struct xwindow_s;
union vector_u;

typedef struct body_s body;
typedef struct camera_s camera;
typedef struct canyon_s canyon;
typedef struct canyonTerrain_s canyonTerrain;
typedef struct canyonZone_s canyonZone;
typedef struct debugtextframe_s debugtextframe;
typedef struct dynamicFog_s dynamicFog;
typedef struct engine_s engine;
typedef struct heapAllocator_s heapAllocator;
typedef struct input_s input;
typedef struct light_s light;
typedef struct map_s map;
typedef struct model_s model;
typedef struct modelInstance_s modelInstance;
typedef struct mesh_s mesh;
typedef struct particleEmitter_s particleEmitter;
typedef struct property_s property;
typedef struct ribbonEmitter_s ribbonEmitter;
typedef struct quaternion_s quaternion;
typedef struct scene_s scene;
typedef struct shader_s shader;
typedef struct texture_s texture;
typedef struct transform_s transform;
typedef struct vertex_s vertex;
typedef struct window_s window;
typedef struct xwindow_s xwindow;

typedef union vector_u vector;
typedef union vector_u color;

typedef int modelHandle; // A Handle into the model array


#endif // __COMMON_FWD_H__
