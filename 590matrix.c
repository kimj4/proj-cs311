/*
 * 520matrix.c
 * Ju Yun Kim
 * Carleton College
 * CS 311
 * altered to play nicely with OpenGL
 * now includes function to convert GLdouble matrices into GLfloat matrices
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*** 2 x 2 Matrices ***/

/* Pretty-prints the given matrix, with one line of text per row of matrix. */
void mat22Print(GLdouble m[2][2]) {
	printf("%f   %f\n%f   %f\n", m[0][0], m[0][1], m[1][0], m[1][1]);
}

/* Returns the determinant of the matrix m. If the determinant is 0.0, then the
matrix is not invertible, and mInv is untouched. If the determinant is not 0.0,
then the matrix is invertible, and its inverse is placed into mInv. */
GLdouble mat22Invert(GLdouble m[2][2], GLdouble mInv[2][2]) {
	GLdouble determinant = (m[0][0] * m[1][1]) - (m[0][1] * m[1][0]);
	if (determinant != 0.0) {
		mInv[0][0] = (1 / determinant) * m[1][1];
		mInv[0][1] = (1 / determinant) * m[0][1] * (-1);
		mInv[1][0] = (1 / determinant) * m[1][0] * (-1);
		mInv[1][1] = (1 / determinant) * m[0][0];
	}
	return determinant;
}

/* Multiplies a 2x2 matrix m by a 2-column v, storing the result in mTimesV.
The output should not */
void mat221Multiply(GLdouble m[2][2], GLdouble v[2], GLdouble mTimesV[2]) {
	mTimesV[0] = (m[0][0] * v[0]) + (m[0][1] * v[1]);
	mTimesV[1] = (m[1][0] * v[0]) + (m[1][1] * v[1]);
}

/* Fills the matrix m from its two columns. */
void mat22Columns(GLdouble col0[2], GLdouble col1[2], GLdouble m[2][2]) {
	m[0][0] = col0[0];
	m[0][1] = col1[0];
	m[1][0] = col0[1];
	m[1][1] = col1[1];
}

/*** 2 x 2 Matrices ***/

/* Pretty-ish print for 3x3 matrices*/
void mat33Print(GLdouble m[3][3]) {
  printf("%f  %f  %f  \n%f  %f  %f  \n%f  %f  %f  \n", m[0][0], m[0][1], m[0][2],
                                                       m[1][0], m[1][1], m[1][2],
                                                       m[2][0], m[2][1], m[2][2]);
}

void mat44Print(GLdouble m[4][4]) {
	printf("%f  %f  %f  %f\n%f  %f  %f  %f\n%f  %f  %f  %f\n%f  %f  %f  %f\n",
																											 m[0][0], m[0][1], m[0][2], m[0][3],
																											 m[1][0], m[1][1], m[1][2], m[1][3],
																											 m[2][0], m[2][1], m[2][2], m[2][3],
																										 	 m[3][0], m[3][1], m[3][2], m[3][3]);

}

/* Multiplies the 3x3 matrix m by the 3x3 matrix n. */
void mat333Multiply(GLdouble m[3][3], GLdouble n[3][3], GLdouble mTimesN[3][3]) {
  mTimesN[0][0] = (m[0][0] * n[0][0]) + (m[0][1] * n[1][0]) + (m[0][2] * n[2][0]);
  mTimesN[0][1] = (m[0][0] * n[0][1]) + (m[0][1] * n[1][1]) + (m[0][2] * n[2][1]);
  mTimesN[0][2] = (m[0][0] * n[0][2]) + (m[0][1] * n[1][2]) + (m[0][2] * n[2][2]);

  mTimesN[1][0] = (m[1][0] * n[0][0]) + (m[1][1] * n[1][0]) + (m[1][2] * n[2][0]);
  mTimesN[1][1] = (m[1][0] * n[0][1]) + (m[1][1] * n[1][1]) + (m[1][2] * n[2][1]);
  mTimesN[1][2] = (m[1][0] * n[0][2]) + (m[1][1] * n[1][2]) + (m[1][2] * n[2][2]);

  mTimesN[2][0] = (m[2][0] * n[0][0]) + (m[2][1] * n[1][0]) + (m[2][2] * n[2][0]);
  mTimesN[2][1] = (m[2][0] * n[0][1]) + (m[2][1] * n[1][1]) + (m[2][2] * n[2][1]);
  mTimesN[2][2] = (m[2][0] * n[0][2]) + (m[2][1] * n[1][2]) + (m[2][2] * n[2][2]);
}

