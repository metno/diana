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

#include "diMeasurementsPlot.h"

#include "diGLPainter.h"
#include "diStaticPlot.h"

#include <puTools/miStringFunctions.h>
#include <diField/diField.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.MeasurementsPlot"
#include <miLogger/miLogging.h>

using namespace miutil;

MeasurementsPlot::MeasurementsPlot()
  : xpos(0)
  , ypos(0)
  , needReprojection(true)
{
  lineWidth = 1;
}


MeasurementsPlot::~MeasurementsPlot()
{
  clearXY();
}


void MeasurementsPlot::clearXY()
{
  delete[] xpos;
  delete[] ypos;
  xpos = ypos = 0;
  needReprojection = true;
}

void MeasurementsPlot::changeProjection(const Area& mapArea, const Rectangle& /*plotSize*/, const diutil::PointI& /*physSize*/)
{
  METLIBS_LOG_SCOPE();
  if (oldProjection != mapArea.P()) {
    needReprojection = true;
    oldProjection = mapArea.P();
  }
}

void MeasurementsPlot::reproject()
{
  int nlon = lon.size();
  if (nlon==0)
    return;

  if (!xpos) {
    xpos = new float[nlon];
    ypos = new float[nlon];
  }

  std::copy(lon.begin(), lon.end(), xpos);
  std::copy(lat.begin(), lat.end(), ypos);
  getStaticPlot()->GeoToMap(nlon, xpos, ypos);
}

void MeasurementsPlot::measurementsPos(const std::vector<std::string>& vstr)
{
  METLIBS_LOG_SCOPE();

  for (const std::string& pin : vstr) {
    METLIBS_LOG_DEBUG(LOGVAL(pin));
    for (const std::string& tok : miutil::split_protected(pin, '"', '"')) {
      const std::vector<std::string> stokens = miutil::split(tok, 0, "=");
      if (stokens.empty())
        continue;
      const std::string key = miutil::to_lower(stokens[0]);
      if (stokens.size() == 1) {
        if (key == "clear") {
        } else if (key == "delete") {
          lat.clear();
          lon.clear();
          clearXY();
        }
      } else if (stokens.size() == 2) {
        const std::string value = miutil::to_lower(stokens[1]);
        if (key == "longitudelatitude" ||key == "latitudelongitude") {
          const std::vector<std::string> ll = miutil::split(value, 0, ",");
          const int npos = ll.size()/2;
          if (key == "longitudelatitude" ) {
            for (int i=0; i<npos; i++) {
              lon.push_back(atof(ll[2*i].c_str()));
              lat.push_back(atof(ll[2*i+1].c_str()));
            }
          } else if (key == "latitudelongitude" ) {
            for (int i=0; i<npos; i++) {
              lat.push_back(atof(ll[2*i].c_str()));
              lon.push_back(atof(ll[2*i+1].c_str()));
            }
          }
          clearXY();
        } else if (key == "colour") {
          colour = Colour(value);
        }
        else if (key == "linewidth")
          lineWidth = atoi(value.c_str());
        else if (key == "linetype")
          lineType = Linetype(value);
      }
    }
  }
}

std::string MeasurementsPlot::getEnabledStateKey() const
{
  return std::string();
}

void MeasurementsPlot::plot(DiGLPainter* gl, PlotOrder porder)
{
  METLIBS_LOG_SCOPE();

  if (!isEnabled() || porder != PO_LINES)
    return;

  if (needReprojection)
    reproject();

  if (!xpos)
    return;

  colour = getStaticPlot()->notBackgroundColour(colour);
  gl->setLineStyle(colour, lineWidth);

  float d= 5*getStaticPlot()->getPhysToMapScaleX();

  // plot  posistions
  int m = lon.size();
  for (int i=0; i<m; i++)
    gl->drawCross(xpos[i], ypos[i], d, true);
}
