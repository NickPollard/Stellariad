// canyon_terrain.c
#include "common.h"
#include "canyon_terrain.h"
//-----------------------
#include "camera.h"
#include "canyon.h"
#include "canyon_zone.h"
#include "collision.h"
#include "terrain_collision.h"
#include "terrain_render.h"
#include "worker.h"
#include "maths/geometry.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/debugdraw.h"
#include "render/graphicsbuffer.h"
#include "render/texture.h"

const float texture_scale = 0.0325f;
const float texture_repeat = 10.f;

texture* terrain_texture = 0;
texture* terrain_texture_cliff = 0;

texture* terrain_texture_2 = 0;
texture* terrain_texture_cliff_2 = 0;


// *** Forward declarations
void canyonTerrainBlock_initVBO( canyonTerrainBlock* b );
void canyonTerrainBlock_calculateBuffers( canyon* c, canyonTerrainBlock* b );
void canyonTerrainBlock_createBuffers( canyonTerrainBlock* b );
void canyonTerrainBlock_generate( canyon* c, canyonTerrainBlock* b );
void canyonTerrain_initElementBuffers( canyonTerrain* t );

// *** Utility functions

int canyonTerrainBlock_renderIndexFromUV( canyonTerrainBlock* b, int u, int v ) {
	// Not adjusted as this is for renderable verts only
	vAssert( u >= 0 && u < b->u_samples );
	vAssert( v >= 0 && v < b->v_samples );
	return u + v * b->u_samples;
}

// Adjusted as we have a 1-vert margin for normal calculation at edges
int indexFromUV( canyonTerrainBlock* b, int u, int v ) { return ( u + 1 ) + ( v + 1 ) * ( b->u_samples + 2 ); }

// As this is just renderable verts, we dont have the extra buffer space for normal generation
int canyonTerrainBlock_renderVertCount( canyonTerrainBlock* b ) { return ( b->u_samples ) * ( b->v_samples ); }

// total verts, including those not rendered but that are generated for correct normal generation at block boundaries
int vertCount( canyonTerrainBlock* b ) { return ( b->u_samples + 2 ) * ( b->v_samples + 2); }

int canyonTerrainBlock_triangleCount( canyonTerrainBlock* b ) { return ( b->u_samples - 1 ) * ( b->v_samples - 1 ) * 2; }

