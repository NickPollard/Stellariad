// ribbon.h
#pragma once
#include "maths/vector.h"
#include "render/vgl.h"

#define kMaxRibbonPairs 32

struct ribbonEmitter_s { 
	transform*	trans;
	vector	begin;
	vector	end;
	vector	vertex_array[kMaxRibbonPairs][2];
	float	vertex_ages[kMaxRibbonPairs];
	int		render_pair_count;
	int		pair_count;
	int		pair_first;
	texture*	diffuse;
	property*	color;

	// Render
	vertex*		vertex_buffer;
	//GLushort*	element_buffer;
};

void ribbonEmitter_staticInit();

ribbonEmitter* ribbonEmitter_create();
void ribbonEmitter_tick( void* emitter, float dt, engine* eng );
void ribbonEmitter_render( void* emitter );

// Load a ribbon definition asset
ribbonEmitter* ribbon_loadAsset( const char* filename );
void ribbonEmitter_setColor( ribbonEmitter* r, property* color );
