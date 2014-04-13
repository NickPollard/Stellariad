// thread.c

#include "common.h"
#include "thread.h"
//-------------------------
#include <sched.h> // for sched_yield

//#define DEBUG_THREAD_CONDITIONS

vcondition	conditions[kMaxConditions];
vmutex		condition_mutices[kMaxConditions];
bool		condition_values[kMaxConditions];

// Static Initializer
void vthread_init() {
	// *** Initialize Conditions
	for ( int i = 0; i < kMaxConditions; i++ ) {
		pthread_cond_init( &conditions[i], NULL );
		pthread_mutex_init( &condition_mutices[i], NULL );
		condition_values[i] = false;
	}
}

// *** Threads

// Kick off a thread
vthread vthread_create( vthreadfunc func, void* args ) {
	vthread t;
	pthread_create( &t,  /*attributes*/ NULL, func, args );
	return t;
}

void vthread_yield() {
	sched_yield();
}

// *** Mutices

// Lock a Mutex, preventing other threads from accessing it
void vmutex_lock( vmutex* mutex ) {
	int error = pthread_mutex_lock( mutex );
	vAssert( !error );
}

// Relinquish the lock on a Mutex, allowing other threads to access it
void vmutex_unlock( vmutex* mutex ) {
	int error = pthread_mutex_unlock( mutex );
	vAssert( !error );
}

// Init a non-static mutex
void vmutex_init( vmutex* mutex ) {
	pthread_mutex_init ( mutex, NULL);
}

// *** Conditions

void vthread_signalCondition( int i ) {
#ifdef DEBUG_THREAD_CONDITIONS
	printf( "THREAD: Setting condition %d.\n", i );
#endif
	vmutex*		condition_mutex	= &condition_mutices[i];
	vcondition*	condition		= &conditions[i];

	vmutex_lock( condition_mutex );
	condition_values[i] = true;
	pthread_cond_signal( condition );
	vmutex_unlock( condition_mutex );
}

void vthread_broadcastCondition( int i ) {
#ifdef DEBUG_THREAD_CONDITIONS
	printf( "THREAD: Setting condition %d.\n", i );
#endif
	vmutex*		condition_mutex	= &condition_mutices[i];
	vcondition*	condition		= &conditions[i];

	vmutex_lock( condition_mutex );
	condition_values[i] = true;
	pthread_cond_broadcast( condition );
	vmutex_unlock( condition_mutex );
}

void vthread_waitCondition( int i ) {
#ifdef DEBUG_THREAD_CONDITIONS
	printf( "THREAD: Waiting for condition %d.\n", i );
#endif
	vmutex*		condition_mutex	= &condition_mutices[i];
	vcondition*	condition		= &conditions[i];

	vmutex_lock( condition_mutex );
	while ( condition_values[i] == false ) {
		pthread_cond_wait( condition, condition_mutex );
	}
#ifdef DEBUG_THREAD_CONDITIONS
	printf( "THREAD: Received condition %d.\n", i );
#endif
	condition_values[i] = false;
	vmutex_unlock( condition_mutex );
}
