// stringops.h
#pragma once
#include <stdlib.h>
#include <string>

// ---- Shows ----
template<typename T> struct Show {
	static const char* toString(const T& t);
};

template<> struct Show<const char*> {
	static const char* toString(const char*& s) { return s; }
};

template<> struct Show<const char[]> {
	static const char* toString(const char s[]) { return s; }
};

struct StringOps {
	StringOps(const char* str) : contents(str) {}
	std::string contents;
	const char* toString() const { return contents.c_str(); }
	void print() { printf( "%s", toString()); }

	template<typename U>
	StringOps operator + (U& u) { return StringOps(contents.append(Show<U>::toString(u)).c_str()); };
	StringOps operator + (StringOps s) { return StringOps(contents.append(s.toString()).c_str()); }
};

template<> struct Show<StringOps> {
	static const char* toString(StringOps& s) { return s.toString(); }
};

template<typename T>
StringOps $$(T& t) {
	return StringOps(Show<T>::toString(t));
};

// Non-templated

StringOps $(const char s[]);

void print(const StringOps& s); 
void println(const StringOps& s);

StringOps operator +(const char* first, StringOps second);
