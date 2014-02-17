// touch.c
#include "common.h"
#include "touch.h"
//---------------------
#include "engine.h"
#include "input.h"
#include "maths/maths.h"
#include "maths/vector.h"
#include "mem/allocator.h"

#ifdef TOUCH
// *** Forward declarations
int touchPad_localX( touchPad* p, int x );
int touchPad_localY( touchPad* p, int y );
void touchPad_tick( touchPad* p, input* in, float dt );

// *** Implementation

// Finds the index of the (first) touch with the given uid
// returns -1 if not found
int touch_indexOfUid( touch* touches, int count, int uid ) {
	int index = kInvalidIndex;
	for ( int i = 0; i < count; ++i ) {
		if ( touches[i].uid == uid ) {
			index = i;
			break;
		}
	}
	return index;
}

void input_registerTouch( input* in, int uid, int x, int y, enum touchAction action ) {
	// Find first available touch slot	
	int i = touch_indexOfUid( &in->touch.touches[0], kMaxMultiTouch, kInvalidTouchUid );
	if ( i == kInvalidIndex ) {
		printf( "Cannot find an empty Touch\n" );
		for ( int j = 0; i < kMaxMultiTouch; ++j ) {
			printf( "Touch: %d, uid: %d.\n", j, in->touch.touches[j].uid );
		}
	}
	vAssert( i != kInvalidIndex );

	if ( action == kTouchDown ) {
		//printf( "Touch down: %d.\n", uid );
	}

	touch* t = &(in->touch.touches[i]);
	t->x = x;
	t->y = y;
	t->uid = uid;

	t->held =		( action != kTouchUp );
	t->pressed =	( action == kTouchDown );
	t->released =	( action == kTouchUp );
}

void input_getTouchDrag( input* in, int pointer, int* x, int* y ) {
	*x = 0;
	*y = 0;
	if ( in->data[in->active].touch.touches[pointer].held && in->data[in->active ^ 0x1].touch.touches[pointer].held ) {
		*x = in->data[in->active].touch.touches[pointer].x - in->data[in->active ^ 0x1].touch.touches[pointer].x;
		*y = in->data[in->active].touch.touches[pointer].y - in->data[in->active ^ 0x1].touch.touches[pointer].y;
	}
}

bool input_touchPressedInternal( input* in, int i ) {
	return ( in->data[in->active].touch.touches[i].held && !(in->data[in->active ^ 0x1].touch.touches[i].held) );
}

bool input_touchPressed( input* in, int x_min, int y_min, int x_max, int y_max ) {
	if ( x_min < 0 ) x_min += in->w;
	if ( y_min < 0 ) y_min += in->h;
	if ( x_max < 0 ) x_max += in->w;
	if ( y_max < 0 ) y_max += in->h;

	bool touched = false;
	int num_touches = 1; // TODO
	for ( int i = 0; i < num_touches && !touched; ++i ) {
		int x = in->data[in->active].touch.touches[i].x;
		int y = in->data[in->active].touch.touches[i].y;
		touched = input_touchPressedInternal( in, i ) &&
					contains( x, x_min, x_max ) &&
					contains( y, y_min, y_max );
	}
	return touched;
}

bool input_touchHeldInternal( input* in, int i ) {
	return in->data[in->active].touch.touches[i].held;
}

bool input_touchValid( input* in, int i ) {
	return in->data[in->active].touch.touches[i].uid != kInvalidTouchUid;
}

// TODO - this should take into account multi-touch, i.e. if ANY touch has occurred
bool input_touchHeld( input* in, int x_min, int y_min, int x_max, int y_max ) {
	if ( x_min < 0 ) x_min += in->w;
	if ( y_min < 0 ) y_min += in->h;
	if ( x_max < 0 ) x_max += in->w;
	if ( y_max < 0 ) y_max += in->h;

	bool held = false;
	for ( int i = 0; i < kMaxMultiTouch && !held; ++i ) {
		touch* t = &in->data[in->active].touch.touches[i];
		int x = t->x;
		int y = t->y;
		held = input_touchValid( in, i ) && input_touchHeldInternal( in, i ) &&
				contains( x, x_min, x_max ) &&
				contains( y, y_min, y_max );
	}
	return held;
}

