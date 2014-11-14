// gamecamera.c

void gamecamera_input( gamecamera* c, input* i ) {
	c->input.translation.x = 1.f * input_get( i, CAMERA_TRANSLATE_X );
	c->input.translation.y = 1.f * input_get( i, CAMERA_TRANSLATE_Y );
	c->input.translation.z = 1.f * input_get( i, CAMERA_TRANSLATE_Z );
}

void gamecamera_update( gamecamera* c, float dt ) {
	if ( c->camera ) {
		camera_translate( c->camera, c->input.translation );
		camera_rotate( c->camera, c->input.rotation );
	}
}

gamecamera* gamecamera_create() {
	gamecamera* c = mem_alloc( sizeof( gamecamera ) );
	memset( c, 0, sizeof( gamecamera ) );
	return c;
}
