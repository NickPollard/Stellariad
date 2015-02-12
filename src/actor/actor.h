// Actor.h
#pragma once
#define MaxActors 1024
#include "system/thread.h"

struct actorSystem_s {
	ActorRef actors[MaxActors];
	int count;
	int last;
	vmutex mutex;
};

actorSystem* actorSystemCreate();

// Send a message to an Actor
void tell( ActorRef a, Msg m );

// Spawn a new actor on this system
ActorRef spawnActor( actorSystem* system );

void stopActor( ActorRef actor );
/*
template <typename T>
struct ActorT {
};

template <typename T>
struct ActorRefT : public channel<T> {
	void tell(const T& t) { push(t); }
};

typename <typename T>
struct channel {
	void push(const T& t) = 0;
};

template <typename T>
struct Props {
	void instantiate();
	static Props<T> propsFor<ActorT<T>>();
};

template <typename T>
ActorRefT<T> actorOf(Props<T>& p) {
};
*/
#include <functional>
using std::function;

template <typename T, typename U>
struct SumCons {
	bool isLeft;
	const void* value;
	template<typename R> void foreach( function<R(T)> f) { if (isLeft) f(*(T*)value); };
	template<typename R, typename V> void foreach( function<R(V)> f) { if (!isLeft) ((U*)value)->foreach(f); };

	SumCons(const T& t) : isLeft( true ), value(&t) {}
	template<typename V> SumCons(const V& v) : isLeft( false ), value(new U(v)) {};
};

struct SumNil {
	template<typename R, typename V>
	void foreach( function<R(V)> f) { (void)f; };
};

template<typename... As> struct Sum {
};

template<typename T, typename... As> struct Sum<T, As...> {
	template<typename R, typename V> void foreach( function<R(V)> f) { sum.foreach(f); };
	SumCons<T, Sum<As...>> sum;
	
	template<typename V> Sum(const V& v) : sum(v) {};
};

template<>
struct Sum<> {
	template<typename R, typename V> void foreach( function<R(V)> f) { sum.foreach(f); };
	SumNil sum;
	
	template<typename V> Sum(const V& v) : sum(v) {};
};

template <typename T, typename U> 
struct PartialFunction {
	function<T(U)> f;
	template <typename... As> void apply( Sum<U, As...>& s ) { s.foreach(f); }
	template <typename V, typename... As> void apply( Sum<V, As...>& s ) { s.foreach(f); } // TODO - actually check types line up?

	PartialFunction( function<T(U)> f_ ) : f(f_) {};
};

template <typename T, typename U>
PartialFunction<T, U> partial( function<T(U)> f ) { return PartialFunction<T,U>(f); }

template <typename... As>
struct PartialFunctionT {
	function<void(Sum<As...>)> f;
	void apply( Sum<As...>& s ){ f(s); }
	template <typename... Vs>
	void apply( Sum<Vs...>& s ){ f(s); };

	PartialFunctionT( function<void(Sum<As...>)> f_ ) : f(f_) {}
};

template<typename T, typename U>
PartialFunctionT<T, U> orElse(PartialFunction<void, T>& f, PartialFunction<void, U>& g) {
	auto ff = [&](Sum<T, U> s) { f.apply(s); g.apply(s); };
	return PartialFunctionT<T,U>( ff );
};

void actorTest();
