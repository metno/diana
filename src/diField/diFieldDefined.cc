#include "diFieldDefined.h"

namespace difield {

const float UNDEF = 1.0e35f;

inline bool is_defined(float f)
{ return (f < UNDEF); }

ValuesDefined checkDefined(const float* data, size_t n)
{
  // check undef
  bool has_undefined = false, has_defined = false;
  for (const float* f = data; f != data + n; ++f) {
    bool defined = is_defined(*f);
    if (defined)
      has_defined = true;
    else
      has_undefined = true;
    if (has_undefined && has_defined)
      break;
  }
  if (has_undefined && has_defined)
    return SOME_DEFINED;
  else if (has_defined)
    return ALL_DEFINED;
  else
    return NONE_DEFINED;
}

ValuesDefined checkDefined(size_t n_undefined, size_t n)
{
  if (n_undefined == 0)
    return ALL_DEFINED;
  else if (n_undefined == n)
    return NONE_DEFINED;
  else
    return SOME_DEFINED;
}

} // namespace difield

const float fieldUndef = difield::UNDEF;