/* Multiplies the 3x3 matrix m by the 3x1 matrix v. */
void mat331Multiply(GLdouble m[3][3], GLdouble v[3], GLdouble mTimesV[3]) {
  mTimesV[0] = (m[0][0] * v[0]) + (m[0][1] * v[1]) + (m[0][2] * v[2]);
  mTimesV[1] = (m[1][0] * v[0]) + (m[1][1] * v[1]) + (m[1][2] * v[2]);
  mTimesV[2] = (m[2][0] * v[0]) + (m[2][1] * v[1]) + (m[2][2] * v[2]);
}

/* Builds a 3x3 matrix representing 2D rotation and translation in homogeneous
coordinates. More precisely, the transformation first rotates through the angle
theta (in radians, counterclockwise), and then translates by the vector (x, y).
*/
void mat33Isometry(GLdouble theta, GLdouble x, GLdouble y, GLdouble isom[3][3]) {
    isom[0][0] = cos(theta);
    isom[0][1] = -sin(theta);
    isom[0][2] = x;
    isom[1][0] = sin(theta);
    isom[1][1] = cos(theta);
    isom[1][2] = y;
    isom[2][0] = 0;
    isom[2][1] = 0;
    isom[2][2] = 1;
}

/* function to set the input 3x3 matrix into the 3x3 identity matrix*/
void mat33Identity(GLdouble identity[3][3]) {
  identity[0][0] = 1.0;
  identity[0][1] = 0.0;
  identity[0][2] = 0.0;
  identity[1][0] = 0.0;
  identity[1][1] = 1.0;
  identity[1][2] = 0.0;
  identity[2][0] = 0.0;
  identity[2][1] = 0.0;
  identity[2][2] = 1.0;
}

/* function to set the input 4x4 matrix into the 4x4 identity matrix*/
void mat44Identity(GLdouble I[4][4]) {
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			if (i == j) {
				I[i][j] = 1.0;
			} else {
				I[i][j] = 0.0;
			}
		}
	}
}

/*helper function to set U */
void mat33SetU(GLdouble axis[3], GLdouble u[3][3]) {
  u[0][0] = 0;
  u[0][1] = -axis[2];
  u[0][2] = axis[1];
  u[1][0] = axis[2];
  u[1][1] = 0;
  u[1][2] = -axis[0];
  u[2][0] = -axis[1];
  u[2][1] = axis[0];
  u[2][2] = 0;
}

/* Given a length-1 3D vector axis and an angle theta (in radians), builds the
rotation matrix for the rotation about that axis through that angle. Based on
Rodrigues' rotation formula R = I + (sin alpha) U + (1 - cos alpha) U^2. */
void mat33AngleAxisRotation(GLdouble alpha, GLdouble axis[3], GLdouble rot[3][3]) {
  GLdouble u[3][3]; // temp matrix to hold u
  mat33SetU(axis, u);

  GLdouble uSquared[3][3]; // temp matrix to hold uu
  mat333Multiply(u, u, uSquared);

  GLdouble I[3][3]; // just holds the identity matrix
  mat33Identity(I);

  int i, j;
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j ++) {
      rot[i][j] = I[i][j] + (sin(alpha) * u[i][j]) + ((1 - cos(alpha)) * uSquared[i][j]);
    }
  }
}

/* takes 3 column vectors and lays them side by side in the matrix*/
void mat33Columns(GLdouble a[3], GLdouble b[3], GLdouble c[3], GLdouble abc[3][3]) {
	abc[0][0] = a[0];
	abc[0][1] = b[0];
	abc[0][2] = c[0];
	abc[1][0] = a[1];
	abc[1][1] = b[1];
	abc[1][2] = c[1];
	abc[2][0] = a[2];
	abc[2][1] = b[2];
	abc[2][2] = c[2];
}

void mat33Transpose(GLdouble a[3][3], GLdouble aTransposed[3][3]) {
	int i,j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			aTransposed[i][j] = a[j][i];
		}
 	}
}

/* Given two length-1 3D vectors u, v that are perpendicular to each other.
Given two length-1 3D vectors a, b that are perpendicular to each other. Builds
the rotation matrix that rotates u to a and v to b. */
void mat33BasisRotation(GLdouble u[3], GLdouble v[3], GLdouble a[3], GLdouble b[3],
        GLdouble rot[3][3]) {

      GLdouble R[3][3];
      GLdouble w[3]; // w = u x v
			vec3Cross(u, v, w);
			mat33Columns(u, v, w, R);
			GLdouble RTransposed[3][3];
			mat33Transpose(R, RTransposed);
			GLdouble aCrossB[3];
			vec3Cross(a, b, aCrossB);
      GLdouble S[3][3];
			mat33Columns(a, b, aCrossB, S);
			mat333Multiply(S, RTransposed, rot);
}

void mat44Copy(GLdouble start[4][4], GLdouble end[4][4]) {
	int i, j;
	for (i = 0; i < 4; i ++) {
		for (j = 0; j < 4; j ++) {
			end[i][j] = start[i][j];
		}
	}
}

