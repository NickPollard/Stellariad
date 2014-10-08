// debugdraw.c

#include "common.h"
#include "render/debugdraw.h"
//-----------------------
#include "maths/maths.h"
#include "maths/vector.h"
#include "render/render.h"

#include "render/vgl.h"

// The total number of verts per frame we can draw using the debug draw system
#define kMaxDebugDrawVerts 0x1 << 20

// Static buffers used to store debug draw verts and indices - these are wiped each frame
vertex debugDraw_vertex_buffer[kMaxDebugDrawVerts];
GLushort debugDraw_element_buffer[kMaxDebugDrawVerts];
int debugDraw_verts_used;

void debugdraw_line2d_gradient( vector from, vector to, vector color, vector color_to ) {
	// Grab a vertex and element buffer from our static ones
	int vert_count = 2;
	vertex* vertex_buffer = &debugDraw_vertex_buffer[debugDraw_verts_used];
	GLushort* element_buffer = &debugDraw_element_buffer[debugDraw_verts_used];
	debugDraw_verts_used += vert_count;
	vAssert( debugDraw_verts_used < kMaxDebugDrawVerts );

	vertex_buffer[0].position = from;
	vertex_buffer[1].position = to;
	vertex_buffer[0].color = color;
	vertex_buffer[1].color = color_to;

	element_buffer[0] = 0;
	element_buffer[1] = 1;

	// Make a drawcall
	const GLuint no_texture = 0;
	drawCall* draw = drawCall_create( &renderPass_debug, resources.shader_debug_2d, vert_count, element_buffer, vertex_buffer, no_texture, modelview );
	draw->elements_mode = GL_LINES;
}

void debugdraw_line2d( vector from, vector to, vector color ) {
	debugdraw_line2d_gradient( from, to, color, color );
}

void debugdraw_line3d( vector from, vector to, vector color ) {
	// Grab a vertex and element buffer from our static ones
	int vert_count = 2;
	vertex* vertex_buffer = &debugDraw_vertex_buffer[debugDraw_verts_used];
	GLushort* element_buffer = &debugDraw_element_buffer[debugDraw_verts_used];
	debugDraw_verts_used += vert_count;
	vAssert( debugDraw_verts_used < kMaxDebugDrawVerts );

	vertex_buffer[0].position = from;
	vertex_buffer[1].position = to;
	vertex_buffer[0].color = color;
	vertex_buffer[1].color = color;

	// TODO - this could be static const
	element_buffer[0] = 0;
	element_buffer[1] = 1;

	render_resetModelView();

	// Make a drawcall
	const GLuint no_texture = 0;
	drawCall* draw = drawCall_create( &renderPass_debug, resources.shader_debug, vert_count, element_buffer, vertex_buffer, no_texture, modelview );
	draw->elements_mode = GL_LINES;
}

// Draw a debug cross at the point *center*
void debugdraw_cross( vector center, float radius, vector color ) {
	vector from = vector_add( center, Vector( radius, 0.f, 0.f, 0.f ));
	vector to = vector_add( center, Vector( -radius, 0.f, 0.f, 0.f ));
	debugdraw_line3d( from, to, color );
	from = vector_add( center, Vector( 0.f, radius, 0.f, 0.f ));
	to = vector_add( center, Vector( 0.f, -radius, 0.f, 0.f ));
	debugdraw_line3d( from, to, color );
	from = vector_add( center, Vector( 0.f, 0.f, radius, 0.f ));
	to = vector_add( center, Vector( 0.f, 0.f, -radius, 0.f ));
	debugdraw_line3d( from, to, color );
}

