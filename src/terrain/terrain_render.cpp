// terrain_render.c
#include "common.h"
#include "terrain_render.h"
//-----------------------
#include "canyon.h"
#include "camera.h"
#include "future.h"
#include "terrain_generate.h"
#include "base/array.h"
#include "maths/geometry.h"
#include "maths/vector.h"
#include "render/drawcall.h"
#include "render/shader.h"
#include "render/graphicsbuffer.h"
#include "render/texture.h"
#include "concurrent/future.h"

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

bool validIndex( CanyonTerrainBlock* b, int u, int v ) { return ( v >= 0 && v < b->v_samples && u >= 0 && u < b->u_samples ); }
// Not adjusted as this is for renderable verts only
int canyonTerrainBlock_renderIndexFromUV( CanyonTerrainBlock* b, int u, int v ) {
	vAssert( u >= 0 && u < b->u_samples ); vAssert( v >= 0 && v < b->v_samples );
	return u + v * b->u_samples;
}

int canyonTerrainBlock_triangleCount( CanyonTerrainBlock* b ) { return ( b->u_samples - 1 ) * ( b->v_samples - 1 ) * 2; }

vector calcUV(CanyonTerrainBlock* b, vector* v, float v_pos) {
	(void)b;
	(void)v_pos;
	return Vector( v->coord.x * texture_scale,
					v->coord.y * texture_scale,
					v->coord.z * texture_scale, 
					canyon_uvMapped( b->v_min * texture_scale, v_pos * texture_scale ));
}

bool canyonTerrainBlock_triangleInvalid( CanyonTerrainBlock* b, int u_index, int v_index, int u_offset, int v_offset ) {
	u_offset = u_offset / 2 + u_offset % 2;
	u_offset = min( u_offset, 0 );
	return ( u_index + u_offset >= b->u_samples - 1 ) ||
		( u_index + u_offset < 0 ) ||
		( v_index + v_offset >= b->v_samples - 1 ) ||
		( v_index + v_offset < 0 );
}

// As this is just renderable verts, we dont have the extra buffer space for normal generation
int canyonTerrainBlock_renderVertCount( CanyonTerrainBlock* b ) {
#if CANYON_TERRAIN_INDEXED
   	return ( b->u_samples ) * ( b->v_samples );
#else
	return b->element_count;
#endif // CANYON_TERRAIN_INDEXED
}

void canyonTerrain_renderInit() {
	static_renderable_pool = pool_terrainRenderable_create( PoolMaxBlocks );
	initialiseDefaultElementBuffer();
	//if ( !terrain_texture ) 		{ terrain_texture		= texture_load( "dat/img/terrain/grass.tga" ); }
	if ( !terrain_texture ) 		{ terrain_texture		= texture_load( "dat/img/terrain/cliff_normal.tga" ); }
	if ( !terrain_texture_cliff )	{ terrain_texture_cliff = texture_load( "dat/img/terrain/cliff_grass.tga" ); }
	if ( !terrain_texture_2 )		{ terrain_texture_2		= texture_load( "dat/img/terrain/ground_industrial.tga" ); }
	if ( !terrain_texture_cliff_2 )	{ terrain_texture_cliff_2 = texture_load( "dat/img/terrain/cliff_industrial.tga" ); }
}

