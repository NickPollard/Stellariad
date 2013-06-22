// ribbon.h
#pragma once
#include "maths/vector.h"
#include "render/render.h"

#define kMaxRibbonPairs 32

struct ribbonEmitterDef_s {
	float	radius;
	texture*	diffuse;
	property*	color;
	bool	billboard;
	float	lifetime;
	vector	begin;
	vector	end;
	bool	static_texture;
};

struct ribbonEmitter_s { 
	ribbonEmitterDef* definition;

	// Runtime properties
	transform*	trans;
	vector	vertex_array[kMaxRibbonPairs][2];
	float	tex_v[kMaxRibbonPairs];
	float	vertex_ages[kMaxRibbonPairs];
	int		pair_count;
	int		pair_first;
	float	next_tex_v;

	// Render
	vertex		vertex_buffer[kMaxRibbonPairs * 2];
};

// *** Ribbon Module Static functions
void ribbonEmitter_staticInit();

// *** Emitter Instance
ribbonEmitter* ribbonEmitter_create( ribbonEmitterDef* def );
ribbonEmitter* ribbonEmitter_copy( ribbonEmitter* src );
void ribbonEmitter_destroy( ribbonEmitter* r );

// *** Definition
ribbonEmitterDef* ribbonEmitterDef_create();

void ribbonEmitter_tick( void* emitter, float dt, engine* eng );
void ribbonEmitter_render( void* emitter, scene* s );

// Load a ribbon definition asset
ribbonEmitterDef* ribbon_loadAsset( const char* filename );
void ribbonEmitterDef_setColor( ribbonEmitterDef* r, property* color );

