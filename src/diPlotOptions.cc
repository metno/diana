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

#include "diPlotOptions.h"

#include "diColourShading.h"
#include "diField/diField.h"
#include "diPattern.h"
#include "util/string_util.h"

#include <mi_fieldcalc/math_util.h>
#include <puTools/miStringFunctions.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.PlotOptions"
#include <miLogger/miLogging.h>

namespace {

void throw_error(const miutil::KeyValue& kv, const std::string& problem)
{
  std::ostringstream msg;
  msg << "plotoption '" << kv.key() << "' has value '" << kv.value() << "' which " << problem;
  throw std::runtime_error(msg.str());
}

bool checkFloatVector(const std::vector<float>& aefv)
{
  if (aefv.empty())
    return false;

  for (size_t j=1; j<aefv.size(); j++) {
    if (aefv[j-1] >= aefv[j])
      return false;
  }

  return true;
}

std::vector<float>&& checkFloatVector(const miutil::KeyValue& kv, std::vector<float>&& aefv)
{
  if (!checkFloatVector(aefv))
    throw_error(kv, "does not describe a float vector");

  return std::move(aefv);
}

template <class D>
std::vector<D>&& checkNotEmpty(const miutil::KeyValue& kv, std::vector<D>&& v)
{
  if (v.empty())
    throw_error(kv, "is an empty vector");

  return std::move(v);
}

const std::string OFF = "off";

bool is_off(const std::string& value)
{
  return value == OFF || miutil::to_lower(value) == OFF;
}

bool operator!=(const Colour& a, const Colour& b)
{
  return a.Name() != b.Name();
}

bool operator!=(const Linetype& a, const Linetype& b)
{
  return a.name != b.name;
}

std::string as_string(const std::string& value)
{
  return value;
}

std::string as_string(bool value)
{
  return value ? "true" : "false";
}

std::string as_string(int value)
{
  return miutil::from_number(value);
}

std::string as_string(float value)
{
  return miutil::from_number(value);
}

std::string as_string(const Linetype& value)
{
  return value.name;
}

std::string as_string(const Colour& value)
{
  return value.Name();
}

std::string as_string(const diutil::FontFace& value)
{
  return diutil::fontFaceToString(value);
}

std::string as_string(const polyStyle& value)
{
  switch (value) {
  case poly_fill:
    return "fill";
  case poly_border:
    return "border";
  case poly_both:
    return "both";
  case poly_none:
    return "none";
  }
}

std::string as_string(const arrowStyle& value)
{
  switch (value) {
  case arrow_wind:
    return "wind";
  case arrow_wind_arrow:
    return "wind_arrow";
  case arrow_wind_value:
    return "wind_value";
  case arrow_wind_colour:
    return "wind_colour";
  case arrow_vector_colour:
    return "vector_colour";
  case arrow_vector:
    return "vector";
  }
}

std::string as_string(Alignment value)
{
  switch (value) {
  case align_left:
    return "left";
  case align_right:
    return "right";
  case align_center:
    return "center";
  case align_top:
    return "top";
  case align_bottom:
    return "bottom";
  }
}

template <class C>
std::string as_string(const std::vector<C>& value)
{
  std::ostringstream ov;
  auto it = value.begin();
  if (it != value.end()) {
    ov << as_string(*it++);
    while (it != value.end())
      ov << ',' << as_string(*it++);
  }
  return ov.str();
}

//! parse value as bool, throwing if this fails
bool to_bool(const miutil::KeyValue& kv)
{
  bool ok;
  const bool val = kv.toBool(ok, false);
  if (!ok)
    throw_error(kv, "cannot be parsed as bool");

  return val;
}

//! parse value as int, throwing if this fails
int to_int(const miutil::KeyValue& kv)
{
  if (!miutil::is_int(kv.value()))
    throw_error(kv, "cannot be parsed as int");

  return miutil::to_int(kv.value());
}

//! parse value as float, throwing if this fails
float to_float(const miutil::KeyValue& kv)
{
  if (!miutil::is_number(kv.value()))
    throw_error(kv, "cannot be parsed as float");

  return std::atof(kv.value().c_str());
}

//! parse value as float, using `off_value` if value == "off" or if parsing fails
float to_float_or_off(const miutil::KeyValue& kv, float off_value)
{
  if (is_off(kv.value()))
    return off_value;
  try {
    return to_float(kv);
  } catch (std::runtime_error& e) {
    return off_value;
  }
}

} // namespace

const std::string fpt_contour      = "contour";
const std::string fpt_contour1     = "contour1";
const std::string fpt_contour2     = "contour2";
const std::string fpt_value        = "value";
const std::string fpt_symbol       = "symbol";
const std::string fpt_alpha_shade  = "alpha_shade";
const std::string fpt_rgb          = "rgb";
const std::string fpt_alarm_box    = "alarm_box";
const std::string fpt_fill_cell    = "fill_cell";
const std::string fpt_wind         = "wind";
const std::string fpt_wind_temp_fl = "wind_temp_fl";
const std::string fpt_wind_value   = "wind_value";
const std::string fpt_vector       = "vector";
const std::string fpt_frame        = "frame";
const std::string fpt_direction    = "direction";
const std::string fpt_streamlines  = "streamlines";

