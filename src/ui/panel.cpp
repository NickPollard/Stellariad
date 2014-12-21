// panel.c

#include "common.h"
#include "panel.h"
//-------------------------
#include "model.h"
#include "particle.h"
#include "maths/vector.h"
#include "mem/allocator.h"
#include "render/drawcall.h"
#include "render/shader.h"
#include "render/texture.h"
#include "system/hash.h"

IMPLEMENT_LIST(animator);

vector ui_color_default = {{ 0.f, 0.5f, 0.7f, 0.5f }};

// *** Forward Declaratations
void panel_tick( void* pnl, float dt, engine* e );
// ***

// Create a Panel
panel* panel_create( engine* e ) {
	panel* p = (panel*)mem_alloc( sizeof( panel ));
	memset( p, 0, sizeof( panel ));

	// Default anchoring: tl to tl
	p->local_anchor = kTopLeft;
	p->remote_anchor = kTopLeft;

	// default UI color
	p->color = ui_color_default;
	p->visible = true;
	p->_texture = NULL;

	p->animators = NULL;

	startTick( e, p, panel_tick );
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
	vAssert( p->_texture );

	// We draw a quad as two triangles
	p->vertex_buffer[0].position = Vector( p->x,			p->y,				0.1f, 1.f );
	p->vertex_buffer[1].position = Vector( p->x + p->width,	p->y,				0.1f, 1.f );
	p->vertex_buffer[2].position = Vector( p->x,			p->y + p->height,	0.1f, 1.f );
	p->vertex_buffer[3].position = Vector( p->x + p->width,	p->y + p->height,	0.1f, 1.f );

	p->vertex_buffer[0].uv = Vec2( 0.f, 0.f );
	p->vertex_buffer[1].uv = Vec2( 1.f, 0.f );
	p->vertex_buffer[2].uv = Vec2( 0.f, 1.f );
	p->vertex_buffer[3].uv = Vec2( 1.f, 1.f );

	vector color = vector_scaled( p->color, p->color.coord.w );
	color.coord.w = p->color.coord.w;
	p->vertex_buffer[0].color = intFromVector(color);
	p->vertex_buffer[1].color = intFromVector(color);
	p->vertex_buffer[2].color = intFromVector(color);
	p->vertex_buffer[3].color = intFromVector(color);

	// Copy our data to the GPU
	// There are now <index_count> vertices, as we have unrolled them
	drawCall* draw = drawCall_create( &renderPass_ui, *Shader::byName( "dat/shaders/ui.s" ), element_count, element_buffer, p->vertex_buffer, p->_texture->gl_tex, modelview );
	draw->depth_mask = GL_FALSE;
}

void panel_render( void* panel_, scene* s ) {
	(void)s;
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

void panel_setAlpha( panel* p, float alpha ) {
	p->color.coord.w = alpha;
}

// *** An animateable property
bool animate( animator* a, float dt ) {
	a->time += dt;
	float f = property_samplef( a->_property, a->time );
	*(a->target) = f;
	return a->time > property_duration( a->_property );;
}

animator* newAnimator( property* p, float* target ) {
	animator* a = (animator*)mem_alloc( sizeof( animator ));
	a->time = 0.f;
	a->_property = p;
	a->target = target;
	return a;
}

void animator_delete( animator* a ) {
	mem_free( a );
}

void panel_tick( void* pnl, float dt, engine* e ) {
	(void)e;
	panel*p = (panel*)pnl;
	animatorlist* prev = NULL;
	for ( animatorlist* a = p->animators; a; ) {
		animatorlist* next = a->tail;
		if (animate( a->head, dt )) {
			if (prev) prev->tail = a->tail; else p->animators = a->tail; // Kill this one
			animator_delete( a->head );
			mem_free( a );
		}
		prev = a;
		a = next;
	}
}

void panel_addAnimator( panel* p, animator* a ) {
	p->animators = animatorlist_cons( a, p->animators );
}

void panel_fadeIn( panel* p, float overTime ) {
	property* prop = property_create( 2 );
	property_addf( prop, 0.f, 0.f );
	property_addf( prop, overTime, 1.f );
	animator* a = newAnimator( prop, &p->color.coord.w );
	panel_addAnimator( p, a );
}

void panel_fadeOut( panel* p, float overTime ) {
	property* prop = property_create( 2 );
	property_addf( prop, 0.f, 1.f );
	property_addf( prop, overTime, 0.f );
	animator* a = newAnimator( prop, &p->color.coord.w );
	panel_addAnimator( p, a );
}