void canyonTerrainBlock_positionsFromUV( CanyonTerrainBlock* b, int u_index, int v_index, float* u, float* v ) {
	int lod_ratio = b->lodRatio();
	if ( u_index == -1 ) u_index = -lod_ratio;
	if ( u_index == b->u_samples ) u_index = b->u_samples -1 + lod_ratio;
	if ( v_index == -1 ) v_index = -lod_ratio;
	if ( v_index == b->v_samples ) v_index = b->v_samples -1 + lod_ratio;

	CanyonTerrain* t = b->terrain;

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
void canyonTerrainBlock_fillTrianglesForVertex( CanyonTerrainBlock* b, vector* positions, vertex* vertices, int u_index, int v_index, vertex* vert ) {
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

// Create GPU vertex buffer objects to hold our data and save transferring to the GPU each frame
// If we've already allocated a buffer at some point, just re-use it
brando::concurrent::Future<bool> terrainBlock_initVBO( CanyonTerrainBlock* b ) {
	int vert_count = canyonTerrainBlock_renderVertCount( b );
	terrainRenderable* r = b->renderable;
	r->vertex_VBO_alt	= render_requestBuffer( GL_ARRAY_BUFFER,			r->vertex_buffer,	sizeof( vertex )	* vert_count );
	r->element_VBO_alt	= render_requestBuffer( GL_ELEMENT_ARRAY_BUFFER, 	r->element_buffer,	sizeof( GLushort ) 	* r->element_count );
	return b->ready();
}

bool canyonTerrainBlock_render( CanyonTerrainBlock* b, scene* s ) {
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
		drawCall* draw = drawCall::create( &renderPass_main, *r->terrainShader, r->element_count_render, r->element_buffer, r->vertex_buffer, b->_canyon->zones[first].texture_ground->gl_tex, modelview );
		draw->texture_b = b->_canyon->zones[first].texture_cliff->gl_tex;
		draw->texture_c = b->_canyon->zones[second].texture_ground->gl_tex;
		draw->texture_d = b->_canyon->zones[second].texture_cliff->gl_tex;
		draw->texture_normal = terrain_texture->gl_tex;
		draw->texture_b_normal = terrain_texture->gl_tex;
		draw->vertex_VBO = *r->vertex_VBO;
		draw->element_VBO = *r->element_VBO;

		drawCall* drawDepth = drawCall::create( &renderPass_depth, *Shader::depth(), r->element_count_render, r->element_buffer, r->vertex_buffer, b->_canyon->zones[first].texture_ground->gl_tex, modelview );
		drawDepth->vertex_VBO = *r->vertex_VBO;
		drawDepth->element_VBO = *r->element_VBO;
	}
	return true;
}

void canyonTerrain_render( void* data, scene* s ) {
	(void)s;
	CanyonTerrain* t = (CanyonTerrain*)data;
	render_resetModelView();
	matrix_mulInPlace( modelview, modelview, t->trans->world );

	int count = 0;
	for ( int i = 0; i < t->total_block_count; ++i ) {
		if ( t->blocks[i] )
			count += canyonTerrainBlock_render( t->blocks[i], s );
	}
	//printf( "Rendering %d out of %d blocks.\n", count, t->total_block_count );
}

void canyonTerrainBlock_generateVertices( CanyonTerrainBlock* b, vector* verts, vector* normals ) {
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
				vector uv = calcUV( b, &verts[i], v );
				r->vertex_buffer[buffer_index].uv = Vec2( uv.coord.x, uv.coord.y );
				r->vertex_buffer[buffer_index].color = intFromVector(Vector( canyonZone_terrainBlend( v ), 0.f, 0.f, 1.f ));
				r->vertex_buffer[buffer_index].normal = normals[i];
				r->vertex_buffer[buffer_index].normal.coord.w = uv.coord.z;
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

void canyonTerrainBlock_createBuffers( CanyonTerrainBlock* b ) {
	terrainRenderable* r = b->renderable;
	r->element_count = canyonTerrainBlock_triangleCount( b ) * 3;
	vAssert( r->element_count > 0 );
	vAssert( r->element_count <= kMaxTerrainBlockElements );

#if CANYON_TERRAIN_INDEXED
	if ( !r->element_buffer ) r->element_buffer = b->terrain->elementBuffers.takeBuffer();
#else
	r->element_buffer = elementBuffer;
#endif // CANYON_TERRAIN_INDEXED
	if ( !r->vertex_buffer ) r->vertex_buffer = b->terrain->vertexBuffers.takeBuffer();
}

terrainRenderable* terrainRenderable_create( CanyonTerrainBlock* b ) {
	terrainRenderable* r = pool_terrainRenderable_allocate( static_renderable_pool ); 
	memset( r, 0, sizeof( terrainRenderable ));
	r->terrainShader = Shader::byName("dat/shaders/terrain.s");
	r->block = b;
	return r;
}

void terrainRenderable_delete( terrainRenderable* r ) {
  r->block->terrain->elementBuffers.releaseBuffer( r->element_buffer );
  r->block->terrain->vertexBuffers.releaseBuffer( r->vertex_buffer );

	pool_terrainRenderable_free( static_renderable_pool, r );
}