const std::string PlotOptions::key_options_1 = "options.1";
const std::string PlotOptions::key_options_2 = "options.2";
const std::string PlotOptions::key_colour= "colour";
const std::string PlotOptions::key_colour_2= "colour_2";
const std::string PlotOptions::key_tcolour= "tcolour";
const std::string PlotOptions::key_lcolour= "lcolour";
const std::string PlotOptions::key_lcolour_2= "lcolour_2";
const std::string PlotOptions::key_fcolour= "fcolour";
const std::string PlotOptions::key_pcolour= "patterncolour";
const std::string PlotOptions::key_bcolour= "bcolour";
const std::string PlotOptions::key_colours= "colours";
const std::string PlotOptions::key_palettecolours= "palettecolours";
const std::string PlotOptions::key_linetype= "linetype";
const std::string PlotOptions::key_linetype_2= "linetype_2";
const std::string PlotOptions::key_linetypes= "linetypes";
const std::string PlotOptions::key_linewidth= "linewidth";
const std::string PlotOptions::key_linewidth_2= "linewidth_2";
const std::string PlotOptions::key_linewidths= "linewidths";
const std::string PlotOptions::key_patterns= "patterns";
const std::string PlotOptions::key_lineinterval= "line.interval";
const std::string PlotOptions::key_lineinterval_2= "line.interval_2";
const std::string PlotOptions::key_minvalue= "minvalue";
const std::string PlotOptions::key_maxvalue= "maxvalue";
const std::string PlotOptions::key_minvalue_2= "minvalue_2";
const std::string PlotOptions::key_maxvalue_2= "maxvalue_2";
const std::string PlotOptions::key_colourcut= "colourcut";
const std::string PlotOptions::key_linevalues= "line.values";
const std::string PlotOptions::key_loglinevalues= "log.line.values";
const std::string PlotOptions::key_linevalues_2= "line.values_2";
const std::string PlotOptions::key_loglinevalues_2= "log.line.values_2";
const std::string PlotOptions::key_limits= "limits";
const std::string /*PlotOptions::*/key_values= "values";
const std::string PlotOptions::key_extremeType= "extreme.type";
const std::string PlotOptions::key_extremeSize= "extreme.size";
const std::string PlotOptions::key_extremeRadius= "extreme.radius";
const std::string /*PlotOptions*/key_extremeLimits= "extreme.limits";
const std::string PlotOptions::key_lineSmooth= "line.smooth";
const std::string PlotOptions::key_fieldSmooth= "field.smooth";
const std::string PlotOptions::key_frame= "frame";
const std::string PlotOptions::key_zeroLine= "zero.line";
const std::string PlotOptions::key_valueLabel= "value.label";
const std::string PlotOptions::key_labelSize= "label.size";
const std::string PlotOptions::key_gridValue= "grid.value";
const std::string PlotOptions::key_gridLines= "grid.lines";
const std::string PlotOptions::key_gridLinesMax= "grid.lines.max";
const std::string PlotOptions::key_plottype= "plottype";
const std::string /*PlotOptions::*/key_rotatevectors= "rotate.vectors";
const std::string PlotOptions::key_discontinuous= "discontinuous";
const std::string PlotOptions::key_table= "table";
const std::string PlotOptions::key_alpha= "alpha";
const std::string PlotOptions::key_repeat= "repeat";
const std::string PlotOptions::key_classes= "classes";
const std::string PlotOptions::key_basevalue= "base";
const std::string PlotOptions::key_basevalue_2= "base_2";
const std::string PlotOptions::key_density= "density";
const std::string PlotOptions::key_densityfactor= "density.factor";
const std::string PlotOptions::key_vectorunit= "vector.unit";
const std::string PlotOptions::key_vectorunitname= "vector.unit.name";
const std::string PlotOptions::key_vectorscale_x= "vector.scale.x";
const std::string PlotOptions::key_vectorscale_y= "vector.scale.y";
const std::string PlotOptions::key_vectorthickness= "vector.thickness";
const std::string PlotOptions::key_forecastLength= "forecast.length";
const std::string PlotOptions::key_forecastValueMin= "forecast.value.min";
const std::string PlotOptions::key_forecastValueMax= "forecast.value.max";
const std::string PlotOptions::key_undefMasking=   "undef.masking";
const std::string PlotOptions::key_undefColour=    "undef.colour";
const std::string PlotOptions::key_undefLinewidth= "undef.linewidth";
const std::string PlotOptions::key_undefLinetype=  "undef.linetype";
const std::string PlotOptions::key_polystyle= "polystyle";
const std::string PlotOptions::key_arrowstyle= "arrowstyle";
const std::string PlotOptions::key_h_alignment= "halign";
const std::string PlotOptions::key_v_alignment= "valign";
const std::string /*PlotOptions::*/key_alignX= "alignX";
const std::string /*PlotOptions::*/key_alignY= "alignY";
const std::string PlotOptions::key_fontname= "font";
const std::string PlotOptions::key_fontface= "face";
const std::string PlotOptions::key_fontsize= "fontsize";
const std::string PlotOptions::key_precision= "precision";
const std::string /*PlotOptions::*/key_dimension= "dim";
const std::string PlotOptions::key_enabled= "enabled";
const std::string PlotOptions::key_fname= "fname";
const std::string PlotOptions::key_legendunits="legendunits";
const std::string PlotOptions::key_legendtitle="legendtitle";
const std::string PlotOptions::key_antialiasing="antialiasing";
const std::string PlotOptions::key_use_stencil="use_stencil";
const std::string PlotOptions::key_update_stencil="update_stencil";
const std::string PlotOptions::key_plot_under="plot_under";
const std::string PlotOptions::key_maxDiagonalInMeters="maxdiagonalinmeters";
const std::string PlotOptions::key_vector_example_x = "vector.example.x";
const std::string PlotOptions::key_vector_example_y = "vector.example.y";
const std::string PlotOptions::key_vector_example_unit_x = "vector.example.unit.x";
const std::string PlotOptions::key_vector_example_unit_y = "vector.example.unit.y";

std::vector<std::vector<std::string>> PlotOptions::plottypes;
std::map< std::string, unsigned int> PlotOptions::enabledOptions;

static const std::string key_contourShape = "contourShape";
static const std::string key_shapefilename = "shapefilename";

