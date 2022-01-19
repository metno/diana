/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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

#include "diPlot.h"

#include "diColour.h"
#include "diPlotModule.h"

Plot::Plot()
    : enabled(true)
    , status_(P_UNKNOWN)
{
}

Plot::~Plot()
{
}

void Plot::setCanvas(DiCanvas*)
{
}

bool Plot::operator==(const Plot&) const
{
  return false;
}

void Plot::changeProjection(const Area& /*mapArea*/, const Rectangle& /*plotSize*/, const diutil::PointI& /*physSize*/)
{
  // ignore
}

void Plot::changeTime(const miutil::miTime& /*newTime*/)
{
  // ignore
}

void Plot::setStatus(PlotStatusValue ps)
{
  status_ = ps;
}

const StaticPlot* Plot::getStaticPlot() const
{
  return PlotModule::instance()->getStaticPlot();
}

void Plot::setEnabled(bool e)
{
  enabled = e;
}

void Plot::getAnnotation(std::string& s, Colour& c) const
{
  c = Colour("black");
  s = getPlotName();
}

void Plot::getDataAnnotations(std::vector<std::string>&) const {}

const std::string& Plot::getPlotName() const
{
  return plotname;
}

const std::string& Plot::getIconName() const
{
  static const std::string EMPTY;
  return EMPTY;
}
