//pair.c
#include "common.h"
#include "pair.h"
//-----------------------
#include "mem/allocator.h"

struct pair_s {
	void* _1;
	void* _2;
};

pair* Pair( void* a, void* b ) {
	pair* p = mem_alloc( sizeof( pair ));
	p->_1 = a;
	p->_2 = b;
	return p;
}

void* _1( pair* p ) { return p->_1; }
void* _2( pair* p ) { return p->_2; }


