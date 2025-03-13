/** @ingroup  dcplaya_math_devel
 *  @file     math_float.h
 *  @author   benjamin gerard
 *  @date     2003/01/21
 *  @brief    floating point macros and defines
 *
 * $Id$
 */

/** @defgroup  dcplaya_math_devel Mathematics
 *  @ingroup   dcplaya_devel
 *  @brief     mathematics various (float, integer, matrix, vertex, fft ...)
 *
 *  @author   benjamin gerard
 *  @{
 */

/** @name Floating point mathematic basics.
 *  @{
 */

#ifndef _MATH_FLOAT_H_
#define _MATH_FLOAT_H_

#ifdef _arch_dreamcast
# include <dc/fmath.h>
#else
# define fcos cosf
# define fsin sinf
# define fsqrt sqrtf
#endif
#include <math.h>

/** Dividing small value threshold. */
#ifndef MF_EPSYLON
# define MF_EPSYLON 1E-5
#endif

#ifndef MF_EPSYLON2
# define MF_EPSYLON2 (MF_EPSYLON*MF_EPSYLON)
#endif

/** PI. */
#ifndef MF_PI
# ifdef M_PI
#  define MF_PI M_PI
# else
#  define MF_PI 3.14159265359f
# endif
#endif

#define Cos(A) fcos((A)) /**< Fast cosinus. */
#define Sin(A) fsin((A)) /**< Fast sinus. */
#define Inv(A) (1.0f/(A)) /**< Fast 1/X. */
#define Sq(A) __mf_sq(A) /**< Fast X^2. */
#define Sqrt(A) sqrtf((A)) /**< Fast square root. */
#define ISqrt(A) Inv(Sqrt((A))) /**< Fast 1/sqrt(X). */
#define Fabs(A) __mf_abs((A)) /**< Fast absolute value. */
#define Fmax(A,B) __mf_max((A),(B)) /**< Fast maximum value. */
#define Fmin(A,B) __mf_min((A),(B)) /**< Fast minimum value. */
#define Fbound(V,MIN,MAX) __mf_bound((V),(MIN),(MAX)) /**< Fast range. */
#define Fblend(A,B,F) __mf_blend((A),(B),(F)) /**< Fast blending. */
#define FnearZero(A) __mf_near_zero((A)) /**< Fast near zero test. */
#define Fsign(A) ((A)<0) /**< Fast sign test. */
#define Fmod(A,B) fmod((A),(B)) /**< Fast modulo. */

inline static float __mf_sq(const float a) {
  return a * a;
}

inline static float __mf_near_zero(const float v) {
  return (v > -MF_EPSYLON && v < MF_EPSYLON);
}

inline static float __mf_blend(const float a, const float b, const float f) {
  return a * f + b * (1.0f-f);
}

inline static float __mf_abs(const float a) {
  return a < 0 ? -a : a;
}

inline static float __mf_min(const float a, const float b) {
  return a <= b ? a : b;
}

inline static float __mf_max(const float a, const float b) {
  return a >= b ? a : b;
}

inline static float __mf_bound(const float a, const float b, const float c) {
  return a < b ? b : (a > c ? c : a);
}

/** @} */

/** @} */

#endif /* #define _MATH_FLOAT_H_ */
