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
#ifndef VPROFTABLES_H
#define VPROFTABLES_H

#include "diColour.h"

#include <puTools/miTime.h>

#include <vector>

class DiPainter;

//=================================================
// Superclass for VprofDiagram and VprofPlot,
// keeping misc. common information.
// Mainly updated by VprofDiagram and used by both.
// All member data are static.
//=================================================


// water vapour saturation pressure table
//        vanndampens metningstrykk, ewt(41).
//        t i grader celsius: -100,-95,-90,...,90,95,100.
//            tc=...    x=(tc+105)*0.2    l=x
//            et=ewt(l)+(ewt(l+1)-ewt(l))*(x-l)
// => fetched from MetNo::Constants

const int mxysize= 10;  // the no. of areas/frames on a diagram (background)

// vertical table resolution in hPa
const int idptab= 5;
// length of vertical tables (0-1300 hPa)
const int mptab= 261;

/// Annotation info for Vertical Profile
struct VprofText {
  int      index;
  Colour   colour;
  std::string modelName;
  std::string posName;
  bool     prognostic;
  int      forecastHour;
  miutil::miTime   validTime;
  float    latitude;
  float    longitude;
  bool     kindexFound;
  float    kindexValue;
};


/**
   \brief Common tables and functions for Vertical Profile plotting

*/
class VprofTables
{
public:
  VprofTables();
  ~VprofTables();

protected:
  void clearCharsizes();
  void addCharsize(float chy);
  void setFontsize(DiPainter* gl, float chy);
  void makeFontsizes(DiPainter* gl, float wx, float wy, int vx, int vy);

  // fontpack and sizes
  static std::vector<float> charsizes;
  static std::vector<float> fontsizes;
  static std::string defaultFont;

  //---------------------------------------------------------------

  static float chxbas, chybas; // standard character size
  static float chxlab, chylab; // labels etc. character size
  static float chxtxt, chytxt; // text (name,time etc.) character size

  static float xysize[mxysize][4];           // position of diagram parts
		     // xysize[n][0] : x min
                     // xysize[n][1] : x max
                     // xysize[n][2] : y min
                     // xysize[n][3] : y max
                            // n=0 : total
                            // n=1 : p-t diagram
                            // n=2 : p-t diagram with labels
                            // n=3 : flight levels (skipped ????)
                            // n=4 : significant wind levels
                            // n=5 : wind (first if multiple)
                            // n=6 : vertical wind
                            // n=7 : relative humidity
                            // n=8 : ducting
                            // n=9 : text

  static int   iptab[mptab];  // pressure in unit hPa
  static float pptab[mptab];  // pressure in unit hPa
  static float pitab[mptab];  // exner function (pi)
  static float pplog[mptab];  // ln(pressure)
  static float yptabd[mptab]; // y
  static float xztabd[mptab]; // x(t.celsius=0)
  static float yptab[mptab];  // y
  static float xztab[mptab];  // x(t.celsius=0)
  static float dx1degree;     // x distance for 1 degree celsius

  static std::vector<VprofText> vptext; // info for text plotting
};

#endif