bool input_touchInsideBounds( input* in, touch* t, int x_min, int y_min, int x_max, int y_max ) {
	if ( x_min < 0 ) x_min += in->w;
	if ( y_min < 0 ) y_min += in->h;
	if ( x_max < 0 ) x_max += in->w;
	if ( y_max < 0 ) y_max += in->h;

	bool inside = contains( t->x, x_min, x_max ) && contains( t->y, y_min, y_max );
	return inside;
}

void input_touchTick( input* in, float dt ) {
	(void)dt;
	// TODO - probably need to sync this properly with Android input thread?

	/*
		We have the previous frames touches (in->data[in->inactive].touch)
		We also have the pending touches (in->touch)

		We use both of these to construct the new frame
		Pending touches take precedence, everything else is filled in from the old
	   */
	
	float time = timer_getGameTimeSeconds( in->timer );

	// Copy the old inputs into our new frame
	// Anything with an invalid UID is ignored
	// Anything that was previously released, we remove
	// This gives us the touches that are still alive
	int active_touches = 0;
	touch* new = &in->data[in->active].touch.touches[0];
	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		touch* old = &in->data[1 - in->active].touch.touches[i];
		if ( old->uid != kInvalidTouchUid && old->released ) {
			//printf( "touch %d released\n", old->uid );
		}
		if ( old->uid != kInvalidTouchUid && !old->released ) {
			*new = *old;
			new->drag_x = 0.f;
			new->drag_y = 0.f;
			new->time = time;
			++new;
			++active_touches;
		}
	}

	// Copy the pending inputs into our new frame 
	// If a touch with the same UID exists, we overwrite and update it
	// If there is no existing touch with that UID, we add a new one
	// (this should be the case for any new presses
	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		touch* pending = &in->touch.touches[i];
		// Skip invalid touches
		if ( pending->uid == kInvalidTouchUid )
			continue;

		int existing_touch_index = touch_indexOfUid( &in->data[in->active].touch.touches[0], kMaxMultiTouch, pending->uid );
		if ( existing_touch_index == kInvalidIndex ) {
			// It's a new touch, should be a new press
			//vAssert( pending->pressed == true );
			// Add it on the end
			vAssert( active_touches < kMaxMultiTouch );
			touch* new = &in->data[in->active].touch.touches[active_touches];
			*new = *pending;
			new->time = time;
			++active_touches;
			//printf( "New touch received\n" );
		}
		else {
			touch* old = &in->data[in->active].touch.touches[existing_touch_index];
			int drag_x = pending->x - old->x;
			int drag_y = pending->y - old->y;
			*old = *pending;
			old->drag_x = drag_x;
			old->drag_y = drag_y;
			old->time = time;
			//printf( "Old touch update received\n" );
		}
	}
	//printf( "Total touches: %d.\n", active_touches );

	// NULL out the old touches no longer used
	for ( int i = active_touches; i < kMaxMultiTouch; ++i )
		in->data[in->active].touch.touches[i].uid = kInvalidTouchUid;

	// Clear out pending touches for next frame
	for ( int i = 0; i < kMaxMultiTouch; ++i )
		in->touch.touches[i].uid = kInvalidTouchUid;

	
	// TODO
	// Calculate drags

	// keep last touched state - we only revert to false if we receive a release event

	touchPanel_tick( &in->touch, in, dt );
}

typedef struct touchHistory_s {
	int	count;
	touch* data;
} touchHistory;

gesture* gesture_create( float distance, float duration, vector direction, float angle_tolerance ) {
	gesture* g = mem_alloc( sizeof( gesture ));
	memset( g, 0, sizeof( gesture ));
	g->distance = distance;
	g->duration = duration;
	g->direction = direction;
	g->angle_tolerance = angle_tolerance;
	return g;
}

