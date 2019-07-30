
#include "diObsData.h"

void ObsData::clear_data()
{
  fdata.clear();
  fdata_rotated.clear();
  stringdata.clear();
}

const std::string* ObsData::get_string(const std::string& key) const
{
  stringdata_t::const_iterator it = stringdata.find(key);
  if (it != stringdata.end())
    return &it->second;
  else
    return nullptr;
}

const float* ObsData::get_float(const std::string& key) const
{
  fdata_t::const_iterator it = fdata_rotated.find(key);
  if (it != fdata_rotated.end())
    return &it->second;

  return get_unrotated_float(key);
}

const float* ObsData::get_unrotated_float(const std::string& key) const
{
  fdata_t::const_iterator it = fdata.find(key);
  if (it != fdata.end())
    return &it->second;

  return nullptr;
}
