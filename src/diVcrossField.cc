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
#include "diVcrossPlot.h"
#include "diLocationPlot.h"

#include <diField/diFieldManager.h>
#include <puDatatypes/miCoordinates.h>

#include <cmath>
#include <set>

#define MILOGGER_CATEGORY "diana.VcrossField"
#include <miLogger/miLogging.h>

using namespace std;

namespace /* anonymous */ {
const int VCOORD_HYBRID = 10;
const int VCOORD_OTHER = 1;
} // namespace anonymous

VcrossField::VcrossField(const std::string& modelname, FieldManager* fieldm)
    : modelName(modelname), fieldManager(fieldm), lastVcross(-1), lastTgpos(-1),
    lastVcrossPlot(0)
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
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(modelName));

  names.clear();
  crossSections.clear();
  startLatitude=stopLatitude=startLongitude=stopLongitude=-9999.0;
  cleanupCache();
  cleanupTGCache();
}

/*
 * Clear the active timegraph
 */
void VcrossField::cleanupTGCache()
{
  METLIBS_LOG_SCOPE();

  // Clear timeGraph
  delete lastVcrossPlot;
  lastVcrossPlot = 0;

  if (lastVcrossData.size()) {
    for (size_t i=0;i<lastVcrossData.size();i++) {
      delete[] lastVcrossData[i];
    }
  }
  lastVcrossData.clear();
  lastVcrossMultiLevel.clear();
  lastTgpos = -1;
  lastVcross = -1;
}


/*
 * Clear the active crossections
 */
void VcrossField::cleanupCache()
{
  METLIBS_LOG_SCOPE();

  for (size_t i = 0; i < VcrossPlotVector.size(); i++)
    delete VcrossPlotVector[i];
  VcrossPlotVector.clear();

  map<int, vector<float*> >::iterator vd, vdend = VcrossDataMap.end();
  for (vd = VcrossDataMap.begin(); vd != vdend; vd++) {
    if (vd->second.size()) {
      for (size_t j = 0; j < vd->second.size(); j++)
        delete[] vd->second[j];
    }
  }
  VcrossDataMap.clear();
}

bool VcrossField::update()
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(modelName));

  return true;
}

/*
 * Get parameters from field source
 */
bool VcrossField::getInventory()
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(modelName));

  // Get parameters
  if (not fieldManager->invVCross(modelName, validTime, forecastHour, params))
    return false;

  // Set dynamic as the choosen crossection
  names.push_back("Dynamic");

  const std::map<miutil::miString,int>::const_iterator pnend = VcrossPlot::vcParName.end();
  vector<int> iparam;
  int vcoord = VCOORD_HYBRID;

  // For every parameter, check if it is specified in the setup file
  for (unsigned int i=0;i<params.size();i++) {
    const map<miutil::miString,int>::const_iterator pn = VcrossPlot::vcParName.find(params[i]);
    if (pn != pnend) {
      METLIBS_LOG_DEBUG("Parameter " << params[i] << "("<<pn->second<<") defined in setup");
      iparam.push_back(pn->second);
    } else {
      METLIBS_LOG_DEBUG("Parameter " << params[i] << " not defined in setup");
      params.erase(params.begin()+i);
      i--;
    }
  }

  // Create the Vcross contents
  VcrossPlot::makeContents(modelName, iparam, vcoord);

#ifdef DEBUGPRINT
  { std::ostringstream p;
    p << "params: ";
    for(int i=0;i<params.size();i++) {
      p << " " << params[i];
    }
    METLIBS_LOG_DEBUG(p);
  }
#endif

  // Put some special parameters (see diVcrossPlot.cc, prepareData for description)
  params.push_back("psurf");
  params.push_back("topography");
  params.push_back("-1001");
  params.push_back("-1002");
  params.push_back("-1007");
  params.push_back("-1004");
  params.push_back("-1005");
  params.push_back("-1006");
  params.push_back("-1008");
  params.push_back("-1009");

  // Init lat/lons for dynamic crossection
  startLatitude = -9999;
  startLongitude = -9999;
  stopLatitude = -9999;
  stopLongitude = -9999;
  return true;
}


