/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2017-2019 met.no

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

#include "diLocationPlotCluster.h"

#include "diLocationPlot.h"

static const std::string LOCATION = "LOCATION";

LocationPlotCluster::LocationPlotCluster()
    : PlotCluster(LOCATION)
{
}
void LocationPlotCluster::processInput(const PlotCommand_cpv&)
{
  // do nothing
}

void LocationPlotCluster::putLocation(const LocationData& locationdata)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  const auto it = find_lp(locationdata.name);
  if (it != plots_.end()) {
    LocationPlot* lp = static_cast<LocationPlot*>(*it);
    const bool visible = lp->isVisible();
    lp->setData(locationdata);
    if (!visible)
      lp->hide();
  } else {
    LocationPlot* lp = new LocationPlot();
    lp->setData(locationdata);
    add(lp);
  }
}

bool LocationPlotCluster::deleteLocation(const std::string& name)
{
  const auto it = find_lp(name);
  if (it == plots_.end())
    return false;

  delete *it;
  plots_.erase(it);
  return true;
}

void LocationPlotCluster::setSelectedLocation(const std::string& name, const std::string& elementname)
{
  const auto it = find_lp(name);
  if (it != plots_.end())
    static_cast<LocationPlot*>(*it)->setSelected(elementname);
}

std::string LocationPlotCluster::findLocation(int x, int y, const std::string& name)
{
  const auto it = find_lp(name);
  if (it == plots_.end())
    return std::string();

  return static_cast<LocationPlot*>(*it)->find(x, y);
}

LocationPlotCluster::Plot_xv::iterator LocationPlotCluster::find_lp(const std::string& name)
{
  return std::find_if(plots_.begin(), plots_.end(), [&](Plot* p) { return name == static_cast<LocationPlot*>(p)->getName(); });
}
