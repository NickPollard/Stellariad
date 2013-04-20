// ringqueue.h
#pragma once

typedef struct ringQueue_s {
	int first;
	int last;
	int size;
	void** items;
} ringQueue;

ringQueue* ringQueue_create( int size );
int ringQueue_count( ringQueue* r );
void* ringQueue_pop( ringQueue* r );
void ringQueue_push( ringQueue* r, void* item );
