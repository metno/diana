/* -*- c++ -*-

  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2018 met.no

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

#ifndef _diCommonFieldTypes_h
#define _diCommonFieldTypes_h

#include <puTools/miTime.h>
#include <string>
#include <vector>
#include <map>

//--------------------------------------------------
// fields
//--------------------------------------------------

struct FieldPlotAxis
{
  std::string name;
  std::vector<std::string> values;
  int default_value_index;

  FieldPlotAxis();
  const std::string& default_value() const;
};

/**
   \brief data for one plot/variable
*/
struct FieldPlotInfo
{
  std::string fieldName;
  std::string variableName;
  std::string groupName;
  std::string standard_name;
  FieldPlotAxis vertical_axis;
  FieldPlotAxis realization_axis; //(EPS clusters, EPS single runs etc.)

  const std::vector<std::string>& vlevels() const { return vertical_axis.values; }
  const std::vector<std::string>& elevels() const { return realization_axis.values; }
  const std::string& default_vlevel() const { return vertical_axis.default_value(); }
  const std::string& default_elevel() const { return realization_axis.default_value(); }
  const std::string& vcoord() const { return vertical_axis.name; }
  const std::string& ecoord() const { return realization_axis.name; };
};
typedef std::vector<FieldPlotInfo> FieldPlotInfo_v;

/**
   \brief data for one plot group
*/
struct FieldPlotGroupInfo
{
  FieldPlotInfo_v plots;
  FieldPlotGroupInfo();
  const std::string& groupName() const;
};
typedef std::vector<FieldPlotGroupInfo> FieldPlotGroupInfo_v;

/**
   \brief data for one model
*/
struct FieldModelInfo
{
  std::string modelName;
  std::string setupInfo;
  FieldModelInfo(const std::string& mn, const std::string& si);
};
typedef std::vector<FieldModelInfo> FieldModelInfo_v;

/**
   \brief data for one file/model group
*/
struct FieldModelGroupInfo
{
  std::string groupName;
  enum { STANDARD_GROUP, ARCHIVE_GROUP } groupType;
  FieldModelInfo_v models;
};
typedef std::vector<FieldModelGroupInfo> FieldModelGroupInfo_v;

struct FieldRequest {
  std::string modelName;
  std::string refTime;
  int refhour;
  int refoffset;
  bool checkSourceChanged;

  std::string paramName;
  bool standard_name;
  bool predefinedPlot;
  std::string unit;

  std::string zaxis;
  std::string plevel;
  bool flightlevel;

  std::string eaxis;
  std::string elevel;

  std::string taxis;
  miutil::miTime ptime;
  int hourOffset;
  int minOffset;
  int time_tolerance;
  std::string output_time;
  bool allTimeSteps;

  std::string palette;

  FieldRequest();
};

#endif