PlotOptions::PlotOptions()
    : options_1(true)
    , options_2(false)
    , textcolour(Colour::BLACK)
    , linecolour(Colour::BLACK)
    , linecolour_2(Colour::BLACK)
    , fillcolour(Colour::BLACK)
    , bordercolour(Colour::BLACK)
    , colours({linecolour})
    , table(true)
    , alpha(255)
    , repeat(false)
    , linewidth(1)
    , linewidth_2(1)
    , colourcut(0.02)
    , lineinterval(0)
    , lineinterval_2(0)
    , base(0.0)
    , base_2(0.0)
    , minvalue(-fieldUndef)
    , minvalue_2(-fieldUndef)
    , maxvalue(fieldUndef)
    , maxvalue_2(fieldUndef)
    , density(0)
    , densityFactor(1.0)
    , vectorunit(1.0)
    , vectorunitname("m/s")
    , vectorscale_x(1)
    , vectorscale_y(1)
    , vectorthickness(0.1)
    , extremeType("None")
    , extremeSize(1.0)
    , extremeRadius(1.0)
    , lineSmooth(0)
    , fieldSmooth(0)
    , frame(1)
    , zeroLine(true)
    , valueLabel(true)
    , labelSize(1.0)
    , gridValue(false)
    , gridLines(0)
    , gridLinesMax(0)
    , undefMasking(0)
    , undefColour(Colour::WHITE)
    , undefLinewidth(1)
    , plottype(fpt_contour)
    , discontinuous(false)
    , contourShading(false)
    , polystyle(poly_fill)
    , arrowstyle(arrow_wind)
    , h_align(align_left)
    , v_align(align_bottom)
    , fontname(defaultFontName())
    , fontface(defaultFontFace())
    , fontsize(defaultFontSize())
    , precision(0)
    , enabled(true)
    , tableHeader(true)
    , antialiasing(false)
    , use_stencil(false)
    , update_stencil(false)
    , plot_under(false)
    , maxDiagonalInMeters(-1.0)
    , vector_example_x(-1)
    , vector_example_y(-1)
{
}

bool PlotOptions::operator==(const PlotOptions& o) const
{
  // clang-format off
  return
       (options_1 == o.options_1)
    && (options_2 == o.options_2)
    && (textcolour == o.textcolour)
    && (linecolour == o.linecolour)
    && (linecolour_2 == o.linecolour_2)
    && (fillcolour == o.fillcolour)
    && (bordercolour == o.bordercolour)
    && (colours == o.colours)
    && (palettecolours == o.palettecolours)
    && (palettecolours_cold == o.palettecolours_cold)
    && (palettename == o.palettename)
    && (patternname == o.patternname)
    && (table == o.table)
    && (alpha == o.alpha)
    && (repeat == o.repeat)
    && (linetype == o.linetype)
    && (linetype_2 == o.linetype_2)
    && (linewidth == o.linewidth)
    && (linewidth_2 == o.linewidth_2)
    && (patterns == o.patterns)
    && (limits == o.limits)
    && (linevalues_ == o.linevalues_)
    && (loglinevalues_ == o.loglinevalues_)
    && (linevalues_2_ == o.linevalues_2_)
    && (loglinevalues_2_ == o.loglinevalues_2_)
    && (colourcut == o.colourcut)
    && (forecastLength == o.forecastLength)
    && (forecastValueMin == o.forecastValueMin)
    && (forecastValueMax == o.forecastValueMax)
    && (lineinterval == o.lineinterval)
    && (lineinterval_2 == o.lineinterval_2)
    && (base == o.base)
    && (base_2 == o.base_2)
    && (minvalue == o.minvalue)
    && (minvalue_2 == o.minvalue_2)
    && (maxvalue == o.maxvalue)
    && (maxvalue_2 == o.maxvalue_2)
    && (density == o.density)
    && (densityFactor == o.densityFactor)
    && (vectorunit == o.vectorunit)
    && (vectorunitname == o.vectorunitname)
    && (vectorscale_x == o.vectorscale_x)
    && (vectorscale_y == o.vectorscale_y)
    && (vectorthickness == o.vectorthickness)
    && (extremeType == o.extremeType)
    && (extremeSize == o.extremeSize)
    && (extremeRadius == o.extremeRadius)
    && (lineSmooth == o.lineSmooth)
    && (fieldSmooth == o.fieldSmooth)
    && (frame == o.frame)
    && (zeroLine == o.zeroLine)
    && (valueLabel == o.valueLabel)
    && (labelSize == o.labelSize)
    && (gridValue == o.gridValue)
    && (gridLines == o.gridLines)
    && (gridLinesMax == o.gridLinesMax)
    && (undefMasking == o.undefMasking)
    && (undefColour == o.undefColour)
    && (undefLinewidth == o.undefLinewidth)
    && (undefLinetype == o.undefLinetype)
    && (plottype == o.plottype)
    && (discontinuous == o.discontinuous)
    && (contourShading == o.contourShading)
    && (classSpecifications == o.classSpecifications)
    && (polystyle == o.polystyle)
    && (arrowstyle == o.arrowstyle)
    && (h_align == o.h_align)
    && (v_align == o.v_align)
    && (fontname == o.fontname)
    && (fontface == o.fontface)
    && (fontsize == o.fontsize)
    && (precision == o.precision)
    && (enabled == o.enabled)
    && (fname == o.fname)
    && (legendunits == o.legendunits)
    && (legendtitle == o.legendtitle)
    && (tableHeader == o.tableHeader)
    && (antialiasing == o.antialiasing)
    && (use_stencil == o.use_stencil)
    && (update_stencil == o.update_stencil)
    && (plot_under == o.plot_under)
    && (maxDiagonalInMeters == o.maxDiagonalInMeters)
    && (vector_example_x == o.vector_example_x)
    && (vector_example_y == o.vector_example_y)
    && (vector_example_unit_x == o.vector_example_unit_x)
    && (vector_example_unit_y == o.vector_example_unit_y)
    ;
  // clang-format on
}

