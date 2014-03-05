// terrain_generate.c
#include "common.h"
#include "terrain_generate.h"
//-----------------------
#include "canyon.h"

typedef struct vertPositions_s {
	int uMin;
	int uCount;
	int vMin;
	int vCount;
	vector* positions; // (uCount * vCount) in size
} vertPositions;

#define NYI vAssert(0);

/*
vector pointForUV( vertPositions* p, int u, int v ) {
	NYI;
	// if it's in p, return the cached version
	// else calculate
	return Vector(0.f, 0.f, 0.f, 1.f);
}

vector normalForUV( vertPositions* p, int u, int v ) {
	NYI;
	vector top,bottom,left,right;
	vector a, b, c, x, y;
	// Calculate vertical vector - Take cross product to calculate normals
	Sub( &a, &bottom, &top );
	Cross( &x, &a, &y_axis );
	Cross( &b, &x, &a );

	// Calculate horizontal vector - Take cross product to calculate normals
	Sub( &a, &right, &left );
	Cross( &y, &a, &y_axis );
	Cross( &c, &y, &a );

	// Make sure these can be RESTRICT
	Normalize( &b, &b );
	Normalize( &c, &c );

	vector total;
	Add( &total, &b, &c );
	total.coord.w = 0.f;
	Normalize( &total, &total );
	return total;
}
*/

// Hopefully this should just be hitting the cache we were given
/*
void generatePoints( c, b, verts ) {
	for ( int u = 0; u < b->u_max; ++u ) {
		for ( int v = 0; v < b->v_max; ++v ) {
			verts[u][v] = pointForUV(u,v);
		}
	}
}

void generateNormals() {
	for ( int u = 0; u < b->u_max; ++u ) {
		for ( int v = 0; v < b->v_max; ++v ) {
			normals[u][v] = normalForUV(u,v);
		}
	}
}
*/

vector lodV( canyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio ) {
	int previous = indexFromUV( b, u, v - ( v % lod_ratio ));
	int next = indexFromUV( b, u, v - ( v % lod_ratio ) + lod_ratio );
	return vector_lerp( &verts[previous], &verts[next], (float)( v % lod_ratio ) / (float)lod_ratio );
}
vector lodU( canyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio ) {
	int previous = indexFromUV( b, u - ( u % lod_ratio ), v );
	int next = indexFromUV( b, u - ( u % lod_ratio ) + lod_ratio, v );
	return vector_lerp( &verts[previous], &verts[next], (float)( v % lod_ratio ) / (float)lod_ratio );
}

void lodVectors( canyonTerrainBlock* b, vector* vectors) {
	int lod_ratio = lodRatio( b );
	for ( int v = 0; v < b->v_samples; ++v ) {
		if ( v % lod_ratio != 0 ) {
			vectors[indexFromUV( b, 0, v)] = lodV( b, vectors, 0, v, lod_ratio );
			vectors[indexFromUV( b, b->u_samples-1, v)] = lodV( b, vectors, b->u_samples-1, v, lod_ratio );
		}
	}
	for ( int u = 0; u < b->u_samples; ++u ) {
		if ( u % lod_ratio != 0 ) {
			vectors[indexFromUV( b, u, 0)] = lodU( b, vectors, u, 0, lod_ratio );
			vectors[indexFromUV( b, u, b->v_samples-1)] = lodU( b, vectors, u, b->v_samples-1, lod_ratio );
		}
	}
}

#define stackArray( type, size )	alloca( sizeof( type ) * size );

// When given an array of vert positions, use them to build a renderable terrainBlock
// (This will be sent from the canyon that has already calculated positions)
void terrainBlock_build( canyon* c, canyonTerrainBlock* b, vector* vs ) {
	(void)vs;
	vector* verts = stackArray( vector, vertCount( b ));
	vector* normals = stackArray( vector, vertCount( b ));

	// These could be parallelised ???
	//generatePoints(c, b, v);
	//generateNormals();

	canyonTerrainBlock_generateVerts( c, b, verts );
	lodVectors( b, verts );

	canyonTerrainBlock_calculateNormals( b, vertCount( b ), verts, normals );
	lodVectors( b, normals );

	canyonTerrainBlock_generateVertices( b, verts, normals );
}
