// list.h
#pragma once

typedef struct list_s {
	void* head;
	void* tail;
} list;

#define DEF_LIST(type)											\
	typedef struct type##list_s {								\
		type* head;												\
		struct type##list_s* tail;								\
	} type##list;												\
	type##list* type##list_cons( type* h, type##list* t );		\
	void type##list_delete( type##list* lst );					\


#define IMPLEMENT_LIST(type)									\
	type##list* type##list_create() {							\
		type##list* list = mem_alloc( sizeof( type##list ));	\
		list->head = NULL;										\
		list->tail = NULL;										\
		return list;											\
	}															\
	type##list* type##list_cons( type* h, type##list* t ) {		\
		type##list* l = mem_alloc( sizeof( type##list ));		\
		l->head = h;											\
		l->tail = t;											\
		return l;												\
	}															\
	void type##list_delete( type##list* lst ) {					\
		type##list* l = lst;									\
		while ( l ) {											\
			type##list* tail = l->tail;							\
			mem_free( l );										\
			l = tail;											\
		};														\
	}															\
	int type##list_length( type##list* lst ) {					\
		type##list* l = lst;									\
		int length = 1;											\
		while (l->tail) {										\
			++length;											\
			l = l->tail;										\
		}														\
		return length;											\
	}