bool PlotOptions::parsePlotOption(const miutil::KeyValue& kv, PlotOptions& po)
{
  if (kv.value().empty())
    return false;

  const std::string& key = kv.key();
  const std::string& value = kv.value();

  if (key == key_colour) {
    po.set_colour(value);

  } else if (key == key_colour_2) {
    po.set_colour_2(value);

  } else if (key == key_options_1) {
    po.options_1 = to_bool(kv);

  } else if (key == key_options_2) {
    po.options_2 = to_bool(kv);

  } else if (key == key_lcolour) {
    po.linecolour = Colour(value);

  } else if (key == key_lcolour_2) {
    po.linecolour_2 = Colour(value);

  } else if (key == key_tcolour) {
    po.textcolour = Colour(value);

  } else if (key == key_fcolour || key == key_pcolour) {
    po.fillcolour = Colour(value);

  } else if (key == key_bcolour) {
    po.bordercolour = Colour(value);

  } else if (key == key_colours) {
    po.set_colours(value);

  } else if (key == key_palettecolours) {
    po.set_palettecolours(value);

  } else if (key == key_patterns) {
    po.set_patterns(value);

  } else if (key == key_linetype) {
    po.set_linetype(value);

  } else if (key == key_linetype_2) {
    po.set_linetype_2(value);

  } else if (key == key_linetypes) {
    // ignored

  } else if (key == key_linewidth) {
    po.set_linewidth(value);

  } else if (key == key_linewidth_2) {
    po.set_linewidth_2(value);

  } else if (key == key_linewidths) {
    // ignored

  } else if (key == key_lineinterval) {
    po.set_lineinterval(value);

  } else if (key == key_lineinterval_2) {
    po.set_lineinterval_2(value);

  } else if (key == key_minvalue) {
    po.set_minvalue(value);

  } else if (key == key_maxvalue) {
    po.set_maxvalue(value);

  } else if (key == key_minvalue_2) {
    po.set_minvalue_2(value);

  } else if (key == key_maxvalue_2) {
    po.set_maxvalue_2(value);

  } else if (key == key_colourcut) {
    po.colourcut = to_float(kv);

  } else if (key == key_linevalues) {
    po.set_linevalues(value);

  } else if (key == key_linevalues_2) {
    po.set_linevalues_2(value);

  } else if (key == key_loglinevalues) {
    po.set_loglinevalues(value);

  } else if (key == key_loglinevalues_2) {
    po.set_loglinevalues_2(value);

  } else if (key == key_limits) {
    po.limits = checkNotEmpty(kv, autoExpandFloatVector(value));

  } else if (key == key_values) {
    // ignored

  } else if (key == key_extremeType) {
    // this version: should be "L+H" or "C+W"...
    po.extremeType = value;

  } else if (key == key_extremeSize) {
    po.extremeSize = to_float(kv);

  } else if (key == key_extremeRadius) {
    po.extremeRadius = to_float(kv);

  } else if (key == key_extremeLimits) {
    // ignored

  } else if (key == key_lineSmooth) {
    po.lineSmooth = to_int(kv);

  } else if (key == key_fieldSmooth) {
    po.fieldSmooth = to_int(kv);

  } else if (key == key_frame) {
    po.frame = miutil::constrain_value(to_int(kv), 0, 3);

  } else if (key == key_zeroLine) {
    po.zeroLine = (kv.value() == "-1") || to_bool(kv);

  } else if (key == key_valueLabel) {
    po.valueLabel = to_bool(kv);

  } else if (key == key_labelSize) {
    po.labelSize = miutil::constrain_value(to_float(kv), 0.2f, 5.0f);

  } else if (key == key_gridValue) {
    po.gridValue = to_bool(kv);

  } else if (key == key_gridLines) {
    po.gridLines = std::max(0, to_int(kv));

  } else if (key == key_gridLinesMax) {
    po.gridLinesMax = std::max(0, to_int(kv));

  } else if (key == key_plottype) {
    const std::string v = miutil::to_lower(value);
    if (v == fpt_contour1 || v == fpt_value || v == fpt_contour2 || v == fpt_contour || v == fpt_rgb || v == fpt_alpha_shade ||
        v == fpt_symbol || v == fpt_alarm_box || v == fpt_fill_cell || v == fpt_wind || v == fpt_vector || v == fpt_wind_temp_fl ||
        v == fpt_wind_value || v == fpt_direction || v == fpt_frame || v == fpt_streamlines) {
      po.plottype = v;
    } else if (v == "wind_colour") {
      po.plottype = fpt_wind;
    } else if (v == "direction_colour") {
      po.plottype = fpt_direction;
    } else if (v == "box_pattern") {
      po.plottype = fpt_contour;
    } else if (v == "layer") {
      po.plottype = fpt_value;
    } else if (v == "wind_number") {
      po.plottype = fpt_wind_value;
    } else if (v == "number") {
      po.plottype = fpt_value;
    } else {
      throw_error(kv, "is not a valid value");
    }

  } else if (key == key_rotatevectors) {
    // ignored

  } else if (key == key_discontinuous) {
    po.discontinuous = to_bool(kv);

  } else if (key == key_table) {
    po.table = to_bool(kv);

  } else if (key == key_alpha) {
    po.alpha = to_int(kv);

  } else if (key == key_repeat) {
    po.repeat = to_bool(kv);

  } else if (key == key_classes) {
    if (is_off(value))
      po.classSpecifications.clear();
    else
      po.classSpecifications = value;

  } else if (key == key_basevalue) {
    po.base = to_float(kv);

  } else if (key == key_basevalue_2) {
    po.base_2 = to_float(kv);

  } else if (key == key_density) {
    po.set_density(value);

  } else if (key == key_densityfactor) {
    po.densityFactor = to_float(kv);

  } else if (key == key_vectorunit) {
    po.vectorunit = to_float(kv);

  } else if (key == key_vectorunitname) {
    po.vectorunitname = value;

  } else if (key == key_vectorscale_x) {
    po.vectorscale_x = to_float(kv);

  } else if (key == key_vectorscale_y) {
    po.vectorscale_y = to_float(kv);

  } else if (key == key_vectorthickness) {
    po.vectorthickness = std::max(0.0f, to_float(kv));

  } else if (key == key_forecastLength) {
    po.forecastLength = checkNotEmpty(kv, intVector(value));

  } else if (key == key_forecastValueMin) {
    po.forecastValueMin = checkNotEmpty(kv, floatVector(value));

  } else if (key == key_forecastValueMax) {
    po.forecastValueMax = checkNotEmpty(kv, floatVector(value));

  } else if (key == key_undefMasking) {
    po.undefMasking = miutil::constrain_value(to_int(kv), 0, 2);

  } else if (key == key_undefColour) {
    po.undefColour = Colour(value);

  } else if (key == key_undefLinewidth) {
    po.undefLinewidth = std::max(0, to_int(kv));

  } else if (key == key_undefLinetype) {
    po.undefLinetype = Linetype(value);

  } else if (key == key_polystyle) {
    if (value == "fill")
      po.polystyle = poly_fill;
    else if (value == "border")
      po.polystyle = poly_border;
    else if (value == "both")
      po.polystyle = poly_both;
    else if (value == "none")
      po.polystyle = poly_none;
    else
      throw_error(kv, "is not a valid value");

  } else if (key == key_arrowstyle) { // warning: only arrow_wind_arrow implemented yet
    if (value == "wind")
      po.arrowstyle = arrow_wind;
    else if (value == "wind_arrow")
      po.arrowstyle = arrow_wind_arrow;
    else if (value == "wind_colour")
      po.arrowstyle = arrow_wind_colour;
    else if (value == "wind_value")
      po.arrowstyle = arrow_wind_value;

  } else if (key == key_h_alignment) {
    if (value == "left")
      po.h_align = align_left;
    else if (value == "right")
      po.h_align = align_right;
    else if (value == "center")
      po.h_align = align_center;

  } else if (key == key_v_alignment) {
    if (value == "top")
      po.v_align = align_top;
    else if (value == "bottom")
      po.v_align = align_bottom;
    else if (value == "center")
      po.v_align = align_center;

  } else if (key == key_alignX) {
    // ignore

  } else if (key == key_alignY) {
    // ignore

  } else if (key == key_fontname) {
    po.fontname = value;

  } else if (key == key_fontface) {
    po.fontface = diutil::fontFaceFromString(value);

  } else if (key == key_fontsize) {
    po.fontsize = to_float(kv);

  } else if (key == key_precision) {
    po.precision = to_float(kv);

  } else if (key == key_dimension) {
    // ignore
  } else if (key == key_enabled) {
    po.enabled = to_bool(kv);

  } else if (key == key_fname) {
    po.fname = value;

  } else if (key == key_contourShape) {
    // ignore
  } else if (key == key_shapefilename) {
    // ignore
  } else if (key == key_legendunits) {
    po.legendunits = value;
  } else if (key == key_legendtitle) {
    po.legendtitle = value;
  } else if (key == key_antialiasing) {
    po.antialiasing = to_bool(kv);
  } else if (key == key_use_stencil) {
    po.use_stencil = to_bool(kv);
  } else if (key == key_update_stencil) {
    po.update_stencil = to_bool(kv);
  } else if (key == key_plot_under) {
    po.plot_under = to_bool(kv);
  } else if (key == key_maxDiagonalInMeters) {
    po.maxDiagonalInMeters = to_float(kv);
  } else if (key == key_vector_example_x) {
    po.vector_example_x = to_float(kv);
  } else if (key == key_vector_example_y) {
    po.vector_example_y = to_float(kv);
  } else if (key == key_vector_example_unit_x) {
    po.vector_example_unit_x = value;
  } else if (key == key_vector_example_unit_y) {
    po.vector_example_unit_y = value;

  } else {
    return false;
  }

  return true;
}

