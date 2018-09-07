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
#ifndef VPROFOPTIONS_H
#define VPROFOPTIONS_H

#include "util/diKeyValue.h"
#include <string>
#include <vector>

/**
   \brief Options/settings/defaults for the Vertical Profile diagram
*/
class VprofOptions
{
public:
  VprofOptions();
  ~VprofOptions();
  void setDefaults();
  void checkValues();
  // read/write setup and log
  std::vector<std::string> writeOptions();
  void readOptions(const miutil::KeyValue_v& kvs);

  // data to be shown
  bool ptttt;    // t
  bool ptdtd;    // td
  bool pwind;    // wind
  bool pvwind;   // vertical wind
  bool prelhum;  // relative humidty
  bool pducting; // ducting
  bool pkindex;  // Kindex
  bool pslwind;  // numbers at significant wind levels

  std::vector<std::string> dataColour;
  std::vector<float> dataLinewidth;
  std::vector<float> windLinewidth;

  bool windseparate; // separate wind columns when multiple data
  float rvwind;      // range of vertical wind in unit hPa/s (-range to +range)
  float ductingMin;  // min ducting in diagram
  float ductingMax;  // max ducting in diagram

  bool ptext;    // text (station name etc.)
  bool pgeotext; // geographic position in text

  //-----------------------------------------------------------------------

  std::string backgroundColour;

  int diagramtype; // 0=Amble  1=Exner(pi)  2=Pressure  3=ln(P)
                   // (Amble: ln(P) below 500hPa, P above 500hPa)
  float tangle;    // angle between the vertical and temperatur lines,
                   // 0. - 80. degrees, 45 degrees on Amble diagram
  int pminDiagram; // min pressure
  int pmaxDiagram; // max pressure
  int tminDiagram; // min temperature at 1000 hPa
  int tmaxDiagram; // max temperature at 1000 hPa

  bool pplines;                  // pressure lines
  bool pplinesfl;                // false=at pressure levels  true=at flight levels
  std::vector<float> plevels;    // pressure
  std::vector<int> flightlevels; // flight levels
  std::string pColour;
  std::string pLinetype;
  float pLinewidth1; // thin lines
  float pLinewidth2;

  bool ptlines; // temperature lines
  int tStep;    // temp. step
  std::string tColour;
  std::string tLinetype;
  float tLinewidth1; // thin lines
  float tLinewidth2;

  bool pdryadiabat;   // dry adiabats
  int dryadiabatStep; // temperature step (C at 1000hPa)
  std::string dryadiabatColour;
  std::string dryadiabatLinetype;
  float dryadiabatLinewidth;

  bool pwetadiabat;   // dry adiabats
  int wetadiabatStep; // temperature step (C at 1000hPa)
  std::string wetadiabatColour;
  std::string wetadiabatLinetype;
  float wetadiabatLinewidth;
  int wetadiabatPmin;
  int wetadiabatTmin;

  bool pmixingratio;  // mixing ratio
  int mixingratioSet; // line set no. (1,2,3,4 available)
  std::string mixingratioColour;
  std::string mixingratioLinetype;
  float mixingratioLinewidth;
  int mixingratioPmin;
  int mixingratioTmin;
  std::vector<std::vector<float>> qtable; // units: g/kg

  bool plabelp; // p labels (numbers)
  bool plabelt; // t labels (numbers)
  bool plabelq; // mixing ratio labels (numbers)
  bool pframe;  // frame
  std::string frameColour;
  std::string frameLinetype;
  float frameLinewidth;
  std::string textColour;

  bool pflevels; // flight levels (numbers/marks on axis only)
  std::string flevelsColour;
  std::string flevelsLinetype;
  float flevelsLinewidth1;
  bool plabelflevels; // labels (numbers)

  float rsvaxis;   // relative size vertical axis
  float rstext;    // relative size text
  float rslabels;  // relative size labels (p and t numbers)
  float rswind;    // relative size wind   (width of column)
  float rsvwind;   // relative size vertical wind (width of column)
  float rsrelhum;  // relative size relative humidity (width of column)
  float rsducting; // relative size ducting (width of column)

  std::string rangeLinetype; // for vertical wind, rel.hum. and ducting
  float rangeLinewidth;

  bool pcotrails; // condensation trail lines
                  // (linjer for vurdering av mulighet for
  //  kondensstriper fra fly)
  std::string cotrailsColour;
  std::string cotrailsLinetype;
  float cotrailsLinewidth;
  int cotrailsPmin;
  int cotrailsPmax;
};

#endif
