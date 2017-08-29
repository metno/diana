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

#include "VcrossOptions.h"

#include "miSetupParser.h"
#include <puTools/miStringFunctions.h>
#include <puTools/miStringBuilder.h>

#define MILOGGER_CATEGORY "diana.VcrossOptions"
#include <miLogger/miLogging.h>

namespace vcross {

VcrossOptions::VcrossOptions()
{
  setDefaults();
}


VcrossOptions::~VcrossOptions()
{
}


vcross::Z_AXIS_TYPE VcrossOptions::getVerticalType() const
{
  if (verticalCoordinate == "Pressure")
    return vcross::Z_TYPE_PRESSURE;
  else
    return vcross::Z_TYPE_ALTITUDE;
}


void VcrossOptions::setDefaults()
{
  METLIBS_LOG_SCOPE();

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

  pInflight= true;
  inflightColour= "red";
  inflightLinetype= "solid";
  inflightLinewidth= 5.;

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

  pCompass= true;

  pHorizontalGridLines= true;
  horgridColour= "grey90";
  horgridLinetype= "dot";
  horgridLinewidth= 1.;

  pVerticalGridLines= false;
  vergridColour= "black";
  vergridLinetype= "solid";
  vergridLinewidth= 1.;

  extrapolateFixedLevels= false;
  extrapolateToBottom= true;

  verticalType= "Pressure/hPa";
  verticalScale= "linear";
  verticalCoordinate= "Pressure";
  verticalUnit= "hPa";

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
}

static const char* asBool(bool b)
{
  return (b ? "on" : "off");
}

std::vector<miutil::KeyValue_v> VcrossOptions::writeOptions() const
{
  METLIBS_LOG_SCOPE();

  using miutil::kv;
  using miutil::KeyValue_v;

  std::vector<KeyValue_v> vkvs;

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("text", pText) << kv("textColour", textColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("PositionNames", pPositionNames)
              << kv("positionNamesColour", positionNamesColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("frame", pFrame)
              << kv("frameColour", frameColour)
              << kv("frameLinetype", frameLinetype)
              << kv("frameLinewidth", frameLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("LevelNumbers", pLevelNumbers);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("UpperLevel", pUpperLevel)
      << kv("upperLevelColour", upperLevelColour)
      << kv("upperLevelLinetype", upperLevelLinetype)
      << kv("upperLevelLinewidth", upperLevelLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("LowerLevel", pLowerLevel)
              << kv("lowerLevelColour", lowerLevelColour)
              << kv("lowerLevelLinetype", lowerLevelLinetype)
              << kv("lowerLevelLinewidth", lowerLevelLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("OtherLevels", pOtherLevels)
              << kv(  "otherLevelsColour", otherLevelsColour)
              << kv("otherLevelsLinetype", otherLevelsLinetype)
              << kv("otherLevelsLinewidth", otherLevelsLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("Surface", pSurface)
              << kv("surfaceColour", surfaceColour)
              << kv("surfaceLinetype", surfaceLinetype)
              << kv("surfaceLinewidth", surfaceLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("Inflight", pInflight)
              << kv("inflightColour", inflightColour)
              << kv("inflightLinetype", inflightLinetype)
              << kv("inflightLinewidth", inflightLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("Distance", pDistance)
              << kv("distanceColour", distanceColour)
              << kv("distanceUnit", distanceUnit);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("XYpos", pXYpos)
              << kv("xyposColour", xyposColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("GeoPos", pGeoPos)
              << kv("geoposColour", geoposColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("Compass", pCompass);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("HorizontalGridLines", pHorizontalGridLines)
              << kv("horgridColour", horgridColour)
              << kv("horgridLinetype", horgridLinetype)
              << kv("horgridLinewidth", horgridLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("VerticalGridLines", pVerticalGridLines)
              << kv("vergridColour", vergridColour)
              << kv("vergridLinetype", vergridLinetype)
              << kv("vergridLinewidth", vergridLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("Markerlines", pMarkerlines)
              << kv("markerlinesColour", markerlinesColour)
              << kv("markerlinesLinetype", markerlinesLinetype)
              << kv("markerlinesLinewidth", markerlinesLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("VerticalMarker", pVerticalMarker)
              << kv("verticalMarkerColour", verticalMarkerColour)
              << kv("verticalMarkerLinetype", verticalMarkerLinetype)
              << kv("verticalMarkerLinewidth", verticalMarkerLinewidth)
              << kv("verticalMarkerLimit", verticalMarkerLimit);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("extrapolateFixedLevels", extrapolateFixedLevels);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("extrapolateToBottom", extrapolateToBottom);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("Vertical", verticalType);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("verticalScale", verticalScale)
              << kv("verticalCoordinate", verticalCoordinate)
              << kv("verticalUnit", verticalUnit);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("keepVerHorRatio", keepVerHorRatio)
              << kv("verHorRatio", verHorRatio);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("stdVerticalArea", stdVerticalArea)
              << kv("minVerticalArea", minVerticalArea)
              << kv("maxVerticalArea", maxVerticalArea);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("stdHorizontalArea", stdHorizontalArea)
              << kv("minHorizontalArea", minHorizontalArea)
              << kv("maxHorizontalArea", maxHorizontalArea);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("backgroundColour", backgroundColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("OnMapColour", vcOnMapColour)
              << kv("OnMapLinetype", vcOnMapLinetype)
              << kv("OnMapLinewidth", vcOnMapLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv("SelectedOnMapColour", vcSelectedOnMapColour)
              << kv("SelectedOnMapLinetype", vcSelectedOnMapLinetype)
              << kv("SelectedOnMapLinewidth", vcSelectedOnMapLinewidth);

  return vkvs;
}


void VcrossOptions::readOptions(const std::vector<miutil::KeyValue_v>& vkvs)
{
  METLIBS_LOG_SCOPE();

  for (const miutil::KeyValue_v& kvs : vkvs) {
    for (const miutil::KeyValue& kv : kvs) {
      const std::string& key = kv.key(), value = kv.value();
      if (value.empty())
        continue;

      if      (key=="text")       pText = kv.toBool();
      else if (key=="textColour") textColour= value;

      else if (key=="PositionNames")       pPositionNames = kv.toBool();
      else if (key=="positionNamesColour") positionNamesColour= value;

      else if (key=="frame")          pFrame = kv.toBool();
      else if (key=="frameColour")    frameColour= value;
      else if (key=="frameLinetype")  frameLinetype= value;
      else if (key=="frameLinewidth") frameLinewidth = kv.toDouble();

      else if (key=="LevelNumbers") pLevelNumbers = kv.toBool();

      else if (key=="UpperLevel")          pUpperLevel = kv.toBool();
      else if (key=="upperLevelColour")    upperLevelColour= value;
      else if (key=="upperLevelLinetype")  upperLevelLinetype= value;
      else if (key=="upperLevelLinewidth") upperLevelLinewidth = kv.toDouble();

      else if (key=="LowerLevel")          pLowerLevel = kv.toBool();
      else if (key=="lowerLevelColour")    lowerLevelColour= value;
      else if (key=="lowerLevelLinetype")  lowerLevelLinetype= value;
      else if (key=="lowerLevelLinewidth") lowerLevelLinewidth = kv.toDouble();

      else if (key=="OtherLevels")          pOtherLevels = kv.toBool();
      else if (key=="otherLevelsColour")    otherLevelsColour= value;
      else if (key=="otherLevelsLinetype")  otherLevelsLinetype= value;
      else if (key=="otherLevelsLinewidth") otherLevelsLinewidth = kv.toDouble();

      else if (key=="Surface")          pSurface = kv.toBool();
      else if (key=="surfaceColour")    surfaceColour= value;
      else if (key=="surfaceLinetype")  surfaceLinetype= value;
      else if (key=="surfaceLinewidth") surfaceLinewidth= kv.toDouble();

      else if (key=="Inflight")         pInflight = kv.toBool();
      else if (key=="inflightColour")    inflightColour= value;
      else if (key=="inflightLinetype")  inflightLinetype= value;
      else if (key=="inflightLinewidth") inflightLinewidth= kv.toDouble();

      else if (key=="Distance")       pDistance = kv.toBool();
      else if (key=="distanceColour") distanceColour= value;
      else if (key=="distanceUnit")   distanceUnit= value;
      else if (key=="distanceStep")   distanceStep= value;

      else if (key=="XYpos")          pXYpos = kv.toBool();
      else if (key=="xyposColour")    xyposColour= value;

      else if (key=="GeoPos")         pGeoPos = kv.toBool();
      else if (key=="geoposColour")   geoposColour= value;

      else if (key=="Compass")        pCompass = kv.toBool();

      else if (key=="HorizontalGridLines") pHorizontalGridLines = kv.toBool();
      else if (key=="horgridColour")       horgridColour= value;
      else if (key=="horgridLinetype")     horgridLinetype= value;
      else if (key=="horgridLinewidth")    horgridLinewidth= kv.toDouble();

      else if (key=="VerticalGridLines") pVerticalGridLines = kv.toBool();
      else if (key=="vergridColour")     vergridColour= value;
      else if (key=="vergridLinetype")   vergridLinetype= value;
      else if (key=="vergridLinewidth")  vergridLinewidth= kv.toDouble();

      else if (key=="Markerlines")           pMarkerlines = kv.toBool();
      else if (key=="markerlinesColour")     markerlinesColour= value;
      else if (key=="markerlinesLinetype")   markerlinesLinetype= value;
      else if (key=="markerlinesLinewidth")  markerlinesLinewidth= kv.toDouble();

      else if (key=="VerticalMarker")
        pVerticalMarker = kv.toBool();
      else if (key=="verticalMarkerColour")
        verticalMarkerColour= value;
      else if (key=="verticalMarkerLinetype")
        verticalMarkerLinetype= value;
      else if (key=="verticalMarkerLinewidth")
        verticalMarkerLinewidth= kv.toDouble();
      else if (key=="verticalMarkerLimit")
        verticalMarkerLimit= kv.toDouble();

      else if (key=="extrapolateFixedLevels") extrapolateFixedLevels = kv.toBool();
      else if (key=="extrapolateToBottom")    extrapolateToBottom = kv.toBool();

      else if (key=="Vertical")               verticalType= value;
      else if (key=="verticalScale")               verticalScale= value;
      else if (key=="verticalCoordinate")               verticalCoordinate= value;
      else if (key=="verticalUnit")               verticalUnit= value;

      else if (key=="keepVerHorRatio") keepVerHorRatio = kv.toBool();
      else if (key=="verHorRatio")     verHorRatio= kv.toDouble();

      else if (key=="stdVerticalArea") stdVerticalArea = kv.toBool();
      else if (key=="minVerticalArea") minVerticalArea= kv.toInt();
      else if (key=="maxVerticalArea") maxVerticalArea= kv.toInt();

      else if (key=="stdHorizontalArea") stdHorizontalArea = kv.toBool();
      else if (key=="minHorizontalArea") minHorizontalArea= kv.toInt();
      else if (key=="maxHorizontalArea") maxHorizontalArea= kv.toInt();

      else if (key=="backgroundColour") backgroundColour= value;

      else if (key=="OnMapColour")    vcOnMapColour= value;
      else if (key=="OnMapLinetype")  vcOnMapLinetype= value;
      else if (key=="OnMapLinewidth") vcOnMapLinewidth= kv.toDouble();

      else if (key=="SelectedOnMapColour")    vcSelectedOnMapColour= value;
      else if (key=="SelectedOnMapLinetype")  vcSelectedOnMapLinetype= value;
      else if (key=="SelectedOnMapLinewidth") vcSelectedOnMapLinewidth= kv.toDouble();
    }
  }
}

} // namespace vcross
