/**
 * @ingroup dcplaya_matrix_devel
 * @file    matrix.h
 * @author  benjamin gerard
 * @date    2002/02/12
 * @brief   Matrix support.
 *
 * $Id$
 */

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "extern_def.h"


DCPLAYA_EXTERN_C_START

#include <kos/vector.h>

#define matrix_t dcpmatrix_t

/** @defgroup  dcplaya_matrix_devel  Matrix
 *  @ingroup   dcplaya_math_devel
 *  @brief     matrix operations
 *
 *  @author    benjamin gerard
 *
 *  @{
 */

#ifndef __MATRIX_H
/** 4x4 matrix type.
 */
typedef float matrix_t[4][4];
#endif

#ifdef DEBUG
void MtxDump(matrix_t m);
#else
# define MtxDump(A)
#endif

/** Copy a matrix. */
void MtxCopy(matrix_t m, matrix_t m2);

/** Copy a matrix with flexible format. */
void MtxCopyFlexible(float *d, const float *s, int nline, int ncol,
					 int dsize, int ssize);

/** Set matrix to identity. */
void MtxIdentity(matrix_t m);

/** Multiply two matrix : m = m * m2. */
void MtxMult(matrix_t m, matrix_t m2);

/** Multiply two matrix : m = m2 * m. */
void MtxInvMult(matrix_t m, matrix_t m2);

/** Multiply two matrix and store result in a third one : d = m * m2. */
void MtxMult3(matrix_t d, matrix_t m, matrix_t m2);

/** Apply matrix to vector. */
void MtxVectMult(float *v, const float *u, matrix_t m);

/** Apply matrix to vectors. */
void MtxVectorsMult(float *v, const float *u, matrix_t m, int nmemb,
					int sizev, int sizeu);


/** Matrix transpose. */
void MtxTranspose(matrix_t m);

/** Matrix 3x3 transpose. */
void MtxTranspose3x3(matrix_t m);

/** Apply scaling to a matrix. */
void MtxScale(matrix_t m, const float s);

/** Apply scaling to a 3x3 matrix. */
void MtxScale3x3(matrix_t m, const float s);

/** Apply scaling to a matrix. */
void MtxScale3(matrix_t m, const float sx, const float sy, const float sz);

/** Apply translation to a matrix. */
void MtxTranslate(matrix_t m, const float x, const float y, const float z);

/** Apply X-axis rotation to a matrix. */
void MtxRotateX(matrix_t m, const float a);

/** Apply Y-axis rotation to a matrix. */
void MtxRotateY(matrix_t m, const float a);

/** Apply Z-axis rotation to a matrix. */
void MtxRotateZ(matrix_t m, const float a);

/** Set a matrix that look at a given position (no rolling). */
void MtxLookAt(matrix_t row, const float x, const float y, const float z);
void MtxLookAt2(matrix_t row,
		const float eyes_x, const float eyes_y, const float eyes_z,
		const float x, const float y, const float z);

/** Set a projection matrix.
 *  @param row          matrix to set
 *  @param openAngle    open angle in radian
 *  @param fov          field of vision (not really that!)
 *  @param aspectRatio  typically width/height
 *  @param zFar         Z mapped to 1 after projection
 *
 *  Parameters of this functions are a bit strange and unusual.
 *
 *  @return zNear (Z mapped to 0 after projection)
 *
 *  @retval -row[3][2]/row[2][2];
 */
float MtxProjection(matrix_t row, const float openAngle, const float fov,
		    const float aspectRatio, const float zFar);

void MtxFrustum(matrix_t row, const float left, const float right,
                              const float top, const float bottom,
                              const float zNear, const float zFar);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _MATRIX_H_ */
