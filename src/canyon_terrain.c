// canyon_terrain.c
#include "common.h"
#include "canyon_terrain.h"
//-----------------------
#include "camera.h"
#include "canyon.h"
#include "canyon_zone.h"
#include "collision.h"
#include "terrain_collision.h"
#include "terrain_generate.h"
#include "terrain_render.h"
#include "worker.h"
#include "actor/actor.h"
#include "base/pair.h"
#include "maths/geometry.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/texture.h"
#include "terrain/cache.h"
#include "terrain/buildCacheTask.h"

const float texture_scale = 0.0325f;
const float texture_repeat = 10.f;

texture* terrain_texture = NULL;
texture* terrain_texture_cliff = NULL;
texture* terrain_texture_2 = NULL;
texture* terrain_texture_cliff_2 = NULL;

void canyonTerrain_updateBlocks( canyon* c, canyonTerrain* t );
canyonTerrainBlock* canyonTerrainBlock_create( canyonTerrain* t, int u, int v );
void canyonTerrainBlock_delete( canyonTerrainBlock* b );

// *** Utility functions

// Not adjusted as this is for renderable verts only
int canyonTerrainBlock_renderIndexFromUV( canyonTerrainBlock* b, int u, int v ) {
	vAssert( u >= 0 && u < b->u_samples ); vAssert( v >= 0 && v < b->v_samples );
	return u + v * b->u_samples;
}

// Adjusted as we have a 1-vert margin for normal calculation at edges
int indexFromUV( canyonTerrainBlock* b, int u, int v ) { return ( u + 1 ) + ( v + 1 ) * ( b->u_samples + 2 ); }

// As this is just renderable verts, we dont have the extra buffer space for normal generation
int canyonTerrainBlock_renderVertCount( canyonTerrainBlock* b ) {
#if CANYON_TERRAIN_INDEXED
   	return ( b->u_samples ) * ( b->v_samples );
#else
	return b->element_count;
#endif // CANYON_TERRAIN_INDEXED
}

bool boundsEqual( int a[2][2], int b[2][2] ) {
	return ( a[0][0] == b[0][0] && a[0][1] == b[0][1] &&
			a[1][0] == b[1][0] && a[1][1] == b[1][1] );
}

// total verts, including those not rendered but that are generated for correct normal generation at block boundaries
int vertCount( canyonTerrainBlock* b ) { return ( b->u_samples + 2 ) * ( b->v_samples + 2); }

int canyonTerrainBlock_triangleCount( canyonTerrainBlock* b ) { return ( b->u_samples - 1 ) * ( b->v_samples - 1 ) * 2; }

void terrain_positionsFromUV( canyonTerrain* t, int u_index, int v_index, float* u, float* v ) {
	float uPerBlock = (2 * t->u_radius) / (float)t->u_block_count;
	float uScale = uPerBlock / (float)(t->uSamplesPerBlock);
	float vPerBlock = (2 * t->v_radius) / (float)t->v_block_count;
	float vScale = vPerBlock / (float)(t->vSamplesPerBlock);
	*u = (float)u_index * uScale;
	*v = (float)v_index * vScale;
}

