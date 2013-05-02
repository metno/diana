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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diVcrossOptions.h>
#include <iostream>

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
  cerr << "VcrossOptions::setDefaults" << endl;
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


vector<miString> VcrossOptions::writeOptions()
{
#ifdef DEBUGPRINT
  cerr << "VcrossOptions::writeOptions" << endl;
#endif

  vector<miString> vstr;
  miString str;

  str=  "text=" + miString(pText ? "on" : "off");
  str+= " textColour=" + textColour;
  vstr.push_back(str);

  str=  "PositionNames=" + miString(pPositionNames ? "on" : "off");
  str+= " positionNamesColour=" + positionNamesColour;
  vstr.push_back(str);

  str=  "frame=" + miString(pFrame ? "on" : "off");
  str+= " frameColour=" + frameColour;
  str+= " frameLinetype=" + frameLinetype;
  str+= " frameLinewidth=" + miString(frameLinewidth);
  vstr.push_back(str);

  str=  "LevelNumbers=" + miString(pLevelNumbers ? "on" : "off");
  vstr.push_back(str);

  str=  "UpperLevel=" + miString(pUpperLevel ? "on" : "off");
  str+= " upperLevelColour=" + upperLevelColour;
  str+= " upperLevelLinetype=" + upperLevelLinetype;
  str+= " upperLevelLinewidth=" + miString(upperLevelLinewidth);
  vstr.push_back(str);

  str=  "LowerLevel=" + miString(pLowerLevel ? "on" : "off");
  str+= " lowerLevelColour=" + lowerLevelColour;
  str+= " lowerLevelLinetype=" + lowerLevelLinetype;
  str+= " lowerLevelLinewidth=" + miString(lowerLevelLinewidth);
  vstr.push_back(str);

  str=  "OtherLevels=" + miString(pOtherLevels ? "on" : "off");
  str+= " otherLevelsColour=" + otherLevelsColour;
  str+= " otherLevelsLinetype=" + otherLevelsLinetype;
  str+= " otherLevelsLinewidth=" + miString(otherLevelsLinewidth);
  vstr.push_back(str);

  str=  "Surface=" + miString(pSurface ? "on" : "off");
  str+= " surfaceColour=" + surfaceColour;
  str+= " surfaceLinetype=" + surfaceLinetype;
  str+= " surfaceLinewidth=" + miString(surfaceLinewidth);
  vstr.push_back(str);

  str=  "Distance=" + miString(pDistance ? "on" : "off");
  str+= " distanceColour=" + distanceColour;
  str+= " distanceUnit=" + distanceUnit;
  vstr.push_back(str);

  str=  "XYpos=" + miString(pXYpos ? "on" : "off");
  str+= " xyposColour=" + xyposColour;
  vstr.push_back(str);

  str=  "GeoPos=" + miString(pGeoPos ? "on" : "off");
  str+= " geoposColour=" + geoposColour;
  vstr.push_back(str);

  str=  "VerticalGridLines=" + miString(pVerticalGridLines ? "on" : "off");
  str+= " vergridColour=" + vergridColour;
  str+= " vergridLinetype=" + vergridLinetype;
  str+= " vergridLinewidth=" + miString(vergridLinewidth);
  vstr.push_back(str);

  str=  "Markerlines=" + miString(pMarkerlines ? "on" : "off");
  str+= " markerlinesColour=" + markerlinesColour;
  str+= " markerlinesLinetype=" + markerlinesLinetype;
  str+= " markerlinesLinewidth=" + miString(markerlinesLinewidth);
  vstr.push_back(str);

  str=  "VerticalMarker=" + miString(pVerticalMarker ? "on" : "off");
  str+= " verticalMarkerColour=" + verticalMarkerColour;
  str+= " verticalMarkerLinetype=" + verticalMarkerLinetype;
  str+= " verticalMarkerLinewidth=" + miString(verticalMarkerLinewidth);
  str+= " verticalMarkerLimit=" + miString(verticalMarkerLimit);
  vstr.push_back(str);

  str=  "extrapolateFixedLevels=" + miString(extrapolateFixedLevels ? "on" : "off");
  vstr.push_back(str);
  str=  "extrapolateToBottom=" + miString(extrapolateToBottom ? "on" : "off");
  vstr.push_back(str);

  str=  "thinArrows=" + miString(thinArrows ? "on" : "off");
  vstr.push_back(str);

  str=  "Vertical=" + verticalType;
  vstr.push_back(str);

  str=  "keepVerHorRatio=" + miString(keepVerHorRatio ? "on" : "off");
  str+= " verHorRatio=" + miString(verHorRatio);
  vstr.push_back(str);

  str=  "stdVerticalArea=" + miString(stdVerticalArea ? "on" : "off");
  str+= " minVerticalArea=" + miString(minVerticalArea);
  str+= " maxVerticalArea=" + miString(maxVerticalArea);
  vstr.push_back(str);

  str=  "stdHorizontalArea=" + miString(stdHorizontalArea ? "on" : "off");
  str+= " minHorizontalArea=" + miString(minHorizontalArea);
  str+= " maxHorizontalArea=" + miString(maxHorizontalArea);
  vstr.push_back(str);

  str= "backgroundColour=" + backgroundColour;
  vstr.push_back(str);

  str=  "OnMapColour=" + vcOnMapColour;
  str+= " OnMapLinetype=" + vcOnMapLinetype;
  str+= " OnMapLinewidth=" + miString(vcOnMapLinewidth);
  vstr.push_back(str);

  str=  "SelectedOnMapColour=" + vcSelectedOnMapColour;
  str+= " SelectedOnMapLinetype=" + vcSelectedOnMapLinetype;
  str+= " SelectedOnMapLinewidth=" + miString(vcSelectedOnMapLinewidth);
  vstr.push_back(str);

  return vstr;
}


