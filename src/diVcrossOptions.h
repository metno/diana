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

using namespace std;

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
  vector<miString> writeOptions();
  void readOptions(const vector<miString>& vstr);

private:
  friend class VcrossSetupDialog;
  friend class VcrossPlot;
  friend class VcrossManager;

  bool changed;

  bool     pText;
  miString textColour;

  bool     pPositionNames;
  miString positionNamesColour;

  bool     pFrame;
  miString frameColour;
  miString frameLinetype;
  float    frameLinewidth;

  bool     pLevelNumbers;

  bool     pUpperLevel;
  miString upperLevelColour;
  miString upperLevelLinetype;
  float    upperLevelLinewidth;

  bool     pLowerLevel;
  miString lowerLevelColour;
  miString lowerLevelLinetype;
  float    lowerLevelLinewidth;

  bool     pOtherLevels;
  miString otherLevelsColour;
  miString otherLevelsLinetype;
  float    otherLevelsLinewidth;

  bool     pSurface;
  miString surfaceColour;
  miString surfaceLinetype;
  float    surfaceLinewidth;

  bool     pMarkerlines;
  miString markerlinesColour;
  miString markerlinesLinetype;
  float    markerlinesLinewidth;

  bool     pVerticalMarker;
  miString verticalMarkerColour;
  miString verticalMarkerLinetype;
  float    verticalMarkerLinewidth;
  float    verticalMarkerLimit;

  bool     pDistance;
  miString distanceColour;
  miString distanceUnit;
  miString distanceStep;

  bool     pXYpos;
  miString xyposColour;

  bool     pGeoPos;
  miString geoposColour;

  bool     pVerticalGridLines;
  miString vergridColour;
  miString vergridLinetype;
  float    vergridLinewidth;

  bool     extrapolateFixedLevels;
  bool     extrapolateToBottom;

  bool     thinArrows;

  miString verticalType;

  bool keepVerHorRatio;
  int  verHorRatio;

  bool stdVerticalArea;
  int  minVerticalArea;
  int  maxVerticalArea;

  bool stdHorizontalArea;
  int  minHorizontalArea;
  int  maxHorizontalArea;

  miString backgroundColour;

  vector<float> fixedPressureLevels;

  //-------------

  miString vcOnMapColour;
  miString vcOnMapLinetype;
  float    vcOnMapLinewidth;

  miString vcSelectedOnMapColour;
  miString vcSelectedOnMapLinetype;
  float    vcSelectedOnMapLinewidth;

};

#endif

