#include "openmp_tools.h"

#include <algorithm>

namespace diutil {

int compute_num_threads(long loop_size)
{
  /* We must look at these two wariables
     OMP_NUM_THREADS, OMP_THREAD_LIMIT
  */

  static int loc_omp_num_threads = -1;

  if (loc_omp_num_threads == -1) {
    if (getenv("OMP_NUM_THREADS") != 0) {
      loc_omp_num_threads = atoi(getenv("OMP_NUM_THREADS"));
    } else if (getenv("OMP_THREAD_LIMIT") != 0) {
      loc_omp_num_threads = atoi(getenv("OMP_THREAD_LIMIT"));
    } else {
      /* The default */
      loc_omp_num_threads = 8;
    }
  }

  if (loop_size < 1000)
    return std::min(1, loc_omp_num_threads);
  else if (loop_size <= 10000)
    return std::min(2, loc_omp_num_threads);
  else if (loop_size <= 100000)
    return std::min(4, loc_omp_num_threads);
  else
    return std::min(8, loc_omp_num_threads);
}

} // namespace diutil