PlotOptions& PlotOptions::set_colour(const std::string& name)
{
  if (is_off(name)) {
    options_1 = false;
  } else {
    options_1 = true;
    linecolour = Colour(name);
  }
  return *this;
}

PlotOptions& PlotOptions::set_colour_2(const std::string& name)
{
  if (is_off(name)) {
    options_2 = false;
  } else {
    options_2 = true;
    linecolour_2 = Colour(name);
  }
  return *this;
}

PlotOptions& PlotOptions::set_colours(const std::string& value)
{
  options_1 = true;
  colours.clear();
  if (!is_off(value)) {
    for (const auto& c : miutil::split(value, 0, ","))
      colours.push_back(Colour(c));
  }
  return *this;
}

PlotOptions& PlotOptions::set_palettecolours(const std::string& value)
{
  palettecolours.clear();
  palettecolours_cold.clear();
  if (!is_off(value)) {
    contourShading = true;
    palettename = value;
    miutil::remove(palettename, '"');
    const auto stokens = miutil::split(palettename, ",");
    const size_t m = stokens.size();
    if (m > 2) {
      const auto ntoken = miutil::split(value, ";");
      const auto stokens = miutil::split(ntoken.front(), ","); // split again, such that the last colour is without ";"
      palettecolours.reserve(stokens.size());
      for (const auto& c : stokens) {
        palettecolours.push_back(Colour(c));
      }
      if (ntoken.size() == 2 && miutil::is_int(ntoken[1])) {
        palettecolours = ColourShading::adaptColourShading(palettecolours, atoi(ntoken[1].c_str()));
      }
    } else {
      if (m > 0) {
        const auto ntoken = miutil::split(stokens[0], 0, ";");
        ColourShading cs(ntoken[0]);
        if (ntoken.size() == 2 && miutil::is_int(ntoken[1])) {
          palettecolours = cs.getColourShading(atoi(ntoken[1].c_str()));
        } else {
          palettecolours = cs.getColourShading();
        }
      }
      if (m > 1) {
        const auto ntoken = miutil::split(stokens[1], 0, ";");
        ColourShading cs(ntoken[0]);
        if (ntoken.size() == 2 && miutil::is_int(ntoken[1]))
          palettecolours_cold = cs.getColourShading(atoi(ntoken[1].c_str()));
        else
          palettecolours_cold = cs.getColourShading();
      }
    }
  } else {
    palettename.clear();
    if (patternname.empty())
      contourShading = false;
  }
  return *this;
}

