// terrain_render.c
#include "common.h"
#include "terrain_render.h"
//-----------------------
#include "canyon.h"
#include "camera.h"
#include "future.h"
#include "terrain_generate.h"
#include "maths/geometry.h"
#include "maths/vector.h"
#include "render/graphicsbuffer.h"
#include "render/texture.h"

const float texture_scale = 0.0325f;
const float texture_repeat = 10.f;

texture* terrain_texture = NULL;
texture* terrain_texture_cliff = NULL;
texture* terrain_texture_2 = NULL;
texture* terrain_texture_cliff_2 = NULL;

DECLARE_POOL(terrainRenderable)
IMPLEMENT_POOL(terrainRenderable)
pool_terrainRenderable* static_renderable_pool = NULL;

// ***
unsigned short elementBuffer[kMaxTerrainBlockElements];

void initialiseDefaultElementBuffer( ) { for ( int i = 0; i < kMaxTerrainBlockElements; i++ ) elementBuffer[i] = i; }

// Mod the texcoord to a sensible value, based on the terrain block coords
// Used to stop our UV coordinates getting so big that floating point rounding issues cause aliasing in the texture
float canyon_uvMapped( float block_minimum, float f ) { return f - ( texture_repeat * floorf( block_minimum / texture_repeat )); }

bool validIndex( canyonTerrainBlock* b, int u, int v ) { return ( v >= 0 && v < b->v_samples && u >= 0 && u < b->u_samples ); }
// Not adjusted as this is for renderable verts only
int canyonTerrainBlock_renderIndexFromUV( canyonTerrainBlock* b, int u, int v ) {
	vAssert( u >= 0 && u < b->u_samples ); vAssert( v >= 0 && v < b->v_samples );
	return u + v * b->u_samples;
}

int canyonTerrainBlock_triangleCount( canyonTerrainBlock* b ) { return ( b->u_samples - 1 ) * ( b->v_samples - 1 ) * 2; }

vector calcUV(canyonTerrainBlock* b, vector* v, float v_pos) {
	return Vector( v->coord.x * texture_scale,
					v->coord.y * texture_scale, 
					v->coord.z * texture_scale, 
					canyon_uvMapped( b->v_min * texture_scale, v_pos * texture_scale ));
}

bool canyonTerrainBlock_triangleInvalid( canyonTerrainBlock* b, int u_index, int v_index, int u_offset, int v_offset ) {
	u_offset = u_offset / 2 + u_offset % 2;
	u_offset = min( u_offset, 0 );
	return ( u_index + u_offset >= b->u_samples - 1 ) ||
		( u_index + u_offset < 0 ) ||
		( v_index + v_offset >= b->v_samples - 1 ) ||
		( v_index + v_offset < 0 );
}

// As this is just renderable verts, we dont have the extra buffer space for normal generation
int canyonTerrainBlock_renderVertCount( canyonTerrainBlock* b ) {
#if CANYON_TERRAIN_INDEXED
   	return ( b->u_samples ) * ( b->v_samples );
#else
	return b->element_count;
#endif // CANYON_TERRAIN_INDEXED
}

