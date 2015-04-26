// marching.h
#pragma once
#include <tuple>
#include <functional>

using std::tuple;
using std::make_tuple;
using std::function;

// Lookup tables
extern int8_t triangles[256][16];
extern int8_t edges[12][2];

template<typename A, typename B>
function<B(A)> make_function(B *(f)(A)) {
	auto fn = function<B(A)>(f);
	return fn;
};

// Grid Typeclass - a 3d cube of radius R
template <typename T, int R> struct Grid {
	T values[R][R][R];

	Grid(T ts[R][R][R]) {
		memcpy( values, ts, sizeof(T) * R * R * R ); 
	}

	Grid() {
		memset( values, 0, sizeof(T) * R * R * R );
	}

	template<typename U>
	auto fproduct(function<U(T)> f) -> Grid<tuple<T, U>, R> {
		auto g = Grid<tuple<T,U>, R>();

		for ( int i = 0; i < R; ++i )
			for ( int j = 0; j < R; ++j )
				for ( int k = 0; k < R; ++k ) {
					T value = values[i][j][k];
					g.values[i][j][k] = make_tuple(value, f(value));
				}

		return g;
	}
};

// Test
void test_cube();
