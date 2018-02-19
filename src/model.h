// model.h
#pragma once

#include "common.fwd.h"
#include "maths/maths.h"
#include "render/render.h"

#define kMaxSubMeshes 4
#define kMaxHardPoints	16
#define kMaxSubTransforms	16
#define kMaxSubEmitters		16

// *** Mesh ***
/*
   A Mesh contains a list of vertices and a list of triangles (as indices to the vertex array)
   */
struct mesh_s {
	transform*	trans;
	shader**		_shader;

	int			  vertCount;
	vector*		verts;

	int			  indexCount;
	uint16_t*	indices;

	int			  normalCount;
	vector*		normals;

	uint16_t*	normalIndices;

	vector*		uvs;
	int			  uvCount;
	uint16_t*	uvIndices;

	vertex*			    vertex_buffer;
	unsigned short*	element_buffer;

	texture*  texture_diffuse;
	texture*  texture_normal;

	GLuint*		vertex_VBO;
	GLuint*		element_VBO;	

	drawCall*	draw;
	drawCall*	drawDepth;

	bool		dontCache; // prevent draw call caching for this mesh
};

/*
struct Mesh {
  using index = uint16_t;

  Transform* transform;

  Array<vector> verts;
  Array<index>  indices;
  Array<index>  normalIndices;
  Array<vector> uvs;

  Texture* diffuseMap;
  Texture* normalMap;

  bool     dontCache;
};

struct Model {
  Array<Mesh*>            meshes;
  Array<Transform*>       transforms;
  Array<ParticleEmitter*> particles;
  Array<RibbonEmitter*>   ribbons;

  obb boundingBox;

  IFDEBUG(const char* filename);
};

// Build a GLdraw struct for this mesh
GLDraw makeDraw( Mesh* m );
*/

typedef struct obb_s {
	vector min;
	vector max;
} obb;

// *** Model ***
/*
   A Model contains many meshes, each of which use a given shader
   */
struct model_s {
	int			meshCount;
	mesh*		meshes[kMaxSubMeshes];

	// Sub-elements
	int	              transform_count;
	int               emitter_count;
	int               ribbon_emitter_count;
	transform*			  transforms[kMaxSubTransforms];

	particleEmitterDef*	emitterDefs[kMaxSubEmitters];
  int                 emitterIndices;
	ribbonEmitterDef*		ribbon_emitters[kMaxSubEmitters];
  int                 ribbonIndices;

	obb					      _obb;

#ifdef DEBUG
	const char*		filename;
#endif
};


// *** Mesh Functions

// Create an empty mesh with vertCount distinct vertices and indexCount vertex indices
//mesh* mesh_createMesh( int vertCount, int indexCount, int normalCount, int uvCount );
mesh* mesh_createMesh( 
    int       vertCount, 
    int       indexCount, 
    int       normalCount, 
    int       uvCount, 
    vector*   vertices,
    uint16_t* indices,
    vector*   normals,
    uint16_t* normalIndices,
    vector*   uvs,
    uint16_t* uvIndices
  );

// Draw the verts of a mesh to the openGL buffer
void mesh_render( mesh* m );

// Build an oriented bounding box for the model
obb obb_calculate( int vertCount, vector* verts );




// *** Model Functions

// Create an empty model with meshCount submeshes
model* model_createModel(int meshCount);

// TODO - remove in favour of RAII
void model_addMesh( model* m, int i, mesh* msh );

// Add a transform to the model
// TODO tackle this too
int model_addTransform( model* m, transform* t );

void model_addParticleEmitter( model* m, particleEmitterDef* p, int index );
void model_addRibbonEmitter( model* m, ribbonEmitterDef* p, int index );

// Draw each submesh of a model
void model_draw(model* m);

// TODO - make a method on modelInstance
model* model_fromInstance( modelInstance* instance );

// Handle lookups
modelHandle model_getHandleFromFilename( const char* filename );
void        model_preload( const char* filename );

// Sub-element lookups
transform* model_transformIndex( model* m, transform* ptr );
transform* model_transformFromIndex( model* m, transform* index );