vector<std::string> VcrossField::getFieldNames()
{
  METLIBS_LOG_SCOPE();
  update();
  return VcrossPlot::getFieldNames(modelName);
}

/*
 * Set a lat/lon pair clicked on the Diana map
 * The first time startL{atitude,ongitude} is set and the second
 * time stopL{atitude,ongitude}.
 */
bool VcrossField::setLatLon(float lat, float lon)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(lat) << LOGVAL(lon));

  if ((startLatitude>-9999) && (startLongitude>-9999) && (stopLatitude<=-9999) && (stopLongitude<=-9999)) {
    // If start already set, compute the crossection
    stopLatitude = lat;
    stopLongitude = lon;

    /*
    var R = 6371; // km
    var dLat = (lat2-lat1).toRad();
    var dLon = (lon2-lon1).toRad();
    var a = Math.sin(dLat/2) * Math.sin(dLat/2) +
            Math.cos(lat1.toRad()) * Math.cos(lat2.toRad()) *
            Math.sin(dLon/2) * Math.sin(dLon/2);
    var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
    var d = R * c;
          var y = Math.sin(dLon) * Math.cos(lat2);
          var x = Math.cos(lat1)*Math.sin(lat2) -
                  Math.sin(lat1)*Math.cos(lat2)*Math.cos(dLon);
          var brng = Math.atan2(y, x).toBrng();

    Destination point
    var lat2 = Math.asin( Math.sin(lat1)*Math.cos(d/R) +
                          Math.cos(lat1)*Math.sin(d/R)*Math.cos(brng) );
    var lon2 = lon1 + Math.atan2(Math.sin(brng)*Math.sin(d/R)*Math.cos(lat1),
                                 Math.cos(d/R)-Math.sin(lat1)*Math.sin(lat2));
          */

    // Compute distance and points on line
    float radius = 6371000.0;
    float TORAD = M_PI/180;
    float dLon = (stopLongitude-startLongitude)*TORAD;
    float lat1 = startLatitude*TORAD;
    float lat2 = stopLatitude*TORAD;
    float lon1 = startLongitude*TORAD;
    float lon2 = stopLongitude*TORAD;

    float y = sin(dLon)*cos(lat2);
    float x = cos(lat1)*sin(lat2) - sin(lat1)*cos(lat2)*cos(dLon);
    float brng = (((int)(atan2(y,x)/TORAD+360))%360)*TORAD;

    float distance = acosf(sinf(lat1)*sin(lat2) +
                cos(lat1)*cos(lat2) * cos(lon2-lon1)) * radius;

    // 10 km between points or minimum 100 points
    int noOfPoints = (int)distance/14000;
    if (noOfPoints<100)
      noOfPoints = 100;
    int elem= crossSections.size()-1;

    float step = distance/(noOfPoints-1.0);
    for(int i=1;i<noOfPoints;i++) {
      float stepLat = (asin(sin(lat1)*cos(i*step/radius) +
                      cos(lat1)*sin(i*step/radius)*cos(brng)))/TORAD;
      float stepLon = (lon1 + atan2(sin(brng)*sin(i*step/radius)*cos(lat1),
          cos(i*step/radius)-sin(lat1)*sin(lat2)))/TORAD;
      crossSections[elem].xpos.push_back(stepLon);
      crossSections[elem].ypos.push_back(stepLat);
    }
    crossSections[elem].name = miutil::miString("(" + miutil::miString(startLatitude) + "," + miutil::miString(startLongitude) +
        ")->(" + miutil::miString(stopLatitude) + "," + miutil::miString(stopLongitude) + ")");

    // Rebuild the crossection list
    names.clear();
    for(size_t i=0;i<crossSections.size();i++)
      names.push_back(crossSections[i].name);
    return true;
  } else {
    // If first time or not all set
    startLatitude = lat;
    startLongitude = lon;
    stopLatitude = -9999;
    stopLongitude = -9999;

    // Push back a new LocationElement
    LocationElement el;
    int nelem= crossSections.size();
    crossSections.push_back(el);
    crossSections[nelem].xpos.push_back(startLongitude);
    crossSections[nelem].ypos.push_back(startLatitude);
    return false;
  }
}

