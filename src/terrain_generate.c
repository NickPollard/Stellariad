// terrain_generate.c
#include "common.h"
#include "terrain_generate.h"
//-----------------------
#include "canyon.h"
#include "worker.h"
#include "base/pair.h"
#include "terrain_render.h"
#include "terrain_collision.h"
#include "terrain/cache.h"

int vertCount( canyonTerrainBlock* b ) { return ( b->u_samples + 2 ) * ( b->v_samples + 2); }

// Adjusted as we have a 1-vert margin for normal calculation at edges
int indexFromUV( canyonTerrainBlock* b, int u, int v ) { return u + 1 + ( v + 1 ) * ( b->u_samples + 2 ); }

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

/*
vector terrainPoint( canyon* c, canyonTerrainBlock* b, int uIndex, int vIndex ) {
	float u, v, x, z;
	const float r = 4 / lodRatio(b);
	terrain_positionsFromUV( b->terrain, r*uIndex + b->uMin, r*vIndex + b->vMin, &u, &v );
	terrain_worldSpaceFromCanyon( c, u, v, &x, &z );
	return Vector( x, canyonTerrain_sampleUV( u, v ), z, 1.f );
}
*/

vector terrainPointCached( canyon* c, canyonTerrainBlock* b, int uRelative, int vRelative ) {
	//printf( "getting cached point for block %d %d.\n", b->uMin, b->vMin );
	const int r = 4 / lodRatio(b);
	const int uReal = b->uMin + r*uRelative;
	const int vReal = b->vMin + r*vRelative;
	const int uOffset = uReal > 0 ? uReal % CacheBlockSize : (CacheBlockSize + (uReal % CacheBlockSize)) % CacheBlockSize;
	const int vOffset = vReal > 0 ? vReal % CacheBlockSize : (CacheBlockSize + (vReal % CacheBlockSize)) % CacheBlockSize;
	const int uMin = uReal - uOffset;
	const int vMin = vReal - vOffset;

	cacheBlock* cache = terrainCached( c->terrainCache, uMin, vMin );
	if (!cache || cache->lod > b->lod_level)
		cache = terrainCacheAdd( c->terrainCache, terrainCacheBlock( c, b->terrain, uMin, vMin, b->lod_level ));
	vector p = cache->positions[uOffset][vOffset];
	cacheBlockFree( cache );
	return p;
}

vector pointForUV( vertPositions* p, int u, int v ) {
	// if it's in p, return the cached version
	if (u >= p->uMin && u < p->uMin + p->uCount && v >= p->vMin && v < p->vMin + p->vCount ) {
		const int u_ = u - p->uMin;
		const int v_ = v - p->vMin;
		return p->positions[u_ + v_ * p->uCount];
	} else { // re-calculate
		printf( "recalcing point.\n" );
		vAssert( 0 );
		return Vector(0.f, 0.f, 0.f, 1.f);
	}
}

void vertPositions_delete( vertPositions* vs ) {
	mem_free( vs->positions );
	mem_free( vs );
}

// Hopefully this should just be hitting the cache we were given
void generatePoints( canyonTerrainBlock* b, vertPositions* vertSources, vector* verts ) {
	for ( int vRelative = -1; vRelative < b->v_samples +1; ++vRelative ) {
		for ( int uRelative = -1; uRelative < b->u_samples +1; ++uRelative ) {
			verts[indexFromUV(b, uRelative, vRelative)] = pointForUV(vertSources,uRelative,vRelative);
		}
	}
}

vector lodV( canyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio ) {
	const int previous = indexFromUV( b, u, v - ( v % lod_ratio ));
	const int next = indexFromUV( b, u, v - ( v % lod_ratio ) + lod_ratio );
	return vector_lerp( &verts[previous], &verts[next], (float)( v % lod_ratio ) / (float)lod_ratio );
}
vector lodU( canyonTerrainBlock* b, vector* verts, int u, int v, int lod_ratio ) {
	const int previous = indexFromUV( b, u - ( u % lod_ratio ), v );
	const int next = indexFromUV( b, u - ( u % lod_ratio ) + lod_ratio, v );
	return vector_lerp( &verts[previous], &verts[next], (float)( v % lod_ratio ) / (float)lod_ratio );
}

void lodVectors( canyonTerrainBlock* b, vector* vectors) {
	const int lod_ratio = lodRatio( b );
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

/*
void generateNormals() {
	for ( int u = 0; u < b->u_max; ++u ) {
		for ( int v = 0; v < b->v_max; ++v ) {
			normals[u][v] = normalForUV(u,v);
		}
	}
}
*/

// Generate Normals
void generateNormals( canyonTerrainBlock* block, int vert_count, vector* verts, vector* normals ) {
	const int lod_ratio = lodRatio( block );
	for ( int v = 0; v < block->v_samples; ++v ) {
		for ( int u = 0; u < block->u_samples; ++u ) {
			const int l = ( v == block->v_samples - 1 ) ? indexFromUV( block, u, v - lod_ratio ) : indexFromUV( block, u, v - 1 );
			const vector left	= verts[l];

			const int r = ( v == 0 ) ?  indexFromUV( block, u, v + lod_ratio ) : indexFromUV( block, u, v + 1 );
			const vector right = verts[r];

			const int t = ( u == block->u_samples - 1 ) ? indexFromUV( block, u - lod_ratio, v ) : indexFromUV( block, u - 1, v );
			vector top = verts[t];

			const int bt = ( u == 0 ) ? indexFromUV( block, u + lod_ratio, v ) : indexFromUV( block, u + 1, v );
			vector bottom = verts[bt];
			
			int i = indexFromUV( block, u, v );

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
	generateNormals( b, vertCount( b ), verts, normals );
	lodVectors( b, normals );

	canyonTerrainBlock_generateVertices( b, verts, normals );
}

void canyonTerrainBlock_generate( vertPositions* vs, canyonTerrainBlock* b ) {
	canyonTerrainBlock_createBuffers( b );

	terrainBlock_build( b, vs );
	terrainBlock_initVBO( b );
	terrainBlock_calculateCollision( b );
	terrainBlock_calculateAABB( b->renderable );
	b->pending = false;

	terrain_setBlock( b->terrain, b->u, b->v, b );
}

void* canyonTerrain_workerGenerateBlock( void* args ) {
	canyonTerrainBlock* b = _2(args);
	vertPositions* verts = _1(args);
	canyonTerrainBlock_generate( verts, b );
	vertPositions_delete( verts );
	mem_free( args );
	return NULL;
}
