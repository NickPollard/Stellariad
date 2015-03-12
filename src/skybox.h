// skybox.h
#pragma once
#include "maths/maths.h"

#define SKYBOX_VERTEX_ATTRIBS( f ) \
	f( position ) \
	f( uv )

typedef struct skybox_vertex_s {
	vector	position;
	vector	uv;
} skybox_vertex;

struct SkyBox {
	void* data;
};

// Initialise static data for the skybox system
void skybox_init( );

// Render the skybox
void skybox_render( void* data, scene* s );

// Create a skybox
SkyBox* skyboxCreate();
