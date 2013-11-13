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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diVcrossField.h"

#include "diLocationPlot.h"

#include <diField/diFieldManager.h>
#include <puDatatypes/miCoordinates.h>
#include <puTools/miStringBuilder.h>

#include <boost/foreach.hpp>

#include <cmath>
#include <set>

#define MILOGGER_CATEGORY "diana.VcrossField"
#include <miLogger/miLogging.h>

VcrossField::VcrossField(const std::string& modelname, FieldManager* fieldm)
  : VcrossSource(modelname)
  , fieldManager(fieldm)
{
  METLIBS_LOG_SCOPE();
}

VcrossField::~VcrossField()
{
  METLIBS_LOG_SCOPE();
  cleanup();
}

void VcrossField::cleanup()
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName()));

  mCrossections.clear();
  startLatitude=stopLatitude=startLongitude=stopLongitude=-9999.0;
}

bool VcrossField::update()
{
  METLIBS_LOG_SCOPE(LOGVAL(modelName()));

  std::vector<VcrossData::Cut> predefinedCrossSections;
  if (not fieldManager->invVCross(modelName(), validTime, forecastHour, params, predefinedCrossSections))
    return false;

  BOOST_FOREACH(const VcrossData::Cut& pcs, predefinedCrossSections) {
    if (pcs.lonlat.empty())
      continue;

    mCrossections.push_back(pcs);
  }

  // Init lat/lons for dynamic crossection
  startLatitude = -9999;
  startLongitude = -9999;
  stopLatitude = -9999;
  stopLongitude = -9999;
  return true;
}

/*
 * Set a lat/lon pair clicked on the Diana map
 * The first time startL{atitude,ongitude} is set and the second
 * time stopL{atitude,ongitude}.
 */
std::string VcrossField::setLatLon(float lat, float lon)
{
  METLIBS_LOG_SCOPE(LOGVAL(lat) << LOGVAL(lon));

  if ((startLatitude>-9999) && (startLongitude>-9999) && (stopLatitude<=-9999) && (stopLongitude<=-9999)) {
    // If start already set, compute the crossection
    stopLatitude = lat;
    stopLongitude = lon;

    // FIXME use proj4/geod; similar in MapPlot::plotGeoGrid
    LonLat p0 = LonLat::fromDegrees(startLongitude, startLatitude);
    LonLat pE = LonLat::fromDegrees(stopLongitude,  stopLatitude);

    const double distance = p0.distanceTo(pE); // in m
    const int noOfPoints = std::max((int)(distance/14000), 100);
    const double step = distance / (noOfPoints-1);
    METLIBS_LOG_DEBUG(LOGVAL(distance) << LOGVAL(noOfPoints) << LOGVAL(step));

    VcrossData::Cut cs;
    cs.lonlat.push_back(p0);
    for(int i=2; i<noOfPoints; i++) {
      p0 = p0.stepTo(step, pE);
      METLIBS_LOG_DEBUG("p[" << (i-1) << "] = " << p0.lonDeg() << "," << p0.latDeg());
      cs.lonlat.push_back(p0);
    }
    cs.lonlat.push_back(pE);

    cs.name = (miutil::StringBuilder()
        << '(' << startLatitude << ',' << startLongitude
        << ")->(" << stopLatitude << ',' << stopLongitude << ')');

    mCrossections.push_back(cs);
    return cs.name;
  } else {
    // If first time or not all set

    startLatitude = lat;
    startLongitude = lon;
    stopLatitude = -9999;
    stopLongitude = -9999;

    return "";
  }
}

VcrossData* VcrossField::getCrossData(const std::string& csname, const std::set<std::string>& parameterIds, const miutil::miTime& time)
{
  METLIBS_LOG_SCOPE();

  if (mCrossections.empty())
    return 0;

  const int csIdx = findCrossection(csname);
  if (csIdx < 0) {
    METLIBS_LOG_WARN("cross section '" << csname << "' unknown");
    return 0;
  }

  const VcrossData::Cut& cs = mCrossections[csIdx];
  METLIBS_LOG_DEBUG(LOGVAL(csname) << " time=" << time.format("%Y%m%d%H%M%S"));

  const std::vector<std::string> parameter_v(parameterIds.begin(), parameterIds.end());
  std::auto_ptr<VcrossData> data(fieldManager->makeVCross(modelName(), time, cs, parameter_v));
  if (not data.get()) {
    METLIBS_LOG_DEBUG("fetch failed, no plot");
    return 0;
  }
  return data.release();
}

VcrossData* VcrossField::getTimeData(const std::string& csname, const std::set<std::string>& parameters, int csPositionIndex)
{
  METLIBS_LOG_SCOPE();

  if (mCrossections.empty())
    return 0;

  METLIBS_LOG_DEBUG(LOGVAL(csname) << LOGVAL(csPositionIndex));

  const int vcross = findCrossection(csname);
  if (vcross < 0)
    return 0;

  std::auto_ptr<VcrossData> data(new VcrossData());

#if 0
  bool gotCrossection = fieldManager->makeVCrossTimeGraph(modelName(), validTime,
      latitude,longitude,params, vcp->alevel,vcp->blevel,
      vcp->nPoint,crossData,multiLevel,vcp->iundef);
  
  if (not gotCrossection) {
    METLIBS_LOG_DEBUG("fetch failed, no plot");
    return 0;
  }
#else
  // fake data
  const int NLEVEL = 40;
  VcrossData::ZAxisPtr zax(new VcrossData::ZAxis());
  zax->name = "fake axis";
  zax->quantity = VcrossData::ZAxis::PRESSURE;
  zax->mValues = boost::shared_array<float>(new float[NLEVEL]);
  for (int i=0; i<NLEVEL; ++i)
      for (size_t j=0; j<validTime.size(); ++j)
        zax->setValue(i, j, 1013 - 0.5*i);
  data->zAxes.push_back(zax);

  int idx = 0;
  BOOST_FOREACH(const std::string& p, parameters) {
    VcrossData::ParameterData pd;
    pd.mPoints = validTime.size();
    pd.mLevels = NLEVEL;
    pd.alloc();
    for (int i=0; i<NLEVEL; ++i) {
      for (size_t j=0; j<validTime.size(); ++j)
        pd.setValue(i, j, idx + drand48());
    }
    data->parameters.insert(std::make_pair(p, pd));
  }
#endif // fake data

  return data.release();
}

int VcrossField::findCrossection(const std::string& csname) const
{
  int vcross = -1;
  for (size_t i=0; i<mCrossections.size(); i++) {
    if (csname == mCrossections[i].name) {
      vcross = i;
      break;
    }
  }

  if (vcross < 0 or mCrossections[vcross].lonlat.empty())
    return -1;

  return vcross;
}
