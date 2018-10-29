/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#include "diPlotArea.h"

#include "diana_config.h"

#define MILOGGER_CATEGORY "diana.PlotArea"
#include <miLogger/miLogging.h>

namespace {
inline float oneIf0(float f)
{
  return (f <= 0) ? 1 : f;
}
} // namespace

PlotArea::PlotArea()
    : mPhys(0, 0) // physical plot size
    , mapborder(0)
    , mPhysToMapScale(1, 1)
{
}

bool PlotArea::setMapArea(const Area& a)
{
  if (!a.P().isDefined() || area == a) {
    return false;
  } else {
    area = a;
    PlotAreaSetup();
    return true;
  }
}

bool PlotArea::setPhysSize(int w, int h)
{
  if (w != mPhys.x() || h != mPhys.y()) {
    mPhys = diutil::PointI(w, h);
    PlotAreaSetup();
    return true;
  } else {
    return false;
  }
}

XY PlotArea::PhysToMap(const XY& phys) const
{
  if (hasPhysSize())
    return phys * mPhysToMapScale + XY(getPlotSize().x1, getPlotSize().y1);
  else
    return phys;
}

XY PlotArea::MapToPhys(const XY& map) const
{
  if (hasPhysSize())
    return (map - XY(getPlotSize().x1, getPlotSize().y1)) / mPhysToMapScale;
  else
    return map;
}

void PlotArea::PlotAreaSetup()
{
  METLIBS_LOG_SCOPE();
  if (!hasPhysSize())
    return;

  const Rectangle& mapr = getMapArea().R();

  const float waspr = mPhys.x() / float(mPhys.y());
  mapsize = diutil::fixedAspectRatio(mapr, waspr, true);

  // update full plot area -- add border
  plotsize = diutil::adjustedRectangle(mapsize, mapborder, mapborder);

  mPhysToMapScale = XY(oneIf0(plotsize.width()) / oneIf0(mPhys.x()), oneIf0(plotsize.height()) / oneIf0(mPhys.y()));
}
