// worker.h
#pragma once

struct worker_task_s {
	taskFunc func;
	void* args;
};

extern int worker_task_count;

void* worker_threadFunc( void* args );

void worker_addTask( worker_task t );
void worker_addImmediateTask( worker_task t );
