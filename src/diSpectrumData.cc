/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2015 met.no

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

#include "diSpectrumData.h"
#include "diSpectrumPlot.h"

#include <puTools/miStringFunctions.h>
#include <diField/VcrossUtil.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.SpectrumData"
#include <miLogger/miLogging.h>

static const float DEG_TO_RAD = M_PI / 180;

using namespace miutil;
using namespace vcross;

SpectrumData::SpectrumData(const std::string& modelname)
  : modelName(modelname), numPos(0),
    numTime(0), numDirec(0), numFreq(0)
{
  METLIBS_LOG_SCOPE();
}

SpectrumData::~SpectrumData()
{
  METLIBS_LOG_SCOPE();
}

bool SpectrumData::readFileHeader(vcross::Setup_p setup, const std::string& reftimestr)
{
  METLIBS_LOG_SCOPE();

  collector = std::make_shared<vcross::Collector>(setup);

  if ( reftimestr.empty() ) {
    reftime = collector->getResolver()->getSource(modelName)->getLatestReferenceTime();
  } else {
    miTime mt(reftimestr);
    reftime = util::from_miTime(mt);
  }

  const vcross::ModelReftime mr(modelName, reftime);

  vcross::Inventory_cp inv = collector->getResolver()->getInventory(mr);
  if (!inv || inv->times.npoint() == 0 || inv->crossections.empty()) {
    METLIBS_LOG_ERROR("no or empty inventory for '" << modelName << "'");
    return false;
  }

  for(vcross::Crossection_cpv::const_iterator itCS = inv->crossections.begin(); itCS != inv->crossections.end(); ++itCS) {
    vcross::Crossection_cp cs = *itCS;
    for (size_t i=0; i<cs->length(); ++i) {
      const LonLat& p = cs->point(i);
      METLIBS_LOG_DEBUG(LOGVAL(p.latDeg()));
      METLIBS_LOG_DEBUG(LOGVAL(p.lonDeg()));
      std::ostringstream ost;
      float lon = int (fabs(p.lonDeg()) * 10);
      lon /= 10.;
      float lat = int (fabs(p.latDeg()) * 10);
      lat /= 10.;
      ost << lon;
      if (p.lonDeg() < 0)
        ost  << "W " ;
      else
        ost  << "E " ;
      ost << lat;
      if (p.latDeg() < 0)
        ost  << "S" ;
      else
        ost  << "N" ;
      posName.push_back( ost.str() );
      posLatitude.push_back(p.latDeg());
      posLongitude.push_back(p.lonDeg());
    }
  }

  for (vcross::Time::timevalue_t time : inv->times.values) {
    validTime.push_back(vcross::util::to_miTime(inv->times.unit, time));
    METLIBS_LOG_DEBUG(LOGVAL(vcross::util::to_miTime(inv->times.unit, time)));
  }

  miTime t = validTime[0];
  for (size_t i = 0; i < validTime.size(); i++) {
    forecastHour.push_back(miTime::hourDiff(validTime[i], t));
  }

  Crossection_cp cs0 = inv->crossections.at(0);
  FieldData_cp freq = inv->findFieldById("freq");
  FieldData_cp dir = inv->findFieldById("direction");
  InventoryBase_cps request;
  request.insert(freq);
  request.insert(dir);
  name2value_t n2v;
  fs = collector->getResolver()->getSource(modelName);
  fs->getWaveSpectrumValues(mr.reftime,cs0, 0, inv->times.at(0), request, n2v, 0);
  Values_cp freq_values = n2v[freq->id()];
  Values_cp dir_values = n2v[dir->id()];

  Values::ShapeIndex idx(freq_values->shape());
  for (int i = 0; i < freq_values->shape().length(0); ++i) {
    idx.set("freq", i);
    frequences.push_back(freq_values->value(idx));
  }
  Values::ShapeIndex idx2(dir_values->shape());
  for (int i = 0; i < dir_values->shape().length(0); ++i) {
    idx2.set("direction", i);
    directions.push_back(dir_values->value(idx2));
  }

  numPos = posName.size();
  numTime = validTime.size();
  numDirec = directions.size();
  numFreq = frequences.size();
  numRealizations = inv->realizationCount;

  return true;
}

