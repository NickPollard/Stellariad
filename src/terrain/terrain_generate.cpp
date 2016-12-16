// terrain_generate.c
#include "common.h"
#include "terrain_generate.h"
//-----------------------
#include "canyon.h"
#include "future.h"
#include "worker.h"
#include "base/pair.h"
#include "terrain_render.h"
#include "terrain_collision.h"
#include "terrain/cache.h"

int vertCount( CanyonTerrainBlock* b ) { return ( b->u_samples + 2 ) * ( b->v_samples + 2); }

// Adjusted as we have a 1-vert margin for normal calculation at edges
int indexFromUV( CanyonTerrainBlock* b, int u, int v ) { return u + 1 + ( v + 1 ) * ( b->u_samples + 2 ); }

// if it's in p, return the cached version
vector pointForUV( vertPositions* p, int u, int v ) {
	vAssert(u >= p->uMin && u < p->uMin + p->uCount && v >= p->vMin && v < p->vMin + p->vCount );
	return p->positions[u - p->uMin + (v - p->vMin) * p->uCount];
}

void vertPositions_delete( vertPositions* vs ) {
	mem_free( vs->positions );
	mem_free( vs );
}

// Hopefully this should just be hitting the cache we were given
void generatePoints( CanyonTerrainBlock* b, vertPositions* vertSources, vector* verts ) {
	for ( int vRelative = -1; vRelative < b->v_samples +1; ++vRelative )
		for ( int uRelative = -1; uRelative < b->u_samples +1; ++uRelative )
			verts[indexFromUV(b, uRelative, vRelative)] = pointForUV(vertSources,uRelative,vRelative);
}

vector lodV( CanyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio ) {
	const int previous = indexFromUV( b, u, v - ( v % lod_ratio ));
	const int next = indexFromUV( b, u, v - ( v % lod_ratio ) + lod_ratio );
	return vector_lerp( &verts[previous], &verts[next], (float)( v % lod_ratio ) / (float)lod_ratio );
}
vector lodU( CanyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio ) {
	const int previous = indexFromUV( b, u - ( u % lod_ratio ), v );
	const int next = indexFromUV( b, u - ( u % lod_ratio ) + lod_ratio, v );
	return vector_lerp( &verts[previous], &verts[next], (float)( v % lod_ratio ) / (float)lod_ratio );
}

void lodVectors( CanyonTerrainBlock* b, vector* vectors ) {
	const int lod_ratio = b->lodRatio();
	for ( int v = 0; v < b->v_samples; ++v )
		if ( v % lod_ratio != 0 ) {
			vectors[indexFromUV( b, 0, v)] = lodV( b, vectors, 0, v, lod_ratio );
			vectors[indexFromUV( b, b->u_samples-1, v)] = lodV( b, vectors, b->u_samples-1, v, lod_ratio );
		}
	for ( int u = 0; u < b->u_samples; ++u )
		if ( u % lod_ratio != 0 ) {
			vectors[indexFromUV( b, u, 0)] = lodU( b, vectors, u, 0, lod_ratio );
			vectors[indexFromUV( b, u, b->v_samples-1)] = lodU( b, vectors, u, b->v_samples-1, lod_ratio );
		}
}

// Generate Normals
void generateNormals( CanyonTerrainBlock* block, int vert_count, vector* verts, vector* normals ) {
	const int lod_ratio = block->lodRatio();
	for ( int v = 0; v < block->v_samples; ++v ) {
		for ( int u = 0; u < block->u_samples; ++u ) {
			const vector left = verts[indexFromUV( block, u, (v == block->v_samples - 1) ? v - lod_ratio : v - 1 )];
			const vector right = verts[indexFromUV( block, u, (v == 0) ? v + lod_ratio : v + 1)];
			const vector top = verts[indexFromUV( block, ( u == block->u_samples - 1 ) ? u - lod_ratio : u - 1, v )];
			const vector bottom = verts[indexFromUV( block, (u == 0) ? u + lod_ratio : u + 1, v )];
			
			int i = indexFromUV( block, u, v );

			vector a = vector_sub(bottom, top);
			vector b = vector_sub(right, left);
			vector n = normalized(vector_cross(a, b));
			vAssert( i < vert_count );
			normals[i] = n;

			/*
			vector a, b, c, x, y;
			// Calculate vertical vector
			// Take cross product to calculate normals
			Sub( &a, &bottom, &top );
			Cross( &x, &a, &y_axis );
			Cross( &b, &x, &a );

			// Calculate horizontal vector
			// Take cross product to calculate normals
			Sub( &a, &right, &left );
			Cross( &y, &a, &y_axis );
			Cross( &c, &y, &a );

			Normalize( &b, &b );
			Normalize( &c, &c );

			// Average normals
			vector total;
			Add( &total, &b, &c );
			total.coord.w = 0.f;
			Normalize( &total, &total );
			vAssert( i < vert_count );
			normals[i] = total;
			*/
		}
	}
}

// When given an array of vert positions, use them to build a renderable terrainBlock
// (This will be sent from the canyon that has already calculated positions)
void terrainBlock_build( CanyonTerrainBlock* b, vertPositions* vertSources ) {
	vector* verts = (vector*)stackArray( vector, vertCount( b ));
	vector* normals = (vector*)stackArray( vector, vertCount( b ));

	generatePoints( b, vertSources, verts );
	lodVectors( b, verts );

	generateNormals( b, vertCount( b ), verts, normals );
	lodVectors( b, normals );

	canyonTerrainBlock_generateVertices( b, verts, normals );
}

void canyonTerrainBlock_generate( vertPositions* vs, CanyonTerrainBlock* b ) {
	canyonTerrainBlock_createBuffers( b );

	terrainBlock_build( b, vs );
	terrainBlock_calculateCollision( b );
	terrainBlock_calculateAABB( b->renderable );

	terrainBlock_initVBO( b ).foreach( [=](bool data){ 
		(void)data;  
		b->terrain->setBlock( b->u, b->v, b ); 
	});
}
