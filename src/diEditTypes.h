/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#ifndef DIANA_DIEDITTYPES_H
#define DIANA_DIEDITTYPES_H

#include <string>
#include <vector>

//--------------------------------------------------
// Objects/Edit
//--------------------------------------------------

/**
   \brief data for one edit tool
*/
struct editToolInfo
{
  std::string name;
  int index;
  std::string colour;
  std::string borderColour;
  int sizeIncrement;
  bool spline;
  std::string linetype;
  std::string filltype;

  editToolInfo(const std::string& newName, const int newIndex, const std::string& newColour = "black", const std::string& newBorderColour = "black",
               const int& newSizeIncrement = 0, const bool& newSpline = true, const std::string& newLinetype = "solid",
               const std::string& newFilltype = std::string())
      : name(newName)
      , index(newIndex)
      , colour(newColour)
      , borderColour(newBorderColour)
      , sizeIncrement(newSizeIncrement)
      , spline(newSpline)
      , linetype(newLinetype)
      , filltype(newFilltype)
  {
  }
};

/**
  \brief global define for the ComplexPressureText tool
*/

#ifndef TOOL_DECREASING
#define TOOL_DECREASING "Decreasing pressure"
#endif

#ifndef TOOL_INCREASING
#define TOOL_INCREASING "Increasing pressure"
#endif

/**
   \brief data for one edit mode (field-editing, object-drawing)
*/
struct editModeInfo
{
  std::string editmode;
  std::vector<editToolInfo> edittools;
  editModeInfo(const std::string& newmode, const std::vector<editToolInfo>& newtools)
      : editmode(newmode)
      , edittools(newtools)
  {
  }
};

/**
   \brief data for one map mode (normal, editing)
*/
struct mapModeInfo
{
  std::string mapmode;
  std::vector<editModeInfo> editmodeinfo;
  mapModeInfo(const std::string& newmode, const std::vector<editModeInfo>& newmodeinfo)
      : mapmode(newmode)
      , editmodeinfo(newmodeinfo)
  {
  }
};

/**
   \brief data for all edititing functions
*/
struct EditDialogInfo
{
  std::vector<mapModeInfo> mapmodeinfo;
};

#endif // DIANA_DIEDITTYPES_H
