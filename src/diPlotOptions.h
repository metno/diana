/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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
#include "diFont.h"
#include "diLinetype.h"
#include "miSetupParser.h"

#include <vector>

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
extern const std::string fpt_contour, fpt_contour1, fpt_contour2,
  fpt_value, fpt_symbol, fpt_alpha_shade, fpt_rgb, fpt_alarm_box,
  fpt_fill_cell, fpt_wind, fpt_wind_temp_fl, fpt_wind_value,
  fpt_vector, fpt_frame, fpt_direction;

/**
   \brief Options for one data plot
*/
class PlotOptions {

public:
  // options1: off,isoline
  static const std::string key_options_1;
  // options1: off,isoline,shading
  static const std::string key_options_2;
  // colour:     main colour
  static const std::string key_colour;
  // colour:     main colour
  static const std::string key_colour_2;
  // tcolour:    text colour
  static const std::string key_tcolour;
  // lcolour:    line colour
  static const std::string key_lcolour;
  // lcolour:    line colour
  static const std::string key_lcolour_2;
  // fcolour:    fill colour
  static const std::string key_fcolour;
  // bcolour:    pattern colour
  static const std::string key_pcolour;
  // bcolour:    border colour
  static const std::string key_bcolour;
  // colours:    list of colours
  static const std::string key_colours;
  // colours:    list of colours in palette
  static const std::string key_palettecolours;
  // linetype:   linetype
  static const std::string key_linetype;
  // linetype:   linetype
  static const std::string key_linetype_2;
  // linetypes:  list of linetypes
  static const std::string key_linetypes;
  // linewidth:  linewidth
  static const std::string key_linewidth;
  // linewidth:  linewidth
  static const std::string key_linewidth_2;
  // linewidths: list of linewidths
  static const std::string key_linewidths;
  // patterns:  list of patterns
  static const std::string key_patterns;
  // line.interval:
  static const std::string key_lineinterval;
  // line.interval:
  static const std::string key_lineinterval_2;
  // value.range: minValue
  static const std::string key_minvalue;
  // value.range: maxValue
  static const std::string key_maxvalue;
  // value.range: minValue
  static const std::string key_minvalue_2;
  // value.range: maxValue
  static const std::string key_maxvalue_2;
  // colourcut 0=off 1=on
  static const std::string key_colourcut;
  // line.values
  static const std::string key_linevalues;
  // logarithmic line.values
  static const std::string key_loglinevalues;
  // limits:
  static const std::string key_linevalues_2;
  // logarithmic line.values
  static const std::string key_loglinevalues_2;
  // limits:
  static const std::string key_limits;
  // values:
  static const std::string key_values;
  // extreme (min,max)
  static const std::string key_extremeType;
  static const std::string key_extremeSize;
  static const std::string key_extremeRadius;
  static const std::string key_extremeLimits;
  // contour line smoothing
  static const std::string key_lineSmooth;
  // field smoothing
  static const std::string key_fieldSmooth;
  // plot frame around complete field area  ( 0=off 1=on)
  static const std::string key_frame;
  // zero line drawing (-1=no_option 0=off 1=on)
  static const std::string key_zeroLine;
  // labels on isolines (0=off 1=on)
  static const std::string key_valueLabel;
  // rel. label size
  static const std::string key_labelSize;
  // show grid values (-1=no_option 0=off 1=on)
  static const std::string key_gridValue;
  // show grid lines (0=off N=density)
  static const std::string key_gridLines;
  // show max grid lines (0=no limit N=maximum, skip if more)
  static const std::string key_gridLinesMax;
  // field plottype:
  static const std::string key_plottype;
  // discontinuous field
  static const std::string key_rotatevectors;
  // discontinuous field
  static const std::string key_discontinuous;
  // table
  static const std::string key_table;
  // alpha shading
  static const std::string key_alpha;
  // repeat palette
  static const std::string key_repeat;
  // class specifications
  static const std::string key_classes;
  // base value
  static const std::string key_basevalue;
  // base value
  static const std::string key_basevalue_2;
  // (vector) density
  static const std::string key_density;
  // (vector) density - auto*factor
  static const std::string key_densityfactor;
  // vector unit
  static const std::string key_vectorunit;
  // vector unit name
  static const std::string key_vectorunitname;
  static const std::string key_vectorscale_x; ///< scaling factor for x component when drawing the vector arrow
  static const std::string key_vectorscale_y; ///< scaling factor for y component when drawing the vector arrow
  static const std::string key_vectorthickness; ///< thickness of vector arrow; 0 is a thin arrow (not scaled with length)
  // forecast length
  static const std::string key_forecastLength;
  // forecast value min
  static const std::string key_forecastValueMin;
  // forecast value max
  static const std::string key_forecastValueMax;
  // undefMasking and options
  static const std::string key_undefMasking;
  static const std::string key_undefColour;
  static const std::string key_undefLinewidth;
  static const std::string key_undefLinetype;
  // polyStyle
  static const std::string key_polystyle;
  // arrowStyle
  static const std::string key_arrowstyle;
  // h_alignment
  static const std::string key_h_alignment;
  // v_alignment
  static const std::string key_v_alignment;
  // alignment for plotted numbers
  static const std::string key_alignX;
  // alignment for plotted numbers
  static const std::string key_alignY;
  // fontname
  static const std::string key_fontname;
  // fontface
  static const std::string key_fontface;
  // fontsize
  static const std::string key_fontsize;
  // value precision
  static const std::string key_precision;
  // dinesion (scalar=1, vector=2)
  static const std::string key_dimension;
  // plot enabled
  static const std::string key_enabled;
  //field names used for plotting
  static const std::string key_fname;
  //legend units
  static const std::string key_legendunits;
  //legend title
  static const std::string key_legendtitle;
  //anti-aliasing
  static const std::string key_antialiasing;
  //use_stencil
  static const std::string key_use_stencil;
  //update_stencil
  static const std::string key_update_stencil;
  static const std::string key_plot_under;
  //only plot if gcd less than maxDiagonalInMeters
  static const std::string key_maxDiagonalInMeters;

