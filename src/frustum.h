// frustum.h
#pragma once

#include "model.h"
#include "maths/vector.h"

typedef struct aabb_s {
	vector min;
	vector max;
} aabb;

void aabb_expand( aabb* bb, vector* points );
bool plane_cull( aabb* bb, vector* plane );
bool frustum_cull( aabb* bb, vector* frustum );
aabb aabb_calculate( obb bb, matrix m );
void debugdraw_aabb( aabb bb );
