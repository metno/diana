/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2020 met.no

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

class DiGLPainter;
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

  std::string suffix;
  PlotOptions poptions;

  LegendPlot(const LegendPlot& rhs) = delete;
  LegendPlot& operator=(const LegendPlot& rhs) = delete;

  void calculateSizes(DiGLPainter* gl, float& xborder, float& yborder, float& tablewidth, float& titlewidth, float& maxheight,
                      std::vector<std::string>& vtitlestring);

public:
  /// Constructor which reads string to get title and make vector of ColourCode
  LegendPlot(const std::string& str);

  ~LegendPlot();

  /// plots the legend plot with top left corner at x,y
  bool plotLegend(DiGLPainter* gl, float x=0.0, float y=0.0);

  void setPlotOptions(const PlotOptions& po)
    { poptions = po; }

  void setTitle(const std::string& t) { titlestring = t; }

  void setBackgroundColour(const Colour& c) { poptions.fillcolour = c; }

  ///sets suffix, usually unit (hPa, mm)
  void setSuffix(const std::string& suffix_){suffix = suffix_;}

  /// calculates and returns total table height
  float height(DiGLPainter* gl);

  /// calculates and returns total table width
  float width(DiGLPainter* gl);
};

#endif // diLegendPlot_h
