// time.c
#include "common.h"
#include "vtime.h"
//-----------------------
#include "mem/allocator.h"

void rand_init() {
	time_v t;
	gettimeofday(&t, NULL);
	srand( t.tv_usec );
}

// Return a random float between floor and ceiling
float frand( float floor, float ceiling ) {
	assert( ceiling > floor );
	float f = (float)rand() / (float)RAND_MAX;
	assert( f <= 1.f );
	assert( f >= 0.f );
	return ( f * ( ceiling - floor ) + floor );
}

// Return a random float between floor and ceiling
float deterministic_frand( randSeq* r, float floor, float ceiling ) {
	assert( ceiling > floor );
	double random;
	random = erand48( r->buffer );
	assert( random <= 1.f );
	assert( random >= 0.f );
	return ( random * ( ceiling - floor ) + floor );
}

void deterministic_seedRandSeq( long int seed, randSeq* r ) {
//		srand48_r( seed, &r->buffer );
	memcpy( &r->buffer[0], &seed, sizeof( randSeq ));
}

void timer_init(frame_timer* timer) {
	time_v t;
	gettimeofday(&t, NULL);
	timer->old_time = t.tv_sec * SecToUSec + t.tv_usec;
	timer->game_start = timer->old_time;
	timer->fps = 0.f;
}

float timer_getDelta(frame_timer* timer) {
	time_v t;
	gettimeofday(&t, NULL);

	unsigned long long newTime = t.tv_sec * SecToUSec + t.tv_usec;
	float delta = (float)(newTime - timer->old_time) * uSecToSec;
	timer->old_time = newTime;

	float fps = 1.f/delta;
	timer->fps = timer->fps * 0.9f + fps * 0.1f;
//	printf("fps: %.2f\n", timer->fps);

	return delta;
}

// Get the time in seconds
// DEPRECATED
float timer_getTimeSeconds() {
	time_v t;
	gettimeofday(&t, NULL);
	float old_time = t.tv_sec * SecToUSec + t.tv_usec;
	return old_time * uSecToSec;
}

// Get the time in seconds
float timer_getGameTimeSeconds(frame_timer* t) {
	return ((float)( t->old_time - t->game_start )) * uSecToSec;
}

frame_timer* vtimer_create() {
	frame_timer* t = (frame_timer*)mem_alloc(sizeof(frame_timer));
	timer_init(t);
	return t;
}
