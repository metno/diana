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

namespace {

const char key_text[] = "text";
const char key_textColour[] = "textcolour";

const char key_PositionNames[] = "positionnames";
const char key_positionNamesColour[] = "positionnamescolour";

const char key_frame[] = "frame";
const char key_frameColour[] = "framecolour";
const char key_frameLinetype[] = "framelinetype";
const char key_frameLinewidth[] = "framelinewidth";

const char key_LevelNumbers[] = "levelnumbers";

const char key_UpperLevel[] = "upperlevel";
const char key_upperLevelColour[] = "upperlevelcolour";
const char key_upperLevelLinetype[] = "upperlevellinetype";
const char key_upperLevelLinewidth[] = "upperlevellinewidth";

const char key_LowerLevel[] = "lowerlevel";
const char key_lowerLevelColour[] = "lowerlevelcolour";
const char key_lowerLevelLinetype[] = "lowerlevellinetype";
const char key_lowerLevelLinewidth[] = "lowerlevellinewidth";

const char key_OtherLevels[] = "otherlevels";
const char key_otherLevelsColour[] = "otherlevelscolour";
const char key_otherLevelsLinetype[] = "otherlevelslinetype";
const char key_otherLevelsLinewidth[] = "otherlevelslinewidth";

const char key_Surface[] = "surface";
const char key_surfaceColour[] = "surfacecolour";
const char key_surfaceLinetype[] = "surfacelinetype";
const char key_surfaceLinewidth[] = "surfacelinewidth";

const char key_Inflight[] = "inflight";
const char key_inflightColour[] = "inflightcolour";
const char key_inflightLinetype[] = "inflightlinetype";
const char key_inflightLinewidth[] = "inflightlinewidth";

const char key_Distance[] = "distance";
const char key_distanceColour[] = "distancecolour";
const char key_distanceUnit[] = "distanceunit";
const char key_distanceStep[] = "distancestep";

const char key_XYpos[] = "xypos";
const char key_xyposColour[] = "xyposcolour";

const char key_GeoPos[] = "geopos";
const char key_geoposColour[] = "geoposcolour";

const char key_Compass[] = "compass";

const char key_HorizontalGridLines[] = "horizontalgridlines";
const char key_horgridColour[] = "horgridcolour";
const char key_horgridLinetype[] = "horgridlinetype";
const char key_horgridLinewidth[] = "horgridlinewidth";

const char key_VerticalGridLines[] = "verticalgridlines";
const char key_vergridColour[] = "vergridcolour";
const char key_vergridLinetype[] = "vergridlinetype";
const char key_vergridLinewidth[] = "vergridlinewidth";

const char key_Markerlines[] = "markerlines";
const char key_markerlinesColour[] = "markerlinescolour";
const char key_markerlinesLinetype[] = "markerlineslinetype";
const char key_markerlinesLinewidth[] = "markerlineslinewidth";

const char key_VerticalMarker[] = "verticalmarker";
const char key_verticalMarkerColour[] = "verticalmarkercolour";
const char key_verticalMarkerLinetype[] = "verticalmarkerlinetype";
const char key_verticalMarkerLinewidth[] = "verticalmarkerlinewidth";
const char key_verticalMarkerLimit[] = "verticalmarkerlimit";

const char key_extrapolateFixedLevels[] = "extrapolatefixedlevels";
const char key_extrapolateToBottom[] = "extrapolatetobottom";

const char key_Vertical[] = "vertical";
const char key_verticalScale[] = "verticalscale";
const char key_verticalCoordinate[] = "verticalcoordinate";
const char key_verticalUnit[] = "verticalunit";

const char key_keepVerHorRatio[] = "keepverhorratio";
const char key_verHorRatio[] = "verhorratio";

const char key_stdVerticalArea[] = "stdverticalarea";
const char key_minVerticalArea[] = "minverticalarea";
const char key_maxVerticalArea[] = "maxverticalarea";

const char key_stdHorizontalArea[] = "stdhorizontalarea";
const char key_minHorizontalArea[] = "minhorizontalarea";
const char key_maxHorizontalArea[] = "maxhorizontalarea";

const char key_backgroundColour[] = "backgroundcolour";

const char key_OnMapColour[] = "onmapcolour";
const char key_OnMapLinetype[] = "onmaplinetype";
const char key_OnMapLinewidth[] = "onmaplinewidth";

const char key_SelectedOnMapColour[] = "selectedonmapcolour";
const char key_SelectedOnMapLinetype[] = "selectedonmaplinetype";
const char key_SelectedOnMapLinewidth[] = "selectedonmaplinewidth";

} // namespace

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

