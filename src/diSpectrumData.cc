/*
 Diana - A Free Meteorological Visualisation Tool

 $Id: diSpectrumData.cc 3822 2013-11-01 19:42:36Z alexanderb $

 Copyright (C) 2006 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diSpectrumData.h>
#include <diSpectrumPlot.h>

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>
#include <diField/VcrossUtil.h>
#include <boost/foreach.hpp>
#include <sstream>

#define MILOGGER_CATEGORY "diana.SpectrumData"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;
using namespace vcross;

// Default constructor
SpectrumData::SpectrumData(const std::string& filename,
    const std::string& modelname) :
    fileName(filename), modelName(modelname), numPos(0), numTime(0), numDirec(
        0), numFreq(0)
{
  METLIBS_LOG_DEBUG("++ SpectrumData::Constructor");
}

// Destructor
SpectrumData::~SpectrumData()
{
  METLIBS_LOG_DEBUG("++ SpectrumData::Destructor");
}

bool SpectrumData::readFileHeader(const std::string& setup_line)
{
  METLIBS_LOG_SCOPE();

  fs = vcross::FimexSource_p(new vcross::FimexSource(fileName, "netcdf"));

  vcross::Inventory_cp inv = fs->getInventory();
  if (not inv) {
    METLIBS_LOG_ERROR("Can't get inventory for " << fileName);
    return false;
  }

  BOOST_FOREACH(vcross::Crossection_cp cs, inv->crossections){

  for( size_t i=0; i<cs->points.size(); ++i ) {
    METLIBS_LOG_DEBUG(LOGVAL(cs->points[i].latDeg()));
    METLIBS_LOG_DEBUG(LOGVAL(cs->points[i].lonDeg()));
    ostringstream ost;
    float lon = int (fabs(cs->points[i].lonDeg()) * 10);
    lon /= 10.;
    float lat = int (fabs(cs->points[i].latDeg()) * 10);
    lat /= 10.;
    ost << lon;
    if ( cs->points[i].lonDeg() < 0. )
      ost  << "W " ;
    else
      ost  << "E " ;
    ost << lat;
    if ( cs->points[i].latDeg() < 0. )
      ost  << "S" ;
    else
      ost  << "N" ;
    posName.push_back( ost.str() );
    posLatitude.push_back( cs->points[i].latDeg() );
    posLongitude.push_back( cs->points[i].lonDeg() );
  }
}

  BOOST_FOREACH(vcross::Time::timevalue_t time, inv->times.values){
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
  const Time reftime = fs->getDefaultReferenceTime();
  fs->getWaveSpectrumValues(reftime, cs0, 2, inv->times.at(0), request, n2v);
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

  return true;
}

SpectrumPlot* SpectrumData::getData(const std::string& name, const miTime& time)
{
  METLIBS_LOG_DEBUG("++ SpectrumData::getData   "<<name<<"   "<<time);

  SpectrumPlot *spp = 0;

  int iPos = 0;
  while (iPos < numPos && posName[iPos] != name)
    iPos++;
  if (iPos == numPos)
    return spp;

  int iTime = 0;
  while (iTime < numTime && validTime[iTime] != time)
    iTime++;
  if (iTime == numTime)
    return spp;


  const LonLat pos = LonLat::fromDegrees(posLongitude[iPos], posLatitude[iPos]);
  const Time user_time(util::from_miTime(time));

  vcross::Inventory_cp inv = fs->getInventory();
  size_t index;
  Crossection_cp cs = inv->findCrossectionPoint(pos, index);
  InventoryBase_cps request;
  FieldData_cp field_spec = inv->findFieldById("SPEC");
  if ( !field_spec.get() )
    return spp;
  request.insert(field_spec);
  FieldData_cp field_hmo = inv->findFieldById("hs");
  if ( field_hmo.get() )
    request.insert(field_hmo);
  FieldData_cp field_wdir = inv->findFieldById("dd");
  if ( field_wdir.get() )
    request.insert(field_wdir);
  FieldData_cp field_wspeed = inv->findFieldById("ff");
  if ( field_wspeed.get() )
    request.insert(field_wspeed);
  FieldData_cp field_tpeak = inv->findFieldById("tp");
  if ( field_tpeak.get() )
    request.insert(field_tpeak);
  FieldData_cp field_ddpeak = inv->findFieldById("Pdir");
  if ( field_ddpeak.get() )
    request.insert(field_ddpeak);

  name2value_t n2v;
  const Time reftime = fs->getDefaultReferenceTime();
  fs->getWaveSpectrumValues(reftime, cs, index, user_time, request, n2v);

  spp = new SpectrumPlot();
  spp->prognostic = true;
  spp->modelName = modelName;
  spp->posName = posName[iPos];
  spp->numDirec = numDirec;
  spp->numFreq = numFreq;
  spp->validTime = validTime[iTime];
  spp->forecastHour = forecastHour[iTime];
  spp->directions = directions;
  spp->frequences = frequences;

  if ( field_hmo.get() ) {
    Values_cp hmo = n2v[field_hmo->id()];
    Values::ShapeIndex idx_hmo(hmo->shape());
    idx_hmo.set(0,0);
    spp->hmo = hmo->value(idx_hmo);
  } else {
    spp->hmo = -1;
  }
  if ( field_wdir.get() ) {
  Values_cp wdir = n2v[field_wdir->id()];
  Values::ShapeIndex idx_wdir(wdir->shape());
  idx_wdir.set(0,0);
  spp->wdir=wdir->value(idx_wdir);
  } else {
    spp->wdir = -1;
  }

  if ( field_wspeed.get() ) {
  Values_cp wspeed = n2v[field_wspeed->id()];
  Values::ShapeIndex idx_wspeed(wspeed->shape());
  idx_wspeed.set(0,0);
  spp->wspeed=wspeed->value(idx_wspeed);
  } else {
    spp->wspeed = -1;
  }

  if ( field_tpeak.get() ) {
  Values_cp tpeak = n2v[field_tpeak->id()];
  Values::ShapeIndex idx_tpeak(tpeak->shape());
  idx_tpeak.set(0,0);
  spp->tPeak = tpeak->value(idx_tpeak);
  } else {
    spp->tPeak = -1;
  }

  if ( field_ddpeak.get() ) {
  Values_cp ddpeak = n2v[field_ddpeak->id()];
  Values::ShapeIndex idx_ddpeak(ddpeak->shape());
  idx_ddpeak.set(0,0);
  spp->ddPeak = ddpeak->value(idx_ddpeak);
  } else {
    spp->ddPeak = -1;
  }

  Values_cp spec_values = n2v[field_spec->id()];
  const Values::Shape& shape(spec_values->shape());
  Values::ShapeIndex idx(spec_values->shape());

  int size = shape.length(0) * shape.length(1);
  float *spec = new float[size];

  int ii = 0;
  for (int i = 0; i < shape.length(1); i++) {
    for (int j = 0; j < shape.length(0); j++) {
      idx.set("direction", j);
      idx.set("freq", i);
      spec[ii] = spec_values->value(idx);
      if (spec[ii] != spec[ii]) {
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

  const float rad = 3.141592654 / 180.;

  float radstep = fabsf(directions[0] - directions[1]) * rad;

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
    angle = (90. - directions[i] - rotation) * rad;
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
  //###############################################
  //  for (int i=0; i<numDirec; i++)
  //    METLIBS_LOG_DEBUG("direc,angle,x,y:  "
  //        <<directions[i]<<"  "<<90.-directions[i]-rotation<<"  "
  //        <<cosdir[i]<<"  "<<sindir[i]);
  //###############################################

  //######################################################################
  /******************************************************************
   float specmin= spec[0];
   float specmax= spec[0];
   for (int i=0; i<numDirec*numFreq; i++) {
   if (specmin>spec[i]) specmin= spec[i];
   if (specmax<spec[i]) specmax= spec[i];
   }
   float sdatmin= sdata[0];
   float sdatmax= sdata[0];
   for (int i=0; i<m; i++) {
   if (sdatmin>sdata[i]) sdatmin= sdata[i];
   if (sdatmax<sdata[i]) sdatmax= sdata[i];
   }
   float etotmin= spp->eTotal[0];
   float etotmax= spp->eTotal[0];
   for (int i=0; i<numFreq; i++) {
   if (etotmin>spp->eTotal[i]) etotmin= spp->eTotal[i];
   if (etotmax<spp->eTotal[i]) etotmax= spp->eTotal[i];
   }
   METLIBS_LOG_DEBUG("   northRotation=   "<<spp->northRotation);
   METLIBS_LOG_DEBUG("   wspeed=          "<<spp->wspeed);
   METLIBS_LOG_DEBUG("   wdir=            "<<spp->wdir);
   METLIBS_LOG_DEBUG("   hmo=             "<<spp->hmo);
   METLIBS_LOG_DEBUG("   tPeak=           "<<spp->tPeak);
   METLIBS_LOG_DEBUG("   ddPeak=          "<<spp->ddPeak);
   METLIBS_LOG_DEBUG("   specmin,specmax: "<<specmin<<"  "<<specmax);
   METLIBS_LOG_DEBUG("   sdatmin,sdatmax: "<<sdatmin<<"  "<<sdatmax);
   METLIBS_LOG_DEBUG("   etotmin,etotmax: "<<etotmin<<"  "<<etotmax);
   ******************************************************************/
  //######################################################################
  if (spp->sdata)
    delete[] spp->sdata;
  if (spp->xdata)
    delete[] spp->xdata;
  if (spp->ydata)
    delete[] spp->ydata;

  spp->sdata = sdata;
  spp->xdata = xdata;
  spp->ydata = ydata;

  delete[] spec;
  delete[] cosdir;
  delete[] sindir;

  return spp;
}
