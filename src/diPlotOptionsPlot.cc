/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2019 met.no

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

#include "diPlotOptionsPlot.h"

std::string PlotOptionsPlot::getEnabledStateKey() const
{
  return miutil::mergeKeyValue(ooptions);
}

void PlotOptionsPlot::setPlotInfo(const miutil::KeyValue_v& kvs)
{
  // fill poptions with values from pinfo
  ooptions.clear();
  PlotOptions::parsePlotOption(kvs, poptions, ooptions);
  setEnabled(poptions.enabled);
}

void PlotOptionsPlot::swap(PlotOptionsPlot& o)
{
  using std::swap;
  swap(poptions, o.poptions);
  swap(ooptions, o.ooptions);
}
