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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diVprofTables.h"
#include "diPainter.h"
#include <cmath>

#define MILOGGER_CATEGORY "diana.VprofTables"
#include <miLogger/miLogging.h>

using std::map;
using std::string;
using std::vector;

// static members

vector<float> VprofTables::charsizes;
vector<float> VprofTables::fontsizes;
std::string VprofTables::defaultFont="BITMAPFONT";

float VprofTables::chxbas= 0.;   // character size
float VprofTables::chybas= 0.;
float VprofTables::chxlab= 0.;   // labels etc. character size
float VprofTables::chylab= 0.;
float VprofTables::chxtxt= 0.;   // text (name etc.) character size
float VprofTables::chytxt= 0.;

float VprofTables::xysize[mxysize][4];

int   VprofTables::iptab[mptab]= { 0  }; // pressure in unit hPa
float VprofTables::pptab[mptab]= { 0. }; // pressure in unit hPa
float VprofTables::pitab[mptab]= { 0. }; // exner function (pi)
float VprofTables::pplog[mptab]= { 0. }; // ln(pressure)
float VprofTables::yptabd[mptab]= { 0. };// y
float VprofTables::xztabd[mptab]= { 0. };// x(t.celsius=0)
float VprofTables::yptab[mptab]= { 0. }; // y
float VprofTables::xztab[mptab]= { 0. }; // x(t.celsius=0)
float VprofTables::dx1degree= 0.;    // x distance for 1 degree celsius

vector<VprofText> VprofTables::vptext; // info for text plotting


VprofTables::VprofTables()
{
}


VprofTables::~VprofTables()
{
}


void VprofTables::clearCharsizes()
{
  charsizes.clear();
  fontsizes.clear();
}


void VprofTables::addCharsize(float chy)
{
  charsizes.push_back(chy);
}


void VprofTables::setFontsize(DiPainter* gl, float chy)
{
  chy*=0.8;
  float d, dnear= 1.e+35;
  int n= charsizes.size();
  int near= -1;
  for (int i=0; i<n; i++) {
    d= fabsf(charsizes[i] - chy);
    if (d<dnear) {
      dnear= d;
      near= i;
    }
  }
  if (near>=0)
    gl->setFontSize(fontsizes[near]);
}


void VprofTables::makeFontsizes(DiPainter* gl, float wx, float wy, int vx, int vy)
{
  gl->setVpGlSize(vx, vy, wx, wy);

  float size, scale= float(vy)/wy;

  fontsizes.clear();

  int n= charsizes.size();

  for (int i=0; i<n; i++) {
    size= charsizes[i]*scale;
    fontsizes.push_back(size);
  }
}
