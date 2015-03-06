/*
  Diana - A Free Meteorological Visualisation Tool

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
#ifndef diLegendPlot_h
#define diLegendPlot_h

#include "diColour.h"
#include "diPlotOptions.h"

#include <string>
#include <vector>

class StaticPlot;

/**
  \brief Plots a legend with colour and text
*/

class LegendPlot /*: public Plot*/ {

public:
  /// Legend text with colour and pattern
  struct ColourCode{
    Colour colour;
    std::string pattern;
    std::string colourstr;
    bool plotBox; //if false, do not plot colour/pattern box
  };

private:
  std::string titlestring;
  std::vector<ColourCode> colourcodes;

  // x,y coordinates of the titlebar... if we click inside this area,
  //change value of showplot;
  float x1title;
  float x2title;
  float y1title;
  float y2title;
  float xRatio,yRatio;
  // if showplot = true: show the whole table. if false: only title bar
  bool showplot;
  std::string suffix;
  PlotOptions poptions;
  StaticPlot* staticPlot_;
  static float xUsed;

  void memberCopy(const LegendPlot& rhs);
  bool operator==(const LegendPlot &rhs) const;

  LegendPlot();
  LegendPlot(const LegendPlot &rhs);
  LegendPlot& operator=(const LegendPlot &rhs);

  /// get width and height of string str with current font
  void getStringSize(std::string str, float& width, float& height);

public:
  /// Constructor which reads string to get title and make vector of ColourCode
  LegendPlot(const std::string& str);
  ~LegendPlot();

  /// sets title and ColourCode vector
  void setData(const std::string& title, const std::vector<ColourCode>& colourcode);

  /// plots the legend plot with top left corner at x,y
  bool plotLegend(float x=0.0, float y=0.0);

  /// if x,y inside title bar, then the table should be hidden or shown
  void showSatTable(int x,int y);
  /// check if x,y inside title bar
  bool inSatTable(int x,int y);
  /// moves plot from x1,y1 to x2,y2
  void moveSatTable(int x1,int y1,int x2,int y2);

  void setPlotOptions(const PlotOptions& po)
    { poptions = po; }

  void setTitle(std::string t)
    { titlestring = t; }

  void setBackgroundColour(Colour c)
    { poptions.fillcolour = c; }

  void setStaticPlot(StaticPlot* sp)
    { staticPlot_ = sp; }

  ///sets suffix, usually unit (hPa, mm)
  void setSuffix(const std::string& suffix_){suffix = suffix_;}

  /// sets alignment
  void setAlignment(Alignment a){poptions.h_align=a;}

  /// calculates and returns total table height
  float height();
  /// calculates and returns total table width
  float width();
};

#endif // diLegendPlot_h
