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

#include "diCommonFieldTypes.h"

FieldPlotAxis::FieldPlotAxis()
    : default_value_index(-1)
{
}

const std::string& FieldPlotAxis::default_value() const
{
  if (default_value_index >= 0 && default_value_index < (int)values.size()) {
    return values[default_value_index];
  } else {
    static const std::string EMPTY;
    return EMPTY;
  }
}

FieldPlotGroupInfo::FieldPlotGroupInfo()
{
}

const std::string& FieldPlotGroupInfo::groupName() const
{
  static std::string EMPTY;
  return !plots.empty() ? plots.front().groupName : EMPTY;
}

FieldModelInfo::FieldModelInfo(const std::string& mn, const std::string& si)
    : modelName(mn)
    , setupInfo(si)
{
}

FieldRequest::FieldRequest()
    : hourOffset(0)
    , minOffset(0)
    , time_tolerance(0)
    , refhour(-1)
    , refoffset(0)
    , allTimeSteps(false)
    , standard_name(false)
    , predefinedPlot(true)
    , checkSourceChanged(true)
    , flightlevel(false)
{
}