PlotOptions& PlotOptions::set_patterns(const std::string& value)
{
  if (!is_off(value)) {
    patternname = value;
    contourShading = true;
    if (miutil::contains(value, ","))
      patterns = miutil::split(value, 0, ",");
    else {
      patterns = Pattern::getPatternInfo(value);
    }
  } else {
    patternname.clear();
    patterns.clear();
    if (palettename.empty())
      contourShading = false;
  }
  return *this;
}

PlotOptions& PlotOptions::set_lineinterval(float value)
{
  lineinterval = std::max(value, 0.0f);
  return *this;
}

PlotOptions& PlotOptions::set_lineinterval(const std::string& value)
{
  return set_lineinterval(to_float_or_off(miutil::kv(key_lineinterval, value), 0));
}

PlotOptions& PlotOptions::set_lineinterval_2(float value)
{
  lineinterval_2 = std::max(value, 0.0f);
  return *this;
}

PlotOptions& PlotOptions::set_lineinterval_2(const std::string& value)
{
  return set_lineinterval_2(to_float_or_off(miutil::kv(key_lineinterval_2, value), 0));
}

PlotOptions& PlotOptions::set_linevalues(const std::string& values)
{
  if (!values.empty()) {
    linevalues_ = checkFloatVector(miutil::kv(key_linevalues, values), autoExpandFloatVector(values));
  } else {
    linevalues_.clear();
  }
  return *this;
}

PlotOptions& PlotOptions::set_linevalues_2(const std::string& values)
{
  if (!values.empty()) {
    linevalues_2_ = checkFloatVector(miutil::kv(key_linevalues_2, values), autoExpandFloatVector(values));
  } else {
    linevalues_2_.clear();
  }
  return *this;
}

PlotOptions& PlotOptions::set_loglinevalues(const std::string& values)
{
  if (!values.empty()) {
    loglinevalues_ = checkFloatVector(miutil::kv(key_loglinevalues, values), autoExpandFloatVector(values));
  } else {
    loglinevalues_.clear();
  }
  return *this;
}

PlotOptions& PlotOptions::set_loglinevalues_2(const std::string& values)
{
  if (!values.empty()) {
    loglinevalues_2_ = checkFloatVector(miutil::kv(key_loglinevalues_2, values), autoExpandFloatVector(values));
  } else {
    loglinevalues_2_.clear();
  }
  return *this;
}

PlotOptions& PlotOptions::set_linetype(const std::string& value)
{
  linetype = Linetype(value);
  return *this;
}

PlotOptions& PlotOptions::set_linetype_2(const std::string& value)
{
  linetype_2 = Linetype(value);
  return *this;
}

PlotOptions& PlotOptions::set_linewidth(const std::string& value)
{
  linewidth = to_int(miutil::kv(key_linewidth, value));
  return *this;
}

PlotOptions& PlotOptions::set_linewidth_2(const std::string& value)
{
  linewidth_2 = to_int(miutil::kv(key_linewidth_2, value));
  return *this;
}

PlotOptions& PlotOptions::set_density(const std::string& value)
{
  if (miutil::is_int(value)) {
    density = miutil::to_int(value);
  } else {
    density = 0;
    size_t pos1 = value.find_first_of("(") + 1;
    size_t pos2 = value.find_first_of(")");
    if (pos2 > pos1) {
      std::string tmp = value.substr(pos1, (pos2 - pos1));
      if (miutil::is_number(tmp)) {
        densityFactor = atof(tmp.c_str());
      }
    }
  }
  return *this;
}

PlotOptions& PlotOptions::set_minvalue(const std::string& value)
{
  minvalue = to_float_or_off(miutil::kv(key_minvalue, value), -fieldUndef);
  return *this;
}

PlotOptions& PlotOptions::set_maxvalue(const std::string& value)
{
  maxvalue = to_float_or_off(miutil::kv(key_maxvalue, value), fieldUndef);
  return *this;
}

PlotOptions& PlotOptions::set_minvalue_2(const std::string& value)
{
  minvalue_2 = to_float_or_off(miutil::kv(key_minvalue_2, value), -fieldUndef);
  return *this;
}

PlotOptions& PlotOptions::set_maxvalue_2(const std::string& value)
{
  maxvalue_2 = to_float_or_off(miutil::kv(key_maxvalue_2, value), fieldUndef);
  return *this;
}

bool PlotOptions::use_lineinterval() const
{
  return lineinterval > 0;
}

bool PlotOptions::use_lineinterval_2() const
{
  return lineinterval_2 > 0;
}

bool PlotOptions::use_linevalues() const
{
  return !use_lineinterval() && !linevalues_.empty() && !use_loglinevalues();
}

bool PlotOptions::use_linevalues_2() const
{
  return !use_lineinterval_2() && !linevalues_2_.empty() && !use_loglinevalues_2();
}

bool PlotOptions::use_loglinevalues() const
{
  return !use_lineinterval() && !loglinevalues_.empty();
}

bool PlotOptions::use_loglinevalues_2() const
{
  return !use_lineinterval_2() && !loglinevalues_2_.empty();
}

// parse a string (possibly) containing plotting options,
// and fill a PlotOptions with appropriate values
bool PlotOptions::parsePlotOption(const miutil::KeyValue_v& opts, PlotOptions& po, miutil::KeyValue_v& unrecognized)
{
  // very frequent METLIBS_LOG_SCOPE();

  bool result = true;

  for (miutil::KeyValue kv : opts) {
    try {
      const bool used = parsePlotOption(kv, po);
      if (!used) {
#if 0
        const auto idx = miutil::find(unrecognized, kv.key());
        if (idx == (size_t)-1)
#endif
          unrecognized.push_back(kv);
#if 0
        else
          unrecognized[idx] = kv;
#endif
      }
    } catch (std::runtime_error& e) {
      result = false; // ignore otherwise
    }
  }
  return result;
}

bool PlotOptions::parsePlotOption(const miutil::KeyValue_v& opts, PlotOptions& po)
{
  miutil::KeyValue_v unusedOptions;
  return parsePlotOption(opts, po, unusedOptions);
}

