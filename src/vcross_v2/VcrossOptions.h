/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#ifndef VCROSSOPTIONS_HH
#define VCROSSOPTIONS_HH

#include <diField/VcrossData.h>

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace vcross {

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
  std::vector<std::string> writeOptions();
  void readOptions(const std::vector<std::string>& vstr);

//private:
  bool changed;

  bool     pText;
  std::string textColour;

  bool     pPositionNames;
  std::string positionNamesColour;

  bool     pFrame;
  std::string frameColour;
  std::string frameLinetype;
  float    frameLinewidth;

  bool     pLevelNumbers;

  bool     pUpperLevel;
  std::string upperLevelColour;
  std::string upperLevelLinetype;
  float    upperLevelLinewidth;

  bool     pLowerLevel;
  std::string lowerLevelColour;
  std::string lowerLevelLinetype;
  float    lowerLevelLinewidth;

  bool     pOtherLevels;
  std::string otherLevelsColour;
  std::string otherLevelsLinetype;
  float    otherLevelsLinewidth;

  bool     pSurface;
  std::string surfaceColour;
  std::string surfaceLinetype;
  float    surfaceLinewidth;

  bool     pInflight;
  std::string inflightColour;
  std::string inflightLinetype;
  float    inflightLinewidth;

  bool     pMarkerlines;
  std::string markerlinesColour;
  std::string markerlinesLinetype;
  float    markerlinesLinewidth;

  bool     pVerticalMarker;
  std::string verticalMarkerColour;
  std::string verticalMarkerLinetype;
  float    verticalMarkerLinewidth;
  float    verticalMarkerLimit;

  bool     pDistance;
  std::string distanceColour;
  std::string distanceUnit;
  std::string distanceStep;

  bool     pXYpos;
  std::string xyposColour;

  bool     pGeoPos;
  std::string geoposColour;

  bool     pCompass;

  bool     pHorizontalGridLines;
  std::string horgridColour;
  std::string horgridLinetype;
  float    horgridLinewidth;

  bool     pVerticalGridLines;
  std::string vergridColour;
  std::string vergridLinetype;
  float    vergridLinewidth;

  bool     extrapolateFixedLevels;
  bool     extrapolateToBottom;

  std::string verticalType;       //!< obsolete, but used !?
  std::string verticalScale;      //!< linear/exner
  std::string verticalCoordinate; //!< pressure/altitude
  std::string verticalUnit;       //!< hPa/FL/m/ft
  vcross::Z_AXIS_TYPE getVerticalType() const;

  bool keepVerHorRatio;
  int  verHorRatio;

  bool stdVerticalArea;
  int  minVerticalArea;
  int  maxVerticalArea;

  bool stdHorizontalArea;
  int  minHorizontalArea;
  int  maxHorizontalArea;

  std::string backgroundColour;

  std::vector<float> fixedPressureLevels;

  //-------------

  std::string vcOnMapColour;
  std::string vcOnMapLinetype;
  float    vcOnMapLinewidth;

  std::string vcSelectedOnMapColour;
  std::string vcSelectedOnMapLinetype;
  float    vcSelectedOnMapLinewidth;
};

typedef boost::shared_ptr<VcrossOptions> VcrossOptions_p;

} // namespace vcross

#endif // VCROSSOPTIONS_HH
