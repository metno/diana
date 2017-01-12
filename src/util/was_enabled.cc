#include "was_enabled.h"

#include "diPlot.h"

namespace diutil {

void was_enabled::save(const Plot* plot)
{
  const std::string key = plot->getEnabledStateKey();
  key_enabled[key] = plot->isEnabled();
}

void was_enabled::restore(Plot* plot) const
{
  const std::string key = plot->getEnabledStateKey();
  key_enabled_t::const_iterator it = key_enabled.find(key);
  if (it != key_enabled.end())
    plot->setEnabled(it->second);
};

} // namespace diutil