bool gestureRecogniser( gesture* target, gesture* candidate ) {
	//printf( "Recogniser gesture: distance %.2f, duration %.2f, tolerance %.2f\n", target->distance, target->duration, target->angle_tolerance );
	//vector_printf( "Direction: ", &target->direction );
	bool performed = candidate->distance >= target->distance &&
					candidate->duration <= target->duration &&
					Dot( &candidate->direction, &target->direction ) > ( 1.f - target->angle_tolerance );
	return performed;
}

touch touchHistory_firstFrame( touchHistory h ) {
	touch t = h.data[h.count - 1];
	return t;
}

touch touchHistory_lastFrame( touchHistory h ) {
	touch t = h.data[0];
	return t;
}

gesture gestureCandidate( touchHistory h ) {
	gesture g;
	touch last_frame = touchHistory_lastFrame( h );
	touch first_frame = touchHistory_firstFrame( h );
	//printf( "GestureCandidate: First frame time %.2f, last frame time %.2f\n", first_frame.time, last_frame.time );
	vector first_pos = Vector( first_frame.x, first_frame.y, 0.f, 1.f );
	vector last_pos = Vector( last_frame.x, last_frame.y, 0.f, 1.f );
	vector travel = vector_sub( last_pos, first_pos );
	g.distance = vector_length( &travel );
	g.direction = normalized( travel );
	g.duration = last_frame.time - first_frame.time;
	//printf( "Gesture candidate: distance %.2f, duration %.2f, direction.", g.distance, g.duration );
	//vector_print( &g.direction );
	//printf( "\n" );
	return g;
}

touchHistory touchPad_touchHistory( touchPad* p, int touch, int history_frames ) {
	touchHistory h;
	h.count = history_frames;
	h.data = p->touch_history[ touch ];
	return h;
}

bool input_gesturePerformed( touchPad* p, gesture* g ) {
	bool performed = false;
	for ( int touch = 0; touch < kMaxMultiTouch; ++touch ) {
		if ( p->touches[touch].uid != kInvalidTouchUid ) {
			//printf( "Calculating touch history for UID %d\n", p->touches[touch].uid );
			for ( int i = 1; i < kMaxTouchFramesHistory; ++i ) {
				if ( p->touch_history[touch][i].uid == kInvalidTouchUid ) {
					break;
				}
				touchHistory history = touchPad_touchHistory( p, touch, i );
				gesture candidate = gestureCandidate( history );
				performed |= gestureRecogniser( g, &candidate );
				if ( candidate.duration > g->duration )
					goto found;
			}
		}
	}
found:
	return performed;
}

// ***************
// Touch Panels
void touchPanel_init( touchPanel* p ) {
	memset( p, 0, sizeof( touchPanel ));
	p->touch_pad_count = 0;
	p->touch_pad = mem_alloc( sizeof( touchPad* ) * kMaxTouchPads );
	p->touch_count = 0;
	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		p->touches[i].uid = kInvalidTouchUid;
	}
}

void touchPanel_tick( touchPanel* panel, input* in, float dt ) {
	for_each( panel->touch_pad, panel->touch_pad_count, touchPad_tick, in, dt );
}

// ***************
// Touch Pads

touchPad* touchPad_create( int x, int y, int w, int h ) {
	touchPad* p = mem_alloc( sizeof( touchPad ));
	p->x = x; p->y = y;
	p->width = w; p->height = h;
	p->active = true;

	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		p->touches[i].uid = kInvalidTouchUid;
		for ( int j = 0; j < kMaxTouchFramesHistory; ++j ) {
			p->touch_history[i][j].uid = kInvalidTouchUid;
			p->touch_history[i][j].time = 0.f;
		}
	}

	return p;
}