// static
std::vector<int> PlotOptions::intVector(const std::string& str)
{
  std::vector<int> v;
  for (const auto& t : miutil::split(str, 0, ",")) {
    if (miutil::is_int(t))
      v.push_back(atoi(t.c_str()));
    else
      return std::vector<int>();
  }
  return v;
}

// static
std::vector<float> PlotOptions::floatVector(const std::string& str)
{
  std::vector<float> values;
  for (const auto& t : miutil::split(str, ",")) {
    if (miutil::is_number(t))
      values.push_back(miutil::to_double(t));
    else
      return std::vector<float>();
  }
  return values;
}

// fill in values and "..." (1,2,3,...10) in a float vector
std::vector<float> PlotOptions::autoExpandFloatVector(const std::string& str)
{
  std::vector<float> values;

  for (const auto& t : miutil::split(str, ",")) {
    if (miutil::is_number(t)) {
      const double v = miutil::to_double(t);
      values.push_back(v);
      continue;
    }

    // try to add a whole interval (10,20,...50)

    if (values.size() < 2) {
      METLIBS_LOG_DEBUG("need 2 values yet to define step size");
      return std::vector<float>();
    }

    const ssize_t no_dot = t.find_first_not_of(".");
    if (no_dot < 2) {
      METLIBS_LOG_DEBUG("not enough dots");
      return std::vector<float>();
    }

    const std::string after_dots = t.substr(no_dot);
    if (not miutil::is_number(after_dots)) {
      METLIBS_LOG_DEBUG("no number after dots (" << after_dots << ")");
      return std::vector<float>();
    }

    const size_t k = values.size();
    const float stop = miutil::to_double(after_dots);
    const float last = values[k-1], second_last = values[k-2], delta = last - second_last;
    // keep previous step, example above: 10,20,30,40,50
    if (delta > 0) {
      for (float step = last + delta; step <= stop; step += delta)
        values.push_back(step);
    }
  }

  return values;
}

miutil::KeyValue_v PlotOptions::toKeyValueListForAnnotation() const
{
  miutil::KeyValue_v ostr;
  miutil::add(ostr, key_tcolour, textcolour.Name());
  miutil::add(ostr, key_fcolour, fillcolour.Name());
  miutil::add(ostr, key_pcolour, fillcolour.Name()); // TODO why is this the same as fcolour?
  miutil::add(ostr, key_bcolour, bordercolour.Name());
  miutil::add(ostr, key_fontname, fontname);
  miutil::add(ostr, key_fontface, fontface);
  miutil::add(ostr, key_fontsize, fontsize);

  miutil::add(ostr, key_polystyle, as_string(polystyle));
  miutil::add(ostr, key_h_alignment, as_string(h_align));
  miutil::add(ostr, key_v_alignment, as_string(v_align));

  return ostr;
}

namespace {

template <class V>
void add_diff(miutil::KeyValue_v& ostr, const std::string& key, const V& f, const V& t, bool force = false)
{
  if (force || f != t)
    miutil::add(ostr, key, as_string(t));
}

template <class V>
void add_diff_off(miutil::KeyValue_v& ostr, const std::string& key, const V& f, const V& t, bool off, bool force = false)
{
  if (force || f != t)
    miutil::add(ostr, key, !off ? as_string(t) : OFF);
}

void add_diff_minmax(miutil::KeyValue_v& ostr, const std::string& k_min, float f_min, float t_min, const std::string& k_max, float f_max, float t_max)
{
  if (f_min != t_min) {
    if (t_min > -fieldUndef)
      miutil::add(ostr, k_min, t_min);
    else
      miutil::add(ostr, k_min, OFF);
  }
  if (f_max != t_max) {
    if (t_max < fieldUndef)
      miutil::add(ostr, k_max, t_max);
    else
      miutil::add(ostr, k_max, OFF);
  }
}

} // namespace

