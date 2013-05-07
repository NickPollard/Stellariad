// debuggraph.c
#include "common.h"
#include "debuggraph.h"
//-----------------------
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/debugdraw.h"

void graph_render( graph* g ) {
	graphData* d = g->data;
	float x = g->x;
	float y = g->y;
	float inv_x = 1.f / g->x_scale;
	float inv_y = 1.f / g->y_scale;
	float x_min = d->data[(d->end)%d->count]._1;
	float w = g->width;
	float h = g->height;
	for ( int i = 0; i < d->count-1; ++i ) {
		int j = ( i + d->end ) % d->count;
		int k = ( i + d->end + 1 ) % d->count;
		vector from =	Vector( (d->data[j]._1 - x_min) * inv_x * w + x, d->data[j]._2 * inv_y * h + y, 0.f, 1.f );
		vector_printf( "from: ", &from );
		vector to =		Vector( (d->data[k]._1 - x_min) * inv_x * w + x, d->data[k]._2 * inv_y * h + y, 0.f, 1.f );
		debugdraw_line2d( from, to, color_green );
	}
	vector tl = Vector( x, y, 0.f, 1.f );
	vector tr = Vector( x+w, y, 0.f, 1.f );
	vector bl = Vector( x, y+h, 0.f, 1.f );
	vector br = Vector( x+w, y+h, 0.f, 1.f );
	debugdraw_line2d( tl, tr, color_red );
	debugdraw_line2d( tr, br, color_red );
	debugdraw_line2d( br, bl, color_red );
	debugdraw_line2d( bl, tl, color_red );
}

graph* graph_new( graphData* g_data, int x, int y, int width, int height, float x_scale, float y_scale ) {
	graph* g = mem_alloc( sizeof( graph ));
	memset( g, 0, sizeof( graph ));
	g->data = g_data;
	g->x = x;
	g->y = y;
	g->width = width;
	g->height = height;
	g->x_scale = x_scale;
	g->y_scale = y_scale;
	return g;
}

graphData* graphData_new( int count ) {
	graphData* g = mem_alloc( sizeof( graphData ));
	g->data = mem_alloc( sizeof( PAIR(float,float) ) * 2 * count );
	g->count = count;
	for ( int i = 0; i < count; ++i ) {
		g->data[i]._1 = 0.f;
		g->data[i]._2 = 0.f;
	}
	g->end = 0;
	return g;
}

void graphData_append( graphData* g, float x, float y ) {
	g->data[g->end]._1 = x;
	g->data[g->end]._2 = y;
	g->end = ( g->end + 1 ) % g->count; 
}

#ifdef TEST
void graph_test() {
	graphData* d = graphData_new( 10 );
	graph* g = graph_new( g, 0, 0, 320, 240, 1.f, 1.f );

	mem_free( g );
	mem_free( d );
}
#endif //TEST
