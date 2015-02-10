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
template <typename T, typename U>
struct Sum {
	int type;
	void* value;
	void foreach( const U& u) { (void)u;/* if there is a T, do something */ };
	void foreach( const T& t) { (void)t;/* if there is a T, do something */ };
	template <typename V>
	void foreach( const V& v) { (void)v; value.foreach<V>(); };
};

/*
Sum<int, Sum<string, double>> mySum;
mySum.foreach<string>(print);
*/

