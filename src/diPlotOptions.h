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

#include <puTools/miString.h>

#include <vector>

using namespace std;

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
const miutil::miString
  fpt_contour         = "contour",
  fpt_value          = "value",
  fpt_symbol          = "symbol",
  fpt_alpha_shade     = "alpha_shade",
  fpt_alarm_box       = "alarm_box",
  fpt_fill_cell       = "fill_cell",
  fpt_wind            = "wind",
  fpt_wind_temp_fl    = "wind_temp_fl",
  fpt_wind_value     = "wind_value",
  fpt_vector          = "vector",
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
  miutil::miString palettename;
  miutil::miString patternname;
  int table;
  int alpha;
  int repeat;
  Linetype linetype;
  Linetype linetype_2;
  vector<Linetype> linetypes;
  int linewidth;
  int linewidth_2;
  vector<int> linewidths;
  vector<miutil::miString> patterns;
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
  miutil::miString vectorunitname;
  miutil::miString extremeType;
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
  miutil::miString plottype;
  int      rotateVectors;
  int      discontinuous;
  int      contourShading;
  miutil::miString classSpecifications; // "value:name,value:name,....." split when used
  polyStyle polystyle;
  Alignment h_align;
  Alignment v_align;
  int      alignX;       // shift position of plotted numbers with ex.alignX=10000
  int      alignY;       // shift position of plotted numbers with ex.alignY=10000
  miutil::miString  fontname;
  miutil::miString  fontface;
  float     fontsize;
  float     precision;
  int       dimension;
  bool      enabled;
  miutil::miString  fname;
  vector <miutil::miString> fdescr;
  int       overlay; //plot in ovelay buffer
  static map<miutil::miString,PlotOptions> fieldPlotOptions;
  static vector<miutil::miString> suffix;
  static vector< vector<miutil::miString> > plottypes;
  static map<miutil::miString, miutil::miString> enabledOptions; //enabledoptions[plotmethod]="list of option groups"
  bool      contourShape;
  miutil::miString  shapefilename;
  miutil::miString unit;
  bool      tableHeader; // whether each table is drawn with a header

  // Constructor
  PlotOptions();

  miutil::miString toString();
  /** parse a string (possibly) containing plotting options,
      and fill a PlotOptions with appropriate values */
  static bool parsePlotOption(const miutil::miString&, PlotOptions&);
  /// update static fieldplotoptions
  static bool updateFieldPlotOptions(const miutil::miString& name, const miutil::miString& optstr);
  /** fill a fieldplotoption from static map, and substitute values
      from a string containing plotoptions */
  static bool fillFieldPlotOptions(miutil::miString, const miutil::miString&,
			    PlotOptions&);
  static void setSuffix(vector<miutil::miString> suff){suffix = suff;}
  static void getAllFieldOptions(vector<miutil::miString>,
				 map<miutil::miString,miutil::miString>& fieldoptions);
  static bool getFieldPlotOptions(const miutil::miString& name, PlotOptions& po);
  static vector< vector<miutil::miString> >& getPlotTypes(){return plottypes;}
  static map< miutil::miString, miutil::miString > getEnabledOptions(){ return enabledOptions;}

private:
  // fill in values in an int vector (error if size==0)
  vector<int> intVector(const miutil::miString&) const;
  // fill in values in a float vector (error if size==0)
  vector<float> floatVector(const miutil::miString&) const;
  // fill in values and "..." in a float vector (error if size==0)
  vector<float> autoExpandFloatVector(const miutil::miString&) const;
  static void removeSuffix(miutil::miString& name);

};



#endif
