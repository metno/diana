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
#ifndef WeatherFront_h
#define WeatherFront_h

#include <diObjectPlot.h>

/**
  \brief A weather front that can be plotted and edited
*/
class WeatherFront: public ObjectPlot
{
private:

  float linewidth;
  static float defaultLineWidth;
  float scaledlinewidth;
  static std::vector<editToolInfo>  allFronts;
  static std::map<std::string,int> frontTypes;  //finds front type number from name
  int npoints;
  float * xplot;
  float * yplot;
  bool first;

  void drawColds();         // draws the blue triangles
  void drawWarms();         // draws the red arc
  void drawStationary();    // draws the red arc
  void drawOccluded();      // draws the occluded
  void drawSquallLine();    // draws the squall line crosses
  void drawSigweather();    // draws significant weather bubbles
  void drawArrowLine();          // draws line with arrow
  void drawTroughLine();        // draws smhi-used troughline
  bool smooth();
  void recalculate();
  float getLineWidth(){return linewidth;}
  virtual void setLineWidth(float w){linewidth=w;}
  int hitPoint(float x,float y);
  bool setSpline(bool s);     // s=true if you want to spline

 public:
  /// constructor
  WeatherFront();
  /// constructor with integer front type as argument
  WeatherFront(int ty);
  /// constructor with name of front type as argument
  WeatherFront(std::string tystring);
  /// copy constructor
  WeatherFront(const WeatherFront &rhs);
  /// Destructor
  ~WeatherFront();
  /// define map to find front type number from name
  static void defineFronts(std::vector<editToolInfo> fronts);
  /// set default line width from setup
  static void setDefaultLineWidth(float w){defaultLineWidth=w;}
  /// set state to active or passive <br> also set whether to do spline interpolation
  void setState(const state s);

  void plot(PlotOrder zorder);

 ///< shows / marks node point/ points on front
  bool showLine(float x, float y);
  /// set front type
  void setType(int ty);
  /// set front type
  bool setType(std::string tystring);
  /// turn front
  void flip();
  /// add qfront to front (the two fronts are merged)
  bool addFront(ObjectPlot * qfront);
  ///splits front in two at x,y <br> pointer to new front is returned
  ObjectPlot* splitFront(float x,float y);
  /// returns true if of x,y on object
  bool isOnObject(float x, float y) {return showLine(x,y);}
  /// writes a string with Object and Type
  std::string writeTypeString();
  /// returns x distance from point to line after onLine is called
  float getDistX(){return distX;}
  /// returns y distance from point to line after onLine is called
  float  getDistY(){return distY;}
};

#endif