void canyonTerrain_renderInit() {
	static_renderable_pool = pool_terrainRenderable_create( PoolMaxBlocks );
	initialiseDefaultElementBuffer();
	if ( !terrain_texture ) 		{ terrain_texture		= texture_load( "dat/img/terrain/grass.tga" ); }
	if ( !terrain_texture_cliff )	{ terrain_texture_cliff = texture_load( "dat/img/terrain/cliff_grass.tga" ); }
	if ( !terrain_texture_2 )		{ terrain_texture_2		= texture_load( "dat/img/terrain/ground_industrial.tga" ); }
	if ( !terrain_texture_cliff_2 )	{ terrain_texture_cliff_2 = texture_load( "dat/img/terrain/cliff_industrial.tga" ); }
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

#if CANYON_TERRAIN_INDEXED
#else
// Given a vertex that has been generated, fill in the correct triangles for it after it has been unrolled
void canyonTerrainBlock_fillTrianglesForVertex( canyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert ) {
	// Each vertex is in a maximum of 6 triangles
	// The triangle indices can be computed as: (where row == ( u_samples - 1 ) * 2)
	//  first row:
	//    row * (v_index - 1) + 2 * u_index - 1
	//    row * (v_index - 1) + 2 * u_index
	//    row * (v_index - 1) + 2 * u_index + 1
	//	second row:
	//    row * v_index + 2 * u_index - 2
	//    row * v_index + 2 * u_index - 1
	//    row * v_index + 2 * u_index
	// (discarding any that fall outside the range 0 -> tri_count)
	// Triangle vert index:
	// top: 0, 2, 1
	// bottom: 1, 2, 0
	// finished index = triangle_index * 3 + triangle_vert_index
	const int triangle_vert_indices[6] = { 0, 2, 1, 1, 2, 0 };
	const int v_offset[6] = { -1, -1, -1, 0, 0, 0 };
	const int u_offset[6] = { -1, 0, 1, -2, -1, 0 };
	
	int triangles_per_row = ( b->u_samples - 1 ) * 2;
	
	for ( int i = 0; i < 6; ++i ) {
		const int row = v_index + ( v_offset[i] );
		const int column = ( 2 * u_index ) + ( u_offset[i] );
		const int triangle_index = triangles_per_row * row + column; 

		if ( canyonTerrainBlock_triangleInvalid( b, u_index, v_index, u_offset[i], v_offset[i]) )
			continue;

		// if it's a valid triangle (not out-of-bounds)
		const int vert_index = triangle_index * 3 + triangle_vert_indices[i];
		vertices[vert_index] = *vert;

		// Cliff coloring
		const int triangle_u_offset[6][2] = { { 0, -1 }, { 1, 0 }, { 1, 1 }, { -1, -1 }, { -1, 0 }, { 0, 1 } };
		const int triangle_v_offset[6][2] = { { -1, 0 }, { -1, -1 }, { 0, -1 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };
		
		// Calculate indices of other 2 triangle verts
		int index_b = indexFromUV( b, u_index + triangle_u_offset[i][0] , v_index + triangle_v_offset[i][0] );
		int index_c = indexFromUV( b, u_index + triangle_u_offset[i][1] , v_index + triangle_v_offset[i][1] );

		vector v_b = positions[index_b];
		vector v_c = positions[index_c];

		vector normal;
		float d;
		plane( vertices[vert_index].position, v_b, v_c, &normal, &d );
		const float angle = acosf( Dot( &normal, &y_axis ));
		const float cliff_angle = PI / 4.f;
		vertices[vert_index].color = angle > cliff_angle ? Vector( 0.2f, 0.3f, 0.5f, 1.f ) : Vector( 0.8f, 0.9f, 1.0f, 0.f );
	}
}
#endif

vertex* canyonTerrain_allocVertexBuffer( canyonTerrain* t ) {
#if CANYON_TERRAIN_INDEXED
	int max_vert_count = ( t->uSamplesPerBlock + 1 ) * ( t->vSamplesPerBlock + 1 );
	return (vertex*)mem_alloc( sizeof( vertex ) * max_vert_count );
#else
	int max_element_count = t->uSamplesPerBlock * t->vSamplesPerBlock * 6;
	return (vertex*)mem_alloc( sizeof( vertex ) * max_element_count );
#endif // CANYON_TERRAIN_INDEX
}

void canyonTerrain_initVertexBuffers( canyonTerrain* t ) {
	// Init w*h*2 buffers that we can use for vertex_buffers
	vAssert( t->vertex_buffers == 0 );
	const int count = t->u_block_count * t->v_block_count * 2;
	t->vertex_buffers = (vertex**)mem_alloc( count * sizeof( vertex* ));
	for ( int i = 0; i < count; i++ ) {
		t->vertex_buffers[i] = canyonTerrain_allocVertexBuffer( t );
	}
}
void canyonTerrain_freeVertexBuffer( canyonTerrain* t, vertex* buffer ) {
	// Find the buffer in the list
	// Switch it with the last
	int count = t->vertex_buffer_count;
	int i = array_find( (void**)t->vertex_buffers, count, buffer );
	t->vertex_buffers[i] = t->vertex_buffers[count-1];
	t->vertex_buffers[count-1] = buffer;
	--t->vertex_buffer_count;
}
vertex* canyonTerrain_nextVertexBuffer( canyonTerrain* t ) {
	vAssert( t->vertex_buffer_count < ( t->u_block_count * t->v_block_count * 3 ));
	void* buffer = t->vertex_buffers[t->vertex_buffer_count++];
	return (vertex*)buffer;
}

unsigned short* canyonTerrain_allocElementBuffer( canyonTerrain* t ) {
	int max_element_count = t->uSamplesPerBlock * t->vSamplesPerBlock * 6;
	return (unsigned short*)mem_alloc( sizeof( unsigned short ) * max_element_count );
}
void canyonTerrain_initElementBuffers( canyonTerrain* t ) {
	// Init w*h*2 buffers that we can use for element_buffers
	vAssert( t->element_buffers == 0 );
	int count = t->u_block_count * t->v_block_count * 3;
	t->element_buffers = (unsigned short**)mem_alloc( count * sizeof( unsigned short* ));
	for ( int i = 0; i < count; i++ ) {
		t->element_buffers[i] = canyonTerrain_allocElementBuffer( t );
	}
}
void canyonTerrain_freeElementBuffer( canyonTerrain* t, unsigned short* buffer ) {
	// Find the buffer in the list
	// Switch it with the last
	int count = t->element_buffer_count;
	int i = array_find( (void**)t->element_buffers, count, buffer );
	t->element_buffers[i] = t->element_buffers[count-1];
	t->element_buffers[count-1] = buffer;
	--t->element_buffer_count;
}
short unsigned int* canyonTerrain_nextElementBuffer( canyonTerrain* t ) {
	//printf( "Allocate\n" );
	vAssert( t->element_buffer_count < ( t->u_block_count * t->v_block_count * 3 ));
	void* buffer = t->element_buffers[t->element_buffer_count++];
	return (short unsigned int*)buffer;
}
// Create GPU vertex buffer objects to hold our data and save transferring to the GPU each frame
// If we've already allocated a buffer at some point, just re-use it
future* terrainBlock_initVBO( canyonTerrainBlock* b ) {
	int vert_count = canyonTerrainBlock_renderVertCount( b );
	terrainRenderable* r = b->renderable;
	r->vertex_VBO_alt	= render_requestBuffer( GL_ARRAY_BUFFER,			r->vertex_buffer,	sizeof( vertex )	* vert_count );
	r->element_VBO_alt	= render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, 	r->element_buffer,	sizeof( GLushort ) 	* r->element_count );
	return b->ready;
}

bool canyonTerrainBlock_render( canyonTerrainBlock* b, scene* s ) {
	terrainRenderable* r = b->renderable;
	// If we have new render buffers, free the old ones and switch to the new
	if (( r->vertex_VBO_alt && *r->vertex_VBO_alt ) && ( r->element_VBO_alt && *r->element_VBO_alt )) {
		render_freeBuffer( r->vertex_VBO );
		render_freeBuffer( r->element_VBO );

		r->vertex_VBO = r->vertex_VBO_alt;
		r->element_VBO = r->element_VBO_alt;
		r->element_count_render = r->element_count;
		r->vertex_VBO_alt = NULL;
		r->element_VBO_alt = NULL;
	}

	if ( frustum_cull( &r->bb, s->cam->frustum ) )
		return false;

	int zone = b->_canyon->current_zone;
	int first = ( zone + zone % 2 ) % b->_canyon->zone_count;
	int second = ( zone + 1 - (zone % 2)) % b->_canyon->zone_count;
	if ( r->vertex_VBO && *r->vertex_VBO && terrain_texture && terrain_texture_cliff ) {
		// TODO - use a cached drawcall
		drawCall* draw = drawCall_create( &renderPass_main, *r->terrainShader, r->element_count_render, r->element_buffer, r->vertex_buffer, b->_canyon->zones[first].texture_ground->gl_tex, modelview );
		draw->texture_b = b->_canyon->zones[first].texture_cliff->gl_tex;
		draw->texture_c = b->_canyon->zones[second].texture_ground->gl_tex;
		draw->texture_d = b->_canyon->zones[second].texture_cliff->gl_tex;
		draw->vertex_VBO = *r->vertex_VBO;
		draw->element_VBO = *r->element_VBO;

		drawCall* drawDepth = drawCall_create( &renderPass_depth, resources.shader_depth, r->element_count_render, r->element_buffer, r->vertex_buffer, b->_canyon->zones[first].texture_ground->gl_tex, modelview );
		drawDepth->vertex_VBO = *r->vertex_VBO;
		drawDepth->element_VBO = *r->element_VBO;
	}
	return true;
}


void canyonTerrain_render( void* data, scene* s ) {
	(void)s;
	canyonTerrain* t = (canyonTerrain*)data;
	render_resetModelView();
	matrix_mulInPlace( modelview, modelview, t->trans->world );

	int count = 0;
	for ( int i = 0; i < t->total_block_count; ++i ) {
		if ( t->blocks[i] )
			count += canyonTerrainBlock_render( t->blocks[i], s );
	}
	//printf( "Rendering %d out of %d blocks.\n", count, t->total_block_count );
}

void canyonTerrainBlock_generateVertices( canyonTerrainBlock* b, vector* verts, vector* normals ) {
	terrainRenderable* r = b->renderable;
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
				r->vertex_buffer[buffer_index].position = verts[i];
				r->vertex_buffer[buffer_index].uv = calcUV( b, &verts[i], v );
				r->vertex_buffer[buffer_index].color = Vector( canyonZone_terrainBlend( v ), 0.f, 0.f, 1.f );
				r->vertex_buffer[buffer_index].normal = normals[i];
			}
		}
	}
	int triangleCount = canyonTerrainBlock_triangleCount( b );
	int i = 0;
	for ( int v = 0; v + 1 < b->v_samples; ++v ) {
		for ( int u = 0; u + 1 < b->u_samples; ++u ) {
			vAssert( i * 3 + 5 < r->element_count );
			vAssert( canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 ) < canyonTerrainBlock_renderVertCount( b ) );
			r->element_buffer[ i * 3 + 0 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v );
			r->element_buffer[ i * 3 + 1 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
			r->element_buffer[ i * 3 + 2 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
			r->element_buffer[ i * 3 + 3 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v );
			r->element_buffer[ i * 3 + 4 ] = canyonTerrainBlock_renderIndexFromUV( b, u + 1, v + 1 );
			r->element_buffer[ i * 3 + 5 ] = canyonTerrainBlock_renderIndexFromUV( b, u, v + 1 );
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

void canyonTerrainBlock_createBuffers( canyonTerrainBlock* b ) {
	terrainRenderable* r = b->renderable;
	r->element_count = canyonTerrainBlock_triangleCount( b ) * 3;
	vAssert( r->element_count > 0 );
	vAssert( r->element_count <= kMaxTerrainBlockElements );

#if CANYON_TERRAIN_INDEXED
	if ( !r->element_buffer ) r->element_buffer = canyonTerrain_nextElementBuffer( b->terrain );
#else
	r->element_buffer = elementBuffer;
#endif // CANYON_TERRAIN_INDEXED
	if ( !r->vertex_buffer ) r->vertex_buffer = canyonTerrain_nextVertexBuffer( b->terrain );
}

terrainRenderable* terrainRenderable_create( canyonTerrainBlock* b ) {
	terrainRenderable* r = pool_terrainRenderable_allocate( static_renderable_pool ); 
	memset( r, 0, sizeof( terrainRenderable ));
	r->terrainShader = render_shaderByName("dat/shaders/terrain.s");
	r->block = b;
	return r;
}

void terrainRenderable_delete( terrainRenderable* r ) {
	canyonTerrain_freeElementBuffer( r->block->terrain, r->element_buffer );
	canyonTerrain_freeVertexBuffer( r->block->terrain, (vertex*)r->vertex_buffer );
	pool_terrainRenderable_free( static_renderable_pool, r );
}
