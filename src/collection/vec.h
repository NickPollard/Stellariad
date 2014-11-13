// vec.h

template<typename A, typename B>
typedef Vec<B> (*bindfunc)(A* a);

template<typename A>
class Vec {
	// back by std::vector?
	std::vector<A> underlying;
	range<A> contents() { return underlying.range(); }

	template<typename B>
	Vec<B> operator >>=( bindfunc<A,B>& f ) = flatmap(this, f)

	Vec<A> operator +( const Vec<A>& that ) {
		auto neo = Vec<A>();
		for (const auto i : this.contents())
			neo.underlying.push_back( i );
		for (const auto i : that.contents())
			neo.underlying.push_back( i );
		return neo;
	}
}

template<typename A, typename B>
Vec<B> flatmap(Vec<A> v, bindfunc<A,B> f) {
	v.map(f);
}

