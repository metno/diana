/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include "diPlotOptions.h"
#include "diColourShading.h"
#include "diPattern.h"

#include <diField/diField.h>
#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/foreach.hpp>

#define MILOGGER_CATEGORY "diana.PlotOptions"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

namespace {
bool checkFloatVector(const vector<float>& aefv)
{
  if (aefv.empty())
    return false;

  for (size_t j=1; j<aefv.size(); j++) {
    if (aefv[j-1] >= aefv[j])
      return false;
  }

  return true;
}
const std::string OFF = "off";
const std::string TRUE = "true";
const std::string FALSE = "false";
} // namespace

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
const std::string PlotOptions::key_filepalette= "file.palette";
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
const std::string PlotOptions::key_values= "values";
const std::string PlotOptions::key_extremeType= "extreme.type";
const std::string PlotOptions::key_extremeSize= "extreme.size";
const std::string PlotOptions::key_extremeRadius= "extreme.radius";
const std::string PlotOptions::key_extremeLimits= "extreme.limits";
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
const std::string PlotOptions::key_rotatevectors= "rotate.vectors";
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
const std::string PlotOptions::key_alignX= "alignX";
const std::string PlotOptions::key_alignY= "alignY";
const std::string PlotOptions::key_fontname= "font";
const std::string PlotOptions::key_fontface= "face";
const std::string PlotOptions::key_fontsize= "fontsize";
const std::string PlotOptions::key_precision= "precision";
const std::string PlotOptions::key_dimension= "dim";
const std::string PlotOptions::key_enabled= "enabled";
const std::string PlotOptions::key_fdescr= "fdesc";
const std::string PlotOptions::key_fname= "fname";
const std::string PlotOptions::key_overlay="overlay";
const std::string PlotOptions::key_contourShape="contourShape";
const std::string PlotOptions::key_shapefilename="shapefilename";
const std::string PlotOptions::key_unit="unit";
const std::string PlotOptions::key_units="units";
const std::string PlotOptions::key_legendunits="legendunits";
const std::string PlotOptions::key_legendtitle="legendtitle";
const std::string PlotOptions::key_antialiasing="antialiasing";
const std::string PlotOptions::key_use_stencil="use_stencil";
const std::string PlotOptions::key_update_stencil="update_stencil";
const std::string PlotOptions::key_plot_under="plot_under";
const std::string PlotOptions::key_maxDiagonalInMeters="maxdiagonalinmeters";

map<std::string,PlotOptions> PlotOptions::fieldPlotOptions;
vector<std::string> PlotOptions::suffix;
vector< vector <std::string> > PlotOptions::plottypes;
map< std::string, std::string > PlotOptions::enabledOptions;

