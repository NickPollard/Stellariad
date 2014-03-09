// terrain_generate.c
#include "common.h"
#include "terrain_generate.h"
//-----------------------
#include "canyon.h"

/*
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

vector terrainPoint( canyon* c, canyonTerrainBlock* b, int u_index, int v_index ) {
	float u, v, x, z;
	canyonTerrainBlock_positionsFromUV( b, u_index, v_index, &u, &v );
	terrain_worldSpaceFromCanyon( c, u, v, &x, &z );
	return Vector( x, canyonTerrain_sampleUV( u, v ), z, 1.f );
}

void canyonTerrainBlock_generateVerts( canyon* c, canyonTerrainBlock* b, vector* verts ) {
	for ( int v = -1; v < b->v_samples + 1; ++v ) {
		for ( int u = -1; u < b->u_samples + 1; ++u ) {
			verts[indexFromUV(b, u, v)] = terrainPoint( c, b, u, v );
		}
	}
}

vector pointForUV( vertPositions* p, int u, int v ) {
	// if it's in p, return the cached version
	if (u >= p->uMin && u < p->uMin + p->uCount && v >= p->vMin && v < p->vMin + p->vCount ) {
		int u_ = u - p->uMin;
		int v_ = v - p->vMin;
		return p->positions[u_ + v_ * p->uCount];
	} else { // re-calculate
		printf( "recalcing point.\n" );
		//vAssert( 0 );
		return Vector(0.f, 0.f, 0.f, 1.f);
	}
}

/*
   Called Sychronously by the main terrain thread; Builds a vertPositions to be passed
   to a child worker to actually construct the terrain.
   This should normally just pull from cache - if blocks aren't there, build them
   */
vertPositions* generatePositions( canyonTerrainBlock* b) {
	vertPositions* vertSources = mem_alloc( sizeof( vertPositions ));
	vertSources->uMin = -1;
	vertSources->vMin = -1;
	vertSources->uCount = b->u_samples + 2;
	vertSources->vCount = b->v_samples + 2;
	vertSources->positions = mem_alloc( sizeof( vector ) * vertCount( b ));
	canyonTerrainBlock_generateVerts( b->canyon, b, vertSources->positions );
	return vertSources;
}

void vertPositions_delete( vertPositions* vs ) {
	mem_free( vs->positions );
	mem_free( vs );
}

// Hopefully this should just be hitting the cache we were given
void generatePoints( canyonTerrainBlock* b, vertPositions* vertSources, vector* verts ) {
	for ( int v = -1; v < b->v_samples +1; ++v ) {
		for ( int u = -1; u < b->u_samples +1; ++u ) {
			verts[indexFromUV(b, u, v)] = pointForUV(vertSources,u,v);
		}
	}
}

/*
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

// When given an array of vert positions, use them to build a renderable terrainBlock
// (This will be sent from the canyon that has already calculated positions)
void terrainBlock_build( canyonTerrainBlock* b, vertPositions* vertSources ) {
	vector* verts = stackArray( vector, vertCount( b ));
	vector* normals = stackArray( vector, vertCount( b ));

	// These could be parallelised ???
	generatePoints( b, vertSources, verts );
	lodVectors( b, verts );

	//generateNormals();
	canyonTerrainBlock_calculateNormals( b, vertCount( b ), verts, normals );
	lodVectors( b, normals );

	canyonTerrainBlock_generateVertices( b, verts, normals );
}
