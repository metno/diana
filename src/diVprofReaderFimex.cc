/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "diana_config.h"

#include "diVprofReaderFimex.h"

#include "diField/VcrossUtil.h"
#include "util/math_util.h"
#include "vcross_v2/VcrossComputer.h"
#include "vcross_v2/VcrossEvaluate.h"

#define MILOGGER_CATEGORY "diana.VprofReaderFimex"
#include <miLogger/miLogging.h>

using namespace vcross;

namespace {

const double RAD_TO_DEG = 180 / M_PI;

const char VP_X_WIND[] = "vp_x_wind_ms";
const char VP_Y_WIND[] = "vp_y_wind_ms";
const char VP_RELATIVE_HUMIDITY[] = "vp_relative_humidity";
const char VP_OMEGA[] = "vp_omega_pas";
const char VP_AIR_TEMPERATURE[] = "vp_air_temperature_celsius";
const char VP_DEW_POINT_TEMPERATURE[] = "vp_dew_point_temperature_celsius";

void copy_vprof_values(Values_cp values, std::vector<float>& values_out)
{
  const int n = values->shape().length(0);
  values_out.reserve(n);
  Values::ShapeIndex idx(values->shape());
  for (int i = 0; i < n; ++i) {
    idx.set(0, i);
    values_out.push_back(values->value(idx));
  }
}

void copy_vprof_values(const name2value_t& n2v, const std::string& id, std::vector<float>& values_out)
{
  METLIBS_LOG_SCOPE(LOGVAL(id));
  name2value_t::const_iterator itN = n2v.find(id);
  if (itN != n2v.end() && itN->second)
    copy_vprof_values(itN->second, values_out);
}
} // namespace

std::vector<std::string> VprofReaderFimex::getReferencetimes(const std::string& modelName)
{
  std::vector<std::string> rf;

  vcross::Collector_p collector = std::make_shared<vcross::Collector>(setup);
  if (vcross::Source_p source = collector->getResolver()->getSource(modelName)) {
    source->update();
    const vcross::Time_s reftimes = source->getReferenceTimes();
    for (const vcross::Time& t : reftimes)
      rf.push_back(vcross::util::to_miTime(t).isoTime("T"));
  } else {
    METLIBS_LOG_WARN("no source for model '" << modelName << "'");
  }
  return rf;
}

VprofData_p VprofReaderFimex::find(const VprofSelectedModel& vsm, const std::string& stationsfilename)
{
  return VprofDataFimex::createData(setup, vsm, stationsfilename);
}

// static
VprofData_p VprofDataFimex::createData(vcross::Setup_p setup, const VprofSelectedModel& vsm, const std::string& stationsfilename)
{
  METLIBS_LOG_SCOPE();

  VprofDataFimex_p vpd = std::make_shared<VprofDataFimex>(vsm.model, stationsfilename);

  vpd->collector = std::make_shared<vcross::Collector>(setup);
  vcross::Source_p source = vpd->collector->getResolver()->getSource(vpd->getModelName());
  if (!source)
    return VprofData_p();

  vpd->fields.push_back(VP_AIR_TEMPERATURE);
  vpd->fields.push_back(VP_DEW_POINT_TEMPERATURE);
  vpd->fields.push_back(VP_X_WIND);
  vpd->fields.push_back(VP_Y_WIND);
  vpd->fields.push_back(VP_RELATIVE_HUMIDITY);
  vpd->fields.push_back(VP_OMEGA);

  if (vsm.reftime.empty()) {
    vpd->reftime = source->getLatestReferenceTime();
  } else {
    vpd->reftime = util::from_miTime(miutil::miTime(vsm.reftime));
  }
  const vcross::ModelReftime mr(vpd->getModelName(), vpd->reftime);

  for (const std::string& f : vpd->fields) {
    METLIBS_LOG_DEBUG(LOGVAL(mr) << LOGVAL(f));
    vpd->collector->requireField(mr, f);
  }

  vcross::Inventory_cp inv = vpd->collector->getResolver()->getInventory(mr);
  if (!inv)
    return VprofData_p();

  if (!stationsfilename.empty()) {
    vpd->readStationNames();
  } else {
    for (vcross::Crossection_cp cs : inv->crossections) {
      if (cs->length() != 1)
        continue;
      vpd->mStations.push_back(stationInfo(cs->label(), cs->point(0).lonDeg(), cs->point(0).latDeg()));
    }
  }

  for (const vcross::Time::timevalue_t& time : inv->times.values) {
    vpd->addValidTime(vcross::util::to_miTime(inv->times.unit, time));
  }

  vpd->numRealizations = inv->realizationCount;

  const miutil::miTime rt = util::to_miTime(mr.reftime);
  for (const miutil::miTime& vt : vpd->getTimes()) {
    vpd->forecastHour.push_back(miutil::miTime::hourDiff(vt, rt));
  }
  return vpd;
}

VprofDataFimex::VprofDataFimex(const std::string& modelname, const std::string& stationsfilename)
    : VprofData(modelname, stationsfilename)
{
}

bool VprofDataFimex::updateStationList(const miutil::miTime& plotTime)
{
  return std::find(getTimes().begin(), getTimes().end(), plotTime) != getTimes().end();
}

