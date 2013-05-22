// time.h
#ifndef __TIME_H__
#define __TIME_H__

#include <sys/time.h>

#define uSecToSec 0.000001
#define SecToUSec 1000000

typedef struct timeval time_v;

typedef struct {
	unsigned long long old_time;
	unsigned long long game_start;
	float fps;
} frame_timer;

typedef struct randSeq_s {
	unsigned short buffer[3];	// For storing random seed
} randSeq;

// Seed the RNG, using a time-based seed
void rand_init();

// Return a random float between floor and ceiling
float frand( float floor, float ceiling );

// Deterministic frand using a given seed
float deterministic_frand( randSeq* r, float floor, float ceiling );
void deterministic_seedRandSeq( long int seed, randSeq* r );

// Initialise the timer
void timer_init(frame_timer* timer);

// Get the time since the last frame
float timer_getDelta(frame_timer* timer);

// Get the time in seconds
float timer_getTimeSeconds();
float timer_getGameTimeSeconds(frame_timer* t);

// Create a new timer
frame_timer* vtimer_create();

#endif // __TIME_H__
