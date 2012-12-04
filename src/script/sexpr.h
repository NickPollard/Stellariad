// sexpr.h
#pragma once

struct sexpr_s;
typedef struct sexpr_s sexpr;

enum sexprType {
	sexprTypeAtom,
	sexprTypeString,
	sexprTypeFloat
};

struct sexpr_s {
	sexpr* child;
	sexpr* next;
	enum sexprType type;
	const char* value;
};

void* sexpr_loadFile( const char* filename );

// Unit Test
void test_sexpr();
