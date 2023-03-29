/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

#include "diGLPainter.h"
#include "diStaticPlot.h"
#include "diTrajectoryPlot.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <QPointF>
#include <QPolygonF>

#include <cmath>

#include <fstream>
#include <iomanip>
#include <sstream>

#define MILOGGER_CATEGORY "diana.TrajectoryPlot"
#include <miLogger/miLogging.h>

using namespace miutil;

namespace {
const float ALPHA_FACTOR = 0.3;
}

TrajectoryPlot::TrajectoryPlot()
{
  oldArea = getStaticPlot()->getMapArea();
  mNeedPrepare = true;

  lineWidth=1;
  timeStep= 900.;
  numIterations= 5;
  plot_on = true;
}


TrajectoryPlot::~TrajectoryPlot()
{
}

void TrajectoryPlot::setTrajectoryData(const TrajectoryData_v& t)
{
  mNeedPrepare = true;
  trajectories = t;
  setPlotName("Trajektorier" + fieldStr);
  switchProjection();
}

void TrajectoryPlot::changeProjection(const Area& /*mapArea*/, const Rectangle& /*plotSize*/, const diutil::PointI& /*physSize*/)
{
  switchProjection();
}

void TrajectoryPlot::switchProjection()
{
  METLIBS_LOG_SCOPE();

  if (!mNeedPrepare && oldArea.P() == getStaticPlot()->getMapArea().P())
    return;

  reprojectedXY.clear();

  if (!trajectories.empty()) {
    for (size_t i=0; i<trajectories.size(); ++i) {
      const int npos = trajectories[i].size();
      float *xpos= new float[npos];
      float *ypos= new float[npos];
      for (int j=0; j<npos; j++) {
        const LonLat& p = trajectories[i].position(j);
        xpos[j] = p.lonDeg();
        ypos[j] = p.latDeg();
      }
      getStaticPlot()->GeoToMap(npos, xpos, ypos);

      QPolygonF r;
      for (int j=0; j<npos; j++)
        r << QPointF(xpos[j], ypos[j]);
      reprojectedXY.push_back(r);

      delete[] xpos;
      delete[] ypos;
    }
  } else if (!startPositions.empty()) {
    const int npos = startPositions.size();
    float *xpos= new float[npos];
    float *ypos= new float[npos];
    for (int j=0; j<npos; j++) {
      const LonLat& p = startPositions[j];
      xpos[j] = p.lonDeg();
      ypos[j] = p.latDeg();
    }
    getStaticPlot()->GeoToMap(npos, xpos, ypos);

    QPolygonF r;
    for (int j=0; j<npos; j++)
      r << QPointF(xpos[j], ypos[j]);
    reprojectedXY.push_back(r);

    delete[] xpos;
    delete[] ypos;
  }

  oldArea = getStaticPlot()->getMapArea();
  mNeedPrepare = false;
}

void TrajectoryPlot::changeTime(const miutil::miTime& newTime)
{
  mapTime_ = newTime;
}

int TrajectoryPlot::trajPos(const std::vector<std::string>& vstr)
{
  METLIBS_LOG_SCOPE();
  if (METLIBS_LOG_DEBUG_ENABLED()) {
    for(size_t i=0;i<vstr.size();i++)
      METLIBS_LOG_DEBUG(vstr[i]);
  }

  int action= 0;  // action to be taken by PlotModule (0=none)

  int nvstr = vstr.size();
  for(int k=0; k<nvstr; k++){

    int numMarker = 1; // number of actual startpositions
    float markerRadius = 50*1000; // size of multi-startposition rectangle, in m

    const std::vector<std::string> tokens = miutil::split_protected(vstr[k], '"','"');
    for (size_t i=0; i<tokens.size(); i++) {
      const std::vector<std::string> stokens = miutil::split(tokens[i], 0, "=");
      if (METLIBS_LOG_DEBUG_ENABLED()) {
        METLIBS_LOG_DEBUG("stokens:");
        for (size_t j=0; j<stokens.size(); j++)
          METLIBS_LOG_DEBUG("  " << stokens[j]);
      }
      std::string value, orig_value, key;
      if (stokens.size() == 1) {
        key= miutil::to_lower(stokens[0]);
        if (key == "clear") {
          clearData();
          action= 1;  // remove annotation
        } else if (key == "delete") {
          clearData();
          startPositions.clear();
        }
      } else if (stokens.size() == 2) {
        key        = miutil::to_lower(stokens[0]);
        orig_value = stokens[1];
        value      = miutil::to_lower(stokens[1]);
        if (key == "plot") {
          if (value == "on")
            plot_on = true;
          else
            plot_on = false;
          action= 1;  // add or remove annotation
        } else if (key == "longitudelatitude" || key == "latitudelongitude") {
          const int dilon = (key == "longitudelatitude") ? 0 : 1, dilat = 1 - dilon;
          const std::vector<std::string> lonlat = miutil::split(value, 0, ",");
          const int n = lonlat.size();
          for (int i=0; i<n; i+=2) {
            const float lon = miutil::to_double(lonlat[i+dilon]),
                lat = miutil::to_double(lonlat[i+dilat]);
            const LonLat sp = LonLat::fromDegrees(lon, lat);
            startPositions.push_back(sp);

            const int numExtra = numMarker - 1;
            if (numExtra > 0 && markerRadius > 0) {
              for (int j=0; j<numExtra; ++j)
                startPositions.push_back(sp.stepDirection(markerRadius, j * (2*M_PI/numExtra)));
            }
          }
        } else if (key == "field" ) {
          fieldStr = diutil::quote_removed(orig_value);
        } else if (key == "colour" )
          colourPast = Colour(value);
        else if (key == "linewidth" )
          lineWidth = atoi(value.c_str());
        else if (key == "linetype" )
          lineType = Linetype(value);
        else if (key == "radius" )
          // "radius" is given in km, we want to keep m
          markerRadius = 1000*miutil::to_float(value);
        else if (key == "numpos" )
          numMarker = miutil::to_int(value);
      }
    }
  }

  colourFuture = colourPast;
  colourFuture.setF(Colour::alpha, ALPHA_FACTOR);
  mNeedPrepare = true;

  return action;
}