/* Multiplies m by n, placing the answer in mTimesN. */
void mat444Multiply(GLdouble m[4][4], GLdouble n[4][4], GLdouble mTimesN[4][4]) {
  mTimesN[0][0] = (m[0][0] * n[0][0]) + (m[0][1] * n[1][0]) + (m[0][2] * n[2][0]) + (m[0][3] * n[3][0]);
  mTimesN[0][1] = (m[0][0] * n[0][1]) + (m[0][1] * n[1][1]) + (m[0][2] * n[2][1]) + (m[0][3] * n[3][1]);
  mTimesN[0][2] = (m[0][0] * n[0][2]) + (m[0][1] * n[1][2]) + (m[0][2] * n[2][2]) + (m[0][3] * n[3][2]);
  mTimesN[0][3] = (m[0][0] * n[0][3]) + (m[0][1] * n[1][3]) + (m[0][2] * n[2][3]) + (m[0][3] * n[3][3]);

  mTimesN[1][0] = (m[1][0] * n[0][0]) + (m[1][1] * n[1][0]) + (m[1][2] * n[2][0]) + (m[1][3] * n[3][0]);
  mTimesN[1][1] = (m[1][0] * n[0][1]) + (m[1][1] * n[1][1]) + (m[1][2] * n[2][1]) + (m[1][3] * n[3][1]);
  mTimesN[1][2] = (m[1][0] * n[0][2]) + (m[1][1] * n[1][2]) + (m[1][2] * n[2][2]) + (m[1][3] * n[3][2]);
  mTimesN[1][3] = (m[1][0] * n[0][3]) + (m[1][1] * n[1][3]) + (m[1][2] * n[2][3]) + (m[1][3] * n[3][3]);

  mTimesN[2][0] = (m[2][0] * n[0][0]) + (m[2][1] * n[1][0]) + (m[2][2] * n[2][0]) + (m[2][3] * n[3][0]);
  mTimesN[2][1] = (m[2][0] * n[0][1]) + (m[2][1] * n[1][1]) + (m[2][2] * n[2][1]) + (m[2][3] * n[3][1]);
  mTimesN[2][2] = (m[2][0] * n[0][2]) + (m[2][1] * n[1][2]) + (m[2][2] * n[2][2]) + (m[2][3] * n[3][2]);
  mTimesN[2][3] = (m[2][0] * n[0][3]) + (m[2][1] * n[1][3]) + (m[2][2] * n[2][3]) + (m[2][3] * n[3][3]);

  mTimesN[3][0] = (m[3][0] * n[0][0]) + (m[3][1] * n[1][0]) + (m[3][2] * n[2][0]) + (m[3][3] * n[3][0]);
  mTimesN[3][1] = (m[3][0] * n[0][1]) + (m[3][1] * n[1][1]) + (m[3][2] * n[2][1]) + (m[3][3] * n[3][1]);
  mTimesN[3][2] = (m[3][0] * n[0][2]) + (m[3][1] * n[1][2]) + (m[3][2] * n[2][2]) + (m[3][3] * n[3][2]);
  mTimesN[3][3] = (m[3][0] * n[0][3]) + (m[3][1] * n[1][3]) + (m[3][2] * n[2][3]) + (m[3][3] * n[3][3]);

}

/* Multiplies m by v, placing the answer in mTimesV. */
void mat441Multiply(GLdouble m[4][4], GLdouble v[4], GLdouble mTimesV[4]) {
  mTimesV[0] = (m[0][0] * v[0]) + (m[0][1] * v[1]) + (m[0][2] * v[2]) + (m[0][3] * v[3]);
  mTimesV[1] = (m[1][0] * v[0]) + (m[1][1] * v[1]) + (m[1][2] * v[2]) + (m[1][3] * v[3]);
  mTimesV[2] = (m[2][0] * v[0]) + (m[2][1] * v[1]) + (m[2][2] * v[2]) + (m[2][3] * v[3]);
  mTimesV[3] = (m[3][0] * v[0]) + (m[3][1] * v[1]) + (m[3][2] * v[2]) + (m[3][3] * v[3]);
}

/* Given a rotation and a translation, forms the 4x4 homogeneous matrix
representing the rotation followed in time by the translation. */
void mat44Isometry(GLdouble rot[3][3], GLdouble trans[3], GLdouble isom[4][4]) {
		int i, j;
		// transcribe the 3x3 rot matrix into the 4x4 isom matrix, left upper corner
		for (i = 0; i < 3; i ++) {
			for (j = 0; j < 3; j ++) {
				isom[i][j] = rot[i][j];
			}
		}
		// transcribe translations
		isom[0][3] = trans[0];
		isom[1][3] = trans[1];
		isom[2][3] = trans[2];

		isom[3][0] = 0;
		isom[3][1] = 0;
		isom[3][2] = 0;
		isom[3][3] = 1;
}

