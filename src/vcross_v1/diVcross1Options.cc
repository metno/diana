/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diVcross1Options.h"

#include <puTools/miStringFunctions.h>
#include <puTools/miStringBuilder.h>

#define MILOGGER_CATEGORY "diana.VcrossOptions"
#include <miLogger/miLogging.h>

using namespace ::miutil;
using namespace std;

VcrossOptions::VcrossOptions()
{
  setDefaults();
}


VcrossOptions::~VcrossOptions()
{
}


void VcrossOptions::setDefaults()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VcrossOptions::setDefaults");
#endif

  pText= true;
  textColour= "black";

  pPositionNames= true;
  positionNamesColour= "red";

  pFrame= true;
  frameColour= "black";
  frameLinetype= "solid";
  frameLinewidth= 1.;

  pLevelNumbers= true;

  pUpperLevel= true;
  upperLevelColour= "black";
  upperLevelLinetype= "solid";
  upperLevelLinewidth= 1.;

  pLowerLevel= true;
  lowerLevelColour= "black";
  lowerLevelLinetype= "solid";
  lowerLevelLinewidth= 1.;

  pOtherLevels= false;
  otherLevelsColour= "black";
  otherLevelsLinetype= "solid";
  otherLevelsLinewidth= 1.;

  pSurface= true;
  surfaceColour= "black";
  surfaceLinetype= "solid";
  surfaceLinewidth= 1.;

  pMarkerlines= true;
  markerlinesColour= "black";
  markerlinesLinetype= "solid";
  markerlinesLinewidth= 3.;

  pVerticalMarker= false;
  verticalMarkerColour= "black";
  verticalMarkerLinetype= "solid";
  verticalMarkerLinewidth= 1.;
  verticalMarkerLimit= 1.;

  pDistance= false;
  distanceColour= "black";
  distanceUnit=   "km";
  distanceStep=   "grid";

  pXYpos= false;
  xyposColour= "black";

  pGeoPos= true;
  geoposColour= "black";

  pVerticalGridLines= false;
  vergridColour= "black";
  vergridLinetype= "solid";
  vergridLinewidth= 1.;

  extrapolateFixedLevels= false;
  extrapolateToBottom= true;

  thinArrows= false;

  verticalType= "standard";

  keepVerHorRatio= true;
  verHorRatio= 150;

  stdVerticalArea= false;
  minVerticalArea= 0;
  maxVerticalArea= 100;

  stdHorizontalArea= false;
  minHorizontalArea= 0;
  maxHorizontalArea= 100;

  backgroundColour= "white";

  fixedPressureLevels.clear();
  fixedPressureLevels.push_back(1000.);
  fixedPressureLevels.push_back(925.);
  fixedPressureLevels.push_back(850.);
  fixedPressureLevels.push_back(700.);
  fixedPressureLevels.push_back(600.);
  fixedPressureLevels.push_back(500.);
  fixedPressureLevels.push_back(400.);
  fixedPressureLevels.push_back(300.);
  fixedPressureLevels.push_back(250.);
  fixedPressureLevels.push_back(200.);
  fixedPressureLevels.push_back(150.);
  fixedPressureLevels.push_back(100.);
  fixedPressureLevels.push_back(70.);
  fixedPressureLevels.push_back(50.);
  fixedPressureLevels.push_back(30.);
  fixedPressureLevels.push_back(10.);

  //-----------------

  vcOnMapColour=    "blue";
  vcOnMapLinetype=  "solid";
  vcOnMapLinewidth= 2.;

  vcSelectedOnMapColour=    "red";
  vcSelectedOnMapLinetype=  "solid";
  vcSelectedOnMapLinewidth= 4.;

  changed= true;
}


