
#include "diObsData.h"

namespace {
const std::string EMPTY;
}

const std::string& ObsData::get_string(const std::string& key) const
{
  stringdata_t::const_iterator it = stringdata.find(key);
  if (it != stringdata.end())
    return it->second;
  else
    return EMPTY;
}

float ObsData::get_float(const std::string& key) const
{
  fdata_t::const_iterator it = fdata.find(key);
  if (it != fdata.end())
    return it->second;
  else
    return 0;
}
