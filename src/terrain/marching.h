// marching.h

template <typename T> struct Range {
	T min;
	T max;
	T increment;

	Range(T _min, T _max, T _inc) : min(_min), max(_max), increment(_inc) {}
};

template <typename T> 
Range<T> BuildRange(T _min, T _max);

template <> 
Range<Int> BuildRange(Int _min, Int _max) {
	return Range<Int>(_min, _max, 1);
}

template <typename T> struct RangeStart {
	T min;
	T increment;

	RangeStart(T _min, T _inc) : min(_min), increment(_inc) {}

	Range<T> to(T _max) {
		return BuildRange<T>(min, _max);
	}
};

template <typename T>
RangeStart<T> from(T _min) {
	return RangeStart<T>(_min);
};
