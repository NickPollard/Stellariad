// debuggraph.h
#pragma once

struct graph_s;
struct graphData;
typedef struct graphData_s graphData;
typedef struct graph_s graph;

struct graph_s {
	int x;
	int y;
	int width;
	int height;
	float x_scale;
	float y_scale;
	graphData* data;
};

#define DEF_PAIR(X_TYPE, Y_TYPE) typedef struct { X_TYPE _1; Y_TYPE _2; } PAIR_##X_TYPE##_##Y_TYPE;
#define PAIR(X_TYPE, Y_TYPE) PAIR_##X_TYPE##_##Y_TYPE

DEF_PAIR(float, float)

struct graphData_s {
	int	count;
	int end;
	PAIR(float, float)* data;
};

void graphData_append( graphData* g, float x, float y );
graphData* graphData_new( int count );
graph* graph_new( graphData* g_data, int x, int y, int width, int height, float x_scale, float y_scale );
graphData* graphData_new( int count );
void graph_render( graph* g );
