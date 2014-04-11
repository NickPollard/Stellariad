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