static FieldData_cp find_request_field(Inventory_cp inv, InventoryBase_cps& request, const std::string& id)
{
  FieldData_cp fld;
  if (inv)
    fld = inv->findFieldById(id);
  if (fld)
    request.insert(fld);
  return fld;
}

static Values_cp field_values(const name2value_t& n2v, FieldData_cp fld)
{
  if (fld) {
    name2value_t::const_iterator it = n2v.find(fld->id());
    if (it != n2v.end())
      return it->second;
  }
  return Values_cp();
}

SpectrumPlot* SpectrumData::getData(const std::string& name, const miTime& time, int realization)
{
  METLIBS_LOG_SCOPE(LOGVAL(name) << LOGVAL(time));

  int iPos = 0;
  while (iPos < numPos && posName[iPos] != name)
    iPos++;
  if (iPos == numPos)
    return 0;

  int iTime = 0;
  while (iTime < numTime && validTime[iTime] != time)
    iTime++;
  if (iTime == numTime)
    return 0;


  const LonLat pos = LonLat::fromDegrees(posLongitude[iPos], posLatitude[iPos]);
  const Time user_time(util::from_miTime(time));

  const vcross::ModelReftime mr(modelName, reftime);
  vcross::Inventory_cp inv = collector->getResolver()->getInventory(mr);
  if (!inv) {
    METLIBS_LOG_WARN("no inventory");
    return 0;
  }

  size_t index;
  Crossection_cp cs = inv->findCrossectionPoint(pos, index);
  if (!cs) {
    METLIBS_LOG_WARN("no crossection");
    return 0;
  }
  METLIBS_LOG_DEBUG(LOGVAL(cs->label()) << LOGVAL(index));

  InventoryBase_cps request;
  FieldData_cp field_spec = find_request_field(inv, request, "SPEC");
  if (!field_spec) {
    METLIBS_LOG_WARN("no SPEC field");
    return 0;
  }
  FieldData_cp field_hmo = find_request_field(inv, request, "hs");
  FieldData_cp field_wdir = find_request_field(inv, request, "dd");
  FieldData_cp field_wspeed = find_request_field(inv, request, "ff");
  FieldData_cp field_tpeak = find_request_field(inv, request, "tp");
  FieldData_cp field_ddpeak = find_request_field(inv, request, "Pdir");

  name2value_t n2v;
  fs->getWaveSpectrumValues(mr.reftime,cs, index, user_time, request, n2v, realization);

  std::unique_ptr<SpectrumPlot> spp(new SpectrumPlot);
  spp->prognostic = true;
  spp->modelName = modelName;
  spp->posName = posName[iPos];
  spp->numDirec = numDirec;
  spp->numFreq = numFreq;
  spp->realization = realization;
  spp->validTime = validTime[iTime];
  spp->forecastHour = forecastHour[iTime];
  spp->directions = directions;
  spp->frequences = frequences;

  if (Values_cp hmo = field_values(n2v, field_hmo)) {
    Values::ShapeIndex idx_hmo(hmo->shape());
    idx_hmo.set(0,0);
    spp->hmo = hmo->value(idx_hmo);
  } else {
    spp->hmo = -1;
  }
  if (Values_cp wdir = field_values(n2v, field_wdir)) {
    Values::ShapeIndex idx_wdir(wdir->shape());
    idx_wdir.set(0,0);
    spp->wdir=wdir->value(idx_wdir);
  } else {
    spp->wdir = -1;
  }

  if (Values_cp wspeed = field_values(n2v, field_wspeed)) {
    Values::ShapeIndex idx_wspeed(wspeed->shape());
    idx_wspeed.set(0,0);
    spp->wspeed=wspeed->value(idx_wspeed);
  } else {
    spp->wspeed = -1;
  }

  if (Values_cp tpeak = field_values(n2v, field_tpeak)) {
    Values::ShapeIndex idx_tpeak(tpeak->shape());
    idx_tpeak.set(0,0);
    spp->tPeak = tpeak->value(idx_tpeak);
  } else {
    spp->tPeak = -1;
  }

  if (Values_cp ddpeak = field_values(n2v, field_ddpeak)) {
    Values::ShapeIndex idx_ddpeak(ddpeak->shape());
    idx_ddpeak.set(0,0);
    spp->ddPeak = ddpeak->value(idx_ddpeak);
  } else {
    spp->ddPeak = -1;
  }

  Values_cp spec_values = field_values(n2v, field_spec);
  if (!spec_values) {
    METLIBS_LOG_WARN("no spec_values");
    return 0;
  }
  const Values::Shape& shape(spec_values->shape());
  Values::ShapeIndex idx(shape);
  const int size = shape.volume(),
      pos_direction = shape.position("direction"),
      pos_freq = shape.position("freq");
  float *spec = new float[size];
  int ii = 0;
  for (int i = 0; i < shape.length(pos_freq); i++) {
    for (int j = 0; j < shape.length(pos_direction); j++) {
      idx.set(pos_direction, j);
      idx.set(pos_freq, i);
      spec[ii] = spec_values->value(idx);
      if (std::isnan(spec[ii])) {
        METLIBS_LOG_DEBUG("NAN");
        return 0;
      }
      ++ii;
    }
  }

  int n = numDirec + 1;
  int m = (numDirec + 1) * numFreq;
  float *sdata = new float[m];
  float *xdata = new float[m];
  float *ydata = new float[m];

  float radstep = fabsf(directions[0] - directions[1]) * DEG_TO_RAD;

  // extend direction size, for graphics
  for (int j = 0; j < numFreq; j++) {
    for (int i = 0; i < numDirec; i++) {
      sdata[j * n + i] = spec[j * numDirec + i];
    }
    sdata[j * n + numDirec] = spec[j * numDirec + 0];
  }

  float smax = 0.0;
  m = numDirec * numFreq;
  for (int i = 0; i < m; i++)
    if (smax < spec[i])
      smax = spec[i];
  spp->specMax = smax;

  // total for all directions
  spp->eTotal.clear();
  float etotmax = 0.0;
  for (int j = 0; j < numFreq; j++) {
    float etot = 0.0;
    for (int i = 0; i < numDirec; i++)
      etot += spec[j * numDirec + i] * radstep;
    spp->eTotal.push_back(etot);
    if (etotmax < etot)
      etotmax = etot;
  }

  // ok until more than one spectrum shown on one diagram !!!
  SpectrumPlot::eTotalMax = etotmax;

  float *cosdir = new float[n];
  float *sindir = new float[n];
  float rotation = 0; //spp->northRotation;
  float angle;
  for (int i = 0; i < numDirec; i++) {
    angle = (90. - directions[i] - rotation) * DEG_TO_RAD;
    cosdir[i] = cos(angle);
    sindir[i] = sin(angle);
  }
  cosdir[n - 1] = cosdir[0];
  sindir[n - 1] = sindir[0];

  for (int j = 0; j < numFreq; j++) {
    for (int i = 0; i < n; i++) {
      xdata[j * n + i] = frequences[j] * cosdir[i];
      ydata[j * n + i] = frequences[j] * sindir[i];
    }
  }

  delete[] spp->sdata;
  delete[] spp->xdata;
  delete[] spp->ydata;

  spp->sdata = sdata;
  spp->xdata = xdata;
  spp->ydata = ydata;

  delete[] spec;
  delete[] cosdir;
  delete[] sindir;

  METLIBS_LOG_DEBUG("got spp data");
  return spp.release();
}
