#include "src/common.h"
#include "profile.h"
//--------------------------------------------------------
#include <math.h>
#include <stdio.h>

#if PROFILE_ENABLE
unsigned long long profileTimers[kMaxProfiles];
unsigned long long profileDurations[kMaxProfiles];

int frameCount = 0;

char profileStrings[kMaxProfiles][32] = BUILD_STRINGS(profiles)
#endif // PROFILE_ENABLE

#if PROFILE_ENABLE
unsigned long long rdtsc() {
  #define rdtsc(low, high) \
         __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

  unsigned long low, high;
  rdtsc(low, high);
  return ((unsigned long long)high << 32) | low;
	#undef rdtsc
}

void profileBegin(int index) {
	unsigned long long timer = rdtsc();
	profileTimers[index] = timer;
}

void profileEnd(int index) {
	unsigned long long timer = rdtsc();
	profileDurations[index] += (timer - profileTimers[index]);
}

void profile_init() {
	for (int i=0; i<kMaxProfiles; i++) {
		profileTimers[i] = 0;
		profileDurations[i] = 0;
	}	
}

void profile_dumpProfileTimes() {
	for (int i=0; i<kMaxProfiles; i++) {
		printf("%.1f%%	%llu	(%s)\n", 100.f * (float)profileDurations[i]/(float)profileDurations[0], profileDurations[i], profileStrings[i]);
	}

	unsigned long long frameTime = profileDurations[0]/frameCount;
	float fps = 1000000000.f / (float)frameTime;
	printf("AVG FRAMES PER SECOND: %.2f\n", fps);
}

void profile_newFrame() {
	frameCount++;
}

#endif // PROFILE_ENABLE