/*
 * Get the crossections for plotting
 */
void VcrossField::getMapData(vector<LocationElement>& elements)
{
  METLIBS_LOG_SCOPE();

  int nelem= elements.size();
  for(size_t i=0;i<crossSections.size();i++) {
    LocationElement el;
    elements.push_back(el);
    int noPoints = crossSections[i].xpos.size();
    elements[nelem].name = crossSections[i].name;
    for(int j=0;j<noPoints;j++) {
      elements[nelem].xpos.push_back(crossSections[i].xpos[j]);
      elements[nelem].ypos.push_back(crossSections[i].ypos[j]);
    }
    nelem++;
  }
}

/*
 * Get a crossection
 */
VcrossPlot* VcrossField::getCrossection(const std::string& name,
    const miutil::miTime& time, int tgpos)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(name) << " time=" << time.format("%Y%m%d%H%M%S") << LOGVAL(tgpos));

  if (crossSections.empty()) {
    METLIBS_LOG_DEBUG("no crossections, no plot");
    return 0;
  }

  int vcross=-1;
  // Find crossection
  for (size_t i=0;i<crossSections.size();i++) {
    if (name == crossSections[i].name)
      vcross = i;
  }
  METLIBS_LOG_DEBUG(LOGVAL(vcross));
  if (vcross < 0)
    return 0;
  METLIBS_LOG_DEBUG(LOGVAL(crossSections[vcross].xpos.size()));
  if (crossSections[vcross].xpos.size() <= 1)
    return 0;

  VcrossPlot *vcp = new VcrossPlot();
  vcp->modelName = modelName;
  vcp->crossectionName = modelName + name;

  vector<float*> crossData;
  vector<bool> multiLevel;

  // Set number of points according to what was computed in setLatLon
  vcp->nPoint = crossSections[vcross].xpos.size();

  // Cleanup cache
  if(tgpos < 0) {
    if(lastVcrossTime != time) {
      cleanupCache();
      for(size_t i=0;i<crossSections.size();i++)
        VcrossPlotVector.push_back(new VcrossPlot());
      lastVcrossTime = miutil::miTime(time);
    }
  } else {
    if(lastTgpos != tgpos && lastVcross != vcross) {
      cleanupTGCache();
    }
  }

  // If tgpos < 0 => not a timeGraph
  if(tgpos < 0) {
    // Check if crossection is cached (same time)
    if(VcrossDataMap[vcross].size() > 0) {
      METLIBS_LOG_DEBUG("Using cached copy for " << crossSections[vcross].name << " time: " <<time.isoTime());
      vcp->alevel = VcrossPlotVector[vcross]->alevel;
      vcp->blevel = VcrossPlotVector[vcross]->blevel;
      vcp->nPoint = VcrossPlotVector[vcross]->nPoint;
      vcp->numLev = VcrossPlotVector[vcross]->numLev;
      vcp->horizontalPosNum = VcrossPlotVector[vcross]->horizontalPosNum;
      vcp->vcoord = VcrossPlotVector[vcross]->vcoord;
      vcp->nTotal = VcrossPlotVector[vcross]->nTotal;
      vcp->validTime = miutil::miTime(VcrossPlotVector[vcross]->validTime);
      vcp->forecastHour = VcrossPlotVector[vcross]->forecastHour;
      vcp->refPosition = VcrossPlotVector[vcross]->refPosition;
      vcp->timeGraph = VcrossPlotVector[vcross]->timeGraph;
      vcp->vrangemin = VcrossPlotVector[vcross]->vrangemin;
      vcp->vrangemax = VcrossPlotVector[vcross]->vrangemax;
      vcp->iundef = VcrossPlotVector[vcross]->iundef;
      for(size_t i=0;i<VcrossDataMap[vcross].size();i++) {
        multiLevel.push_back(VcrossMultiLevelMap[vcross][i]);
        if(VcrossMultiLevelMap[vcross][i]) {
          float* data = new float[vcp->nPoint*vcp->numLev];
          for(int j=0;j<(vcp->nPoint*vcp->numLev);j++) {
            data[j] = VcrossDataMap[vcross][i][j];
          }
          crossData.push_back(data);
        } else {
          float* data = new float[vcp->nPoint];
          for(int j=0;j<vcp->nPoint;j++) {
            data[j] = VcrossDataMap[vcross][i][j];
          }
          crossData.push_back(data);
        }
      }
    } else {
      METLIBS_LOG_DEBUG("fetching crossection data from diFieldManager (not cached)");
      vector< boost::shared_array<float> > crossDataS;
      bool gotCrossection = fieldManager->makeVCross(modelName,time,
          crossSections[vcross].ypos,crossSections[vcross].xpos,params,
          vcp->alevel,vcp->blevel,crossDataS,multiLevel,vcp->iundef);

      if (not gotCrossection) {
        METLIBS_LOG_DEBUG("fetch failed, no plot");
        delete vcp;
        return 0;
      }

      vcp->numLev = vcp->alevel.size();
      vcp->horizontalPosNum = vcp->nPoint;
      if (vcp->blevel.size())
        vcp->vcoord = VCOORD_HYBRID;
      else
        vcp->vcoord = VCOORD_OTHER;
      vcp->nTotal = (vcp->nPoint * vcp->numLev);
      vcp->validTime = miutil::miTime(time);
      vcp->forecastHour = 0;
      vcp->refPosition=0.;
      vcp->timeGraph = false;
      vcp->vrangemin = 50;
      vcp->vrangemax = 1000;

      METLIBS_LOG_DEBUG(LOGVAL(vcp->nPoint) << LOGVAL(vcp->nTotal));

      if(int(VcrossPlotVector.size()) < (vcross+1))
        VcrossPlotVector.push_back(new VcrossPlot());
      // Cache the crossection
      VcrossPlotVector[vcross]->alevel = vcp->alevel;
      VcrossPlotVector[vcross]->blevel = vcp->blevel;
      VcrossPlotVector[vcross]->nPoint = vcp->nPoint;
      VcrossPlotVector[vcross]->numLev = vcp->numLev;
      VcrossPlotVector[vcross]->horizontalPosNum = vcp->horizontalPosNum;
      VcrossPlotVector[vcross]->vcoord = vcp->vcoord;
      VcrossPlotVector[vcross]->nTotal = vcp->nTotal;
      VcrossPlotVector[vcross]->validTime = miutil::miTime(vcp->validTime);
      VcrossPlotVector[vcross]->forecastHour = vcp->forecastHour;
      VcrossPlotVector[vcross]->refPosition = vcp->refPosition;
      VcrossPlotVector[vcross]->timeGraph = vcp->timeGraph;
      VcrossPlotVector[vcross]->vrangemin = vcp->vrangemin;
      VcrossPlotVector[vcross]->vrangemax = vcp->vrangemax;
      VcrossPlotVector[vcross]->iundef = vcp->iundef;

      for(size_t i=0;i<crossDataS.size();i++) {
        VcrossMultiLevelMap[vcross].push_back(multiLevel[i]);
        const int ndata = multiLevel[i] ? vcp->nTotal : vcp->nPoint;
        METLIBS_LOG_DEBUG(LOGVAL(params[i]) << LOGVAL(multiLevel[i]) << LOGVAL(ndata));
          
        float* data = new float[ndata];
        const float dxy = 10000;
        if (crossDataS[i]) {
          METLIBS_LOG_DEBUG("values from crossDataS");
          std::copy(crossDataS[i].get(), crossDataS[i].get() + ndata, data);
        } else if ((params[i] == "-1001" or params[i] == "-1002" or params[i] == "-1003") and not multiLevel[i]) {
          METLIBS_LOG_DEBUG("fake x/y/s grid data");
          for(int j=0; j<ndata; j += 1)
            data[j] = 0;//dxy*j;
        } else if (params[i] == "-1005" and not multiLevel[i]) {
          METLIBS_LOG_DEBUG("copying lat data");
          std::copy(crossSections[vcross].ypos.begin(), crossSections[vcross].ypos.end(), data);
        } else if (params[i] == "-1006" and not multiLevel[i]) {
          METLIBS_LOG_DEBUG("copying lon data");
          std::copy(crossSections[vcross].xpos.begin(), crossSections[vcross].xpos.end(), data);
        } else if (params[i] == "-1007" and not multiLevel[i]) {
          METLIBS_LOG_DEBUG("fake dx data");
          const std::vector<float>& lons = crossSections[vcross].xpos, &lats = crossSections[vcross].ypos;
          const size_t nPoint = lons.size();
          if (nPoint >= 2) {
            data[0] = 0;
            miCoordinates c0(lons[0], lats[0]);
            for (size_t c=1; c<nPoint; ++c) {
              const miCoordinates c1(lons[c], lats[c]);
              data[c] = c0.distanceTo(c1);
              c0 = c1;
            }
          }
        } else if ((params[i] == "-1008" or params[i] == "-1009") and not multiLevel[i]) {
          METLIBS_LOG_DEBUG("fake rot data");
          std::fill(data, data+ndata, std::sqrt(2)/2);
        } else {
          METLIBS_LOG_DEBUG("no crossdata for '" << params[i] << "', using 0s");
          std::fill(data, data+ndata, 0);
        }
        VcrossDataMap[vcross].push_back(data);

        // FIXME this copy is only to have something in crossdata (instead of crossDataS)
        float* cdata = new float[ndata];
        std::copy(data, data+ndata, cdata);
        crossData.push_back(cdata);

        { std::ostringstream debug;
          std::copy(data, data+std::min(10, ndata), std::ostream_iterator<float>(debug, " << "));
          METLIBS_LOG_DEBUG(LOGVAL(params[i]) << " values=" << debug.str());
        }
      }
    }
  } else {
    if(lastVcross == vcross && tgpos == lastTgpos) {
      // Use cached timeGraph (only used to be able to change parameters)
      METLIBS_LOG_DEBUG("Using cached timeGraph for " << crossSections[vcross].name << "," << tgpos);
      vcp->alevel = lastVcrossPlot->alevel;
      vcp->blevel = lastVcrossPlot->blevel;
      vcp->nPoint = lastVcrossPlot->nPoint;
      vcp->numLev = lastVcrossPlot->numLev;
      vcp->horizontalPosNum = lastVcrossPlot->horizontalPosNum;
      vcp->vcoord = lastVcrossPlot->vcoord;
      vcp->nTotal = lastVcrossPlot->nTotal;
      vcp->validTimeSeries = lastVcrossPlot->validTimeSeries;
      vcp->forecastHourSeries = lastVcrossPlot->forecastHourSeries;
      vcp->refPosition = lastVcrossPlot->refPosition;
      vcp->timeGraph = lastVcrossPlot->timeGraph;
      vcp->vrangemin = lastVcrossPlot->vrangemin;
      vcp->vrangemax = lastVcrossPlot->vrangemax;
      vcp->iundef = lastVcrossPlot->iundef;
      for(size_t i=0;i<lastVcrossData.size();i++) {
        multiLevel.push_back(lastVcrossMultiLevel[i]);
        if(lastVcrossMultiLevel[i]) {
          float* data = new float[vcp->nPoint*vcp->numLev];
          for(int j=0;j<(vcp->nPoint*vcp->numLev);j++) {
            data[j] = lastVcrossData[i][j];
          }
          crossData.push_back(data);
        } else {
          float* data = new float[vcp->nPoint];
          for(int j=0;j<vcp->nPoint;j++) {
            data[j] = lastVcrossData[i][j];
          }
          crossData.push_back(data);
        }
      }
    } else {
      METLIBS_LOG_DEBUG("Using noncached timeGraph for " << crossSections[vcross].name << "," << tgpos);
      // No cached timeGraph
      vector<float> xpos;
      vector<float> ypos;
      vector<int> forecastHours;
      vector<miutil::miTime> validTimes;
      xpos.push_back(crossSections[vcross].xpos[tgpos]);
      ypos.push_back(crossSections[vcross].ypos[tgpos]);
      // TimeGraph for every sixth hour
      for(unsigned int step=0; step<validTime.size(); step+=6) {
        forecastHours.push_back(step);
        validTimes.push_back(validTime[step]);
      }
      vcp->nPoint = validTimes.size();
      float latitude = crossSections[vcross].ypos[tgpos];
      float longitude = crossSections[vcross].xpos[tgpos];
      bool gotCrossection = fieldManager->makeVCrossTimeGraph(modelName,validTimes,
          latitude,longitude,params, vcp->alevel,vcp->blevel,
          vcp->nPoint,crossData,multiLevel,vcp->iundef);
      if(!gotCrossection) {
        delete vcp;
        return 0;
      }
      vcp->numLev = vcp->alevel.size();
      vcp->nTotal = vcp->nPoint*vcp->numLev;
      vcp->horizontalPosNum = forecastHours.size();
      if (vcp->blevel.size())
        vcp->vcoord = VCOORD_HYBRID;
      else
        vcp->vcoord = VCOORD_OTHER;
      vcp->validTimeSeries = validTimes;
      vcp->forecastHourSeries = forecastHours;
      vcp->refPosition=0.;
      vcp->timeGraph = true;
      vcp->vrangemin = 100;
      vcp->vrangemax = 1050;

      // Save a copy
      if(lastVcrossPlot == 0)
        lastVcrossPlot = new VcrossPlot();
      VcrossPlotVector.push_back(new VcrossPlot());
      lastVcrossPlot->alevel = vcp->alevel;
      lastVcrossPlot->blevel = vcp->blevel;
      lastVcrossPlot->nPoint = vcp->nPoint;
      lastVcrossPlot->numLev = vcp->numLev;
      lastVcrossPlot->horizontalPosNum = vcp->horizontalPosNum;
      lastVcrossPlot->vcoord = vcp->vcoord;
      lastVcrossPlot->nTotal = vcp->nTotal;
      lastVcrossPlot->validTimeSeries = vcp->validTimeSeries;
      lastVcrossPlot->forecastHourSeries = vcp->forecastHourSeries;
      lastVcrossPlot->refPosition = vcp->refPosition;
      lastVcrossPlot->timeGraph = vcp->timeGraph;
      lastVcrossPlot->vrangemin = vcp->vrangemin;
      lastVcrossPlot->vrangemax = vcp->vrangemax;
      lastVcrossPlot->iundef = vcp->iundef;

      if(lastVcrossData.size()) {
        for(size_t i=0;i<lastVcrossData.size();i++)
          delete[] lastVcrossData[i];
      }
      lastVcrossData.clear();
      lastVcrossMultiLevel.clear();

      for(size_t i=0;i<crossData.size();i++) {
        lastVcrossMultiLevel.push_back(multiLevel[i]);
        if(lastVcrossMultiLevel[i]) {
          float* data = new float[lastVcrossPlot->nPoint*lastVcrossPlot->numLev];
          for(int j=0;j<(lastVcrossPlot->nPoint*lastVcrossPlot->numLev);j++) {
            data[j] = crossData[i][j];
          }
          lastVcrossData.push_back(data);
        } else {
          float* data = new float[lastVcrossPlot->nPoint];
          for(int j=0;j<lastVcrossPlot->nPoint;j++) {
            data[j] = crossData[i][j];
          }
          lastVcrossData.push_back(data);
        }
      }
      lastVcross = vcross;
      lastTgpos = tgpos;
    }
  }

