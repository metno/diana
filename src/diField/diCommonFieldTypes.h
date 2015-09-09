/* -*- c++ -*-

  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013 met.no

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

/**
   \brief  detailed data for one variable
*/
struct FieldInfo {
  std::string fieldName;
  std::string groupName;
  std::string standard_name;
  std::string units;
  std::vector<std::string>  vlevels;
  std::vector<std::string> elevels; //(EPS clusters, EPS single runs etc.)
  std::string default_vlevel;
  std::string default_elevel;
  std::string vcoord;
  std::string ecoord;
};
/**
   \brief GUI detailed data for one fieldgroup
*/
struct FieldGroupInfo {
  std::string groupName;
  bool plotDefinitions;
  std::vector<std::string> fieldNames;
  std::map<std::string,FieldInfo> fields;
  FieldGroupInfo() : plotDefinitions(true) {}
};

/**
   \brief GUI data for one file/model group
*/
struct FieldDialogInfo {
  std::string groupName;
  std::string groupType;
  std::vector<std::string> modelNames;
};

struct FieldRequest {
  std::string modelName;
  std::string paramName;
  std::string zaxis;;
  std::string eaxis;;
  std::string taxis;;
  std::string plevel;
  std::string elevel;
  std::string grid;
  std::string version;
  std::string refTime;
  miutil::miTime ptime;
  std::string unit;
  std::string palette;
  int hourOffset;
  int minOffset;
  int time_tolerance;
  int refhour;
  int refoffset;
  std::string output_time;
  bool allTimeSteps;
  bool standard_name;
  bool plotDefinition;
  bool checkSourceChanged;
  bool flightlevel;
  FieldRequest() : hourOffset(0), minOffset(0), time_tolerance(0), refhour(-1),
      refoffset(0), allTimeSteps(false), standard_name(false),
      plotDefinition(true), checkSourceChanged(true), flightlevel(false){}
};

#endif
