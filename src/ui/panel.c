// panel.c

#include "common.h"
#include "panel.h"
//-------------------------
#include "model.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/shader.h"
#include "render/texture.h"
#include "system/hash.h"

vector ui_color_default = {{ 0.f, 0.5f, 0.7f, 0.5f }};

// Create a Panel
panel* panel_create() {
	panel* p = mem_alloc( sizeof( panel ));
	memset( p, 0, sizeof( panel ));

	// Default anchoring: tl to tl
	p->local_anchor = kTopLeft;
	p->remote_anchor = kTopLeft;

	// default UI color
	p->color = ui_color_default;
	p->visible = true;
	p->texture = NULL;

	return p;
}

static GLushort element_buffer[] = {
	2, 1, 0,
	1, 2, 3
};
static const int element_count = 6;
static const int vert_count = 4;

// The draw function gets passed the current 'cursor' x and y
void panel_draw( panel* p, float x, float y ) {
	(void)x;
	(void)y;
	vAssert( p->texture );

	// We draw a quad as two triangles
	p->vertex_buffer[0].position = Vector( p->x,				p->y,				0.1f, 1.f );
	p->vertex_buffer[1].position = Vector( p->x + p->width,	p->y,				0.1f, 1.f );
	p->vertex_buffer[2].position = Vector( p->x,				p->y + p->height,	0.1f, 1.f );
	p->vertex_buffer[3].position = Vector( p->x + p->width,	p->y + p->height,	0.1f, 1.f );

	p->vertex_buffer[0].uv = Vector( 0.f, 0.f, 0.f, 0.f );
	p->vertex_buffer[1].uv = Vector( 1.f, 0.f, 0.f, 0.f );
	p->vertex_buffer[2].uv = Vector( 0.f, 1.f, 0.f, 0.f );
	p->vertex_buffer[3].uv = Vector( 1.f, 1.f, 0.f, 0.f );

	p->vertex_buffer[0].color = p->color;
	p->vertex_buffer[1].color = p->color;
	p->vertex_buffer[2].color = p->color;
	p->vertex_buffer[3].color = p->color;

	// Copy our data to the GPU
	// There are now <index_count> vertices, as we have unrolled them
	drawCall* draw = drawCall_create( &renderPass_ui, resources.shader_ui, element_count, element_buffer, p->vertex_buffer, p->texture->gl_tex, modelview );
	draw->depth_mask = GL_FALSE;
}

void panel_render( void* panel_ ) {
	panel_draw( (panel*)panel_, 0.f, 0.f );
}

void panel_hide( engine* e, panel* p ) {
	if ( p->visible )
		engine_removeRender( e, p, panel_render );
	p->visible = false;
}

void panel_show( engine* e, panel* p ) {
	if ( !p->visible )
		engine_addRender( e, p, panel_render );
	p->visible = true;
}
