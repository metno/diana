
#include "VcrossSource.h"

#include "VcrossUtil.h"

#define MILOGGER_CATEGORY "vcross.VcrossSource"
#include "miLogger/miLogging.h"

namespace vcross {

ReftimeSource::~ReftimeSource()
{
}

void ReftimeSource::flush()
{
}

bool ReftimeSource::supportsDynamicCrossections()
{
  return false;
}

Crossection_cp ReftimeSource::addDynamicCrossection(std::string, const LonLat_v&)
{
  return Crossection_cp();
}

void ReftimeSource::dropDynamicCrossection(Crossection_cp)
{
}

void ReftimeSource::dropDynamicCrossections()
{
}

// ########################################################################

Source::~Source()
{
}

void Source::updateIfNoReftimeSources()
{
  if (mReftimeSources.empty())
    update();
}

Time_s Source::getReferenceTimes()
{
  updateIfNoReftimeSources();
  Time_s rt;
  for (ReftimeSource_pv::const_iterator it = mReftimeSources.begin(); it != mReftimeSources.end(); ++it)
    rt.insert((*it)->getReferenceTime());
  return rt;
}

Inventory_cp Source::getInventory(const Time& reftime)
{
  if (ReftimeSource_p rs = findSource(reftime))
    return rs->getInventory();
  else
    return Inventory_cp();
}

Time Source::getLatestReferenceTime()
{
  updateIfNoReftimeSources();
  Time rt;
  for (ReftimeSource_pv::const_iterator it = mReftimeSources.begin(); it != mReftimeSources.end(); ++it) {
    const Time& srt = (*it)->getReferenceTime();
    if (!rt.valid() || rt < srt)
      rt = srt;
  }
  return rt;
}

void Source::getCrossectionValues(const Time& reftime, Crossection_cp cs, const Time& time,
    const InventoryBase_cps& data, name2value_t& n2v)
{
  if (ReftimeSource_p rs = findSource(reftime))
    rs->getCrossectionValues(cs, time, data, n2v);
}

void Source::getTimegraphValues(const Time& reftime, Crossection_cp cs, size_t crossection_index,
    const InventoryBase_cps& data, name2value_t& n2v)
{
  if (ReftimeSource_p rs = findSource(reftime))
    rs->getTimegraphValues(cs, crossection_index, data, n2v);
}

void Source::getPointValues(const Time& reftime, Crossection_cp crossection, size_t crossection_index, const Time& time,
    const InventoryBase_cps& data, name2value_t& n2v)
{
  if (ReftimeSource_p rs = findSource(reftime))
    rs->getPointValues(crossection, crossection_index, time, data, n2v);
}

void Source::getWaveSpectrumValues(const Time& reftime, Crossection_cp crossection, size_t crossection_index, const Time& time,
    const InventoryBase_cps& data, name2value_t& n2v)
{
  if (ReftimeSource_p rs = findSource(reftime))
    rs->getWaveSpectrumValues(crossection, crossection_index, time, data, n2v);
}

void Source::flush()
{
  for (ReftimeSource_pv::const_iterator it = mReftimeSources.begin(); it != mReftimeSources.end(); ++it)
    (*it)->flush();
}

bool Source::supportsDynamicCrossections(const Time& reftime)
{
  if (ReftimeSource_p rs = findSource(reftime))
    return rs->supportsDynamicCrossections();
  else
    return false;
}

Crossection_cp Source::addDynamicCrossection(const Time& reftime, std::string name, const LonLat_v& positions)
{
  if (ReftimeSource_p rs = findSource(reftime))
    return rs->addDynamicCrossection(name, positions);
  else
    return Crossection_cp();
}

void Source::dropDynamicCrossection(const Time& reftime, Crossection_cp cs)
{
  if (ReftimeSource_p rs = findSource(reftime))
    return rs->dropDynamicCrossection(cs);
}

void Source::dropDynamicCrossections(const Time& reftime)
{
  if (ReftimeSource_p rs = findSource(reftime))
    return rs->dropDynamicCrossections();
}

ReftimeSource_p Source::findSource(const Time& reftime) const
{
  METLIBS_LOG_SCOPE(util::to_miTime(reftime));
  for (ReftimeSource_pv::const_iterator it = mReftimeSources.begin(); it != mReftimeSources.end(); ++it)
    if (reftime == (*it)->getReferenceTime())
      return *it;
  METLIBS_LOG_DEBUG("no source found");
  return ReftimeSource_p();
}

} // namespace vcross
