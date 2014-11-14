//pair.c
#include "common.h"
#include "pair.h"
//-----------------------
#include "mem/allocator.h"

struct pair_s {
	void* _1;
	void* _2;
};

struct triple_s {
	void* _1;
	void* _2;
	void* _3;
};

struct quad_s {
	void* _1;
	void* _2;
	void* _3;
	void* _4;
};

#ifndef ANDROID
#ifndef __cplusplus
_Static_assert(offsetof(struct pair_s, _1) == offsetof(struct triple_s, _1), "tuples not interchangeable");
_Static_assert(offsetof(struct pair_s, _2) == offsetof(struct triple_s, _2), "tuples not interchangeable");
_Static_assert(offsetof(struct triple_s, _1) == offsetof(struct quad_s, _1), "tuples not interchangeable");
_Static_assert(offsetof(struct triple_s, _2) == offsetof(struct quad_s, _2), "tuples not interchangeable");
_Static_assert(offsetof(struct triple_s, _3) == offsetof(struct quad_s, _3), "tuples not interchangeable");
#endif
#endif

pair* Pair( void* a, void* b ) {
	pair* p = (pair*)mem_alloc( sizeof( pair ));
	p->_1 = a;
	p->_2 = b;
	return p;
}

triple* Triple( void* a, void* b, void* c ) {
	triple* t = (triple*)mem_alloc( sizeof( triple ));
	t->_1 = a;
	t->_2 = b;
	t->_3 = c;
	return t;
}

quad* Quad( void* a, void* b, void* c, void* d ) {
	quad* q = (quad*)mem_alloc( sizeof( quad ));
	q->_1 = a;
	q->_2 = b;
	q->_3 = c;
	q->_4 = d;
	return q;
}

void* _1( void* p ) { return ((pair*)p)->_1; }
void* _2( void* p ) { return ((pair*)p)->_2; }
void* _3( void* p ) { return ((triple*)p)->_3; }
void* _4( void* p ) { return ((quad*)p)->_4; }


