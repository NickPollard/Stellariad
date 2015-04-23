// mathstypes.h
#pragma once

/****************************************************************************
   Universal math types (Vector, Matrix, Quaternion etc.) that are used 
   widely across Vitae modules
****************************************************************************/

/*
   Vector - a 4-dimensional vector of 32-bit floats (128bit total size).

   Vitae only supports 4-dimensional vectors for alignment and simplicity
   reasons. If you want a 3-dimensional vector, use a 4d one anyway. If
   you need to pack data as efficiently as possible, then use a custom
   packing type, but upcast to 4d vector for mathematical operations
   */

union vector_u {
  struct {
		float x;
		float y;
		float z;
		float w;
	} coord;
	float	val[4];
	struct {
		float x;
		float y;
		float z;
		float w;
	};
};

/*
   Matrix - a 4x4 Matrix

   Internally, we use matrices to pre-multiply, and so matrix is COLUMN MAJOR
   This means the first index specifies column, the second specifies row.
   m[0][2] means the value in the first column, third row.

   This means that column are stored contiguously in memory - if you think of the
   matrix as a 16 float buffer, then 0-3 = col 0, 4-7 = col 1, etc.
   A row is 4 floats that are each 4 * 32 = 128 bits apart.

   Translation values are in the 12th, 13th, 14th, 15th addresses of the buffer

   A typical matrix:

   on paper:
   { xx yx zx tx
   	 xy yy zy ty
	 xz yz zz tz
	 0  0  0  1  }

   in memory:
   { xx xy xz 0 yx yy yz 0 zx zy zz 0 tx ty tz 1 }

*/

typedef float matrix[4][4];
extern matrix matrix_identity;

/*
   Quaternion - a 4dimensional vector representing a quaternion, i.e. a point in 4-dimensional Hamiltonian Space

   Quaternions are used for rotation, where they specify an arbitrary rotation of (angle) around (axis).
   The quaternion stores the values as:
   x,y,z = sin (t/2) (x,y,z)
   s	= cos (t/2)

   (where t (theta) is the angle of rotation)

   The x,y,z elements are equivalent to the i,j,k elements in complex notation. The s element is the real part.
   */

struct quaternion_s {
	float s;
	float x;
	float y;
	float z;
};


/* vec2 - a 2d vector of 32-bit floats */
struct vec2 {
	float x;
	float y;
};
