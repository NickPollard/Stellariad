// dynamicfog.h
#pragma once
#include "src/maths/maths.h"

#define kNumFogs 4

struct dynamicFog_s {
	scene* _scene;
	float time;
	int fog_count;
	vector fog_colors[kNumFogs];
	vector sky_colors[kNumFogs];
};

// Create a new default dynamicFog
dynamicFog* dynamicFog_create();

// the dynamicFog tick function implementation
void dynamicFog_tick( void* v, float dt, engine* eng );

// blend the sky and fog to the correct interpolated value
void dynamicFog_blend( dynamicFog* fog, int previous, int next, float blend );
