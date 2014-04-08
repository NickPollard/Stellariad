// Actor.h
#pragma once
#define MaxActors 1024

struct actorSystem_s {
	ActorRef actors[MaxActors];
	int count;
	int last;
};

actorSystem* actorSystemCreate();

// Send a message to an Actor
void actorMsg( ActorRef a, Msg m );

// Spawn a new actor on this system
ActorRef spawnActor( actorSystem* system );
