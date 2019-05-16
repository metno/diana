/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diVprofSimpleData.h"
#include "diVprofSimpleValues.h"
#include "diVprofUtils.h"
#include "diField/VcrossUtil.h"
#include "vcross_v2/VcrossComputer.h"
#include "vcross_v2/VcrossEvaluate.h"

#include <mi_fieldcalc/math_util.h>

#define MILOGGER_CATEGORY "diana.VprofReaderFimex"
#include <miLogger/miLogging.h>

using namespace vcross;

namespace {

VprofSimpleData_p copy_vprof_values(Values_cp zvalues, const name2value_t& n2v, const std::string& id, const std::string& z_unit, const std::string& x_unit)
{
  METLIBS_LOG_SCOPE(LOGVAL(id));
  name2value_t::const_iterator it1 = n2v.find(id);
  if (it1 == n2v.end() || !it1->second)
    return VprofSimpleData_p();

  Values_cp values1 = it1->second;
  const int n1 = values1->shape().length(0);
  const int nz = zvalues->shape().length(0);
  if (n1 != nz)
    return VprofSimpleData_p();

  VprofSimpleData_p out = std::make_shared<VprofSimpleData>(id, z_unit, x_unit);
  out->reserve(nz);
  Values::ShapeIndex idx1(values1->shape());
  Values::ShapeIndex idxz(zvalues->shape());
  for (int i = 0; i < n1; ++i) {
    idxz.set(0, i);
    idx1.set(0, i);
    out->add(zvalues->value(idxz), values1->value(idx1));
  }
  return out;
}

} // namespace

std::set<std::string> VprofReaderFimex::getReferencetimes(const std::string& modelName)
{
  std::set<std::string> rf;
  vcross::Collector_p collector = std::make_shared<vcross::Collector>(setup);
  if (vcross::Source_p source = collector->getResolver()->getSource(modelName)) {
    source->update();
    const vcross::Time_s reftimes = source->getReferenceTimes();
    for (const vcross::Time& t : reftimes)
      rf.insert(vcross::util::to_miTime(t).isoTime("T"));
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
  source->update();

  if (vsm.reftime.empty()) {
    vpd->reftime = source->getLatestReferenceTime();
  } else {
    vpd->reftime = util::from_miTime(miutil::miTime(vsm.reftime));
  }
  const vcross::ModelReftime mr(vpd->getModelName(), vpd->reftime);

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

VprofValues_cp VprofDataFimex::readValues(const VprofValuesRequest& req)
{
  METLIBS_LOG_SCOPE();
  if (req.name.empty() || req.variables.empty()) {
    METLIBS_LOG_WARN("cannot fetch data without station name or variables list");
    return VprofValues_cp();
  }

  const std::vector<stationInfo>::const_iterator itP = std::find_if(mStations.begin(), mStations.end(), diutil::eq_StationName(req.name));
  if (itP == mStations.end()) {
    METLIBS_LOG_WARN("station '" << req.name << "' not found");
    return VprofValues_cp();
  }
  const std::vector<miutil::miTime>::const_iterator itT = std::find(getTimes().begin(), getTimes().end(), req.time);
  if (itT == getTimes().end()) {
    METLIBS_LOG_WARN("time '" << req.time << "' not found");
    return VprofValues_cp();
  }

  const int iTime = std::distance(getTimes().begin(), itT);

  const LonLat pos = LonLat::fromDegrees(itP->lon, itP->lat);
  const Time user_time(util::from_miTime(req.time));
  // This replaces the current dynamic crossection, if present.
  // TODO: Should be tested when more than one time step is available.
  if (!getStationsFileName().empty()) {
    Source_p source = collector->getResolver()->getSource(getModelName());
    if (!source)
      return VprofValues_cp();
    source->addDynamicCrossection(reftime, itP->name, LonLat_v(1, pos));
  }

  const vcross::ModelReftime mr(getModelName(), reftime);

  collector->clear();
  for (const std::string& v : req.variables) {
    collector->requireField(mr, v);
  }
  collector->requireVertical(req.vertical_axis);

  model_values_m model_values;
  try {
    model_values = vc_fetch_pointValues(collector, pos, user_time, req.realization);
  } catch (std::exception& e) {
    METLIBS_LOG_ERROR("exception: " << e.what());
    return VprofValues_p();
  } catch (...) {
    METLIBS_LOG_ERROR("unknown exception");
    return VprofValues_cp();
  }

  model_values_m::iterator itM = model_values.find(mr);
  if (itM == model_values.end()) {
    METLIBS_LOG_WARN("no data for model=" << mr.model << " reftime=" << util::to_miTime(mr.reftime));
    return VprofValues_cp();
  }
  name2value_t& n2v = itM->second;

  const vcross::string_v fields(req.variables.begin(), req.variables.end());
  vc_evaluate_fields(collector, model_values, mr, fields);

  VprofSimpleValues_p vv = std::make_shared<VprofSimpleValues>();
  vv->text.index = -1;
  vv->text.prognostic = true;
  vv->text.modelName = getModelName();
  vv->text.posName = itP->name;
  vv->text.forecastHour = forecastHour[iTime];
  vv->text.validTime = *itT;
  vv->text.latitude = itP->lat;
  vv->text.longitude = itP->lon;
  vv->text.kindexFound = false;
  vv->text.realization = req.realization;
  for (const std::string& v : req.variables) {
    FieldData_cp var = std::dynamic_pointer_cast<const FieldData>(collector->getResolvedField(mr, v));
    if (!var) {
      // this is okay for a derived variable
      continue;
    }
    ZAxisData_cp zaxis = var->zaxis();
    if (!zaxis) {
      METLIBS_LOG_WARN("variable '" << v << "' has no vertical axis");
      continue;
    }

    Values_cp z_values = vc_evaluate_z(zaxis, req.vertical_axis, n2v);
    if (!z_values) {
      METLIBS_LOG_WARN("no z values for model='" << mr.model << " reftime=" << util::to_miTime(mr.reftime) << " var='" << v << "'");
      continue;
    }

    vv->add(copy_vprof_values(z_values, n2v, v, zAxisUnit(req.vertical_axis), "FIXME"));
  }

  vv->calculate();
  if (vv->isDefined() == miutil::NONE_DEFINED) {
    METLIBS_LOG_WARN("no defined values");
    return VprofValues_cp();
  }
  return vv;
}

VprofValues_cpv VprofDataFimex::getValues(const VprofValuesRequest& req)
{
  METLIBS_LOG_SCOPE(req.name << "  " << req.time << "  " << getModelName());
  METLIBS_LOG_DEBUG(LOGVAL(getTimes().size()) << LOGVAL(mStations.size()));

  if (cachedRequest.time.undef() || cachedRequest != req) {
    cache.clear();
    cachedRequest = req;
    VprofValuesRequest req_(req);
    VprofSimpleValues::addCalculationInputVariables(req_.variables);
    int i_begin, i_end;
    if (req.realization < 0) {
      i_begin = 0;
      i_end = numRealizations;
    } else {
      i_begin = req.realization;
      i_end = i_begin + 1;
    }
    for (int i = i_begin; i < i_end; ++i) {
      req_.realization = i;
      if (VprofValues_cp vv = readValues(req_))
        cache.push_back(vv);
    }
  }

  return cache;
}