PlotOptions::PlotOptions():
  options_1(true),options_2(false),
  textcolour(BlackC), linecolour(BlackC), linecolour_2(BlackC),
  fillcolour(BlackC), bordercolour(BlackC), table(0),alpha(255), repeat(0),
  linewidth(1), linewidth_2(1), colourcut(1), lineinterval(10.0), lineinterval_2(10.0),
  base(0.0), base_2(0.0), minvalue(-fieldUndef), minvalue_2(-fieldUndef),
  maxvalue(fieldUndef), maxvalue_2(fieldUndef), density(0), densityFactor(1.0),
  vectorunit(1.0), vectorunitname("m/s"), vectorscale_x(1), vectorscale_y(1), vectorthickness(0.1),
  extremeType("None"), extremeSize(1.0), extremeRadius(1.0),
  lineSmooth(0), fieldSmooth(0), frame(1), zeroLine(-1), valueLabel(1), labelSize(1.0),
  gridValue(0),gridLines(0), gridLinesMax(0),
  undefMasking(0), undefColour(WhiteC), undefLinewidth(1),
  plottype(fpt_contour), rotateVectors(1), discontinuous(0), contourShading(0),
  polystyle(poly_fill), arrowstyle(arrow_wind), h_align(align_left), v_align(align_bottom),
  alignX(0), alignY(0),
  fontname(defaultFontName()), fontface(defaultFontFace()), fontsize(defaultFontSize()), precision(0),
  dimension(1), enabled(true), overlay(0), contourShape(0), tableHeader(true),
  antialiasing(false), use_stencil(false), update_stencil(false), plot_under(false), maxDiagonalInMeters(-1.0)
{
  limits.clear();
  values.clear();
  linevalues.clear();
  loglinevalues.clear();
  linevalues_2.clear();
  loglinevalues_2.clear();
  forecastLength.clear();
  forecastValueMin.clear();
  forecastValueMax.clear();
  extremeLimits.clear();

  //init plottypes
  vector< std::string> plottypes_all;
  plottypes_all.push_back(fpt_contour);
  plottypes_all.push_back(fpt_contour2);
  plottypes_all.push_back(fpt_value);
  plottypes_all.push_back(fpt_symbol);
  plottypes_all.push_back(fpt_alpha_shade);
  plottypes_all.push_back(fpt_alarm_box);
  plottypes_all.push_back(fpt_fill_cell);
  plottypes_all.push_back(fpt_direction);
  plottypes_all.push_back(fpt_wind);
  plottypes_all.push_back(fpt_vector);
  plottypes_all.push_back(fpt_wind_temp_fl);
  plottypes_all.push_back(fpt_wind_value);
  plottypes_all.push_back(fpt_frame);
  plottypes.push_back(plottypes_all);

  vector< std::string> plottypes_1dim;
  plottypes_1dim.push_back(fpt_contour);
  plottypes_1dim.push_back(fpt_contour2);
  plottypes_1dim.push_back(fpt_value);
  plottypes_1dim.push_back(fpt_symbol);
  plottypes_1dim.push_back(fpt_alpha_shade);
  plottypes_1dim.push_back(fpt_alarm_box);
  plottypes_1dim.push_back(fpt_fill_cell);
  plottypes_1dim.push_back(fpt_direction       );
  plottypes_1dim.push_back(fpt_frame);
  plottypes.push_back(plottypes_1dim);

  vector< std::string> plottypes_2dim;
  plottypes_2dim.push_back(fpt_wind            );
  plottypes_2dim.push_back(fpt_vector          );
  plottypes_2dim.push_back(fpt_value);
  plottypes_2dim.push_back(fpt_frame);
  plottypes.push_back(plottypes_2dim);

  vector< std::string> plottypes_3dim;
  plottypes_3dim.push_back(fpt_wind            );
  plottypes_3dim.push_back(fpt_vector          );
  plottypes_3dim.push_back(fpt_value);
  plottypes_3dim.push_back(fpt_wind_temp_fl    );
  plottypes_3dim.push_back(fpt_wind_value    );
  plottypes_3dim.push_back(fpt_frame);
  plottypes.push_back(plottypes_3dim);

  vector< std::string> plottypes_4dim;
  plottypes_4dim.push_back(fpt_value);
  plottypes_4dim.push_back(fpt_frame);
  plottypes.push_back(plottypes_4dim);

  enabledOptions[fpt_contour] = " extreme line shading contour font";
  enabledOptions[fpt_contour2] = " extreme line shading contour font";
  enabledOptions[fpt_value] = "font density";
  enabledOptions[fpt_symbol] = "font density";
  enabledOptions[fpt_alpha_shade] = " extreme";
  enabledOptions[fpt_fill_cell] = "line_interval shading density";
  enabledOptions[fpt_wind] = "density line unit";
  enabledOptions[fpt_vector] = "density line unit";
  enabledOptions[fpt_direction] = "density line unit";
  enabledOptions[fpt_wind_temp_fl] = "density line unit font";
  enabledOptions[fpt_wind_value] = "density line unit font";

}