void debugdraw_sphere( vector origin, float radius, vector color ) {
	// Grab a vertex and element buffer from our static ones
	int vert_count = 24;
	vertex* vertex_buffer = &debugDraw_vertex_buffer[debugDraw_verts_used];
	GLushort* element_buffer = &debugDraw_element_buffer[debugDraw_verts_used];
	debugDraw_verts_used += vert_count;
	vAssert( debugDraw_verts_used < kMaxDebugDrawVerts );

	// Set up verts
	vector offset = Vector( 0.f, radius, 0.f, 0.f );
	Add( &vertex_buffer[0].position, &origin, &offset );
	Sub( &vertex_buffer[1].position, &origin, &offset );
	offset = Vector( radius, 0.f, 0.f, 0.f );
	Add( &vertex_buffer[2].position, &origin, &offset );
	Sub( &vertex_buffer[3].position, &origin, &offset );
	offset = Vector( 0.f, 0.f, radius, 0.f );
	Add( &vertex_buffer[4].position, &origin, &offset );
	Sub( &vertex_buffer[5].position, &origin, &offset );

	// Colors
	vertex_buffer[0].color = color;
	vertex_buffer[1].color = color;
	vertex_buffer[2].color = color;
	vertex_buffer[3].color = color;
	vertex_buffer[4].color = color;
	vertex_buffer[5].color = color;

	// TODO - this could be static const
	element_buffer[0] = 0;
	element_buffer[1] = 2;
	element_buffer[2] = 2;
	element_buffer[3] = 1;
	element_buffer[4] = 1;
	element_buffer[5] = 3;
	element_buffer[6] = 3;
	element_buffer[7] = 0;

	element_buffer[8] = 0;
	element_buffer[9] = 4;
	element_buffer[10] = 4;
	element_buffer[11] = 1;
	element_buffer[12] = 1;
	element_buffer[13] = 5;
	element_buffer[14] = 5;
	element_buffer[15] = 0;

	element_buffer[16] = 4;
	element_buffer[17] = 2;
	element_buffer[18] = 2;
	element_buffer[19] = 5;
	element_buffer[20] = 5;
	element_buffer[21] = 3;
	element_buffer[22] = 3;
	element_buffer[23] = 4;

	render_resetModelView();

	// Make a drawcall
	const GLuint no_texture = 0;
	drawCall* draw = drawCall_create( &renderPass_debug, resources.shader_debug, vert_count, element_buffer, vertex_buffer, no_texture, modelview );
	draw->elements_mode = GL_LINES;
}

// Draw a wireframe mesh
void debugdraw_wireframeMesh( int vert_count, vector* verts, int index_count, uint16_t* indices, matrix trans, vector color ) {
	vAssert( index_count >= vert_count );
	// Grab a vertex and element buffer from our static ones
	vertex* vertex_buffer = &debugDraw_vertex_buffer[debugDraw_verts_used];
	GLushort* element_buffer = &debugDraw_element_buffer[debugDraw_verts_used];
	debugDraw_verts_used += index_count*2;
	vAssert( debugDraw_verts_used < kMaxDebugDrawVerts );

	for ( int i = 0; i < vert_count; ++i ) {
		vertex_buffer[i].position = verts[i];
		vertex_buffer[i].color = color;
	}

	// For each triangle (3 indices), we draw 3 lines (6 indices)
	for ( int j = 0; j < index_count; j+=3 ) {
		element_buffer[j*2+0] = indices[j+0];
		element_buffer[j*2+1] = indices[j+1];
		
		element_buffer[j*2+2] = indices[j+1];
		element_buffer[j*2+3] = indices[j+2];
	
		element_buffer[j*2+4] = indices[j+2];
		element_buffer[j*2+5] = indices[j+0];
	}

	render_resetModelView();
	matrix_mul( modelview, modelview, trans );

	// Make a drawcall
	const GLuint no_texture = 0;
	drawCall* draw = drawCall_create( &renderPass_debug, resources.shader_debug, index_count*2, element_buffer, vertex_buffer, no_texture, modelview );
	draw->elements_mode = GL_LINES;
}

void debugdraw_preTick( float dt ) {
	(void)dt;
	debugDraw_verts_used = 0;
}

	/*
void debugdraw_drawRect2D( vector* tl, vector* br ) {
	render_set2D();
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glPushMatrix();

	glBegin( GL_TRIANGLES );

	glColor4f( 1.f, 1.f, 1.f, 1.f );
	glNormal3f( 0.f, 0.f, 1.f );

	glTexCoord2f( tl->coord.z, tl->coord.w );
	glVertex3f( tl->coord.x, tl->coord.y, -0.5f );	

	glTexCoord2f( tl->coord.z, br->coord.w );
	glVertex3f( tl->coord.x, br->coord.y, -0.5f );	

	glTexCoord2f( br->coord.z, tl->coord.w );
	glVertex3f( br->coord.x, tl->coord.y, -0.5f );	

	glTexCoord2f( br->coord.z, tl->coord.w );
	glVertex3f( br->coord.x, tl->coord.y, -0.5f );	

	glTexCoord2f( tl->coord.z, br->coord.w );
	glVertex3f( tl->coord.x, br->coord.y, -0.5f );	

	glTexCoord2f( br->coord.z, br->coord.w );
	glVertex3f( br->coord.x, br->coord.y, -0.5f );	

	glEnd();
	glPopMatrix();
}
	*/