std::vector<miutil::KeyValue_v> VcrossOptions::writeOptions() const
{
  METLIBS_LOG_SCOPE();

  using miutil::kv;
  using miutil::KeyValue_v;

  std::vector<KeyValue_v> vkvs;

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_text, pText)
              << kv(key_textColour, textColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_PositionNames, pPositionNames)
              << kv(key_positionNamesColour, positionNamesColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_frame, pFrame)
              << kv(key_frameColour, frameColour)
              << kv(key_frameLinetype, frameLinetype)
              << kv(key_frameLinewidth, frameLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_LevelNumbers, pLevelNumbers);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_UpperLevel, pUpperLevel)
      << kv(key_upperLevelColour, upperLevelColour)
      << kv(key_upperLevelLinetype, upperLevelLinetype)
      << kv(key_upperLevelLinewidth, upperLevelLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_LowerLevel, pLowerLevel)
              << kv(key_lowerLevelColour, lowerLevelColour)
              << kv(key_lowerLevelLinetype, lowerLevelLinetype)
              << kv(key_lowerLevelLinewidth, lowerLevelLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_OtherLevels, pOtherLevels)
              << kv(key_otherLevelsColour, otherLevelsColour)
              << kv(key_otherLevelsLinetype, otherLevelsLinetype)
              << kv(key_otherLevelsLinewidth, otherLevelsLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_Surface, pSurface)
              << kv(key_surfaceColour, surfaceColour)
              << kv(key_surfaceLinetype, surfaceLinetype)
              << kv(key_surfaceLinewidth, surfaceLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_Inflight, pInflight)
              << kv(key_inflightColour, inflightColour)
              << kv(key_inflightLinetype, inflightLinetype)
              << kv(key_inflightLinewidth, inflightLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_Distance, pDistance)
              << kv(key_distanceColour, distanceColour)
              << kv(key_distanceUnit, distanceUnit);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_XYpos, pXYpos)
              << kv(key_xyposColour, xyposColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_GeoPos, pGeoPos)
              << kv(key_geoposColour, geoposColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_Compass, pCompass);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_HorizontalGridLines, pHorizontalGridLines)
              << kv(key_horgridColour, horgridColour)
              << kv(key_horgridLinetype, horgridLinetype)
              << kv(key_horgridLinewidth, horgridLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_VerticalGridLines, pVerticalGridLines)
              << kv(key_vergridColour, vergridColour)
              << kv(key_vergridLinetype, vergridLinetype)
              << kv(key_vergridLinewidth, vergridLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_Markerlines, pMarkerlines)
              << kv(key_markerlinesColour, markerlinesColour)
              << kv(key_markerlinesLinetype, markerlinesLinetype)
              << kv(key_markerlinesLinewidth, markerlinesLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_VerticalMarker, pVerticalMarker)
              << kv(key_verticalMarkerColour, verticalMarkerColour)
              << kv(key_verticalMarkerLinetype, verticalMarkerLinetype)
              << kv(key_verticalMarkerLinewidth, verticalMarkerLinewidth)
              << kv(key_verticalMarkerLimit, verticalMarkerLimit);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_extrapolateFixedLevels, extrapolateFixedLevels);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_extrapolateToBottom, extrapolateToBottom);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_Vertical, verticalType);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_verticalScale, verticalScale)
              << kv(key_verticalCoordinate, verticalCoordinate)
              << kv(key_verticalUnit, verticalUnit);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_keepVerHorRatio, keepVerHorRatio)
              << kv(key_verHorRatio, verHorRatio);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_stdVerticalArea, stdVerticalArea)
              << kv(key_minVerticalArea, minVerticalArea)
              << kv(key_maxVerticalArea, maxVerticalArea);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_stdHorizontalArea, stdHorizontalArea)
              << kv(key_minHorizontalArea, minHorizontalArea)
              << kv(key_maxHorizontalArea, maxHorizontalArea);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_backgroundColour, backgroundColour);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_OnMapColour, vcOnMapColour)
              << kv(key_OnMapLinetype, vcOnMapLinetype)
              << kv(key_OnMapLinewidth, vcOnMapLinewidth);

  vkvs.push_back(KeyValue_v());
  vkvs.back() << kv(key_SelectedOnMapColour, vcSelectedOnMapColour)
              << kv(key_SelectedOnMapLinetype, vcSelectedOnMapLinetype)
              << kv(key_SelectedOnMapLinewidth, vcSelectedOnMapLinewidth);

  return vkvs;
}


