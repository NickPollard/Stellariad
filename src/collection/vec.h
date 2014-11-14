// vec.h
#pragma once
#include <functional>
#include <vector>

//template<typename A, typename B> typedef Vec<B> (*bindfunc)(A* a);
//template<typename A, typename B> typedef B (*func)(A* a);

template<typename A>
struct Vec {
	Vec() : underlying() {}
	Vec(const std::vector<A>& init) : underlying(init) {}
	// back by std::vector?
	std::vector<A> underlying;
//	range<A> contents() { return underlying.range(); }

//	template<typename B>
//	Vec<B> operator >>=( bindfunc<A,B>& f ) = flatmap(this, f)

	template<typename B>
	Vec<B> operator | ( std::function<A(B)> f ) {
	   return fmap(*this, f);
	};

	template<typename B>
	Vec<B> map ( std::function<B(A)> f ) {
	   return fmap(*this, f);
	};

	template<typename B>
	Vec<B> flatmap( std::function<Vec<B>(A)> f ) const {
	   return fflatmap(*this, f);
	};

	// Vector-like interface
	int size() const { return underlying.size(); }
	A& operator [](int i) { return underlying[i]; }
	const A& operator [](int i) const { return underlying[i]; }

	Vec<A> operator +( const Vec<A>& that ) {
		auto neo = Vec<A>();
		for (const auto i : this.contents())
			neo.underlying.push_back( i );
		for (const auto i : that.contents())
			neo.underlying.push_back( i );
		return neo;
	}

	void operator +=( const A& a ) { underlying.push_back(a); }
};

template<typename A, typename B>
Vec<B> fflatmap(Vec<A> v, std::function<Vec<B>(A)> f) {
	Vec<B> out = Vec<B>();
	for (const auto i : v.underlying) {
		Vec<B> vTemp = f(i);
		for( const auto j : vTemp.underlying) {
			out.underlying.push_back(j);
		}
		// TODO vTemp.delete?
	}
	return out;
}

template<typename A, typename B>
Vec<B> fmap(Vec<A> v, std::function<B(A)> f) {
	Vec<B> out = Vec<B>();
	for (const auto i : v.underlying) {
		out.underlying.push_back(f(i));
	}
	return out;
};

template<typename A>
Vec<A> vec(const std::vector<A>& stdvec) {
	return Vec<A>(stdvec);
};