// static
miutil::KeyValue_v PlotOptions::diff(const PlotOptions& from, const PlotOptions& to)
{
  miutil::KeyValue_v ostr;

  add_diff_off(ostr, key_colours, from.colours, to.colours, to.colours.empty());
  // applying "colours" sets option_1 to true, therefore we write "colours" first and add a tweak for "options_1"
  if (to.options_1 && from.linecolour != to.linecolour && !is_off(as_string(to.linecolour))) {
    miutil::add(ostr, key_colour, as_string(to.linecolour));
  } else {
    const bool change_colours = (from.colours != to.colours);
    add_diff(ostr, key_options_1, from.options_1, to.options_1, !from.options_1 && change_colours);
    add_diff(ostr, key_lcolour, from.linecolour, to.linecolour);
  }
  add_diff(ostr, key_tcolour, from.textcolour, to.textcolour);
  add_diff(ostr, key_fcolour, from.fillcolour, to.fillcolour);
  // pcolour / patterncolour is the same as fillcolour
  add_diff(ostr, key_bcolour, from.bordercolour, to.bordercolour);

  add_diff(ostr, key_options_2, from.options_2, to.options_2);
  add_diff(ostr, key_lcolour_2, from.linecolour_2, to.linecolour_2);

  // contourshading is set by palettecolours and "patterns"
  add_diff_off(ostr, key_palettecolours, from.palettename, to.palettename, to.palettename.empty()); // toKeyValueList used "palettename"
  add_diff_off(ostr, key_patterns, from.patternname, to.patternname, to.patternname.empty());       // toKeyValueList used "patterns"

  add_diff(ostr, key_linetype, from.linetype, to.linetype);
  add_diff(ostr, key_linetype_2, from.linetype_2, to.linetype_2);

  add_diff(ostr, key_linewidth, from.linewidth, to.linewidth);
  add_diff(ostr, key_linewidth_2, from.linewidth_2, to.linewidth_2);

  add_diff(ostr, key_lineinterval, from.lineinterval, to.lineinterval);
  add_diff(ostr, key_lineinterval_2, from.lineinterval_2, to.lineinterval_2);

  add_diff_minmax(ostr, key_minvalue, from.minvalue, to.minvalue, key_maxvalue, from.maxvalue, to.maxvalue);
  add_diff_minmax(ostr, key_minvalue_2, from.minvalue_2, to.minvalue_2, key_maxvalue_2, from.maxvalue_2, to.maxvalue_2);

  add_diff(ostr, key_colourcut, from.colourcut, to.colourcut);

  add_diff(ostr, key_linevalues, from.linevalues_, to.linevalues_);
  add_diff(ostr, key_loglinevalues, from.loglinevalues_, to.loglinevalues_);
  add_diff(ostr, key_linevalues_2, from.linevalues_2_, to.linevalues_2_);
  add_diff(ostr, key_loglinevalues_2, from.loglinevalues_2_, to.loglinevalues_2_);

  add_diff(ostr, key_limits, from.limits, to.limits);

  add_diff(ostr, key_extremeType, from.extremeType, to.extremeType);
  add_diff(ostr, key_extremeSize, from.extremeSize, to.extremeSize);
  add_diff(ostr, key_extremeRadius, from.extremeRadius, to.extremeRadius);

  add_diff(ostr, key_lineSmooth, from.lineSmooth, to.lineSmooth);
  add_diff(ostr, key_fieldSmooth, from.fieldSmooth, to.fieldSmooth);

  add_diff(ostr, key_frame, from.frame, to.frame);
  add_diff(ostr, key_zeroLine, from.zeroLine, to.zeroLine);
  add_diff(ostr, key_valueLabel, from.valueLabel, to.valueLabel);
  add_diff(ostr, key_labelSize, from.labelSize, to.labelSize);
  add_diff(ostr, key_gridValue, from.gridValue, to.gridValue);
  add_diff(ostr, key_gridLines, from.gridLines, to.gridLines);
  add_diff(ostr, key_gridLinesMax, from.gridLinesMax, to.gridLinesMax);

  add_diff(ostr, key_plottype, from.plottype, to.plottype);

  add_diff(ostr, key_discontinuous, from.discontinuous, to.discontinuous);
  add_diff(ostr, key_table, from.table, to.table);
  add_diff(ostr, key_alpha, from.alpha, to.alpha);
  add_diff(ostr, key_repeat, from.repeat, to.repeat);
  add_diff_off(ostr, key_classes, from.classSpecifications, to.classSpecifications, to.classSpecifications.empty());

  add_diff(ostr, key_basevalue, from.base, to.base);
  add_diff(ostr, key_basevalue_2, from.base_2, to.base_2);

  add_diff(ostr, key_density, from.density, to.density);
  add_diff(ostr, key_densityfactor, from.densityFactor, to.densityFactor);

  add_diff(ostr, key_vectorunit, from.vectorunit, to.vectorunit);
  add_diff(ostr, key_vectorunitname, from.vectorunitname, to.vectorunitname); // OBS apply removes '"' (should be ok if set by KeyValue)
  add_diff(ostr, key_vectorscale_x, from.vectorscale_x, to.vectorscale_x);
  add_diff(ostr, key_vectorscale_y, from.vectorscale_y, to.vectorscale_y);
  add_diff(ostr, key_vectorthickness, from.vectorthickness, to.vectorthickness);

  add_diff(ostr, key_forecastLength, from.forecastLength, to.forecastLength);
  add_diff(ostr, key_forecastValueMin, from.forecastValueMin, to.forecastValueMin);
  add_diff(ostr, key_forecastValueMax, from.forecastValueMax, to.forecastValueMax);

  add_diff(ostr, key_undefMasking, from.undefMasking, to.undefMasking);
  add_diff(ostr, key_undefColour, from.undefColour.Name(), to.undefColour.Name());
  add_diff(ostr, key_undefLinewidth, from.undefLinewidth, to.undefLinewidth);
  add_diff(ostr, key_undefLinetype, from.undefLinetype, to.undefLinetype);

  add_diff(ostr, key_polystyle, from.polystyle, to.polystyle);
  add_diff(ostr, key_arrowstyle, from.arrowstyle, to.arrowstyle);
  add_diff(ostr, key_h_alignment, from.h_align, to.h_align);
  add_diff(ostr, key_v_alignment, from.v_align, to.v_align);

  add_diff(ostr, key_fontname, from.fontname, to.fontname);
  add_diff(ostr, key_fontface, from.fontface, to.fontface);

  add_diff(ostr, key_precision, from.precision, to.precision);
  add_diff(ostr, key_enabled, from.enabled, to.enabled);
  add_diff(ostr, key_fname, from.fname, to.fname);

  add_diff(ostr, key_legendunits, from.legendunits, to.legendunits);
  add_diff(ostr, key_legendtitle, from.legendtitle, to.legendtitle);

  add_diff(ostr, key_antialiasing, from.antialiasing, to.antialiasing);
  add_diff(ostr, key_use_stencil, from.use_stencil, to.use_stencil);
  add_diff(ostr, key_update_stencil, from.update_stencil, to.update_stencil);
  add_diff(ostr, key_plot_under, from.plot_under, to.plot_under);
  add_diff(ostr, key_maxDiagonalInMeters, from.maxDiagonalInMeters, to.maxDiagonalInMeters);
  add_diff(ostr, key_vector_example_x, from.vector_example_x, to.vector_example_x);
  add_diff(ostr, key_vector_example_y, from.vector_example_y, to.vector_example_y);
  add_diff(ostr, key_vector_example_unit_x, from.vector_example_unit_x, to.vector_example_unit_x);
  add_diff(ostr, key_vector_example_unit_y, from.vector_example_unit_y, to.vector_example_unit_y);

  return ostr;
}

miutil::KeyValue_v PlotOptions::toKeyValueList() const
{
  static const PlotOptions reference;
  return diffFrom(reference);
}

// static
const std::string& PlotOptions::defaultFontName()
{
  return diutil::SCALEFONT;
}

// static
diutil::FontFace PlotOptions::defaultFontFace()
{
  return diutil::F_NORMAL;
}

// static
float PlotOptions::defaultFontSize()
{
  return 10.0;
}