#ifdef DEBUGPRINT
  for(int i=0;i<vcp->alevel.size();i++)
    cerr << "vcp->alevel["<<i<<"]: " << vcp->alevel[i] << endl;
  for(int i=0;i<vcp->blevel.size();i++)
    cerr << "vcp->blevel["<<i<<"]: " << vcp->blevel[i] << endl;
  cerr << "number of data values: " << crossData.size() << endl;
  cerr << "parameters: " << endl;
  for(int i=0;i<params.size();i++) {
    cerr << params[i] << endl;
    /*if(multiLevel[i] == false)
      for(int j=0;j<vcp->nPoint;j++)
        cerr << "["<<j<<"]: " << crossData[i][j] << endl;*/
  }
#endif


  // Insert data into VcrossPlot
  map<miutil::miString, int>::iterator pn, pnend= vcp->vcParName.end();
  for(size_t i=0;i<params.size();i++) {
    int param = miutil::to_int(params[i],0);
    if(param < 0) {
      if(multiLevel[i])
        vcp->addPar2d(param,crossData[i]);
      else
        vcp->addPar1d(param,crossData[i]);
    } else {
      pn= vcp->vcParName.find(params[i]);
      if(pn!=pnend) {
        if(multiLevel[i])
          vcp->addPar2d(pn->second,crossData[i]);
        else
          vcp->addPar1d(pn->second,crossData[i]);
      } else {
        METLIBS_LOG_DEBUG("Parameter " << params[i] << " not defined in setup");
      }
    }
  }

  /* Compute max/min mslp
   *(done every time, could be put in vector like in diVcrossFile.cc)
   */
  int psurf = 0;
  for(size_t i=0;i<params.size();i++)
    if(params[i] == "psurf")
      psurf=i;
  METLIBS_LOG_DEBUG(LOGVAL(psurf) << LOGVAL(vcp->vcoord));
  float pressure=50;
  for(size_t i=0;i<vcp->alevel.size();i++) {
    for(int j=0;j<vcp->nPoint;j++) {
      if(crossData[psurf][j] < fieldUndef) {
        if (vcp->vcoord == VCOORD_OTHER)
          pressure=vcp->alevel[i];
    	else
          pressure=vcp->alevel[i]+vcp->blevel[i]*crossData[psurf][j];
      }
      if (pressure>0.1 and pressure < 1200) {
        if(pressure>vcp->vrangemax)
          vcp->vrangemax=pressure;
        if(pressure<vcp->vrangemin)
          vcp->vrangemin=pressure;
      }
    }
  }
  METLIBS_LOG_DEBUG(LOGVAL(vcp->vrangemin) << LOGVAL(vcp->vrangemax));

