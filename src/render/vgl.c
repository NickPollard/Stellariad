// vgl.c

#include "common.h"
#include "vgl.h"
//-----------------------
#include <assert.h>

// Build a new texture from the given bitmap buffer
// Plus properties
vglTexture vgl_buildTexture( const ubyte* img, int w, int h, int type, int format ) {
	vglTexture tex = 0;
   	glGenTextures( 1, &tex );
	assert( tex != 0 );
	glBindTexture( GL_TEXTURE_2D, tex );

	// Set up sampling parameters, use defaults for now
	// Bilinear interpolation, clamped
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE );

	glTexImage2D( GL_TEXTURE_2D,
			0,			// No Mipmaps for now
			type,	// 1-channel, 8-bits per channel (8-bit stride)
			(GLsizei)w, (GLsizei)h,
			0,			// Border, unused
			format,		// TGA uses BGR order internally
			GL_UNSIGNED_BYTE,	// 8-bits per channel
			img );
	return tex;
}

// Bind a texture, activating it for use
void vglBindTexture( vglTexture tex ) {
	glBindTexture( GL_TEXTURE_2D, tex );
}


