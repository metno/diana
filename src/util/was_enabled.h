#ifndef UTIL_WAS_ENABLED_H
#define UTIL_WAS_ENABLED_H

#include <map>
#include <string>

class Plot;

namespace diutil {

class was_enabled {
  typedef std::map<std::string, bool> key_enabled_t;
  key_enabled_t key_enabled;
public:
  void save(const Plot* plot);
  void restore(Plot* plot) const;
};

} // namespace diutil

#endif // UTIL_WAS_ENABLED_H
