// sexpr.h
#pragma once

struct sexpr_s;
typedef struct sexpr_s sexpr;

struct sexpr_s {
	sexpr* child;
	sexpr* next;
	const char* value;
};
