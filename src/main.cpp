/*
 Vitae

 A Game Engine by Nick Pollard
 (c) 2010
 */

#ifndef ANDROID

#include "common.h"
#include "collision.h"
#include "engine.h"
#include "input.h"
#include "maths/maths.h"
#include "particle.h"
#include "mem/allocator.h"
#include "render/modelinstance.h"
#include "system/file.h"
#include "system/hash.h"
#include "system/string.h"
#include "script/sexpr.h"
#include "terrain/marching.h"

void test_lisp();

// ###################################

#if UNIT_TEST
void runTests() {
	// Memory Tests
	test_allocator();

	test_hash();

	// System Tests
	test_sexpr();
	//test_lisp();

	test_maths();

	test_property();

	test_string();

	test_input();

	//test_collision();

	//test_cube();
}
#endif // UNIT_TEST

// ###################################

int main(int argc, char** argv) {
	printf("Loading Vitae.\n");

	test_allocator();
	init(argc, argv);

	// *** Initialise Engine
	engine* e = engine_create();
	engine_init( e, argc, argv );

#if UNIT_TEST
	runTests();
#endif

	engine_run( e );

	// Exit Gracefully
	return 0;
}
#endif // ANDROID
