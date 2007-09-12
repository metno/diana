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
#ifndef VPROFTABLES_H
#define VPROFTABLES_H

#include <diFontManager.h>
#include <diPrintOptions.h>
#include <diColour.h>
#include <miString.h>
#include <miTime.h>
#include <vector>
#include <GL/gl.h>

using namespace std;

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
const int mewtab= 41;
const float ewt[mewtab]=
		  { .000034,.000089,.000220,.000517,.001155,.002472,
                    .005080,.01005, .01921, .03553, .06356, .1111,
                    .1891,  .3139,  .5088,  .8070,  1.2540, 1.9118,
                    2.8627, 4.2148, 6.1078, 8.7192, 12.272, 17.044,
                    23.373, 31.671, 42.430, 56.236, 73.777, 95.855,
                    123.40, 157.46, 199.26, 250.16, 311.69, 385.56,
                    473.67, 578.09, 701.13, 845.28, 1013.25 };

const int mxysize= 10;  // the no. of areas/frames on a diagram (background)

// vertical table resolution in hPa
const int idptab= 5;
// length of vertical tables (0-1300 hPa)
const int mptab= 261;

class GLPfile;

/// Annotation info for Vertical Profile
struct VprofText {
  int      index;
  Colour   colour;
  miString modelName;
  miString posName;
  bool     prognostic;
  int      forecastHour;
  miTime   validTime;
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

  static void xyclip(int npos, float *x, float *y, float xylim[4]);

  // postscript output
  bool startPSoutput(const printOptions& po);
  void addHCStencil(const int& size,
		    const float* x, const float* y);
  void addHCScissor(const int x0, const int y0,
		    const int  w, const int  h);
  void removeHCClipping();
  void UpdateOutput();
  bool startPSnewpage();
  void resetPage();
  bool endPSoutput();

protected:

  void clearCharsizes();
  void addCharsize(float chy);
  void setFontsize(float chy);
  void makeFontsizes(float wx, float wy, int vx, int vy);

  // fontpack and sizes
  static FontManager* fp;
  static vector<float> charsizes;
  static vector<float> fontsizes;
  static miString defaultFont;

  // PostScript module
  static GLPfile* psoutput;  // PostScript module
  static bool hardcopy;      // producing postscript

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

  static vector<VprofText> vptext; // info for text plotting

};

#endif