void canyonTerrainBlock_positionsFromUV( canyonTerrainBlock* b, int u_index, int v_index, float* u, float* v ) {
	int lod_ratio = lodRatio( b );
	if ( u_index == -1 ) u_index = -lod_ratio;
	if ( u_index == b->u_samples ) u_index = b->u_samples -1 + lod_ratio;
	if ( v_index == -1 ) v_index = -lod_ratio;
	if ( v_index == b->v_samples ) v_index = b->v_samples -1 + lod_ratio;

	float u_interval = ( b->u_max - b->u_min ) / (float)( b->u_samples - 1 );
	float v_interval = ( b->v_max - b->v_min ) / (float)( b->v_samples - 1 );
	*u = (float)u_index * u_interval + b->u_min;
	*v = (float)v_index * v_interval + b->v_min;
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

bool canyonTerrainBlock_render( canyonTerrainBlock* b, scene* s ) {
	// If we have new render buffers, free the old ones and switch to the new
	if (( b->vertex_VBO_alt && *b->vertex_VBO_alt ) && ( b->element_VBO_alt && *b->element_VBO_alt )) {
		render_freeBuffer( b->vertex_VBO );
		render_freeBuffer( b->element_VBO );

		b->vertex_VBO = b->vertex_VBO_alt;
		b->element_VBO = b->element_VBO_alt;
		b->element_count_render = b->element_count;
		b->vertex_VBO_alt = NULL;
		b->element_VBO_alt = NULL;
	}

	vector frustum[6];
	camera_calculateFrustum( s->cam, frustum );
	if ( frustum_cull( &b->bb, frustum ) )
		return false;

	int zone = b->canyon->current_zone;
	int first = ( zone + zone % 2 ) % b->canyon->zone_count;
	int second = ( zone + 1 - (zone % 2)) % b->canyon->zone_count;
	if ( b->vertex_VBO && *b->vertex_VBO && terrain_texture && terrain_texture_cliff ) {
		drawCall* draw = drawCall_create( &renderPass_main, resources.shader_terrain, b->element_count_render, b->element_buffer, b->vertex_buffer, b->canyon->zones[first].texture_ground->gl_tex, modelview );
		draw->texture_b = b->canyon->zones[first].texture_cliff->gl_tex;
		draw->texture_c = b->canyon->zones[second].texture_ground->gl_tex;
		draw->texture_d = b->canyon->zones[second].texture_cliff->gl_tex;
		draw->vertex_VBO = *b->vertex_VBO;
		draw->element_VBO = *b->element_VBO;
	}
	return true;
}

int canyonTerrain_blockIndexFromUV( canyonTerrain* t, int u, int v ) { return u + v * t->u_block_count; }

void canyonTerrain_render( void* data, scene* s ) {
	(void)s;
	canyonTerrain* t = data;
	render_resetModelView();
	matrix_mulInPlace( modelview, modelview, t->trans->world );

	int count = 0;
	for ( int i = 0; i < t->total_block_count; ++i ) {
		count += canyonTerrainBlock_render( t->blocks[i], s );
	}
}

canyonTerrainBlock* canyonTerrainBlock_create( canyonTerrain* t ) {
	canyonTerrainBlock* b = mem_alloc( sizeof( canyonTerrainBlock ));
	memset( b, 0, sizeof( canyonTerrainBlock ));
	b->u_samples = t->u_samples_per_block;
	b->v_samples = t->v_samples_per_block;
	b->terrain = t;
	return b;
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
	(void)coord;
#ifdef TERRAIN_FORCE_NO_LOD
	int level = 0;
#else
	int level = canyonTerrain_lodLevelForBlock( c, t, coord );
#endif
	vAssert( level < 3 );
	// Set samples based on U offset
	// We add one so that we always get a centre point
	int f = max( 1, 2 * level );
	b->u_samples = t->u_samples_per_block / f + 1;
	b->v_samples = t->v_samples_per_block / f + 1;
	b->lod_level = level;
}

void canyonTerrainBlock_calculateExtents( canyon* c, canyonTerrainBlock* b, canyonTerrain* t, int coord[2] ) {
	float u = (2 * t->u_radius) / (float)t->u_block_count;
	float v = (2 * t->v_radius) / (float)t->v_block_count;
	b->u_min = ((float)coord[0] - 0.5f) * u;
	b->v_min = ((float)coord[1] - 0.5f) * v;
	b->u_max = b->u_min + u;
	b->v_max = b->v_min + v;
	
	canyonTerrainBlock_calculateSamplesForLoD( c, b, t, coord );
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

void canyonTerrain_createBlocks( canyon* c, canyonTerrain* t ) {
	vAssert( !t->blocks );
	vAssert( t->u_block_count > 0 );
	vAssert( t->v_block_count > 0 );
	t->total_block_count = t->u_block_count * t->v_block_count;
	t->blocks = mem_alloc( sizeof( canyonTerrainBlock* ) * t->total_block_count );

	canyonTerrain_calculateBounds( c, t->bounds, t, &t->sample_point );

	// Calculate block extents
	for ( int v = 0; v < t->v_block_count; v++ ) {
		for ( int u = 0; u < t->u_block_count; u++ ) {
			int i = canyonTerrain_blockIndexFromUV( t, u, v );
			t->blocks[i] = canyonTerrainBlock_create( t );
			canyonTerrainBlock* b = t->blocks[i];
			b->coord[0] = t->bounds[0][0] + u;
			b->coord[1] = t->bounds[0][1] + v;
			canyonTerrainBlock_generate( c, b );
			t->blocks[i]->canyon = t->canyon;
		}
	}
}

canyonTerrain* canyonTerrain_create( canyon* c, int u_blocks, int v_blocks, int u_samples, int v_samples, float u_radius, float v_radius ) {
	canyonTerrain* t = mem_alloc( sizeof( canyonTerrain ));
	memset( t, 0, sizeof( canyonTerrain ));
	t->canyon = c;
	t->u_block_count = u_blocks;
	t->v_block_count = v_blocks;
	vAssert( u_samples <= kMaxTerrainBlockWidth );
	vAssert( v_samples <= kMaxTerrainBlockWidth );
	t->u_samples_per_block = u_samples;
	t->v_samples_per_block = v_samples;
	t->u_radius = u_radius;
	t->v_radius = v_radius;
	t->lod_interval_u = 3;
	t->lod_interval_v = 2;

	canyonTerrain_initVertexBuffers( t );
	if ( CANYON_TERRAIN_INDEXED ) canyonTerrain_initElementBuffers( t );
	canyonTerrain_createBlocks( c, t );

	t->trans = transform_create();
	return t;
}

void canyonTerrain_setLodIntervals( canyonTerrain* t, int u, int v ) {
	t->lod_interval_u = u;
	t->lod_interval_v = v;
}

vector calcUV(canyonTerrainBlock* b, vector* v, float v_pos) {
	return Vector( 
			v->coord.x * texture_scale,
			v->coord.y * texture_scale, 
			v->coord.z * texture_scale, 
			canyon_uvMapped( b->v_min * texture_scale, v_pos * texture_scale )
			);
}

#if CANYON_TERRAIN_INDEXED
#else
void canyonTerrainBlock_generateVertices( canyonTerrainBlock* b, vector* verts, vector* normals ) {
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
}
#endif // CANYON_TERRAIN_INDEXED

int lodRatio( canyonTerrainBlock* b ) { return b->u_samples / ( b->terrain->u_samples_per_block / 4 ); }

// Generate Normals
void canyonTerrainBlock_calculateNormals( canyonTerrainBlock* block, int vert_count, vector* verts, vector* normals ) {
	(void)vert_count;
	int lod_ratio = lodRatio( block );

	for ( int v = 0; v < block->v_samples; ++v ) {
		for ( int u = 0; u < block->u_samples; ++u ) {
			int index;
			if ( v == block->v_samples - 1 )
				index = indexFromUV( block, u, v - lod_ratio );
			else
				index = indexFromUV( block, u, v - 1 );
			vector left		= verts[index];

			if ( v == 0 )
				index = indexFromUV( block, u, v + lod_ratio );
			else
				index = indexFromUV( block, u, v + 1 );
			vector right	= verts[index];

			if ( u == block->u_samples - 1 )
				index = indexFromUV( block, u - lod_ratio, v );
			else
				index = indexFromUV( block, u - 1, v );
			vector top		= verts[index];

			if ( u == 0 )
				index = indexFromUV( block, u + lod_ratio, v );
			else
				index = indexFromUV( block, u + 1, v );
			vector bottom	= verts[index];
			
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
			vector total = Vector( 0.f, 0.f, 0.f, 0.f );
			Add( &total, &total, &b );
			Add( &total, &total, &c );
			total.coord.w = 0.f;
			Normalize( &total, &total );
			vAssert( i < vert_count );
			normals[i] = total;
		}
	}
}





void canyonTerrainBlock_createBuffers( canyonTerrainBlock* b ) {
	b->element_count = canyonTerrainBlock_triangleCount( b ) * 3;
	vAssert( b->element_count > 0 );
	vAssert( b->element_count <= kMaxTerrainBlockElements );

#if CANYON_TERRAIN_INDEXED
	if ( !b->element_buffer )
		b->element_buffer = canyonTerrain_nextElementBuffer( b->terrain );
	if ( !b->vertex_buffer )
		b->vertex_buffer = canyonTerrain_nextVertexBuffer( b->terrain );
#else
	if ( !b->vertex_buffer )
		b->vertex_buffer = canyonTerrain_nextVertexBuffer( b->terrain );
	b->element_buffer = elementBuffer;
#endif // CANYON_TERRAIN_INDEXED
}

bool validIndex( canyonTerrainBlock* b, int u, int v ) {
	return ( v >= 0 &&
		   	v < b->v_samples &&
		   	u >= 0 &&
		   	u < b->u_samples );
}

#define CacheSize 256
typedef struct cachedPoint_s {
	vector vec;
	int u;
	int v;
} cachedPoint;

cachedPoint terrainCache[CacheSize][CacheSize];
cachedPoint* CachedPoint( int u, int v ) {
	cachedPoint* p = &terrainCache[abs(u) % CacheSize][abs(v) % CacheSize];
	return ( p->u == u && p->v == v ) ? p : NULL;
}

void setCachedPoint( int u, int v, vector vec ) {
	cachedPoint* p = &terrainCache[abs(u) % CacheSize][abs(v) % CacheSize];
	p->vec = vec;
	p->u = u;
	p->v = v;
}

vector terrainPoint( canyon* c, canyonTerrainBlock* b, int u_index, int v_index ) {
	//cachedPoint* p = CachedPoint(u_index, v_index);
	//if ( p ) 
		//return p->vec;
	float u, v;
	canyonTerrainBlock_positionsFromUV( b, u_index, v_index, &u, &v );
	float x, z;
	terrain_worldSpaceFromCanyon( c, u, v, &x, &z );
	vector vec = Vector( x, canyonTerrain_sampleUV( u, v ), z, 1.f );
	//setCachedPoint(u, v, vec);
	return vec;
}

void canyonTerrainBlock_generateVerts( canyon* c, canyonTerrainBlock* b, vector* verts ) {
	for ( int v = -1; v < b->v_samples + 1; ++v ) {
		for ( int u = -1; u < b->u_samples + 1; ++u ) {
			int i = indexFromUV( b, u, v );
			verts[i] = terrainPoint( c, b, u, v );
		}
	}
}

void initNormals( vector* normals, int count )		{ for ( int i = 0; i < count; ++i ) normals[i] = y_axis; }

void canyonTerrainBlock_calculateBuffers( canyon* c, canyonTerrainBlock* b ) {
	int vert_count = vertCount( b );
	
	vector* verts = alloca( sizeof( vector ) * vert_count );
	memset( verts, 0, sizeof( vector ) * vert_count );
	vector* normals = alloca( sizeof( vector ) * vert_count );

	canyonTerrainBlock_generateVerts( c, b, verts );
	
	int lod_ratio = lodRatio( b );

#if 1
	// Force low-LOD edges
	for ( int v_index = -1; v_index < b->v_samples + 1; ++v_index ) {
		for ( int u_index = -1; u_index < b->u_samples + 1; ++u_index ) {
			int i = indexFromUV( b, u_index, v_index );

			// If it's an intermediary value
			if ( ( v_index <= 0 || v_index >= b->v_samples - 1 ) && ( u_index % lod_ratio != 0 ) && ( u_index >= 0 && u_index < b->u_samples )) {
				int prev = indexFromUV( b, u_index - ( u_index % lod_ratio ), v_index );
				int next = indexFromUV( b, u_index - ( u_index % lod_ratio ) + lod_ratio, v_index );
				if ( prev >= 0 && prev < vert_count && next >= 0 && next < vert_count ) {
					verts[i] = vector_lerp( &verts[prev], &verts[next], (float)( u_index % lod_ratio ) / (float)lod_ratio);
				}
			}

			if ( ( u_index <= 0 || u_index >= b->u_samples - 1 ) && ( v_index % lod_ratio != 0 ) && ( v_index >= 0 && v_index < b->v_samples )) {
				int previous = indexFromUV( b, u_index, v_index - ( v_index % lod_ratio ));
				int next = indexFromUV( b, u_index, v_index - ( v_index % lod_ratio ) + lod_ratio );
				if ( previous >= 0 && previous < vert_count && next >= 0 && next < vert_count ) {
					verts[i] = vector_lerp( &verts[previous], &verts[next], (float)( v_index % lod_ratio ) / (float)lod_ratio );
				}
			}

		}
	}
#endif

	canyonTerrainBlock_calculateNormals( b, vert_count, verts, normals );

	// Force low-LOD edges
	for ( int v_index = 0; v_index < b->v_samples; ++v_index ) {
		for ( int u_index = 0; u_index < b->u_samples; ++u_index ) {
			int i = indexFromUV( b, u_index, v_index );
			// If it's an intermediary value
			if ( ( v_index == 0 || v_index == b->v_samples - 1 ) && ( u_index % lod_ratio != 0 ) && ( u_index >= 0 && u_index < b->u_samples )) {
				int previous = indexFromUV( b, u_index - ( u_index % lod_ratio ), v_index );
				int next = indexFromUV( b, u_index - ( u_index % lod_ratio ) + lod_ratio, v_index );
				if ( previous >= 0 && previous < vert_count && next >= 0 && next < vert_count ) {
					float factor = (float)( u_index % lod_ratio ) / (float)lod_ratio;
					normals[i] = vector_lerp( &normals[previous], &normals[next], factor );
				}
			}
			if ( ( u_index == 0 || u_index == b->u_samples - 1 ) && 
					( v_index % lod_ratio != 0 ) &&
					( v_index >= 0 && v_index < b->v_samples )) {
				int previous = indexFromUV( b, u_index, v_index - ( v_index % lod_ratio ));
				int next = indexFromUV( b, u_index, v_index - ( v_index % lod_ratio ) + lod_ratio );

				if ( previous >= 0 && previous < vert_count && next >= 0 && next < vert_count ) {
					float factor = (float)( v_index % lod_ratio ) / (float)lod_ratio;
					normals[i] = vector_lerp( &normals[previous], &normals[next], factor );
				}
			}
		}
	}

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
#endif // CANYON_TERRAIN_INDEXED

#if CANYON_TERRAIN_INDEXED
	int triangle_count = canyonTerrainBlock_triangleCount( b );
	int i = 0;
	for ( int v = 0; v + 1 < b->v_samples; ++v ) {
		for ( int u = 0; u + 1 < b->u_samples; ++u ) {
			vAssert( i * 3 + 5 < b->element_count );
			vAssert( canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 ) < canyonTerrainBlock_renderVertCount( b ) );
			b->element_buffer[ i * 3 + 0 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v );
			b->element_buffer[ i * 3 + 1 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
			b->element_buffer[ i * 3 + 2 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
			i++;
			b->element_buffer[ i * 3 + 0 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
			b->element_buffer[ i * 3 + 1 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 );
			b->element_buffer[ i * 3 + 2 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
			i++;
		}
	}
	vAssert( i == triangle_count );
#else
	// Unroll Verts
	canyonTerrainBlock_generateVertices( b, verts, normals );
#endif // CANYON_TERRAIN_INDEXED

	canyonTerrainBlock_initVBO( b );
}

// Create GPU vertex buffer objects to hold our data and save transferring to the GPU each frame
// If we've already allocated a buffer at some point, just re-use it
void canyonTerrainBlock_initVBO( canyonTerrainBlock* b ) {
#if CANYON_TERRAIN_INDEXED
	int vert_count = canyonTerrainBlock_renderVertCount( b );
#else
	int vert_count = b->element_count;
#endif // CANYON_TERRAIN_INDEXED
	b->vertex_VBO_alt	= render_requestBuffer( GL_ARRAY_BUFFER,			b->vertex_buffer,	sizeof( vertex )	* vert_count );
	b->element_VBO_alt	= render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, 	b->element_buffer,	sizeof( GLushort ) 	* b->element_count );
}

void canyonTerrainBlock_generate( canyon* c, canyonTerrainBlock* b ) {
	canyonTerrainBlock_calculateExtents( c, b, b->terrain, b->coord );
	canyonTerrainBlock_createBuffers( b );
	canyonTerrainBlock_calculateBuffers( c, b );
	terrainBlock_calculateCollision( b );
	terrainBlock_calculateAABB( b );
}

typedef struct pair_s {
	void* _1;
	void* _2;
} pair;

pair* Pair( void* a, void* b ) {
	pair* p = mem_alloc( sizeof( pair ));
	p->_1 = a;
	p->_2 = b;
	return p;
}

void* canyonTerrain_workerGenerateBlock( void* args ) {
	//printf( "Worker generate terrain block.\n" );
	pair* p = args;
	canyon* c = p->_1;
	canyonTerrainBlock* b = p->_2;
	mem_free( p );
	if ( b->pending ) {
		canyonTerrainBlock_generate( c, b );
		b->pending = false;
	}
	return NULL;
}

// Set up a task for the worker thread to generate the terrain block
void canyonTerrain_queueWorkerTaskGenerateBlock( canyon* c, canyonTerrainBlock* b ) {
	worker_task terrain_block_task;
	terrain_block_task.func = canyonTerrain_workerGenerateBlock;
	terrain_block_task.args = Pair( c, b );
	worker_addTask( terrain_block_task );
}

void canyonTerrain_updateBlocks( canyon* c, canyonTerrain* t ) {
	/*
	   We have a set of current blocks, B
	   We have a set of projected blocks based on the new position, B'

	   Calculate the intersection I = B n B';
	   All blocks ( b | b is in I ) we keep, shifting their pointers to the correct position
	   All other blocks fill up the empty spaces, then are recalculated

	   We are not freeing or allocating any blocks here; only reusing existing ones
	   The block pointer array remains sorted
	   */

	int bounds[2][2];
	int intersection[2][2];
	canyonTerrain_calculateBounds( c, bounds, t, &t->sample_point );

	// If the bounds are the exact same as before, we don't need to do *any* updating
	if ( bounds[0][0] == t->bounds[0][0] &&
			bounds[0][1] == t->bounds[0][1] &&
			bounds[1][0] == t->bounds[1][0] &&
			bounds[1][1] == t->bounds[1][1] )
		return;

	boundsIntersection( intersection, bounds, t->bounds );

	// Using alloca for dynamic stack allocation (just moves the stack pointer up)
	canyonTerrainBlock** new_blocks = alloca( sizeof( canyonTerrainBlock* ) * t->total_block_count );

	int empty_index = 0;
	// For Each old block
	for ( int i = 0; i < t->total_block_count; i++ ) {
		int coord[2];
		coord[0] = t->bounds[0][0] + ( i % t->u_block_count );
		coord[1] = t->bounds[0][1] + ( i / t->u_block_count );
		// if in new bounds
		if ( boundsContains( intersection, coord )) {
			// copy to new array;
			int new_u = coord[0] - bounds[0][0];
			int new_v = coord[1] - bounds[0][1];
			int new_index = new_u + ( new_v * t->u_block_count );
			vAssert( new_index >= 0 );
			vAssert( new_index < t->total_block_count );
			new_blocks[new_index] = t->blocks[i];
		}
		else {
			// Copy unused blocks
			// Find next empty index
			int new_coord[2];
			while ( true ) {
				new_coord[0] = bounds[0][0] + ( empty_index % t->u_block_count );
				new_coord[1] = bounds[0][1] + ( empty_index / t->u_block_count );
				if ( !boundsContains( intersection, new_coord ))
					break;
				empty_index++;
			}
			new_blocks[empty_index] = t->blocks[i];
			empty_index++;
		}
	}

	// For each new block
	for ( int i = 0; i < t->total_block_count; i++ ) {
		int coord[2];
		coord[0] = bounds[0][0] + ( i % t->u_block_count );
		coord[1] = bounds[0][1] + ( i / t->u_block_count );
		canyonTerrainBlock* b = new_blocks[i];
		// if not in old bounds
		// Or if we need to change lod level
#ifdef TERRAIN_FORCE_NO_LOD
		if ( !boundsContains( intersection, coord ))
#else
		if ( !boundsContains( intersection, coord ) || 
				( !b->pending && ( b->lod_level != canyonTerrain_lodLevelForBlock( c, t, coord ))))
#endif
		{
			memcpy( b->coord, coord, sizeof( int ) * 2 );
			// mark it as new, buffers will be filled in later
			b->pending = true;
		}
	}
	memcpy( t->bounds, bounds, sizeof( int ) * 2 * 2 );
	memcpy( t->blocks, new_blocks, sizeof( canyonTerrainBlock* ) * t->total_block_count );

	for ( int i = 0; i < t->total_block_count; ++i ) {
		canyonTerrainBlock* b = t->blocks[i];
		if ( b->pending ) {
#if TERRAIN_USE_WORKER_THREAD
			canyonTerrain_queueWorkerTaskGenerateBlock( c, b );
#else
			canyonTerrainBlock_generate( c, b );
			b->pending = false;
			break;
#endif // TERRAIN_USE_WORKER_THREAD
		}
	}
}

void canyonTerrain_tick( void* data, float dt, engine* eng ) {
	(void)dt;
	(void)eng;
	canyonTerrain* t = data;

	vector v = Vector( 0.0, 0.0, 30.0, 1.0 );
	t->sample_point = matrix_vecMul( theScene->cam->trans->world, &v );
	canyon_seekForWorldPosition( t->canyon, t->sample_point );
	zone_sample_point = t->sample_point;

	canyonTerrain_updateBlocks( t->canyon, t );
}

