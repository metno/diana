/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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
#ifndef VCROSSOPTIONS_H
#define VCROSSOPTIONS_H

#include <puTools/miString.h>
#include <vector>

/**
  \brief Vertical Crossection diagram settings

   Contains diagram layout settings and defaults.
*/
class VcrossOptions
{
public:
  VcrossOptions();
  ~VcrossOptions();
  void setDefaults();

  // log and setup
  std::vector<miutil::miString> writeOptions();
  void readOptions(const std::vector<miutil::miString>& vstr);

private:
  friend class VcrossSetupDialog;
  friend class VcrossPlot;
  friend class VcrossManager;

  bool changed;

  bool     pText;
  miutil::miString textColour;

  bool     pPositionNames;
  miutil::miString positionNamesColour;

  bool     pFrame;
  miutil::miString frameColour;
  miutil::miString frameLinetype;
  float    frameLinewidth;

  bool     pLevelNumbers;

  bool     pUpperLevel;
  miutil::miString upperLevelColour;
  miutil::miString upperLevelLinetype;
  float    upperLevelLinewidth;

  bool     pLowerLevel;
  miutil::miString lowerLevelColour;
  miutil::miString lowerLevelLinetype;
  float    lowerLevelLinewidth;

  bool     pOtherLevels;
  miutil::miString otherLevelsColour;
  miutil::miString otherLevelsLinetype;
  float    otherLevelsLinewidth;

  bool     pSurface;
  miutil::miString surfaceColour;
  miutil::miString surfaceLinetype;
  float    surfaceLinewidth;

  bool     pMarkerlines;
  miutil::miString markerlinesColour;
  miutil::miString markerlinesLinetype;
  float    markerlinesLinewidth;

  bool     pVerticalMarker;
  miutil::miString verticalMarkerColour;
  miutil::miString verticalMarkerLinetype;
  float    verticalMarkerLinewidth;
  float    verticalMarkerLimit;

  bool     pDistance;
  miutil::miString distanceColour;
  miutil::miString distanceUnit;
  miutil::miString distanceStep;

  bool     pXYpos;
  miutil::miString xyposColour;

  bool     pGeoPos;
  miutil::miString geoposColour;

  bool     pVerticalGridLines;
  miutil::miString vergridColour;
  miutil::miString vergridLinetype;
  float    vergridLinewidth;

  bool     extrapolateFixedLevels;
  bool     extrapolateToBottom;

  bool     thinArrows;

  miutil::miString verticalType;

  bool keepVerHorRatio;
  int  verHorRatio;

  bool stdVerticalArea;
  int  minVerticalArea;
  int  maxVerticalArea;

  bool stdHorizontalArea;
  int  minHorizontalArea;
  int  maxHorizontalArea;

  miutil::miString backgroundColour;

  std::vector<float> fixedPressureLevels;

  //-------------

  miutil::miString vcOnMapColour;
  miutil::miString vcOnMapLinetype;
  float    vcOnMapLinewidth;

  miutil::miString vcSelectedOnMapColour;
  miutil::miString vcSelectedOnMapLinetype;
  float    vcSelectedOnMapLinewidth;

};

#endif