void VcrossOptions::readOptions(const std::vector<miutil::KeyValue_v>& vkvs)
{
  METLIBS_LOG_SCOPE();

  for (const miutil::KeyValue_v& kvs : vkvs) {
    for (const miutil::KeyValue& kv : kvs) {
      const std::string& key = kv.key();
      const std::string& value = kv.value();
      if (value.empty())
        continue;
      METLIBS_LOG_DEBUG(LOGVAL(kv));

      if      (key==key_text)       pText = kv.toBool();
      else if (key==key_textColour) textColour= value;

      else if (key==key_PositionNames)       pPositionNames = kv.toBool();
      else if (key==key_positionNamesColour) positionNamesColour= value;

      else if (key==key_frame)          pFrame = kv.toBool();
      else if (key==key_frameColour)    frameColour= value;
      else if (key==key_frameLinetype)  frameLinetype= value;
      else if (key==key_frameLinewidth) frameLinewidth = kv.toDouble();

      else if (key==key_LevelNumbers) pLevelNumbers = kv.toBool();

      else if (key==key_UpperLevel)          pUpperLevel = kv.toBool();
      else if (key==key_upperLevelColour)    upperLevelColour= value;
      else if (key==key_upperLevelLinetype)  upperLevelLinetype= value;
      else if (key==key_upperLevelLinewidth) upperLevelLinewidth = kv.toDouble();

      else if (key==key_LowerLevel)          pLowerLevel = kv.toBool();
      else if (key==key_lowerLevelColour)    lowerLevelColour= value;
      else if (key==key_lowerLevelLinetype)  lowerLevelLinetype= value;
      else if (key==key_lowerLevelLinewidth) lowerLevelLinewidth = kv.toDouble();

      else if (key==key_OtherLevels)          pOtherLevels = kv.toBool();
      else if (key==key_otherLevelsColour)    otherLevelsColour= value;
      else if (key==key_otherLevelsLinetype)  otherLevelsLinetype= value;
      else if (key==key_otherLevelsLinewidth) otherLevelsLinewidth = kv.toDouble();

      else if (key==key_Surface)          pSurface = kv.toBool();
      else if (key==key_surfaceColour)    surfaceColour= value;
      else if (key==key_surfaceLinetype)  surfaceLinetype= value;
      else if (key==key_surfaceLinewidth) surfaceLinewidth= kv.toDouble();

      else if (key==key_Inflight)         pInflight = kv.toBool();
      else if (key==key_inflightColour)    inflightColour= value;
      else if (key==key_inflightLinetype)  inflightLinetype= value;
      else if (key==key_inflightLinewidth) inflightLinewidth= kv.toDouble();

      else if (key==key_Distance)       pDistance = kv.toBool();
      else if (key==key_distanceColour) distanceColour= value;
      else if (key==key_distanceUnit)   distanceUnit= value;
      else if (key==key_distanceStep)   distanceStep= value;

      else if (key==key_XYpos)          pXYpos = kv.toBool();
      else if (key==key_xyposColour)    xyposColour= value;

      else if (key==key_GeoPos)         pGeoPos = kv.toBool();
      else if (key==key_geoposColour)   geoposColour= value;

      else if (key==key_Compass)        pCompass = kv.toBool();

      else if (key==key_HorizontalGridLines) pHorizontalGridLines = kv.toBool();
      else if (key==key_horgridColour)       horgridColour= value;
      else if (key==key_horgridLinetype)     horgridLinetype= value;
      else if (key==key_horgridLinewidth)    horgridLinewidth= kv.toDouble();

      else if (key==key_VerticalGridLines) pVerticalGridLines = kv.toBool();
      else if (key==key_vergridColour)     vergridColour= value;
      else if (key==key_vergridLinetype)   vergridLinetype= value;
      else if (key==key_vergridLinewidth)  vergridLinewidth= kv.toDouble();

      else if (key==key_Markerlines)           pMarkerlines = kv.toBool();
      else if (key==key_markerlinesColour)     markerlinesColour= value;
      else if (key==key_markerlinesLinetype)   markerlinesLinetype= value;
      else if (key==key_markerlinesLinewidth)  markerlinesLinewidth= kv.toDouble();

      else if (key==key_VerticalMarker)
        pVerticalMarker = kv.toBool();
      else if (key==key_verticalMarkerColour)
        verticalMarkerColour= value;
      else if (key==key_verticalMarkerLinetype)
        verticalMarkerLinetype= value;
      else if (key==key_verticalMarkerLinewidth)
        verticalMarkerLinewidth= kv.toDouble();
      else if (key==key_verticalMarkerLimit)
        verticalMarkerLimit= kv.toDouble();

      else if (key==key_extrapolateFixedLevels) extrapolateFixedLevels = kv.toBool();
      else if (key==key_extrapolateToBottom)    extrapolateToBottom = kv.toBool();

      else if (key==key_Vertical)               verticalType= value;
      else if (key==key_verticalScale)               verticalScale= value;
      else if (key==key_verticalCoordinate)               verticalCoordinate= value;
      else if (key==key_verticalUnit)               verticalUnit= value;

      else if (key==key_keepVerHorRatio) keepVerHorRatio = kv.toBool();
      else if (key==key_verHorRatio)     verHorRatio= kv.toDouble();

      else if (key==key_stdVerticalArea) stdVerticalArea = kv.toBool();
      else if (key==key_minVerticalArea) minVerticalArea= kv.toInt();
      else if (key==key_maxVerticalArea) maxVerticalArea= kv.toInt();

      else if (key==key_stdHorizontalArea) stdHorizontalArea = kv.toBool();
      else if (key==key_minHorizontalArea) minHorizontalArea= kv.toInt();
      else if (key==key_maxHorizontalArea) maxHorizontalArea= kv.toInt();

      else if (key==key_backgroundColour) backgroundColour= value;

      else if (key==key_OnMapColour)    vcOnMapColour= value;
      else if (key==key_OnMapLinetype)  vcOnMapLinetype= value;
      else if (key==key_OnMapLinewidth) vcOnMapLinewidth= kv.toDouble();

      else if (key==key_SelectedOnMapColour)    vcSelectedOnMapColour= value;
      else if (key==key_SelectedOnMapLinetype)  vcSelectedOnMapLinetype= value;
      else if (key==key_SelectedOnMapLinewidth) vcSelectedOnMapLinewidth= kv.toDouble();
      else {
        METLIBS_LOG_WARN("unknown vcross option " << kv);
      }
    }
  }
}

} // namespace vcross
