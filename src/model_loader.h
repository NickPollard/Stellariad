// model_loader.h

#pragma once

#define kObjMaxVertices 4096
#define kObjMaxIndices 8192

mesh* mesh_loadObj( const char* filename );
model* model_load( const char* filename );