/* Given a rotation and translation, forms the 4x4 homogeneous matrix
representing the inverse translation followed in time by the inverse rotation.
That is, the isom produced by this function is the inverse to the isom
produced by mat44Isometry on the same inputs. */
void mat44InverseIsometry(GLdouble rot[3][3], GLdouble trans[3],
        GLdouble isom[4][4]) {
		// isom inverse = M inverse * T inverse
		GLdouble transInv[3];
		GLdouble MInv[3][3];
		mat33Transpose(rot, MInv);
		mat331Multiply(MInv, trans, transInv);
		vecScale(3, -1, transInv, transInv);

		int i,j;
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				isom[i][j] = MInv[i][j];
			}
		}

		isom[0][3] = transInv[0];
		isom[1][3] = transInv[1];
		isom[2][3] = transInv[2];
		isom[3][0] = 0;
		isom[3][1] = 0;
		isom[3][2] = 0;
		isom[3][3] = 1;
}

/* Builds a 4x4 matrix representing orthographic projection with a boxy viewing
volume [left, right] x [bottom, top] x [far, near]. That is, on the near plane
the box is the rectangle R = [left, right] x [bottom, top], and on the far
plane the box is the same rectangle R. Keep in mind that 0 > near > far. Maps
the viewing volume to [-1, 1] x [-1, 1] x [-1, 1]. */
void mat44Orthographic(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
        GLdouble far, GLdouble near, GLdouble proj[4][4]) {
		// this was GLdouble checked
		proj[0][0] = 2 / (right - left);
		proj[0][1] = 0;
		proj[0][2] = 0;
		proj[0][3] = (-right - left) / (right - left);

		proj[1][0] = 0;
		proj[1][1] = 2 / (top - bottom);
		proj[1][2] = 0;
		proj[1][3] = (-top - bottom) / (top - bottom);

		proj[2][0] = 0;
		proj[2][1] = 0;
		proj[2][2] = -(2 / (near - far));
		proj[2][3] = -((-near - far) / (near - far));

		proj[3][0] = 0;
		proj[3][1] = 0;
		proj[3][2] = 0;
		proj[3][3] = 1;
}


/* Builds a 4x4 matrix representing perspective projection. The viewing frustum
is contained between the near and far planes, with 0 > near > far. On the near
plane, the frustum is the rectangle R = [left, right] x [bottom, top]. On the
far plane, the frustum is the rectangle (far / near) * R. Maps the viewing
volume to [-1, 1] x [-1, 1] x [-1, 1]. */
void mat44Perspective(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
        GLdouble far, GLdouble near, GLdouble proj[4][4]) {
		proj[0][0] = (-2 * near) / (right - left);
		proj[0][1] = 0;
		proj[0][2] = (right + left) / (right - left);
		proj[0][3] = 0;

		proj[1][0] = 0;
		proj[1][1] = (-2 * near) / (top - bottom);
		proj[1][2] = (top + bottom) / (top - bottom);
		proj[1][3] = 0;

		proj[2][0] = 0;
		proj[2][1] = 0;
		proj[2][2] = -((-near - far) / (near - far));
		proj[2][3] = -((2 * near * far) / (near - far));

		proj[3][0] = 0;
		proj[3][1] = 0;
		proj[3][2] = -1;
		proj[3][3] = 0;
}

/* Builds a 4x4 matrix that maps a projected viewing volume
[-1, 1] x [-1, 1] x [-1, 1] to screen [0, w - 1] x [0, h - 1] x [-1, 1]. */
void mat44Viewport(GLdouble width, GLdouble height, GLdouble view[4][4]) {
		view[0][0] = (width - 1) / 2;
		view[0][1] = 0;
		view[0][2] = 0;
		view[0][3] = (width - 1) / 2;

		view[1][0] = 0;
		view[1][1] = (height - 1) / 2;
		view[1][2] = 0;
		view[1][3] = (height - 1) / 2;

		view[2][0] = 0;
		view[2][1] = 0;
		view[2][2] = 1;
		view[2][3] = 0;

		view[3][0] = 0;
		view[3][1] = 0;
		view[3][2] = 0;
		view[3][3] = 1;
}

/* We want to pass matrices into OpenGL, but there are two obstacles. First,
our matrix library uses GLdouble matrices, but OpenGL 2.x expects GLfloat
matrices. Second, C matrices are implicitly stored one-row-after-another, while
OpenGL expects matrices to be stored one-column-after-another. This function
plows through both of those obstacles. */
void mat44OpenGL(GLdouble m[4][4], GLfloat openGL[4][4]) {
	for (int i = 0; i < 4; i += 1) {
		for (int j = 0; j < 4; j += 1) {
			openGL[i][j] = m[j][i];
		}
	}
}
