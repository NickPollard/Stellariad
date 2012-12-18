// ribbon.h
#pragma once
#include "maths/vector.h"
#include "render/vgl.h"

#define kMaxRibbonPairs 32

struct ribbonEmitter_s { 
	// Definition properties
	float	radius;
	texture*	diffuse;
	property*	color;
	bool	billboard;
	float	lifetime;

	// Runtime properties
	transform*	trans;
	vector	begin;
	vector	end;
	vector	vertex_array[kMaxRibbonPairs][2];
	float	vertex_ages[kMaxRibbonPairs];
	int		pair_count;
	int		pair_first;

	// Render
	vertex*		vertex_buffer;
};

void ribbonEmitter_staticInit();

ribbonEmitter* ribbonEmitter_create();
ribbonEmitter* ribbonEmitter_copy( ribbonEmitter* src );
void ribbonEmitter_destroy( ribbonEmitter* r );

void ribbonEmitter_tick( void* emitter, float dt, engine* eng );
void ribbonEmitter_render( void* emitter );

// Load a ribbon definition asset
ribbonEmitter* ribbon_loadAsset( const char* filename );
void ribbonEmitter_setColor( ribbonEmitter* r, property* color );

