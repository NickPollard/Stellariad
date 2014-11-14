// stringops.cpp
#include "common.h"
#include "stringops.h"
//--------------------------------------------------------
#include <stdio.h>

StringOps operator +(const char* first, StringOps second) {
	return StringOps(first) + second;
}

void print(const StringOps& s) { printf( "%s", s.toString() ); }
void println(const StringOps& s) { printf( "%s\n", s.toString() ); }

StringOps $(const char s[]) { return StringOps(s); }
