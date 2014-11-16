// Geometry.h
#pragma once

#include "maths/mathstypes.h"

typedef struct aabb2d_s {
	float x_min;
	float x_max;
	float z_min;
	float z_max;
} aabb2d;

void plane( vector a, vector b, vector c, vector* normal, float* d );

float segment_closestPoint( vector a, vector b, vector point, vector* closest );

vector eulerAngles( float yaw, float pitch, float roll );

vector normal2d( vector line );

bool overlap( aabb2d first, aabb2d second );

aabb2d Aabb2d( float xMin, float xMax, float zMin, float zMax );