touchPad* touchPanel_addTouchPad( touchPanel* panel, touchPad* pad ) {
	array_add( (void**)panel->touch_pad, &panel->touch_pad_count, pad );
	return pad;
}

void touchPanel_removeTouchPad( touchPanel* panel, touchPad* pad ) {
	array_remove( (void**)panel->touch_pad, &panel->touch_pad_count, pad );
}

int touchPad_findHistoryIndexWithUID( touchPad* p, int uid ) {
	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		if ( p->touch_history[i][1].uid == uid )
			return i;
	}
	return -1;
}

void touchPad_tick( touchPad* p, input* in, float dt ) {
	(void)dt;

	// Update History
	// Push all frames back one
	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		for ( int j = kMaxTouchFramesHistory - 1; j > 0; --j ) {
			p->touch_history[i][j] = p->touch_history[i][j - 1];
		}
	}

	// Grab touches for this frame
	int count = 0;
	touch* local_touch = &p->touches[0];
	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		touch* global_touch = &in->data[in->active].touch.touches[i];
		if ( input_touchValid( in, i ) && input_touchInsideBounds( in, global_touch, p->x, p->y, p->x + p->width, p->y + p->height )) {
			// copy all touch data, but then update positions to pad space
			*local_touch = *global_touch;
			local_touch->x = touchPad_localX( p, global_touch->x );	
			local_touch->y = touchPad_localY( p, global_touch->y );	
			++local_touch;
			++count;
		}
	}
	//printf( "touchpad has %d touches.\n", count );

	// Init new history to defaults
	touch new_history[kMaxMultiTouch][kMaxTouchFramesHistory];
	memset( new_history, 0, sizeof( touch ) * kMaxMultiTouch * kMaxTouchFramesHistory );
	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		for ( int j = 0; j < kMaxTouchFramesHistory; ++j ) {
			new_history[i][j].uid = kInvalidTouchUid;
			new_history[i][j].time = 0.f;
		}
	}

	// Try to sync up history with current touches, by UID
	for ( int i = 0; i < count; ++i ) {
		int uid = p->touches[i].uid;
		int history_index = touchPad_findHistoryIndexWithUID( p, uid );
		if ( history_index != -1 ) {
			memcpy( new_history[i], p->touch_history[history_index], sizeof( touch ) * kMaxTouchFramesHistory );
		}
		else {
			// Mark as invalid
			for ( int j = 0; j < kMaxTouchFramesHistory; ++j ) {
				new_history[i][j].uid = kInvalidTouchUid;
				new_history[i][j].time = 0.f;
			}
		}
		new_history[i][0] = p->touches[i];
	}

	// Copy new histories back over old
	for ( int i = 0; i < kMaxMultiTouch; ++i ) {
		memcpy( p->touch_history[i], new_history[i], sizeof( touch ) * kMaxTouchFramesHistory );
	}

	// Blank out unused ones
	while ( local_touch < &p->touches[kMaxMultiTouch] ) {
		local_touch->x = -1;
		local_touch->y = -1;
		local_touch->uid = kInvalidTouchUid;
		++local_touch;
	}
}

bool touchPad_dragged( touchPad* p, int* x, int* y ) {
	*x = p->touches[0].drag_x;
	*y = p->touches[0].drag_y;
	return ( p->touches[0].uid != kInvalidTouchUid ) &&
			!p->touches[0].pressed && 
			!p->touches[0].released;
}

bool touchPad_touched( touchPad* p, int* x, int* y ) {
	*x = p->touches[0].x;
	*y = p->touches[0].y;
	return p->touches[0].uid != kInvalidTouchUid;
}

void touchPad_activate( touchPad* p ) {
	p->active = true;
}

void touchPad_deactivate( touchPad* p ) {
	p->active = false;
}

int touchPad_localX( touchPad* p, int x ) {
	return ( x - p->x );
}

int touchPad_localY( touchPad* p, int y ) {
	return ( y - p->y );
}

#endif // TOUCH