// parse a string (possibly) containing plotting options,
// and fill a PlotOptions with appropriate values
bool PlotOptions::parsePlotOption( std::string& optstr, PlotOptions& po, bool returnMergedOptionString)
{
  // very frequent METLIBS_LOG_SCOPE();

  Linetype linetype;

  std::string origStr;

  bool result=true;

  const vector<string> tokens= miutil::split_protected(optstr, '"', '"');
  BOOST_FOREACH(const string& token, tokens) {
    const vector<string> etokens = miutil::split(token, "=");
    const size_t l = etokens.size();
    if (l > 1) {
      const string& key = etokens[0];
      string value = etokens[1];
      if (value[0]=='\'' && value[value.length()-1]=='\'')
        value= value.substr(1,value.length()-2);

      if (key==key_colour){
        po.colours.clear();
        if(value==OFF){
          po.options_1= false;
        } else {
          po.options_1= true;
          const Colour c(value);
          po.textcolour= c;
          po.linecolour= c;
          po.fillcolour= c;
          po.bordercolour= c;
          po.colours.push_back(c);
        }

      } else if (key==key_colour_2){
        if(value==OFF){
          po.options_2= false;
        } else {
          po.options_2= true;
          const Colour c(value);
          po.linecolour_2= c;
          po.colours.push_back(c);
        }

      } else if (key==key_tcolour){
        const Colour c(value);
        po.textcolour= c;

      } else if (key==key_fcolour){
        const Colour c(value);
        po.fillcolour= c;

      } else if (key==key_pcolour){
        const Colour c(value);
        po.fillcolour= c;

      } else if (key==key_bcolour){
        const Colour c(value);
        po.bordercolour= c;

      } else if (key==key_colours){
        po.colours.clear();
        // if(value==OFF){
        //   po.options_1= false;
        // } else {
        po.options_1= true;
        const vector<string> stokens= miutil::split(value, 0, ",");
        BOOST_FOREACH(const string& c, stokens) {
          po.colours.push_back(Colour(c));
        }

      } else if (key==key_palettecolours){
        po.palettecolours.clear();
        po.palettecolours_cold.clear();
        po.contourShading = ( value != OFF);
        miutil::remove(value,'"');
        po.palettename=value;
        const vector<string> stokens = miutil::split(value, ",");
        const size_t m= stokens.size();
        if(m>2){
          BOOST_FOREACH(const string& c, stokens) {
            po.palettecolours.push_back(Colour(c));
          }
        } else {
          if(m>0){
            vector<std::string> ntoken=miutil::split(stokens[0], 0, ";");
            ColourShading cs(ntoken[0]);
            if(ntoken.size()==2 && miutil::is_int(ntoken[1])){
              po.palettecolours = cs.getColourShading(atoi(ntoken[1].c_str()));
            }
            else {
              po.palettecolours = cs.getColourShading();
            }
          }
          if(m>1){
            vector<std::string> ntoken=miutil::split(stokens[1], 0, ";");
            ColourShading cs(ntoken[0]);
            if(ntoken.size()==2 && miutil::is_int(ntoken[1]))
              po.palettecolours_cold = cs.getColourShading(atoi(ntoken[1].c_str()));
            else
              po.palettecolours_cold = cs.getColourShading();
          }
        }

      } else if (key==key_filepalette){
        po.filePalette = value;

      } else if (key==key_linetype){
        po.linetype = Linetype(value);
        po.linetypes.clear();
        po.linetypes.push_back(Linetype(value));

      } else if (key==key_linetype_2){
        po.linetype_2 = Linetype(value);
        po.linetypes.push_back(Linetype(value));

      } else if (key==key_linetypes){
        const vector<string> stokens = miutil::split(value, 0, ",");
        BOOST_FOREACH(const string& l, stokens) {
          po.linetypes.push_back(Linetype(l));
        }

      } else if (key==key_linewidth){
        if (miutil::is_int(value)){
          po.linewidth= atoi(value.c_str());
          po.linewidths.clear();
          po.linewidths.push_back(atoi(value.c_str()));
        }
        else result=false;

      } else if (key==key_linewidth_2){
        if (miutil::is_int(value)){
          po.linewidth_2= atoi(value.c_str());
          po.linewidths.push_back(atoi(value.c_str()));
        }
        else result=false;

      } else if (key==key_linewidths){
        po.linewidths.clear();
        po.linewidths= po.intVector(value);
        if (po.linewidths.size()==0) result= false;

      } else if (key==key_patterns){
        if(value!=OFF) {
          po.patternname = value;
          po.contourShading=1;
        }
        if(miutil::contains(value, ","))
          po.patterns = miutil::split(value, 0, ",");
        else {
          po.patterns = Pattern::getPatternInfo(value);
        }

      } else if (key==key_lineinterval){
        if (miutil::is_number(value)) {
          po.lineinterval= atof(value.c_str());
          po.linevalues.clear();
          po.loglinevalues.clear();
        } else {
          result=false;
        }

      } else if (key==key_lineinterval_2){
        if (miutil::is_number(value)) {
          po.lineinterval_2= atof(value.c_str());
          po.linevalues_2.clear();
          po.loglinevalues_2.clear();
        } else {
          result=false;
        }

      } else if (key==key_minvalue){
        if (miutil::is_number(value)){
          po.minvalue = atof(value.c_str());
        }

      } else if (key==key_maxvalue){
        if (miutil::is_number(value)){
          po.maxvalue = atof(value.c_str());
        }

      } else if (key==key_minvalue_2){
        if (miutil::is_number(value)) {
          po.minvalue_2 = atof(value.c_str());
        }

      } else if (key==key_maxvalue_2){
        if (miutil::is_number(value)){
          po.maxvalue_2 = atof(value.c_str());
        }

      } else if (key==key_colourcut){
        if (miutil::is_number(value))
          po.colourcut = atoi(value.c_str());

      } else if (key==key_linevalues){
        po.linevalues = autoExpandFloatVector(value);
        if (not checkFloatVector(po.linevalues))
          result = false;

      } else if (key==key_linevalues_2){
        po.linevalues_2 = autoExpandFloatVector(value);
        if (not checkFloatVector(po.linevalues_2))
          result = false;

      } else if (key==key_loglinevalues){
        po.loglinevalues = po.floatVector(value);
        if (not checkFloatVector(po.loglinevalues))
          result = false;

      } else if (key==key_loglinevalues_2){
        po.loglinevalues_2 = po.floatVector(value);
        if (not checkFloatVector(po.loglinevalues_2))
          result = false;

      } else if (key==key_limits){
        po.limits = autoExpandFloatVector(value);
        if (po.limits.size()==0) result= false;

      } else if (key==key_values){
        po.values= po.floatVector(value);
        if (po.values.size()==0) result= false;

      } else if (key==key_extremeType){
        // this version: should be "L+H" or "C+W"...
        po.extremeType= value;

      } else if (key==key_extremeSize){
        if (miutil::is_number(value))
          po.extremeSize= atof(value.c_str());
        else result=false;

      } else if (key==key_extremeRadius){
        if (miutil::is_number(value))
          po.extremeRadius= atof(value.c_str());
        else result=false;

      } else if (key==key_extremeLimits){
        po.extremeLimits = autoExpandFloatVector(value);
        if (po.extremeLimits.size()==0) result= false;

      } else if (key==key_lineSmooth){
        if (miutil::is_int(value))
          po.lineSmooth= atoi(value.c_str());
        else result=false;

      } else if (key==key_fieldSmooth){
        if (miutil::is_int(value))
          po.fieldSmooth= atoi(value.c_str());
        else result=false;

      } else if (key==key_frame){
        if (miutil::is_int(value)) {
          po.frame= atoi(value.c_str());
          if (po.frame<0) po.frame=0;
          if (po.frame> 3) po.frame= 3;
        } else result=false;

      } else if (key==key_zeroLine){
        if (miutil::is_int(value)) {
          po.zeroLine= atoi(value.c_str());
          if (po.zeroLine<-1) po.zeroLine=-1;
          if (po.zeroLine> 1) po.zeroLine= 1;
        } else result=false;

      } else if (key==key_valueLabel){
        if (miutil::is_int(value)) {
          po.valueLabel= atoi(value.c_str());
          if (po.valueLabel<0) po.valueLabel= 0;
          if (po.valueLabel>1) po.valueLabel= 1;
        } else result=false;

      } else if (key==key_labelSize){
        if (miutil::is_number(value)) {
          po.labelSize= atof(value.c_str());
          if (po.labelSize<0.2) po.labelSize= 0.2;
          if (po.labelSize>5.0) po.labelSize= 5.0;
        } else result=false;

      } else if (key==key_gridValue){
        if (miutil::is_int(value)) {
          po.gridValue= atoi(value.c_str());
          if (po.gridValue<-1) po.gridValue=-1;
          if (po.gridValue> 1) po.gridValue= 1;
        } else result=false;

      } else if (key==key_gridLines){
        if (miutil::is_int(value)) {
          po.gridLines= atoi(value.c_str());
          if (po.gridLines<0) po.gridLines= 0;
        } else result=false;

      } else if (key==key_gridLinesMax){
        if (miutil::is_int(value)) {
          po.gridLinesMax= atoi(value.c_str());
          if (po.gridLinesMax<0) po.gridLinesMax= 0;
        } else result=false;

      } else if (key==key_plottype){
        value= miutil::to_lower(value);
        if (value==fpt_contour         || value==fpt_value ||
            value==fpt_contour2 ||
            value==fpt_alpha_shade     || value==fpt_symbol     ||
            value==fpt_alarm_box       || value==fpt_fill_cell  ||
            value==fpt_wind            || value==fpt_vector     ||
            value==fpt_wind_temp_fl    || value==fpt_wind_value ||
            value==fpt_direction       || value==fpt_frame)
        {
          po.plottype= value;
        } else if(value == "wind_colour") {
          po.plottype = fpt_wind;
        } else if(value == "direction_colour") {
          po.plottype = fpt_direction;
        } else if(value == "box_pattern") {
          po.plottype = fpt_contour;
        } else if(value == "layer") {
          po.plottype = fpt_value;
        } else if(value == "wind_number") {
          po.plottype = fpt_wind_value;
        } else  if(value == "number") {
          po.plottype = fpt_value;
            METLIBS_LOG_DEBUG("po.plottype ="<< value);
        } else {
          result= false;
        }

      } else if (key==key_rotatevectors){
        if (miutil::is_int(value))
          po.rotateVectors= atoi(value.c_str());
        else result=false;

      } else if (key==key_discontinuous){
        if (miutil::is_int(value))
          po.discontinuous= atoi(value.c_str());
        else result=false;

      } else if (key==key_table){
        if (miutil::is_int(value))
          po.table= atoi(value.c_str());
        else result=false;

      } else if (key==key_alpha){
        if (miutil::is_int(value))
          po.alpha= atoi(value.c_str());
        else result=false;

      } else if (key==key_repeat){
        if (miutil::is_int(value))
          po.repeat= atoi(value.c_str());
        else result=false;

      } else if (key==key_classes){
        po.classSpecifications= value;
      } else if (key==key_basevalue){
        if (miutil::is_number(value))
          po.base= atof(value.c_str());
        else result=false;

      } else if (key==key_basevalue_2){
        if (miutil::is_number(value))
          po.base_2= atof(value.c_str());
        else result=false;

      } else if (key==key_density){
        if (miutil::is_int(value)) {
          po.density= atoi(value.c_str());
        } else {
          po.density=0;
          size_t pos1 = value.find_first_of("(")+1;
          size_t pos2 = value.find_first_of(")");
          if ( pos2 > pos1 ) {
          std::string tmp = value.substr(pos1,(pos2-pos1));
          if (miutil::is_number(tmp) ) {
            po.densityFactor= atof(tmp.c_str());
          }
          }
        }

      } else if (key==key_densityfactor){
        if (miutil::is_number(value)) {
          po.densityFactor= atof(value.c_str());
        } else {
          result=false;
        }

      } else if (key==key_vectorunit){
        if (miutil::is_number(value))
          po.vectorunit= atof(value.c_str());
        else result=false;

      } else if (key==key_vectorunitname){
        po.vectorunitname= value;
        miutil::remove(po.vectorunitname, '"');

      } else if (key==key_vectorscale_x) {
        if (miutil::is_number(value))
          po.vectorscale_x = miutil::to_float(value);
        else result=false;

      } else if (key==key_vectorscale_y) {
        if (miutil::is_number(value))
          po.vectorscale_y = miutil::to_float(value);
        else result=false;

      } else if (key==key_vectorthickness){
        if (miutil::is_number(value))
          po.vectorthickness = std::max(0.0f, miutil::to_float(value));
        else result=false;

      } else if (key==key_forecastLength){
        po.forecastLength= po.intVector(value);
        if (po.forecastLength.size()==0) result= false;

      } else if (key==key_forecastValueMin){
        po.forecastValueMin= po.floatVector(value);
        if (po.forecastValueMin.size()==0) result= false;

      } else if (key==key_forecastValueMax){
        po.forecastValueMax= po.floatVector(value);
        if (po.forecastValueMax.size()==0) result= false;

      } else if (key==key_undefMasking){
        if (miutil::is_int(value))
          po.undefMasking= atoi(value.c_str());
        else result=false;

      } else if (key==key_undefColour){
        const Colour c(value);
        po.undefColour= c;

      } else if (key==key_undefLinewidth){
        if (miutil::is_int(value))
          po.undefLinewidth= atoi(value.c_str());
        else result=false;

      } else if (key==key_undefLinetype){
        po.undefLinetype= Linetype(value);

      } else if (key==key_polystyle){
        if (value=="fill") po.polystyle= poly_fill;
        else if (value=="border") po.polystyle= poly_border;
        else if (value=="both") po.polystyle= poly_both;
        else if (value=="none") po.polystyle= poly_none;

      } else if (key==key_arrowstyle){ // warning: only arrow_wind_arrow implemented yet
        if (value=="wind") po.arrowstyle= arrow_wind;
        else if (value=="wind_arrow") po.arrowstyle= arrow_wind_arrow;
        else if (value=="wind_colour") po.arrowstyle= arrow_wind_colour;
        else if (value=="wind_value") po.arrowstyle= arrow_wind_value;

      } else if (key==key_h_alignment){
        if (value=="left") po.h_align= align_left;
        else if (value=="right") po.h_align= align_right;
        else if (value=="center") po.h_align= align_center;

      } else if (key==key_v_alignment){
        if (value=="top") po.v_align= align_top;
        else if (value=="bottom") po.v_align= align_bottom;
        else if (value=="center") po.v_align= align_center;

      } else if (key==key_alignX){
        po.alignX= atoi(value.c_str());

      } else if (key==key_alignY){
        po.alignY= atoi(value.c_str());

      } else if (key==key_fontname){
        po.fontname= value;

      } else if (key==key_fontface){
        po.fontface= value;

      } else if (key==key_fontsize){
        if (miutil::is_number(value))
          po.fontsize= atof(value.c_str());
        else result=false;

      } else if (key==key_precision){
        if (miutil::is_number(value))
          po.precision= atof(value.c_str());
        else result=false;

      } else if (key==key_dimension){
        if (miutil::is_int(value))
          po.dimension= miutil::to_int(value);
        else result=false;

      } else if (key==key_enabled){
        po.enabled= (value == TRUE);
      } else if (key==key_fdescr){
        po.fdescr=miutil::split(value, ":");
      } else if (key==key_fname){
        po.fname=value;
      } else if (key==key_overlay){
        po.overlay=atoi(value.c_str());
      } else if (key==key_contourShape){
        po.contourShape = (value == TRUE);
      } else if (key==key_shapefilename){
        po.shapefilename=value.c_str();
      } else if (key==key_unit || key==key_units){
        po.unit=value.c_str();
      } else if (key==key_legendunits){
        po.legendunits=value.c_str();
      } else if (key==key_legendtitle){
        po.legendtitle=value.c_str();
      } else if (key==key_antialiasing){
        po.antialiasing=(value == TRUE);
      } else if (key==key_use_stencil){
        po.use_stencil=(value == TRUE);
      } else if (key==key_update_stencil){
        po.update_stencil=(value == TRUE);
      } else if (key==key_plot_under){
        po.plot_under=(value == TRUE);
      } else if (key==key_maxDiagonalInMeters){
        po.maxDiagonalInMeters=atof(value.c_str());

      } else {
       origStr += " " + key + "=" + value;
      }
    } else {
      origStr += " " + etokens[0];
    }

  }

  if (po.linetypes.size() == 0 ) {
    po.linetypes.push_back(po.linetype);
  }
  if (po.linewidths.size() == 0 ) {
    po.linewidths.push_back(po.linewidth);
  }

  if (returnMergedOptionString) {
    if ( origStr.empty() ) {
      optstr = po.toString();
    } else {
      optstr = origStr + " " + po.toString();
    }
  }

  return result;
}

