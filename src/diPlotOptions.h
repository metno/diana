/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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
#include "util/diKeyValue.h"

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
  // extreme (min,max)
  static const std::string key_extremeType;
  static const std::string key_extremeSize;
  static const std::string key_extremeRadius;
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
  // fontname
  static const std::string key_fontname;
  // fontface
  static const std::string key_fontface;
  // fontsize
  static const std::string key_fontsize;
  // value precision
  static const std::string key_precision;
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
  bool table;
  int alpha;
  bool repeat;
  Linetype linetype;
  Linetype linetype_2;
  int linewidth;
  int linewidth_2;
  std::vector<std::string> patterns;
  std::vector<float> limits;
  std::vector<float> linevalues_;
  std::vector<float> loglinevalues_;
  std::vector<float> linevalues_2_;
  std::vector<float> loglinevalues_2_;
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
  int      lineSmooth;
  int      fieldSmooth;
  int      frame;  // 0=off 1=on; +2 for filling frame with background colour
  bool     zeroLine;
  bool     valueLabel;
  float    labelSize;
  bool     gridValue;
  int      gridLines;     // 0=no gridlines otherwise step
  int      gridLinesMax;  // 0=no limit otherwise skip if more than max
  int      undefMasking;
  Colour   undefColour;
  int      undefLinewidth;
  Linetype undefLinetype;
  std::string plottype;
  bool     discontinuous;
  bool     contourShading;
  std::string classSpecifications; // "value:name,value:name,....." split when used
  polyStyle polystyle;
  arrowStyle arrowstyle;
  Alignment h_align;
  Alignment v_align;
  std::string  fontname;
  diutil::FontFace fontface;
  float     fontsize;
  float     precision;
  bool      enabled;
  std::string  fname;
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

  static std::vector<std::vector<std::string>> plottypes;
  static std::map<std::string, unsigned int> enabledOptions; // enabledoptions[plotmethod]="OR of EnabledOptions values"

  PlotOptions();
  bool operator==(const PlotOptions& o) const;
  bool operator!=(const PlotOptions& o) const { return !(*this == o); }

  /*! Produce difference between from and to.
   * Passing the returned key-value pairs, to 'from.apply()', makes 'from' equal to 'to'.
   */
  static miutil::KeyValue_v diff(const PlotOptions& from, const PlotOptions& to);
  miutil::KeyValue_v diffFrom(const PlotOptions& from) const { return diff(from, *this); }
  miutil::KeyValue_v diffTo(const PlotOptions& to) const { return diff(*this, to); }

  miutil::KeyValue_v toKeyValueList() const;
  miutil::KeyValue_v toKeyValueListForAnnotation() const;

  /** parse a string (possibly) containing plotting options,
      and fill a PlotOptions with appropriate values */
  static bool parsePlotOption(const miutil::KeyValue_v&, PlotOptions&, miutil::KeyValue_v& unrecognized);
  static bool parsePlotOption(const miutil::KeyValue_v&, PlotOptions&);
  static bool parsePlotOption(const miutil::KeyValue& kv, PlotOptions& po);

  PlotOptions& apply(const miutil::KeyValue_v& v) { parsePlotOption(v, *this); return *this; }
  PlotOptions& apply(const miutil::KeyValue&   v) { parsePlotOption(v, *this); return *this; }

  PlotOptions& apply(const miutil::KeyValue_v& v, miutil::KeyValue_v& u) { parsePlotOption(v, *this, u); return *this; }

  PlotOptions& set_colour(const std::string& name);
  PlotOptions& set_colour_2(const std::string& name);
  PlotOptions& set_colours(const std::string& values);
  PlotOptions& set_palettecolours(const std::string& value);
  PlotOptions& set_patterns(const std::string& value);

  PlotOptions& set_lineinterval(float value);
  PlotOptions& set_lineinterval(const std::string& value);
  PlotOptions& set_lineinterval_2(float value);
  PlotOptions& set_lineinterval_2(const std::string& value);

  PlotOptions& set_linevalues(const std::string& values);
  PlotOptions& set_linevalues_2(const std::string& values);
  PlotOptions& set_loglinevalues(const std::string& values);
  PlotOptions& set_loglinevalues_2(const std::string& values);

  PlotOptions& set_linetype(const std::string& value);
  PlotOptions& set_linetype_2(const std::string& value);
  PlotOptions& set_linewidth(const std::string& value);
  PlotOptions& set_linewidth_2(const std::string& value);
  PlotOptions& set_density(const std::string& value);
  PlotOptions& set_minvalue(const std::string& value);
  PlotOptions& set_maxvalue(const std::string& value);
  PlotOptions& set_minvalue_2(const std::string& value);
  PlotOptions& set_maxvalue_2(const std::string& value);

  //! True if lineinterval > 0.
  bool use_lineinterval() const;

  //! True if lineinterval_2 > 0.
  bool use_lineinterval_2() const;

  //! True if linevalues is not empty and both use_lineinterval() and use_loglinevalues() are false.
  bool use_linevalues() const;

  //! True if linevalues_2 is not empty and both use_lineinterval_2() and use_loglinevalues_2() are false.
  bool use_linevalues_2() const;

  //! True if loglinevalues is not empty and use_lineinterval() is false.
  bool use_loglinevalues() const;

  //! True if loglinevalues_2 is not empty and use_lineinterval_2() is false.
  bool use_loglinevalues_2() const;

  const std::vector<float>& linevalues() const { return linevalues_; }
  const std::vector<float>& loglinevalues() const { return loglinevalues_; }
  const std::vector<float>& linevalues_2() const { return linevalues_2_; }
  const std::vector<float>& loglinevalues_2() const { return loglinevalues_2_; }

  static const std::string& defaultFontName();
  static diutil::FontFace defaultFontFace();
  static float defaultFontSize();

public:
  //! fill in values and "..." in a float vector (error if size==0)
  static std::vector<float> autoExpandFloatVector(const std::string&);

  // fill in values in an int vector (error if size==0)
  static std::vector<int> intVector(const std::string&);

  // fill in values in a float vector (error if size==0)
  static std::vector<float> floatVector(const std::string&);
};

#endif
