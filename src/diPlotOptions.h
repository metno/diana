/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diPlotOptions.h 3893 2012-07-05 12:09:33Z lisbethb $

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
#ifndef diPlotOptions_h
#define diPlotOptions_h

#include "diColour.h"
#include "diLinetype.h"

#include <vector>

const Colour WhiteC(255,255,255);
const Colour BlackC(0,0,0);

/**
   \brief ways of drawing polygons
*/
enum polyStyle {
  poly_fill,
  poly_border,
  poly_both,
  poly_none
};

/**
   \brief ways of drawing arrows
*/
enum arrowStyle {
  arrow_wind,
  arrow_wind_arrow,
  arrow_wind_value,
  arrow_wind_colour,
  arrow_vector_colour,
  arrow_vector
};

/**
   \brief alignment types
*/
enum Alignment {
  align_left,
  align_right,
  align_top,
  align_bottom,
  align_center
};


/**
   \brief string constants, field plot types
*/
const std::string
  fpt_contour         = "contour",
  fpt_value           = "value",
  fpt_symbol          = "symbol",
  fpt_alpha_shade     = "alpha_shade",
  fpt_alarm_box       = "alarm_box",
  fpt_fill_cell       = "fill_cell",
  fpt_wind            = "wind",
  fpt_wind_temp_fl    = "wind_temp_fl",
  fpt_wind_value      = "wind_value",
  fpt_vector          = "vector",
  fpt_frame           = "frame",
  fpt_direction       = "direction";

/**
   \brief Options for one data plot
*/
class PlotOptions {

public:
  bool options_1;
  bool options_2;
  Colour textcolour,linecolour,linecolour_2,fillcolour,bordercolour;
  vector<Colour> colours;
  vector<Colour> palettecolours;
  vector<Colour> palettecolours_cold;
  std::string palettename;
  std::string patternname;
  int table;
  int alpha;
  int repeat;
  Linetype linetype;
  Linetype linetype_2;
  vector<Linetype> linetypes;
  int linewidth;
  int linewidth_2;
  vector<int> linewidths;
  vector<std::string> patterns;
  vector<float> limits;
  vector<float> values;
  vector<float> linevalues;
  vector<float> loglinevalues;
  vector<float> linevalues_2;
  vector<float> loglinevalues_2;
  int colourcut;
  vector<int>   forecastLength;
  vector<float> forecastValueMin;
  vector<float> forecastValueMax;
  float lineinterval;
  float lineinterval_2;
  float base;
  float base_2;
  float minvalue;
  float minvalue_2;
  float maxvalue;
  float maxvalue_2;
  int   density;
  float densityFactor;
  float vectorunit;
  std::string vectorunitname;
  std::string extremeType;
  float    extremeSize;
  float    extremeRadius;
  vector<float> extremeLimits;
  int      lineSmooth;
  int      fieldSmooth;
  int      frame;  // 0=off 1=on
  int      zeroLine;      // -1=no_option 0=off 1=on
  int      valueLabel;    //              0=off 1=on
  float    labelSize;
  int      gridValue;     // -1=no_option 0=off 1=on
  int      gridLines;     // 0=no gridlines otherwise step
  int      gridLinesMax;  // 0=no limit otherwise skip if more than max
  int      undefMasking;
  Colour   undefColour;
  int      undefLinewidth;
  Linetype undefLinetype;
  std::string plottype;
  int      rotateVectors;
  int      discontinuous;
  int      contourShading;
  std::string classSpecifications; // "value:name,value:name,....." split when used
  polyStyle polystyle;
  arrowStyle arrowstyle;
  Alignment h_align;
  Alignment v_align;
  int      alignX;       // shift position of plotted numbers with ex.alignX=10000
  int      alignY;       // shift position of plotted numbers with ex.alignY=10000
  std::string  fontname;
  std::string  fontface;
  float     fontsize;
  float     precision;
  int       dimension;
  bool      enabled;
  std::string  fname;
  vector <std::string> fdescr;
  int       overlay; //plot in ovelay buffer
  static map<std::string,PlotOptions> fieldPlotOptions;
  static vector<std::string> suffix;
  static vector< vector<std::string> > plottypes;
  static map<std::string, std::string> enabledOptions; //enabledoptions[plotmethod]="list of option groups"
  bool      contourShape;
  std::string  shapefilename;
  std::string unit;        // used to get data in right unit
  std::string legendunits; //used in legends
  bool      tableHeader; // whether each table is drawn with a header
  bool      antialiasing;
  bool      use_stencil;    // whether a stencil is used to mask out plotting of the current field
  bool      update_stencil; // whether a stencil is updated with the plot area of the current field
  bool      plot_under;     // plot field together with shade plots

  // Constructor
  PlotOptions();

  std::string toString();
  /** parse a string (possibly) containing plotting options,
      and fill a PlotOptions with appropriate values */
  static bool parsePlotOption(std::string&, PlotOptions&, bool returnMergedOptionString=false);
  /// update static fieldplotoptions
  static bool updateFieldPlotOptions(const std::string& name, const std::string& optstr);
  /** fill a fieldplotoption from static map, and substitute values
      from a string containing plotoptions */
  static bool fillFieldPlotOptions(std::string, std::string&,
			    PlotOptions&);
  static void setSuffix(vector<std::string> suff){suffix = suff;}
  static void getAllFieldOptions(vector<std::string>,
				 map<std::string,std::string>& fieldoptions);
  static bool getFieldPlotOptions(const std::string& name, PlotOptions& po);
  static vector< vector<std::string> >& getPlotTypes(){return plottypes;}
  static map< std::string, std::string > getEnabledOptions(){ return enabledOptions;}

private:
  // fill in values in an int vector (error if size==0)
  vector<int> intVector(const std::string&) const;
  // fill in values in a float vector (error if size==0)
  vector<float> floatVector(const std::string&) const;
  // fill in values and "..." in a float vector (error if size==0)
  vector<float> autoExpandFloatVector(const std::string&) const;
  static void removeSuffix(std::string& name);
};

#endif
