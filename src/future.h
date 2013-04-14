// future.h

typedef void* (handler*)(void*)

DEF_LIST(handler)

typedef struct future_s {
	void* value;
	bool complete;
	handler_list* on_complete;
} future;

