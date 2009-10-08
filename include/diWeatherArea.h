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
#ifndef WeatherArea_h
#define WeatherArea_h

#include <diObjectPlot.h>
#include <puTools/miString.h>

using namespace std;

/**

  \brief A weather area that can be plotted and edited


*/
class WeatherArea: public ObjectPlot
{
private:
  float linewidth;                // width of border (PlotOptions?)
  static float defaultLineWidth;
  bool fillArea;                  // fill area with pattern
  bool drawSig;                   // draw sig weather arcs.

  static vector<editToolInfo>  allAreas;
  static map<miString,int> areaTypes;  //finds area type number from name

  void recalculate();
  void drawSigweather();
  bool smooth();
  bool orientationClockwise();

  float * xplot;
  float * yplot;
  int npoints;
  bool first; //sigweather drawing
  void setPlotVariables();
  float getLineWidth(){return linewidth;}
  virtual void setLineWidth(float w){linewidth=w;}
  bool setSpline(bool s);     // s=true if you want to spline
  bool setFillArea(bool f);   // f=true if you want to fill Area

public:
  /// default constructor
  WeatherArea();
  /// constructor with integer area type as argumet
  WeatherArea(int ty);
  /// constructor with name of area type as argumet
  WeatherArea(miString tystring);
  /// Destructor
  ~WeatherArea();
  /// define map to find area type number from name
  static void defineAreas(vector<editToolInfo> areas);
  /// set default line width from setup
  static void setDefaultLineWidth(float w){defaultLineWidth=w;}
  /// set state to active or passive <br> also set whether to do spline interpolation and whether area should be filled
  void setState(const state s);
  bool plot();  ///< draws the area
  bool plot(const int){return false;}
  /// shows / marks node point/ points on area
  bool showLine(float x, float y);
  /// set type of area
  void setType(int ty);
  /// set type of area
  bool setType(miString tystring);
  /// turn area
  void flip();
  /// returns true if point x,y on area line
  virtual bool isOnObject(float x, float y) {return showLine(x,y);}
  /// writes a string with Object and Type
  miString writeTypeString();
  /// selects or unselects area
  virtual void setSelected(bool s);
  /// returns true if x,y inside area
  virtual bool isInsideArea(float x, float y);
};

#endif