vector<std::string> VcrossOptions::writeOptions()
{
  METLIBS_LOG_SCOPE();

  using miutil::StringBuilder;

  std::vector<std::string> vstr;
  vstr.push_back((StringBuilder()
          << "text=" << (pText ? "on" : "off")
          << " textColour=" << textColour));

  vstr.push_back((StringBuilder()
          << "PositionNames=" << (pPositionNames ? "on" : "off")
          << " positionNamesColour=" << positionNamesColour));

  vstr.push_back((StringBuilder()
          << "frame=" << (pFrame ? "on" : "off")
          << " frameColour=" << frameColour
          << " frameLinetype=" << frameLinetype
          << " frameLinewidth=" << frameLinewidth));

  vstr.push_back((StringBuilder()
          << "LevelNumbers=" << (pLevelNumbers ? "on" : "off")));

  vstr.push_back((StringBuilder()
          << "UpperLevel=" << (pUpperLevel ? "on" : "off")
          << " upperLevelColour=" << upperLevelColour
          << " upperLevelLinetype=" << upperLevelLinetype
          << " upperLevelLinewidth=" << upperLevelLinewidth));

  vstr.push_back((StringBuilder()
          << "LowerLevel=" << (pLowerLevel ? "on" : "off")
          << " lowerLevelColour=" << lowerLevelColour
          << " lowerLevelLinetype=" << lowerLevelLinetype
          << " lowerLevelLinewidth=" << lowerLevelLinewidth));

  vstr.push_back((StringBuilder()
          << "OtherLevels=" << (pOtherLevels ? "on" : "off")
          << " otherLevelsColour=" << otherLevelsColour
          << " otherLevelsLinetype=" << otherLevelsLinetype
          << " otherLevelsLinewidth=" << otherLevelsLinewidth));
      
  vstr.push_back((StringBuilder()
          << "Surface=" << (pSurface ? "on" : "off")
          << " surfaceColour=" << surfaceColour
          << " surfaceLinetype=" << surfaceLinetype
          << " surfaceLinewidth=" << surfaceLinewidth));

  vstr.push_back((StringBuilder()
          << "Distance=" << (pDistance ? "on" : "off")
          << " distanceColour=" << distanceColour
          << " distanceUnit=" << distanceUnit));

  vstr.push_back((StringBuilder()
          << "XYpos=" << std::string(pXYpos ? "on" : "off")
          << " xyposColour=" << xyposColour));
  
  vstr.push_back((StringBuilder()
          << "GeoPos=" << std::string(pGeoPos ? "on" : "off")
          << " geoposColour=" << geoposColour));
          
  vstr.push_back((StringBuilder()
          << "VerticalGridLines=" << (pVerticalGridLines ? "on" : "off")
          << " vergridColour=" << vergridColour
          << " vergridLinetype=" << vergridLinetype
          << " vergridLinewidth=" << vergridLinewidth));

  vstr.push_back((StringBuilder()
          << "Markerlines=" << (pMarkerlines ? "on" : "off")
          << " markerlinesColour=" << markerlinesColour
          << " markerlinesLinetype=" << markerlinesLinetype
          << " markerlinesLinewidth=" << markerlinesLinewidth));

  vstr.push_back((StringBuilder()
          << "VerticalMarker=" << (pVerticalMarker ? "on" : "off")
          << " verticalMarkerColour=" << verticalMarkerColour
          << " verticalMarkerLinetype=" << verticalMarkerLinetype
          << " verticalMarkerLinewidth=" << verticalMarkerLinewidth
          << " verticalMarkerLimit=" << verticalMarkerLimit));

  vstr.push_back((StringBuilder()
          << "extrapolateFixedLevels=" << (extrapolateFixedLevels ? "on" : "off")));
  vstr.push_back((StringBuilder()
          << "extrapolateToBottom=" << (extrapolateToBottom ? "on" : "off")));

  vstr.push_back((StringBuilder()
          << "thinArrows=" << (thinArrows ? "on" : "off")));

  vstr.push_back((StringBuilder()
          << "Vertical=" << verticalType));

  vstr.push_back((StringBuilder()
          << "keepVerHorRatio=" << (keepVerHorRatio ? "on" : "off")
          << " verHorRatio=" << verHorRatio));

  vstr.push_back((StringBuilder()
          << "stdVerticalArea=" << (stdVerticalArea ? "on" : "off")
          << " minVerticalArea=" << minVerticalArea
          << " maxVerticalArea=" << maxVerticalArea));

  vstr.push_back((StringBuilder()
          << "stdHorizontalArea=" << (stdHorizontalArea ? "on" : "off")
          << " minHorizontalArea=" << minHorizontalArea
          << " maxHorizontalArea=" << maxHorizontalArea));

  vstr.push_back((StringBuilder()
          << "backgroundColour=" << backgroundColour));

  vstr.push_back((StringBuilder()
          << "OnMapColour=" << vcOnMapColour
          << " OnMapLinetype=" << vcOnMapLinetype
          << " OnMapLinewidth=" << vcOnMapLinewidth));

  vstr.push_back((StringBuilder()
          <<  "SelectedOnMapColour=" << vcSelectedOnMapColour
          << " SelectedOnMapLinetype=" << vcSelectedOnMapLinetype
          << " SelectedOnMapLinewidth=" << vcSelectedOnMapLinewidth));

  return vstr;
}