#ifdef DEBUGPRINT
  cerr << "vcp->nPoint: " << vcp->nPoint << endl;
  cerr << "vcp->numLev: " << vcp->numLev << endl;
  cerr << "vcp->horizontalPosNum: " << vcp->horizontalPosNum << endl;
  cerr << "vcp->vcoord: " << vcp->vcoord << endl;
  cerr << "vcp->nTotal: " << vcp->nTotal << endl;
  cerr << "vcp->validTime: " << vcp->validTime << endl;
  cerr << "vcp->forecastHour: " << vcp->forecastHour << endl;
  cerr << "vcp->refPosition: " << vcp->refPosition << endl;
  cerr << "vcp->timeGraph: " << vcp->timeGraph << endl;
  cerr << "vcp->vrangemin: " << vcp->vrangemin << endl;
  cerr << "vcp->vrangemax: " << vcp->vrangemax << endl;
  cerr << "vcp->iundef: " << vcp->iundef << endl;
  /*for(int p=0;p<vcp->cdata2d.size();p++)
    for(int q=0;q<vcp->nTotal;q++)
      cerr << "vcp->cdata2d["<<p<<"]["<<q<<"]: " << vcp->cdata2d[p][q] << endl;
  for(int p=0;p<vcp->cdata1d.size();p++)
    for(int q=0;q<vcp->nPoint;q++)
      cerr << "vcp->cdata1d["<<p<<"]["<<q<<"]: " << vcp->cdata1d[p][q] << endl;*/
#endif

  // Try to prepare the data
  if (!vcp->prepareData(modelName)) {
    delete vcp;
    vcp= 0;
  }
  return vcp;
}