bool PlotOptions::parsePlotOption(const std::string& optstr, PlotOptions& po)
{
  std::string optstring(optstr);
  return parsePlotOption(optstring, po, false);
}


// update static fieldplotoptions
bool PlotOptions::updateFieldPlotOptions(const std::string& name,
    const std::string& optstr)
{
  std::string tmpOpt = optstr;
  return parsePlotOption(tmpOpt, fieldPlotOptions[name]);
}

// fill a fieldplotoption from static map, and substitute values
// from a string containing plotoptions
bool PlotOptions::fillFieldPlotOptions(std::string name,
    std::string& optstr,
    PlotOptions& po)
{
  removeSuffix(name);

  map<std::string,PlotOptions>::iterator p;
  // if field-spec not found, simply add a new (for default CONTOUR plot)
  if ((p=fieldPlotOptions.find(name))
      != fieldPlotOptions.end())
    po= p->second;
  else
    fieldPlotOptions[name]= po;

  parsePlotOption(optstr,po,true);

  return true;
}

// fill in values in an int vector
vector<int> PlotOptions::intVector(const std::string& str) const {
  vector<int> v;
  bool error= false;
  vector<std::string> stokens= miutil::split(str, 0, ",");
  int m= stokens.size();
  for (int j=0; j<m; j++){
    if (miutil::is_int(stokens[j]))
      v.push_back(atoi(stokens[j].c_str()));
    else
      error= true;
  }
  if (error) v.clear();
  return v;
}


