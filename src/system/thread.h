// thread.h
#pragma once
#include <pthread.h>

// Use pthread by default for internal threading
typedef pthread_t vthread;
typedef pthread_mutex_t vmutex;
typedef pthread_cond_t	vcondition;
#define kMutexInitialiser PTHREAD_MUTEX_INITIALIZER;

enum conditions {
	start_render,
	finished_render,
	work_exists,
	kMaxConditions
};

// Static Initializer
void vthread_init();

//
// *** Threads
//

// Thread functions take a void* (Args) and return a void*
typedef void* (*vthreadfunc)( void* );

// Kick off a thread
vthread vthread_create( vthreadfunc func, void* args );

// Stop running this thread and add it to the ready-queue, allowing another thread to begin
// executing
void vthread_yield();

//
// *** Mutices
//

// Lock a Mutex, preventing other threads from accessing it
void vmutex_lock( vmutex* mutex );
// Relinquish the lock on a Mutex, allowing other threads to access it
void vmutex_unlock( vmutex* mutex );

//
// *** Conditions
//

void vthread_signalCondition( int i );
void vthread_broadcastCondition( int i );
void vthread_waitCondition( int i );
