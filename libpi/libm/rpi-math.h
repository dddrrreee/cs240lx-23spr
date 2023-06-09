#ifndef __RPI_MATH_H__
#define __RPI_MATH_H__

#define LIB_VERSION _IEEE_

#include <stdint.h>

// typedef unsigned u_int32_t;
// typedef int int32_t;
double my_cos(double);

#include "openlibm.h"
#include "math-protos.h"

// this isn't appearing for some people.  if you need other constants
// maybe we should pull in the others?
#ifndef M_PI 
#define M_PI        3.14159265358979323846  /* pi */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923  /* pi/2 */
#endif

#if 0
#define M_E     2.7182818284590452354   /* e */
#define M_LOG2E     1.4426950408889634074   /* log 2e */
#define M_LOG10E    0.43429448190325182765  /* log 10e */
#define M_LN2       0.69314718055994530942  /* log e2 */
#define M_LN10      2.30258509299404568402  /* log e10 */
#define M_PI_4      0.78539816339744830962  /* pi/4 */
#define M_1_PI      0.31830988618379067154  /* 1/pi */
#define M_2_PI      0.63661977236758134308  /* 2/pi */
#define M_2_SQRTPI  1.12837916709551257390  /* 2/sqrt(pi) */
#define M_SQRT2     1.41421356237309504880  /* sqrt(2) */
#define M_SQRT1_2   0.70710678118654752440  /* 1/sqrt(2) */
#endif

#endif