// fill in values in a float vector
vector<float> PlotOptions::floatVector(const std::string& str) const
{
  vector<float> values;
  const vector<std::string> tokens= miutil::split(str, ",");
  BOOST_FOREACH(const string& t, tokens) {
    if (miutil::is_number(t))
      values.push_back(miutil::to_double(t));
    else
      return vector<float>();
  }
  return values;
}

// fill in values and "..." (1,2,3,...10) in a float vector
vector<float> PlotOptions::autoExpandFloatVector(const std::string& str)
{
  vector<float> values;

  const vector<std::string> stokens = miutil::split(str, ",");
  BOOST_FOREACH(const string& t, stokens) {
    if (miutil::is_number(t)) {
      const double v = miutil::to_double(t);
      values.push_back(v);
      continue;
    }

    // try to add a whole interval (10,20,...50)

    if (values.size() < 2) {
      METLIBS_LOG_DEBUG("need 2 values yet to define step size");
      return vector<float>();
    }

    const ssize_t no_dot = t.find_first_not_of(".");
    if (no_dot < 2) {
      METLIBS_LOG_DEBUG("not enough dots");
      return vector<float>();
    }

    const string after_dots = t.substr(no_dot);
    if (not miutil::is_number(after_dots)) {
      METLIBS_LOG_DEBUG("no number after dots (" << after_dots << ")");
      return vector<float>();
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

void PlotOptions::removeSuffix(std::string& name)
{

  int n = suffix.size();
  for(int i=0; i<n; i++)
    miutil::replace(name, suffix[i],"");

}


void PlotOptions::getAllFieldOptions(vector<std::string> fieldNames,
    map<std::string,std::string>& fieldoptions)
{

  // The selected PlotOptions elements are used to activate elements
  // in the FieldDialog (any remaining will be used unchanged from setup)
  // Also return any field prefixes and suffixes used.

  fieldoptions.clear();

  PlotOptions po;


  int n= fieldNames.size();

  for (int i=0; i<n; i++) {
    getFieldPlotOptions(fieldNames[i], po);
    fieldoptions[fieldNames[i]]= po.toString();
  }

}

static void addOption(std::ostream& ostr, const std::string& key, bool value)
{
  ostr << ' ' << key << '=' << (value ? TRUE : FALSE);
}

static void addOption(std::ostream& ostr, const std::string& key, int value)
{
  ostr << ' ' << key << '=' << value;
}

static void addOption(std::ostream& ostr, const std::string& key, float value)
{
  ostr << ' ' << key << '=' << value;
}

static void addOption(std::ostream& ostr, const std::string& key, const std::string& value)
{
  ostr << ' ' << key << '=' << value;
}

static void addOption(std::ostream& ostr, const std::string& key,
    const std::vector<float>& values)
{
  std::ostringstream ov;
  std::vector<float>::const_iterator it = values.begin();
  ov << *it++;
  for (; it != values.end(); ++it)
    ov << ',' << *it;
  addOption(ostr, key, ov.str());
}

std::string PlotOptions::toString()
{
  Linetype defaultLinetype= Linetype::getDefaultLinetype();
  if (linetype.name.empty())      linetype= defaultLinetype;
  if (undefLinetype.name.empty()) undefLinetype= defaultLinetype;

  ostringstream ostr;

  addOption(ostr, key_colour, options_1 ? linecolour.Name() : OFF);

  addOption(ostr, key_plottype, plottype);
  addOption(ostr, key_linetype, linetype.name);
  addOption(ostr, key_linewidth, linewidth);
  addOption(ostr, key_basevalue, base);

  if(minvalue>-fieldUndef) {
    addOption(ostr, key_minvalue, minvalue);
  }
  if(maxvalue < fieldUndef) {
    addOption(ostr, key_maxvalue, maxvalue);
  }

  if (frame>=0)
    addOption(ostr, key_frame, frame);
  if (zeroLine>=0)
    addOption(ostr, key_zeroLine, zeroLine);

  if (colours.size() > 2) {
    std::ostringstream ov;
    ov << colours.front().Name();
    for (size_t i=1; i<colours.size(); ++i)
      ov << ',' << colours.at(i).Name();
    addOption(ostr, key_colours, ov.str());
  }

  if(dimension == 1){

    if (linevalues.empty() && loglinevalues.empty()) {
      addOption(ostr,  key_lineinterval, lineinterval);
    } else if (not linevalues.empty()) {
      addOption(ostr, key_linevalues, linevalues);
    } else if (not loglinevalues.empty()) {
      addOption(ostr, key_loglinevalues, loglinevalues);
    }

    addOption(ostr, key_extremeType, extremeType);
    addOption(ostr, key_extremeSize, extremeSize);
    addOption(ostr, key_extremeRadius, extremeRadius);
    addOption(ostr, key_palettecolours, !palettecolours.empty() ? palettename : OFF);

    if( !filePalette.empty() ) {
      addOption(ostr, key_filepalette, filePalette);
    }

    addOption(ostr, key_patterns, !patterns.empty() ? patternname : OFF);

    addOption(ostr, key_table, table);
    addOption(ostr, key_repeat, repeat);
    addOption(ostr, key_valueLabel, valueLabel);
  }

  if(plottype == fpt_vector || plottype == fpt_direction || plottype == fpt_wind
      || plottype == fpt_wind_temp_fl || plottype == fpt_wind_value ) {
    addOption(ostr, key_vectorunit, vectorunit);
  }

  if (discontinuous==0) {
    addOption(ostr, key_lineSmooth, lineSmooth);
    addOption(ostr, key_fieldSmooth, fieldSmooth);
  }

  addOption(ostr, key_labelSize, labelSize);
  addOption(ostr, key_gridLines, gridLines);
  addOption(ostr, key_gridLinesMax, gridLinesMax);
  addOption(ostr, key_undefMasking, undefMasking);
  addOption(ostr, key_undefColour, undefColour.Name());
  addOption(ostr, key_undefLinewidth, undefLinewidth);
  addOption(ostr, key_undefLinetype, undefLinetype.name);

  if (gridValue>=0)
    addOption(ostr, key_gridValue, gridValue);

  if(!options_2){
    addOption(ostr, key_colour_2, OFF);
  } else{
    addOption(ostr, key_colour_2, linecolour_2.Name());
    // addOption(ostr, key_lineinterval_2, lineinterval_2);
    addOption(ostr, key_linewidth_2, linewidth_2);
    addOption(ostr, key_basevalue_2, base_2);

    if (linevalues_2.empty() && loglinevalues_2.empty()) {
      addOption(ostr, key_lineinterval_2, lineinterval_2);
    } else if (!linevalues_2.empty()) {
      addOption(ostr, key_linevalues_2, linevalues_2);
    } else if (loglinevalues_2.size() != 0 ) {
      addOption(ostr, key_loglinevalues_2, loglinevalues_2);
    }

    if(minvalue_2>-fieldUndef) {
      addOption(ostr, key_minvalue_2, minvalue_2);
    }
    if(maxvalue_2 < fieldUndef) {
      addOption(ostr, key_maxvalue_2, maxvalue_2);
    }
  }

  addOption(ostr, key_dimension, dimension);

  if(!unit.empty()) {
    addOption(ostr, key_unit, unit);
  }

  if(!legendunits.empty()) {
    addOption(ostr, key_legendunits, legendunits);
  }

  if(!legendtitle.empty()) {
    addOption(ostr, key_legendtitle, legendtitle);
  }

  if( precision > 0 ) {
    addOption(ostr, key_precision, precision);
  }

  if (antialiasing)
    addOption(ostr, key_antialiasing, antialiasing);

  if (use_stencil)
    addOption(ostr, key_use_stencil, use_stencil);

  if (update_stencil)
    addOption(ostr, key_update_stencil, update_stencil);

  if (!enabled)
    addOption(ostr, key_enabled, enabled);

  return ostr.str();
}


bool PlotOptions::getFieldPlotOptions(const std::string& name, PlotOptions& po)
{
  map<std::string,PlotOptions>::iterator p;
  if ((p=fieldPlotOptions.find(name))
      != fieldPlotOptions.end()){
    po= p->second;

  } else {
    fieldPlotOptions[name]= po;
  }
  return true;
}
