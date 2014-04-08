// worker.h
#pragma once

struct worker_task_s {
	taskFunc func;
	void* args;
	struct worker_task_s* onComplete;
};

extern int worker_task_count;

void* worker_threadFunc( void* args );

void worker_addTask( worker_task t );
void worker_addImmediateTask( worker_task t );

worker_task onComplete( worker_task first, worker_task andThen );

// Create an actor task message
Msg task( taskFunc func, void* args );