void TrajectoryPlot::plot(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();

  if (!plot_on || !isEnabled() || zorder != PO_LINES)
    return;

  switchProjection();

  const Colour& cP = getStaticPlot()->notBackgroundColour(colourPast);
  const Colour& cF = getStaticPlot()->notBackgroundColour(colourFuture);
  gl->Enable(DiGLPainter::gl_BLEND);
  gl->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);
  gl->setLineStyle(cP, lineWidth, lineType);

  const float d = 5*getStaticPlot()->getPhysToMapScaleX();
  if (trajectories.empty() && reprojectedXY.size() == 1) {
    // only draw marker positions, trajectories not yet calculated/set
    const QPolygonF& r = reprojectedXY.at(0);
    for (int i=0; i<r.size(); ++i)
      gl->drawCircle(true, r.at(i).x(), r.at(i).y(), d);
    return;
  } else if (trajectories.size() != reprojectedXY.size()) {
    METLIBS_LOG_ERROR("trajectories.size() != reprojectedXY.size()");
    return;
  }

  for (size_t i=0; i<reprojectedXY.size(); ++i) {
    const TrajectoryData& t = trajectories[i];
    const QPolygonF& r = reprojectedXY[i];

    QPolygonF past, future;
    for (int j = 0; j < t.size(); j += 1) {
      if (t.time(j) <= mapTime_)
        past << r.at(j);
      if (t.time(j) >= mapTime_)
        future << r.at(j);
    }

    if (past.size() >= 2) {
      gl->setColour(cP);
      gl->drawPolyline(past);
    }

    if (future.size() >= 2) {
      gl->setColour(cF);
      gl->drawPolyline(future);
    }

    if (r.size() >= 2) {
      // use color from past or future as set before

      // TODO find last motion (e.g. in case there is no wind at the end)
      const QPointF& p1 = r.at(r.size() - 2), p2 = r.at(r.size() - 1);
      gl->drawArrowHead(p1.x(), p1.y(), p2.x(), p2.y(), d*10);
    }

    if (t.time(t.mStartIndex) <= mapTime_)
      gl->setColour(cP);
    else
      gl->setColour(cF);
    gl->drawCircle(true, r.at(t.mStartIndex).x(), r.at(t.mStartIndex).y(), d);
  }
}

std::string TrajectoryPlot::getEnabledStateKey() const
{
  return "trajectory-" + fieldStr;
}

void TrajectoryPlot::clearData()
{
  trajectories.clear();
  reprojectedXY.clear();
  setPlotName("");
}

void TrajectoryPlot::getAnnotation(std::string& s, Colour& c) const
{
  if (plot_on && !trajectories.empty()) {
    s = getPlotName();
    c = getStaticPlot()->notBackgroundColour(colourPast);
  } else {
    s.clear();
  }
}

bool TrajectoryPlot::printTrajectoryPositions(const std::string& filename)
{
  METLIBS_LOG_SCOPE();

  if (trajectories.empty()) {
    METLIBS_LOG_ERROR("no trajectories, cannot save to '" <<filename << "'");
    return false;
  }

  std::ofstream fs(filename.c_str());
  if (!fs) {
    METLIBS_LOG_ERROR("cannot open file '" <<filename << "'");
    return false;
  }

  fs << "[NAME TRAJECTORY]" << std::endl
     << "[COLUMNS " << std::endl
     << "Date:d   Time:t      Lon:r   Lat:r   No:r]" << std::endl
     << "[DATA]" << std::endl;
  fs.setf(std::ios::showpoint);
  fs.precision(7);

  for (size_t i=0; i<trajectories.size(); ++i) {
    const TrajectoryData& t = trajectories[i];
    const int npos = t.size();
    for (int j=0; j<npos; j++) {
      const LonLat& p = t.position(j);
      fs << t.time(j).isoTime()
         << "  " << std::setw(10) << p.lonDeg()
         << "  " << std::setw(10) << p.latDeg()
         << "  T" << j << std::endl;
    }
  }

  fs.close();

  return true;
}