VprofValues_cp VprofDataFimex::readValues(const std::string& name, const miutil::miTime& time, int realization)
{
  if (name.empty())
    return VprofValues_cp();

  std::vector<stationInfo>::const_iterator itP = std::find_if(mStations.begin(), mStations.end(), diutil::eq_StationName(name));
  std::vector<miutil::miTime>::const_iterator itT = std::find(getTimes().begin(), getTimes().end(), time);
  if (itP == mStations.end() || itT == getTimes().end())
    return VprofValues_cp();

  const int iTime = std::distance(getTimes().begin(), itT);

  METLIBS_LOG_DEBUG("readFomFimex"); //<<LOGVAL(reftime));
  const LonLat pos = LonLat::fromDegrees(itP->lon, itP->lat);
  const Time user_time(util::from_miTime(time));
  // This replaces the current dynamic crossection, if present.
  // TODO: Should be tested when more than one time step is available.
  if (!getStationsFileName().empty()) {
    Source_p source = collector->getResolver()->getSource(getModelName());
    if (!source)
      return VprofValues_cp();
    source->addDynamicCrossection(reftime, itP->name, LonLat_v(1, pos));
  }

  const vcross::ModelReftime mr(getModelName(), reftime);

  FieldData_cp air_temperature = std::dynamic_pointer_cast<const FieldData>(collector->getResolvedField(mr, VP_AIR_TEMPERATURE));
  if (not air_temperature)
    return VprofValues_p();
  ZAxisData_cp zaxis = air_temperature->zaxis();
  if (!zaxis)
    return VprofValues_cp();

  collector->requireVertical(Z_TYPE_PRESSURE);

  model_values_m model_values;
  try {
    model_values = vc_fetch_pointValues(collector, pos, user_time, realization);
  } catch (std::exception& e) {
    METLIBS_LOG_ERROR("exception: " << e.what());
    return VprofValues_p();
  } catch (...) {
    METLIBS_LOG_ERROR("unknown exception");
    return VprofValues_cp();
  }

  model_values_m::iterator itM = model_values.find(mr);
  if (itM == model_values.end())
    return VprofValues_cp();
  name2value_t& n2v = itM->second;

  vc_evaluate_fields(collector, model_values, mr, fields);

  Values_cp z_values;
  if (util::unitsConvertible(zaxis->unit(), "hPa"))
    z_values = vc_evaluate_field(zaxis, n2v);
  else if (InventoryBase_cp pfield = zaxis->pressureField())
    z_values = vc_evaluate_field(pfield, n2v);
  if (not z_values)
    return VprofValues_cp();

  VprofValues_p vv = std::make_shared<VprofValues>();
  vv->text.index = -1;
  vv->text.prognostic = true;
  vv->text.modelName = getModelName();
  vv->text.posName = itP->name;
  vv->text.forecastHour = forecastHour[iTime];
  vv->text.validTime = *itT;
  vv->text.latitude = itP->lat;
  vv->text.longitude = itP->lon;
  vv->text.kindexFound = false;
  vv->text.realization = realization;
  vv->windInKnots = false;
  copy_vprof_values(z_values, vv->ptt);
  copy_vprof_values(n2v, VP_AIR_TEMPERATURE, vv->tt);
  copy_vprof_values(n2v, VP_DEW_POINT_TEMPERATURE, vv->td);
  copy_vprof_values(n2v, VP_X_WIND, vv->uu);
  copy_vprof_values(n2v, VP_Y_WIND, vv->vv);
  copy_vprof_values(n2v, VP_RELATIVE_HUMIDITY, vv->rhum);
  copy_vprof_values(n2v, VP_OMEGA, vv->om);

  const size_t numLevel = vv->ptt.size();
  vv->maxLevels = numLevel;

  // dd,ff and significant levels (as in temp observation...)
  if (vv->uu.size() == numLevel && vv->vv.size() == numLevel) {
    int kmax = 0;
    for (size_t k = 0; k < numLevel; k++) {
      float uew = vv->uu[k];
      float vns = vv->vv[k];
      int ff = int(diutil::absval(uew, vns) + 0.5);
      if (!vv->windInKnots)
        ff *= 1.94384; // 1 knot = 1 m/s * 3600s/1852m
      int dd = int(270. - RAD_TO_DEG * atan2f(vns, uew) + 0.5);
      if (dd > 360)
        dd -= 360;
      if (dd <= 0)
        dd += 360;
      if (ff == 0)
        dd = 0;
      vv->dd.push_back(dd);
      vv->ff.push_back(ff);
      vv->sigwind.push_back(0);
      if (ff > vv->ff[kmax])
        kmax = k;
    }
    for (size_t l = 0; l < vv->sigwind.size(); l++) {
      for (size_t k = 1; k < numLevel - 1; k++) {
        if (vv->ff[k] < vv->ff[k - 1] && vv->ff[k] < vv->ff[k + 1])
          vv->sigwind[k] = 1; // local ff minimum
        if (vv->ff[k] > vv->ff[k - 1] && vv->ff[k] > vv->ff[k + 1])
          vv->sigwind[k] = 2; // local ff maximum
      }
    }
    vv->sigwind[kmax] = 3; // "global" ff maximum
  }

  vv->calculate();
  if (vv->isDefined() == difield::NONE_DEFINED)
    return VprofValues_cp();
  return vv;
}

VprofValues_cpv VprofDataFimex::getValues(const std::string& name, const miutil::miTime& time, int realization)
{
  METLIBS_LOG_SCOPE(name << "  " << time << "  " << getModelName());
  METLIBS_LOG_DEBUG(LOGVAL(getTimes().size()) << LOGVAL(mStations.size()));

  if (cachedTime.undef() || cachedName.empty() || realization != cachedRealization || name != cachedName || time != cachedTime) {
    realization = -1;
    cache.clear();
    cachedTime = time;
    cachedName = name;
    cachedRealization = realization;
    if (realization < 0) {
      for (int i = 0; i < numRealizations; ++i)
        if (VprofValues_cp vv = readValues(name, time, i))
          cache.push_back(vv);
    } else {
      if (VprofValues_cp vv = readValues(name, time, realization))
        cache.push_back(vv);
    }
  }

  return cache;
}