void VcrossOptions::readOptions(const vector<miString>& vstr)
{
#ifdef DEBUGPRINT
  cerr << "VcrossOptions::readOptions" << endl;
#endif

  vector<miString> vs,tokens;
  miString key,value;

  int n= vstr.size();

  for (int i=0; i<n; i++) {

    vs= vstr[i].split(' ');

    int m= vs.size();

    for (int j=0; j<m; j++) {

      tokens= vs[j].split('=');

      if (tokens.size()==2) {

//	key=   tokens[0].downcase();
	key=   tokens[0];
	value= tokens[1];

	if      (key=="text")       pText= (value.downcase()=="on");
	else if (key=="textColour") textColour= value;

	else if (key=="PositionNames")       pPositionNames= (value.downcase()=="on");
	else if (key=="positionNamesColour") positionNamesColour= value;

	else if (key=="frame")          pFrame= (value.downcase()=="on");
	else if (key=="frameColour")    frameColour= value;
	else if (key=="frameLinetype")  frameLinetype= value;
	else if (key=="frameLinewidth") frameLinewidth= atof(value.c_str());

	else if (key=="LevelNumbers") pLevelNumbers= (value.downcase()=="on");

	else if (key=="UpperLevel")          pUpperLevel= (value.downcase()=="on");
	else if (key=="upperLevelColour")    upperLevelColour= value;
	else if (key=="upperLevelLinetype")  upperLevelLinetype= value;
	else if (key=="upperLevelLinewidth") upperLevelLinewidth= atof(value.c_str());

	else if (key=="LowerLevel")          pLowerLevel= (value.downcase()=="on");
	else if (key=="lowerLevelColour")    lowerLevelColour= value;
	else if (key=="lowerLevelLinetype")  lowerLevelLinetype= value;
	else if (key=="lowerLevelLinewidth") lowerLevelLinewidth= atof(value.c_str());

	else if (key=="OtherLevels")          pOtherLevels= (value.downcase()=="on");
	else if (key=="otherLevelsColour")    otherLevelsColour= value;
	else if (key=="otherLevelsLinetype")  otherLevelsLinetype= value;
	else if (key=="otherLevelsLinewidth") otherLevelsLinewidth= atof(value.c_str());

	else if (key=="Surface")          pSurface= (value.downcase()=="on");
	else if (key=="surfaceColour")    surfaceColour= value;
	else if (key=="surfaceLinetype")  surfaceLinetype= value;
	else if (key=="surfaceLinewidth") surfaceLinewidth= atof(value.c_str());

	else if (key=="Distance")       pDistance= (value.downcase()=="on");
	else if (key=="distanceColour") distanceColour= value;
	else if (key=="distanceUnit")   distanceUnit= value;
	else if (key=="distanceStep")   distanceStep= value;

	else if (key=="XYpos")          pXYpos= (value.downcase()=="on");
	else if (key=="xyposColour")    xyposColour= value;

	else if (key=="GeoPos")         pGeoPos= (value.downcase()=="on");
	else if (key=="geoposColour")   geoposColour= value;

	else if (key=="VerticalGridLines") pVerticalGridLines= (value.downcase()=="on");
	else if (key=="vergridColour")     vergridColour= value;
	else if (key=="vergridLinetype")   vergridLinetype= value;
	else if (key=="vergridLinewidth")  vergridLinewidth= atof(value.c_str());

	else if (key=="Markerlines")           pMarkerlines= (value.downcase()=="on");
	else if (key=="markerlinesColour")     markerlinesColour= value;
	else if (key=="markerlinesLinetype")   markerlinesLinetype= value;
	else if (key=="markerlinesLinewidth")  markerlinesLinewidth= atof(value.c_str());

	else if (key=="VerticalMarker")
	  pVerticalMarker= (value.downcase()=="on");
	else if (key=="verticalMarkerColour")
	  verticalMarkerColour= value;
	else if (key=="verticalMarkerLinetype")
	  verticalMarkerLinetype= value;
	else if (key=="verticalMarkerLinewidth")
	  verticalMarkerLinewidth= atof(value.c_str());
	else if (key=="verticalMarkerLimit")
	  verticalMarkerLimit= atof(value.c_str());

	else if (key=="extrapolateFixedLevels") extrapolateFixedLevels= (value.downcase()=="on");
	else if (key=="extrapolateToBottom")    extrapolateToBottom= (value.downcase()=="on");

	else if (key=="thinArrows")    thinArrows= (value.downcase()=="on");

	else if (key=="Vertical")               verticalType= value;

	else if (key=="keepVerHorRatio") keepVerHorRatio= (value.downcase()=="on");
	else if (key=="verHorRatio")     verHorRatio= atoi(value.c_str());

	else if (key=="stdVerticalArea") stdVerticalArea= (value.downcase()=="on");
	else if (key=="minVerticalArea") minVerticalArea= atoi(value.c_str());
	else if (key=="maxVerticalArea") maxVerticalArea= atoi(value.c_str());

	else if (key=="stdHorizontalArea") stdHorizontalArea= (value.downcase()=="on");
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
