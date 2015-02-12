// Actor.c
#include "common.h"
#include "actor.h"
//-----------------------
#include "engine.h"
#include "future.h"
#include "mem/allocator.h"
#include "worker.h"
#include "system/string.h"

#define MaxActorTasks 16

struct actor_s {
	vmutex mutex;
	Msg queue[MaxActorTasks];
	actorSystem* system;
	int pending;
	bool active;
};

// Forward Declarations //
void	actorPush( ActorRef a, Msg m );
Msg		actorNextTask( ActorRef a );
void*	runSystem( void* args );
void*	dummyTask( void* args );
// //

void actorSystem_add( actorSystem* system, ActorRef a ) {
	vmutex_lock( &system->mutex ); {
		vAssert( system->count + 1 < MaxActors );
		system->actors[system->count++] = a;
	} vmutex_unlock( &system->mutex );
}

ActorRef spawnActor( actorSystem* system ) {
	ActorRef a = (ActorRef)mem_alloc( sizeof( actor ));
	a->pending = 0;
	a->active = false;
	a->system = system;
	memset( a->queue, 0, sizeof( Msg ) * MaxActorTasks );
	vmutex_init( &a->mutex );
	actorSystem_add( system, a );
	return a;
}

void actorSystem_remove( actorSystem* system, ActorRef a ) {
	vmutex_lock( &system->mutex ); {
		array_remove( (void**)&system->actors, &system->count, a );
	} vmutex_unlock( &system->mutex );
}

void stopActor( ActorRef a ) {
	actorSystem_remove( a->system, a );
	mem_free( a );
}

void actorLock( ActorRef a ) { vmutex_lock( &a->mutex ); }
void actorUnlock( ActorRef a ) { vmutex_unlock( &a->mutex ); }

void tell( ActorRef a, Msg m ) {
	actorLock( a ); {
		actorPush( a, m );
	} actorUnlock( a );
	worker_addTask( task( runSystem, a->system ));
	//worker_addTask( task( dummyTask, a->system ));
}

void actorReceive( ActorRef a ) {
	vAssert( a );
	// Guard with active flag
	Msg msg;
	bool found = false;
	actorLock( a ); {
		if ( a->pending > 0 ) {
			a->active = true;
			msg = actorNextTask( a );
			found = true;
		}
	} actorUnlock( a );

	if ( found ) {
		msg.func(msg.args);
		a->active = false;
	}
	(void)msg;

	actorLock( a ); {
		a->active = false;
	} actorUnlock( a );
}

// Private //
void actorPush( ActorRef a, Msg m ) { a->queue[a->pending++] = m; }

Msg actorNextTask( ActorRef a ) {
	Msg w = a->queue[0];
	// move Down
	for ( int i = 0; i+1 < a->pending; ++i )
		a->queue[i] = a->queue[i+1];
	--a->pending;
	return w;
}

actorSystem* actorSystemCreate() {
	actorSystem* a = (actorSystem*)mem_alloc( sizeof(actorSystem));
	memset( a->actors, 0, sizeof( ActorRef ) * MaxActors );
	a->last = 0;
	a->count = 0;
	vmutex_init( &a->mutex );
	return a;
}

bool msgPending( ActorRef a ) {
	bool b = false;
	actorLock( a ); {
		b = !a->active && a->pending > 0;
	} actorUnlock( a );
	return b;
}

// Returns the next actor with pending messages
ActorRef systemNext( actorSystem* system ) {
	// lock System mutex
	ActorRef a = NULL;
	while ( !a ) {
		for ( int i = 0; i < system->count; ++i ) {
			int index = ( i + system->last ) % system->count;
			vAssert( index < system->count );
			if ( msgPending( system->actors[index] )) {
				system->last = index;
				a = system->actors[index];
				break;
			}
		}
	}
	// unlock system mutex
	return a;
}

//@worker
void* runSystem( void* system ) {
	// Find next available actor - has messages and not currently running
	ActorRef a = systemNext( (actorSystem*)system );
	actorReceive( a );
	return NULL;
}

void* dummyTask( void* system ) {
	return system;
}

void print(const char* c) {
	printf( "String: %s.\n", c );
}
void printDouble(double d) {
	printf( "Double: %.2f.\n", d );
}

struct blarg {
	int i;
};

void printBlarg(blarg b) {
	printf( "Blarg: %d.\n", b.i );
}

void actorTest() {
	function<void(const char*)> f = print;
	function<void(double)> g = printDouble;
	function<void(blarg)> h = printBlarg;
	
	Sum<const char*, double> test(2.6);
	test.foreach(f);
	test.foreach(g);
	
	Sum<double, const char*> test1(2.7);
	test1.foreach(f);
	test1.foreach(g);
	
	Sum<const char*, double> test2(string_createCopy("Hello world"));
	test2.foreach(f);
	test2.foreach(g);

	blarg b = { 12 };
	Sum<const char*, double, blarg> test3(b);
	test3.foreach(f);
	test3.foreach(g);
	test3.foreach(h);

	auto ph = partial(h);
	auto pf = partial(f);
	auto pg = partial(g);
	//pf.apply(test3);
//	pg.apply(test3);
//	ph.apply(test3);

	auto pgh = orElse(pg, ph);
	pgh.apply( test3 );
//	test.foreach(1);
//	test.foreach("h");
}