void VcrossOptions::readOptions(const vector<std::string>& vstr)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VcrossOptions::readOptions");
#endif

  vector<std::string> vs,tokens;
  std::string key,value;

  int n= vstr.size();

  for (int i=0; i<n; i++) {

    vs= miutil::split(vstr[i], 0, " ");

    int m= vs.size();

    for (int j=0; j<m; j++) {

      tokens= miutil::split(vs[j], 0, "=");

      if (tokens.size()==2) {

//	key=   miutil::to_lower(tokens[0]);
	key=   tokens[0];
	value= tokens[1];

	if      (key=="text")       pText= (miutil::to_lower(value)=="on");
	else if (key=="textColour") textColour= value;

	else if (key=="PositionNames")       pPositionNames= (miutil::to_lower(value)=="on");
	else if (key=="positionNamesColour") positionNamesColour= value;

	else if (key=="frame")          pFrame= (miutil::to_lower(value)=="on");
	else if (key=="frameColour")    frameColour= value;
	else if (key=="frameLinetype")  frameLinetype= value;
	else if (key=="frameLinewidth") frameLinewidth= atof(value.c_str());

	else if (key=="LevelNumbers") pLevelNumbers= (miutil::to_lower(value)=="on");

	else if (key=="UpperLevel")          pUpperLevel= (miutil::to_lower(value)=="on");
	else if (key=="upperLevelColour")    upperLevelColour= value;
	else if (key=="upperLevelLinetype")  upperLevelLinetype= value;
	else if (key=="upperLevelLinewidth") upperLevelLinewidth= atof(value.c_str());

	else if (key=="LowerLevel")          pLowerLevel= (miutil::to_lower(value)=="on");
	else if (key=="lowerLevelColour")    lowerLevelColour= value;
	else if (key=="lowerLevelLinetype")  lowerLevelLinetype= value;
	else if (key=="lowerLevelLinewidth") lowerLevelLinewidth= atof(value.c_str());

	else if (key=="OtherLevels")          pOtherLevels= (miutil::to_lower(value)=="on");
	else if (key=="otherLevelsColour")    otherLevelsColour= value;
	else if (key=="otherLevelsLinetype")  otherLevelsLinetype= value;
	else if (key=="otherLevelsLinewidth") otherLevelsLinewidth= atof(value.c_str());

	else if (key=="Surface")          pSurface= (miutil::to_lower(value)=="on");
	else if (key=="surfaceColour")    surfaceColour= value;
	else if (key=="surfaceLinetype")  surfaceLinetype= value;
	else if (key=="surfaceLinewidth") surfaceLinewidth= atof(value.c_str());

	else if (key=="Distance")       pDistance= (miutil::to_lower(value)=="on");
	else if (key=="distanceColour") distanceColour= value;
	else if (key=="distanceUnit")   distanceUnit= value;
	else if (key=="distanceStep")   distanceStep= value;

	else if (key=="XYpos")          pXYpos= (miutil::to_lower(value)=="on");
	else if (key=="xyposColour")    xyposColour= value;

	else if (key=="GeoPos")         pGeoPos= (miutil::to_lower(value)=="on");
	else if (key=="geoposColour")   geoposColour= value;

	else if (key=="VerticalGridLines") pVerticalGridLines= (miutil::to_lower(value)=="on");
	else if (key=="vergridColour")     vergridColour= value;
	else if (key=="vergridLinetype")   vergridLinetype= value;
	else if (key=="vergridLinewidth")  vergridLinewidth= atof(value.c_str());

	else if (key=="Markerlines")           pMarkerlines= (miutil::to_lower(value)=="on");
	else if (key=="markerlinesColour")     markerlinesColour= value;
	else if (key=="markerlinesLinetype")   markerlinesLinetype= value;
	else if (key=="markerlinesLinewidth")  markerlinesLinewidth= atof(value.c_str());

	else if (key=="VerticalMarker")
	  pVerticalMarker= (miutil::to_lower(value)=="on");
	else if (key=="verticalMarkerColour")
	  verticalMarkerColour= value;
	else if (key=="verticalMarkerLinetype")
	  verticalMarkerLinetype= value;
	else if (key=="verticalMarkerLinewidth")
	  verticalMarkerLinewidth= atof(value.c_str());
	else if (key=="verticalMarkerLimit")
	  verticalMarkerLimit= atof(value.c_str());

	else if (key=="extrapolateFixedLevels") extrapolateFixedLevels= (miutil::to_lower(value)=="on");
	else if (key=="extrapolateToBottom")    extrapolateToBottom= (miutil::to_lower(value)=="on");

	else if (key=="thinArrows")    thinArrows= (miutil::to_lower(value)=="on");

	else if (key=="Vertical")               verticalType= value;

	else if (key=="keepVerHorRatio") keepVerHorRatio= (miutil::to_lower(value)=="on");
	else if (key=="verHorRatio")     verHorRatio= atoi(value.c_str());

	else if (key=="stdVerticalArea") stdVerticalArea= (miutil::to_lower(value)=="on");
	else if (key=="minVerticalArea") minVerticalArea= atoi(value.c_str());
	else if (key=="maxVerticalArea") maxVerticalArea= atoi(value.c_str());

	else if (key=="stdHorizontalArea") stdHorizontalArea= (miutil::to_lower(value)=="on");
	else if (key=="minHorizontalArea") minHorizontalArea= atoi(value.c_str());
	else if (key=="maxHorizontalArea") maxHorizontalArea= atoi(value.c_str());

	else if (key=="backgroundColour") backgroundColour= value;

	else if (key=="OnMapColour")    vcOnMapColour= value;
	else if (key=="OnMapLinetype")  vcOnMapLinetype= value;
	else if (key=="OnMapLinewidth") vcOnMapLinewidth= atof(value.c_str());

	else if (key=="SelectedOnMapColour")    vcSelectedOnMapColour= value;
	else if (key=="SelectedOnMapLinetype")  vcSelectedOnMapLinetype= value;
	else if (key=="SelectedOnMapLinewidth") vcSelectedOnMapLinewidth= atof(value.c_str());

      }
    }
  }
}
