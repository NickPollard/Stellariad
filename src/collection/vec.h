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

	std::vector<A> underlying; /* backed by std::vector */

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
	   return bind(*this, f);
	};

	// Vector-like interface
	int size() const { return underlying.size(); }
	inline A& operator [](int i) { return underlying[i]; }
	inline const A& operator [](int i) const { return underlying[i]; }
	typename std::vector<A>::iterator begin() { return underlying.begin(); }
	typename std::vector<A>::iterator end() { return underlying.end(); }
	void clear() { underlying.clear(); }

	Vec<A> operator +( const A& that ) {
		Vec<A>& neo = this->copy();
		neo.underlying.push_back( that );
		return neo;
	}

	Vec<A> operator +( const Vec<A>& that ) {
		Vec<A>& neo = this->copy();
		for (const auto i : that.contents()) neo+= i;
		return neo;
	}

	void operator +=( const A& a ) { underlying.push_back(a); }
	void operator +=( const Vec<A>& that ) { for (const auto i : that.contents()) *this += i; }
};

template<typename A, typename B>
Vec<B> bind(Vec<A> v, std::function<Vec<B>(A)> f) {
	Vec<B> out = Vec<B>();
	for (const auto i : v.underlying) {
		Vec<B> vTemp = f(i);
		for( const auto j : vTemp.underlying)
			out.underlying.push_back(j);
	}
	return out;
}

template<typename A, typename B>
Vec<B> fmap(Vec<A> v, std::function<B(A)> f) {
	Vec<B> out = Vec<B>();
	for (const auto i : v.underlying)
		out.underlying.push_back(f(i));
	return out;
};

template<typename A> Vec<A> vec(const std::vector<A>& stdvec) { return Vec<A>(stdvec); };
