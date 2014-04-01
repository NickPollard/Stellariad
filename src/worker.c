// worker.c
#include "common.h"
#include "worker.h"
//-----------------------
#include "mem/allocator.h"
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
		//printf("Adding worker task\n");
		worker_tasks[worker_task_count++] = t;
		vthread_broadcastCondition( work_exists );
	}
	vmutex_unlock( &worker_task_mutex );
}

worker_task worker_nextTask() {
	worker_task task = { NULL, NULL, NULL };
	vmutex_lock( &worker_task_mutex );
	{
		if ( worker_task_count > 0 ) {
			task = worker_tasks[0];
			// Move worker tasks down
			for ( int i = 0; i < worker_task_count + 1; ++i ) {
				worker_tasks[i] = worker_tasks[i + 1];
			}
			--worker_task_count;
			//printf("Done worker task\n");
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
		//printf("Adding immediate worker task\n");
		worker_immediate_tasks[worker_immediate_task_count++] = t;
		vthread_broadcastCondition( work_exists );
	}
	vmutex_unlock( &worker_task_mutex );
}
worker_task worker_nextImmediateTask() {
	worker_task task = { NULL, NULL, NULL };
	vmutex_lock( &worker_task_mutex );
	{
		if ( worker_immediate_task_count > 0 ) {
			task = worker_immediate_tasks[0];
			// Move worker tasks down
			for ( int i = 0; i < worker_immediate_task_count + 1; ++i ) {
				worker_immediate_tasks[i] = worker_immediate_tasks[i + 1];
			}
			--worker_immediate_task_count;
			//printf("Done immediate worker task\n");
		}
	}
	vmutex_unlock( &worker_task_mutex );
	
	return task;
}

void* worker_threadFunc( void* args ) {
	(void)args;
	while ( true ) {
		if ( worker_immediate_task_count > 0 ) {
			// Grab the first task
			worker_task task = worker_nextImmediateTask();
			if ( task.func )
				task.func( task.args );
			if ( task.onComplete )
				worker_addImmediateTask( *task.onComplete );
		}
		else if ( worker_task_count > 0 ) {
			// Grab the first task
			worker_task task = worker_nextTask();
			if ( task.func )
				task.func( task.args );
			if ( task.onComplete )
				worker_addTask( *task.onComplete );
		}

		bool workWaiting = false;
		vmutex_lock( &worker_task_mutex );
		{
			workWaiting = (worker_immediate_task_count == 0 && worker_task_count == 0 );
		}
		vmutex_unlock( &worker_task_mutex );
		if ( workWaiting ) {
			vthread_waitCondition( work_exists );
		}
	}
}

worker_task onComplete( worker_task first, worker_task andThen ) {
	first.onComplete = mem_alloc( sizeof( worker_task ));
	memcpy( first.onComplete, &andThen, sizeof( worker_task )); // TODO - memory implications!
	return first;
}

worker_task task( taskFunc func, void* args ) {
	worker_task w;
	w.func = func;
	w.args = args;
	w.onComplete = NULL;
	return w;
}
