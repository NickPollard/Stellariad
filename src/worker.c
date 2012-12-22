// worker.c
/*
	Vitae Worker thread
   */
#include "common.h"
#include "worker.h"
//-----------------------
#include "system/thread.h"
#include <unistd.h>

#define kMaxWorkerTasks 512
int worker_task_count = 0;
int worker_immediate_task_count = 0;
vmutex worker_task_mutex = kMutexInitialiser;
worker_task worker_tasks[kMaxWorkerTasks];
worker_task worker_immediate_tasks[kMaxWorkerTasks];

// TODO worker task adding/removing should be lock free
void worker_addTask( worker_task t ) {
	vmutex_lock( &worker_task_mutex );
	{
		vAssert( worker_task_count < kMaxWorkerTasks );
		worker_tasks[worker_task_count++] = t;
	}
	vmutex_unlock( &worker_task_mutex );
}

worker_task worker_nextTask() {
	worker_task task = { NULL, NULL };
	vmutex_lock( &worker_task_mutex );
	{
		if ( worker_task_count > 0 ) {
			task = worker_tasks[0];
			// Move worker tasks down
			for ( int i = 0; i < worker_task_count + 1; ++i ) {
				worker_tasks[i] = worker_tasks[i + 1];
			}
			--worker_task_count;
		}
	}
	vmutex_unlock( &worker_task_mutex );
	
	return task;
}

// Immediate tasks
void worker_addImmediateTask( worker_task t ) {
	vmutex_lock( &worker_task_mutex );
	{
		vAssert( worker_immediate_task_count < kMaxWorkerTasks );
		worker_immediate_tasks[worker_immediate_task_count++] = t;
	}
	vmutex_unlock( &worker_task_mutex );
}
worker_task worker_nextImmediateTask() {
	worker_task task = { NULL, NULL };
	vmutex_lock( &worker_task_mutex );
	{
		if ( worker_immediate_task_count > 0 ) {
			task = worker_immediate_tasks[0];
			// Move worker tasks down
			for ( int i = 0; i < worker_immediate_task_count + 1; ++i ) {
				worker_immediate_tasks[i] = worker_immediate_tasks[i + 1];
			}
			--worker_immediate_task_count;
		}
	}
	vmutex_unlock( &worker_task_mutex );
	
	return task;
}

void* worker_threadFunc( void* args ) {
	(void)args;
	while ( true ) {
		if ( worker_immediate_task_count > 0 ) {
			//printf( "Worker performing immediate task.\n" );
			//printf( "Task count: %d\n", worker_immediate_task_count );
			// Grab the first task
			worker_task task = worker_nextImmediateTask();
			if ( task.func )
				task.func( task.args );
		}
		else if ( worker_task_count > 0 ) {
			//printf( "Worker performing task.\n" );
			//printf( "Task count: %d\n", worker_task_count );
			// Grab the first task
			worker_task task = worker_nextTask();
			if ( task.func )
				task.func( task.args );
		}

		//usleep( 5 );
		//vthread_yield();
	}
}
