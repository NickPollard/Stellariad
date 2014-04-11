// bounds.h
#pragma once

bool boundsEqual( int a[2][2], int b[2][2] );
bool boundsContains( int bounds[2][2], int coord[2] );
void boundsIntersection( int intersection[2][2], int a[2][2], int b[2][2] );
