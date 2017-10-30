#ifndef DIFIELDDEFINED_H
#define DIFIELDDEFINED_H

#include <cstdlib>

extern const float fieldUndef;

namespace difield {

extern const float UNDEF;

enum ValuesDefined { ALL_DEFINED=0, NONE_DEFINED, SOME_DEFINED };

ValuesDefined checkDefined(const float* data, size_t n);

ValuesDefined checkDefined(size_t n_undefined, size_t n);

} // namespace difield

#endif // DIFIELDDEFINED_H
