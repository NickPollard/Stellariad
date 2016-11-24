// buildCacheTask.h
#pragma once
#include "concurrent/task.h"

using brando::concurrent::Executor;

// *** Worker Task ***
Msg generateVertices( CanyonTerrainBlock* b, Executor& ex );
