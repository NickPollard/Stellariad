#include "common.h"
//---------------------------
#include "base/array.h"
#include "base/sequence.h"
#include "base/delegate.h"
#include "mem/allocator.h"

// Create a main() for the catch test library
#define CATCH_CONFIG_MAIN
#include "ext/catch.hpp"

#include "concurrent/task.h"

// Import the main brando implementations (need to be defined once)
#define BRANDO_MAIN
#include "brando.h"
#undef BRANDO_MAIN

using vitae::Array;
using vitae::Sequence;

TEST_CASE( "Arrays", "[Array]" ) {
  mem_init(0, nullptr);
  auto& arr = *(new Array<int>(10));
  for (int i = 0; i < 10; ++i) {
    arr.add(i);
  }
  int total = 0;
  for (auto& el : arr)
    total += el;
  REQUIRE( total == 45 );
}

TEST_CASE( "Sequences", "[Sequence]" ) {
  auto& seq = *(new Sequence<int>(10));
  for (int i = 0; i < 10; ++i)
    seq.add(i);
  int total = 0;
  for (auto& el : seq)
    total += el;
  //for (auto el = seq.begin(); el != seq.end(); ++el)
    //total += *el;
  REQUIRE( total == 45 );
}