  static const std::string key_vector_example_x;
  static const std::string key_vector_example_y;
  static const std::string key_vector_example_unit_x;
  static const std::string key_vector_example_unit_y;

public:
  bool options_1;
  bool options_2;
  Colour textcolour,linecolour,linecolour_2,fillcolour,bordercolour;
  std::vector<Colour> colours;
  std::vector<Colour> palettecolours;
  std::vector<Colour> palettecolours_cold;
  std::string palettename;
  std::string patternname;
  int table;
  int alpha;
  int repeat;
  Linetype linetype;
  Linetype linetype_2;
  std::vector<Linetype> linetypes;
  int linewidth;
  int linewidth_2;
  std::vector<int> linewidths;
  std::vector<std::string> patterns;
  std::vector<float> limits;
  std::vector<float> values;
  std::vector<float> linevalues;
  std::vector<float> loglinevalues;
  std::vector<float> linevalues_2;
  std::vector<float> loglinevalues_2;
  float colourcut;
  std::vector<int>   forecastLength;
  std::vector<float> forecastValueMin;
  std::vector<float> forecastValueMax;
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
  float vectorscale_x; ///< scaling factor for x component when drawing the vector arrow
  float vectorscale_y; ///< scaling factor for y component when drawing the vector arrow
  float vectorthickness; ///< thickness of vector arrow; 0 is a thin arrow (not scaled with length)
  std::string extremeType;
  float    extremeSize;
  float    extremeRadius;
  std::vector<float> extremeLimits;
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
  diutil::FontFace fontface;
  float     fontsize;
  float     precision;
  int       dimension;
  bool      enabled;
  std::string  fname;
  static std::vector< std::vector<std::string> > plottypes;
  static std::map<std::string, unsigned int> enabledOptions; //enabledoptions[plotmethod]="OR of EnabledOptions values"
  std::string legendunits; //used in legends
  std::string legendtitle; //used in legends
  bool      tableHeader; // whether each table is drawn with a header
  bool      antialiasing;
  bool      use_stencil;    // whether a stencil is used to mask out plotting of the current field
  bool      update_stencil; // whether a stencil is updated with the plot area of the current field
  bool      plot_under;     // plot field together with shade plots
  float     maxDiagonalInMeters;

  float vector_example_x; // example vector x-position, for vcross, in relative coordinates
  float vector_example_y; // example vector y-position, for vcross, in relative coordinates
  std::string vector_example_unit_x; // unit for x-component of sample vector
  std::string vector_example_unit_y; // unit for y-component of sample vector

  PlotOptions();

  miutil::KeyValue_v toKeyValueList();
  miutil::KeyValue_v toKeyValueListForAnnotation() const;

  /** parse a string (possibly) containing plotting options,
      and fill a PlotOptions with appropriate values */
  static bool parsePlotOption(const miutil::KeyValue_v&, PlotOptions&, miutil::KeyValue_v& unusedOptions);
  static bool parsePlotOption(const miutil::KeyValue_v&, PlotOptions&);
  static bool parsePlotOption(const miutil::KeyValue& kv, PlotOptions& po);

  static const std::vector< std::vector<std::string> >& getPlotTypes();
  enum EnabledOptions {
    POE_CONTOUR = 1<<0,
    POE_EXTREME = 1<<1,
    POE_SHADING = 1<<2,
    POE_LINE    = 1<<3,
    POE_FONT    = 1<<4,
    POE_DENSITY = 1<<5,
    POE_UNIT    = 1<<6
  };
  static const std::map<std::string, unsigned int>& getEnabledOptions();

  static const std::string& defaultFontName();
  static diutil::FontFace defaultFontFace();
  static float defaultFontSize();

public:
  //! fill in values and "..." in a float vector (error if size==0)
  static std::vector<float> autoExpandFloatVector(const std::string&);

private:
  // fill in values in an int vector (error if size==0)
  std::vector<int> intVector(const std::string&) const;
  // fill in values in a float vector (error if size==0)
  std::vector<float> floatVector(const std::string&) const;
};

#endif
