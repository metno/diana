
#ifndef DIANA_UTIL_MATH_UTIL_H
#define DIANA_UTIL_MATH_UTIL_H 1

#include <cmath>

namespace diutil {

template<typename T>
inline T square(T x)
{ return x*x; }

/**
 * Calculate the squared length of a 2-dim vector, x*x + y*y.
 */
template<typename T>
inline T absval2(T x, T y)
{ return square(x) + square(y); }

/**
 * Calculate the length of a 2-dim vector, sqrt(x*x + y*y).
 */
template<typename T>
inline T absval(T x, T y)
{ return std::sqrt(absval2(x, y)); }

float GreatCircleDistance(float lat1_deg, float lat2_deg, float lon1_deg, float lon2_deg);

} // namespace diutil

#endif // DIANA_UTIL_MATH_UTIL_H