void canyonTerrainBlock_positionsFromUV( canyonTerrainBlock* b, int u_index, int v_index, float* u, float* v ) {
	int lod_ratio = lodRatio( b );
	if ( u_index == -1 ) u_index = -lod_ratio;
	if ( u_index == b->u_samples ) u_index = b->u_samples -1 + lod_ratio;
	if ( v_index == -1 ) v_index = -lod_ratio;
	if ( v_index == b->v_samples ) v_index = b->v_samples -1 + lod_ratio;

	canyonTerrain* t = b->terrain;

	float r = 4 / lod_ratio;
	float uPerBlock = (2 * t->u_radius) / (float)t->u_block_count;
	float uScale = uPerBlock / (float)(t->uSamplesPerBlock);
	float vPerBlock = (2 * t->v_radius) / (float)t->v_block_count;
	float vScale = vPerBlock / (float)(t->vSamplesPerBlock);
	*u = (float)(u_index * r + b->uMin) * uScale;
	*v = (float)(v_index * r + b->vMin) * vScale;
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

// Mod the texcoord to a sensible value, based on the terrain block coords
// Used to stop our UV coordinates getting so big that floating point rounding issues cause aliasing in the texture
float canyon_uvMapped( float block_minimum, float f ) { return f - ( texture_repeat * floorf( block_minimum / texture_repeat )); }

// ***
#define kMaxTerrainBlockWidth 80
#define kMaxTerrainBlockElements (kMaxTerrainBlockWidth * kMaxTerrainBlockWidth * 6)
unsigned short elementBuffer[kMaxTerrainBlockElements];

void initialiseDefaultElementBuffer( ) { for ( int i = 0; i < kMaxTerrainBlockElements; i++ ) elementBuffer[i] = i; }

void canyonTerrain_staticInit() {
	initialiseDefaultElementBuffer();

	if ( !terrain_texture ) 		{ terrain_texture		= texture_load( "dat/img/terrain/grass.tga" ); }
	if ( !terrain_texture_cliff )	{ terrain_texture_cliff = texture_load( "dat/img/terrain/cliff_grass.tga" ); }
	if ( !terrain_texture_2 )		{ terrain_texture_2		= texture_load( "dat/img/terrain/ground_industrial.tga" ); }
	if ( !terrain_texture_cliff_2 )	{ terrain_texture_cliff_2 = texture_load( "dat/img/terrain/cliff_industrial.tga" ); }
}

// ***
int canyonTerrain_blockIndexFromUV( canyonTerrain* t, int u, int v ) { return u + v * t->u_block_count; }

void canyonTerrain_calculateBounds( canyon* c, int bounds[2][2], canyonTerrain* t, vector* sample_point );

int blockCoord( int coord[2], int bounds[2][2], int uCount ) {
	return (coord[0] - bounds[0][0]) + (coord[1] - bounds[0][1]) * uCount;
}

void canyonTerrain_updateBlocks( canyon* c, canyonTerrain* t ) {
	/* We have a set of current blocks, B, and a set of projected blocks based on the new position, B'
	   Calculate the intersection I = B n B';
	   All blocks ( b | b is in I ) we keep, shifting their pointers to the correct position
	   All other blocks fill up the empty spaces, then are recalculated
	   The block pointer array remains sorted */
	int bounds[2][2];
	int intersection[2][2];
	canyonTerrain_calculateBounds( c, bounds, t, &t->sample_point );

	const int vTrim = ((float)bounds[0][1] - 0.5f) * ((2 * t->v_radius) / (float)t->v_block_count);
	terrainCache_trim( c->terrainCache, vTrim );

	if (!t->firstUpdate && boundsEqual( bounds, t->bounds ))
		return;

	vmutex_lock( &t->mutex ); {
		boundsIntersection( intersection, bounds, t->bounds );
		canyonTerrainBlock** newBlocks = stackArray( canyonTerrainBlock*, t->total_block_count );
		memset( newBlocks, 0, sizeof( canyonTerrainBlock* ) * t->total_block_count );

		// Move old blocks to new array
		for ( int i = 0; i < t->total_block_count; i++ ) {
			int coord[2];
			coord[0] = t->bounds[0][0] + ( i % t->u_block_count );
			coord[1] = t->bounds[0][1] + ( i / t->u_block_count );
			if ( boundsContains( intersection, coord ))
				newBlocks[blockCoord(coord, bounds, t->u_block_count)] = t->blocks[i];
			else if ( t->blocks[i] )
				canyonTerrainBlock_delete( t->blocks[i] );
		}
		// queue generate new blocks
		for ( int v = 0; v < t->v_block_count; v++ ) {
			for ( int u = 0; u < t->u_block_count; u++ ) {
				int i = canyonTerrain_blockIndexFromUV( t, u, v );
				if ( newBlocks[i] == NULL ) {
					canyonTerrainBlock* b = canyonTerrainBlock_create( t, u, v );
					printf( "Queueing block %d %d.\n", u, v );
					actorMsg( b->actor, generateVertices( b ));
				}
			}
		}

		memcpy( t->bounds, bounds, sizeof( int ) * 2 * 2 );
		memcpy( t->blocks, newBlocks, sizeof( canyonTerrainBlock* ) * t->total_block_count );
		t->firstUpdate = false;
	} vmutex_unlock( &t->mutex );
}


canyonTerrainBlock* canyonTerrainBlock_create( canyonTerrain* t, int u, int v ) {
	canyonTerrainBlock* b = mem_alloc( sizeof( canyonTerrainBlock ));
	memset( b, 0, sizeof( canyonTerrainBlock ));
	b->u_samples = t->uSamplesPerBlock;
	b->v_samples = t->vSamplesPerBlock;
	b->terrain = t;
	b->actor = spawnActor( t->system );
	b->canyon = t->canyon;
	b->coord[0] = t->bounds[0][0] + u;
	b->coord[1] = t->bounds[0][1] + v;
	b->u = u;
	b->v = v;
	return b;
}

void canyonTerrainBlock_delete( canyonTerrainBlock* b ) {
	//canyonTerrain_freeElementBuffer( b->terrain, b->element_buffer );
	//canyonTerrain_freeVertexBuffer( b->terrain, b->vertex_buffer );
	stopActor( b->actor );
	mem_free( b ); 
}

void canyonTerrain_blockContaining( canyon* c, int coord[2], canyonTerrain* t, vector* point ) {
	float u, v;
	canyonSpaceFromWorld( c, point->coord.x, point->coord.z, &u, &v );
	float block_width = (2 * t->u_radius) / (float)t->u_block_count;
	float block_height = (2 * t->v_radius) / (float)t->v_block_count;
	coord[0] = fround( u / block_width, 1.f );
	coord[1] = fround( v / block_height, 1.f );
}

int canyonTerrain_lodLevelForBlock( canyon* c, canyonTerrain* t, int coord[2] ) {
	int block[2];
	canyonTerrain_blockContaining( c, block, t, &t->sample_point );
	vAssert( t->lod_interval_u > 0 );
	vAssert( t->lod_interval_v > 0 );
	return min( 2, ( abs( coord[0] - block[0] ) / t->lod_interval_u ) + ( abs( coord[1] - block[1]) / t->lod_interval_v ));
}

void canyonTerrainBlock_calculateSamplesForLoD( canyon* c, canyonTerrainBlock* b, canyonTerrain* t, int coord[2] ) {
	b->lod_level = canyonTerrain_lodLevelForBlock( c, t, coord );
	// Set samples based on U offset, We add one so that we always get a centre point
	int ratio = max( 1, 2 * b->lod_level );
	b->u_samples = t->uSamplesPerBlock / ratio + 1;
	b->v_samples = t->vSamplesPerBlock / ratio + 1;
}

void canyonTerrainBlock_calculateExtents( canyonTerrainBlock* b, canyonTerrain* t, int coord[2] ) {
	float u = (2 * t->u_radius) / (float)t->u_block_count;
	float v = (2 * t->v_radius) / (float)t->v_block_count;
	b->u_min = ((float)coord[0] - 0.5f) * u;
	b->v_min = ((float)coord[1] - 0.5f) * v;
	b->u_max = b->u_min + u;
	b->v_max = b->v_min + v;
	b->uMin = coord[0] * t->uSamplesPerBlock - (t->uSamplesPerBlock / 2);
	b->vMin = coord[1] * t->vSamplesPerBlock - (t->vSamplesPerBlock / 2);
	
	canyonTerrainBlock_calculateSamplesForLoD( b->canyon, b, t, coord );
}

// Calculate the block bounds for the terrain, at a given sample point
void canyonTerrain_calculateBounds( canyon* c, int bounds[2][2], canyonTerrain* t, vector* sample_point ) {
	/* Find the block we are in, Then extend by the correct radius
	   The center block [0] is from -half_block_width to half_block_width */
	int block[2];
	canyonTerrain_blockContaining( c, block, t, sample_point );
	int rx = ( t->u_block_count - 1 ) / 2;
	int ry = ( t->v_block_count - 1 ) / 2;
	bounds[0][0] = block[0] - rx;
	bounds[0][1] = block[1] - ry;
	bounds[1][0] = block[0] + rx;
	bounds[1][1] = block[1] + ry;
}

void canyonTerrainBlock_createBuffers( canyonTerrainBlock* b ) {
	b->element_count = canyonTerrainBlock_triangleCount( b ) * 3;
	vAssert( b->element_count > 0 );
	vAssert( b->element_count <= kMaxTerrainBlockElements );

#if CANYON_TERRAIN_INDEXED
	if ( !b->element_buffer ) b->element_buffer = canyonTerrain_nextElementBuffer( b->terrain );
#else
	b->element_buffer = elementBuffer;
#endif // CANYON_TERRAIN_INDEXED
	if ( !b->vertex_buffer ) b->vertex_buffer = canyonTerrain_nextVertexBuffer( b->terrain );
}

void terrain_setBlock( canyonTerrain* t, int u, int v, canyonTerrainBlock* b ) {
	vmutex_lock( &t->mutex ); {
		int i = canyonTerrain_blockIndexFromUV( t, u, v );
		if ( t->blocks[i] )
			canyonTerrainBlock_delete( t->blocks[i] );
		printf( "Terrain setting block %d %d.\n", u, v );
		t->blocks[i] = b;
	} vmutex_unlock( &t->mutex );
}

void canyonTerrainBlock_generate( vertPositions* vs, canyonTerrainBlock* b ) {
	canyonTerrainBlock_createBuffers( b );

	terrainBlock_build( b, vs );
	terrainBlock_initVBO( b );
	terrainBlock_calculateCollision( b );
	terrainBlock_calculateAABB( b );
	b->pending = false;

	terrain_setBlock( b->terrain, b->u, b->v, b );
}

void canyonTerrain_createBlocks( canyon* c, canyonTerrain* t ) {
	vAssert( !t->blocks );
	vAssert( t->u_block_count > 0 );
	vAssert( t->v_block_count > 0 );
	t->total_block_count = t->u_block_count * t->v_block_count;
	t->blocks = mem_alloc( sizeof( canyonTerrainBlock* ) * t->total_block_count );
	memset( t->blocks, 0, sizeof( canyonTerrainBlock* ) * t->total_block_count );

	canyonTerrain_calculateBounds( c, t->bounds, t, &t->sample_point );
}

void canyonTerrain_setLodIntervals( canyonTerrain* t, int u, int v ) {
	t->lod_interval_u = u;
	t->lod_interval_v = v;
}

canyonTerrain* canyonTerrain_create( canyon* c, int u_blocks, int v_blocks, int u_samples, int v_samples, float u_radius, float v_radius ) {
	canyonTerrain* t = mem_alloc( sizeof( canyonTerrain ));
	memset( t, 0, sizeof( canyonTerrain ));
	t->canyon = c;
	t->u_block_count = u_blocks;
	t->v_block_count = v_blocks;
	vAssert( u_samples <= kMaxTerrainBlockWidth && v_samples <= kMaxTerrainBlockWidth );
	t->uSamplesPerBlock = u_samples;
	t->vSamplesPerBlock = v_samples;
	t->u_radius = u_radius;
	t->v_radius = v_radius;
	t->firstUpdate = true;
	canyonTerrain_setLodIntervals( t, 3, 2 );
	t->system = actorSystemCreate();
	vmutex_init( &t->mutex );

	canyonTerrain_initVertexBuffers( t );
	if ( CANYON_TERRAIN_INDEXED ) canyonTerrain_initElementBuffers( t );
	canyonTerrain_createBlocks( c, t );
	canyonTerrain_updateBlocks( c, t );

	t->trans = transform_create();
	return t;
}

vector calcUV(canyonTerrainBlock* b, vector* v, float v_pos) {
	return Vector( v->coord.x * texture_scale,
					v->coord.y * texture_scale, 
					v->coord.z * texture_scale, 
					canyon_uvMapped( b->v_min * texture_scale, v_pos * texture_scale ));
}

bool validIndex( canyonTerrainBlock* b, int u, int v ) { return ( v >= 0 && v < b->v_samples && u >= 0 && u < b->u_samples ); }

void canyonTerrainBlock_generateVertices( canyonTerrainBlock* b, vector* verts, vector* normals ) {
#if CANYON_TERRAIN_INDEXED
	for ( int v_index = 0; v_index < b->v_samples; ++v_index ) {
		for ( int u_index = 0; u_index < b->u_samples; ++u_index ) {
			int i = indexFromUV( b, u_index, v_index );
			float u,v;
			canyonTerrainBlock_positionsFromUV( b, u_index, v_index, &u, &v );
			if (validIndex( b, u_index, v_index )) {
				int buffer_index = canyonTerrainBlock_renderIndexFromUV( b, u_index, v_index );
				vAssert( buffer_index < canyonTerrainBlock_renderVertCount( b ));
				vAssert( buffer_index >= 0 );
				b->vertex_buffer[buffer_index].position = verts[i];
				b->vertex_buffer[buffer_index].uv = calcUV( b, &verts[i], v );
				b->vertex_buffer[buffer_index].color = Vector( canyonZone_terrainBlend( v ), 0.f, 0.f, 1.f );
				b->vertex_buffer[buffer_index].normal = normals[i];
			}
		}
	}
	int triangleCount = canyonTerrainBlock_triangleCount( b );
	int i = 0;
	for ( int v = 0; v + 1 < b->v_samples; ++v ) {
		for ( int u = 0; u + 1 < b->u_samples; ++u ) {
			vAssert( i * 3 + 5 < b->element_count );
			vAssert( canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 ) < canyonTerrainBlock_renderVertCount( b ) );
			b->element_buffer[ i * 3 + 0 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v );
			b->element_buffer[ i * 3 + 1 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
			b->element_buffer[ i * 3 + 2 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
			b->element_buffer[ i * 3 + 3 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
			b->element_buffer[ i * 3 + 4 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 );
			b->element_buffer[ i * 3 + 5 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
			i+=2;
		}
	}
	vAssert( i == triangleCount );
#else
	for ( int v = 0; v < b->v_samples; v ++ ) {
		for ( int u = 0; u < b->u_samples; u ++  ) {
			int i = indexFromUV( b, u, v );
			vertex vert;
			vert.position = verts[i];
			vert.normal = normals[i];

			float u_pos, v_pos;
			canyonTerrainBlock_positionsFromUV( b, u, v, &u_pos, &v_pos );
			vert.uv = calcUV( b, &vert.position, v_pos );
			canyonTerrainBlock_fillTrianglesForVertex( b, verts, b->vertex_buffer, u, v, &vert );
		}
	}
	#endif // CANYON_TERRAIN_INDEXED
}

int lodRatio( canyonTerrainBlock* b ) { return b->u_samples / ( b->terrain->uSamplesPerBlock / 4 ); }

void initNormals( vector* normals, int count )		{ for ( int i = 0; i < count; ++i ) normals[i] = y_axis; }

void* canyonTerrain_workerGenerateBlock( void* args ) {
	canyonTerrainBlock* b = _2(args);
	vertPositions* verts = _1(args);
	canyonTerrainBlock_generate( verts, b );
	vertPositions_delete( verts );
	mem_free( args );
	return NULL;
}

void canyonTerrain_tick( void* data, float dt, engine* eng ) {
	(void)dt; (void)eng;
	canyonTerrain* t = data;

	vector v = Vector( 0.0, 0.0, 30.0, 1.0 );
	t->sample_point = matrix_vecMul( theScene->cam->trans->world, &v );
	canyon_seekForWorldPosition( t->canyon, t->sample_point );
	zone_sample_point = t->sample_point;

	canyonTerrain_updateBlocks( t->canyon, t );
}
