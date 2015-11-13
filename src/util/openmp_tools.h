#ifndef UTIL_OPENMP_TOOLS_H
#define UTIL_OPENMP_TOOLS_H 1

#ifdef HAVE_OPENMP
#include <omp.h>
#endif

#ifdef HAVE_OPENMP

#define DIUTIL_STRING_HELPER0(x) #x
#define DIUTIL_STRING_HELPER1(x) DIUTIL_STRING_HELPER0(x)

#define DIUTIL_OPENMP(options)                  \
  _Pragma(DIUTIL_STRING_HELPER1(omp options))

#define DIUTIL_OPENMP_PARALLEL(loopsize,options)                        \
  omp_set_dynamic(0);                                                   \
  omp_set_num_threads(diutil::compute_num_threads(loopsize));           \
  _Pragma(DIUTIL_STRING_HELPER1(omp parallel options))

#else // !HAVE_OPENMP

#define DIUTIL_OPENMP(options) /* nothing */
#define DIUTIL_OPENMP_PARALLEL(loopsize,options) /* nothing */

#endif

namespace diutil {

/*! A little utility function that dynamically computes the best number of openmp threads
  based on the number of iterations (nx*ny) for example. */
int compute_num_threads(long loop_size);

} // namespace diutil

#endif
