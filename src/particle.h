// particle.h
#pragma once
#include "maths/maths.h"
#include "render/render.h"

#ifdef DEBUG
//#define DEBUG_PARTICLE_LIVENESS_TEST
#define DEBUG_PARTICLE_SOURCES
#endif

#define kMaxParticles 128
#define kMaxParticleVerts (kMaxParticles * 6)

#define kMaxPropertyValues 16

enum particleEmitter_flags {
	kParticleWorldSpace = 0x1,
	kParticleRandomRotation = 0x2,
	kParticleBurst = 0x4
};

typedef struct particle_s {
	float age;
	vector position;
	float rotation;
} particle;

struct property_s {
	int count;
	int stride;
	float* data;
};

typedef uint8_t particle_flags_t;

typedef struct particleEmitterDef_s {
	float	lifetime;
	vector	spawn_box;
	// Properties
	property* size;
	property* color;
	property* spawn_rate;
	vector	velocity;
	texture*	texture_diffuse;
	particle_flags_t	flags;
} particleEmitterDef;

struct particleEmitter_s {
	transform*	trans;
	particleEmitterDef*	definition;
	int		start;
	int		count;
	float	next_spawn;
	float	emitter_age;
	bool	dead;		// We've been explicitly killed; just cleaning up
	bool	dying;		// We've been asked to die gracefully once we're finished
	bool	oneshot;

	particle	particles[kMaxParticles];
	vertex		vertex_buffer[kMaxParticleVerts];

#ifdef DEBUG
	const char* debug_creator;
#endif // DEBUG
};

// *** System static init
void particle_init();

// *** EmitterDef functions
particleEmitterDef* particleEmitterDef_create();
void particleEmitterDef_deInit( particleEmitterDef* def );

// *** Emitter functions
particleEmitter* particleEmitter_create();
particleEmitter* particle_newEmitter( particleEmitterDef* definition );
void particleEmitter_render( void* data, scene* s );
void particleEmitter_tick( void* e, float dt, engine* eng );
void particleEmitter_destroy( particleEmitter* e );
void particleEmitter_delete( particleEmitter* e );

// *** Asset loading
particleEmitterDef* particle_loadAsset( const char* particle_file );

property* property_create( int stride );
void property_delete( property* p );
property* property_copy( property* p );
void property_addf( property* p, float time, float value );
void property_addfv( property* p, float time, float* values );
void property_addv( property* p, float time, vector value );
vector property_samplev( property* p, float time );
float property_samplef( property* p, float time );
float property_duration( property* p );

void particle_staticInit();

#ifdef DEBUG_PARTICLE_LIVENESS_TEST
void particleEmitter_assertActive( particleEmitter* e );
#endif // DEBUG_PARTICLE_LIVENESS_TEST

// *** Test
void test_property();
