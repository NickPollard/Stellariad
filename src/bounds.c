// bounds.c
#include "common.h"
#include "bounds.h"
//-----------------------
#include "maths/maths.h"

bool boundsEqual( int a[2][2], int b[2][2] ) {
	return ( a[0][0] == b[0][0] && a[0][1] == b[0][1] &&
			a[1][0] == b[1][0] && a[1][1] == b[1][1] );
}

bool boundsContains( int bounds[2][2], int coord[2] ) {
	return ( coord[0] >= bounds[0][0] &&
			coord[1] >= bounds[0][1] &&
			coord[0] <= bounds[1][0] &&
			coord[1] <= bounds[1][1] );
}

// Calculate the intersection of the two block bounds specified
void boundsIntersection( int intersection[2][2], int a[2][2], int b[2][2] ) {
	intersection[0][0] = max( a[0][0], b[0][0] );
	intersection[0][1] = max( a[0][1], b[0][1] );
	intersection[1][0] = min( a[1][0], b[1][0] );
	intersection[1][1] = min( a[1][1], b[1][1] );
}


