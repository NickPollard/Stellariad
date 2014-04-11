// debugdraw.h
#ifndef __DEBUGDRAW__
#define __DEBUGDRAW__

#include "src/maths/maths.h"

void debugdraw_drawRect2D( vector* from, vector* to );

// *** Debug draw primitives
void debugdraw_line2d( vector from, vector to, vector color );
void debugdraw_line2d_gradient( vector from, vector to, vector color, vector color_to );
void debugdraw_line3d( vector from, vector to, vector color );
void debugdraw_cross( vector center, float radius, vector color );
void debugdraw_sphere( vector origin, float radius, vector color );
void debugdraw_wireframeMesh( int vert_count, vector* verts, int index_count, uint16_t* indices, matrix trans, vector color );

void debugdraw_preTick( float dt );

#endif // __DEBUGDRAW__
