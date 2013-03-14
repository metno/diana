/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diPlotOptions.cc 3893 2012-07-05 12:09:33Z lisbethb $

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diPlotOptions.h"
#include "diColourShading.h"
#include "diPattern.h"
#include <diField/diField.h>

//#define DEBUGPRINT 
using namespace std;
using namespace miutil;

map<miString,PlotOptions> PlotOptions::fieldPlotOptions;
vector<miString> PlotOptions::suffix;
vector< vector <miString> > PlotOptions::plottypes;
map< miutil::miString, miutil::miString > PlotOptions::enabledOptions;

PlotOptions::PlotOptions():
  options_1(true),options_2(false),
  textcolour(BlackC), linecolour(BlackC), linecolour_2(BlackC),
  fillcolour(BlackC), bordercolour(BlackC), table(0),alpha(255), repeat(0),
  linewidth(1), linewidth_2(1), colourcut(1), lineinterval(10.0), lineinterval_2(10.0),
  base(0.0), base_2(0.0), minvalue(-fieldUndef), minvalue_2(-fieldUndef),
  maxvalue(fieldUndef), maxvalue_2(fieldUndef), density(0), densityFactor(1.0),
  vectorunit(1.0), vectorunitname("m/s"),
  extremeType("None"), extremeSize(1.0), extremeRadius(1.0),
  lineSmooth(0), fieldSmooth(0), frame(1), zeroLine(-1), valueLabel(1), labelSize(1.0),
  gridValue(0),gridLines(0), gridLinesMax(0),
  undefMasking(0), undefColour(WhiteC), undefLinewidth(1),
  plottype(fpt_contour), rotateVectors(1), discontinuous(0), contourShading(0),
  polystyle(poly_fill), arrowstyle(arrow_wind), h_align(align_left), v_align(align_bottom),
  alignX(0), alignY(0),
  fontname("SCALEFONT"), fontface("NORMAL"), fontsize(10.0), precision(0),
  dimension(1), enabled(true), overlay(0), contourShape(0), tableHeader(true),
  antialiasing(false)
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
  vector< miString> plottypes_all;
  plottypes_all.push_back(fpt_contour);
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

  vector< miString> plottypes_1dim;
  plottypes_1dim.push_back(fpt_contour);
  plottypes_1dim.push_back(fpt_value);
  plottypes_1dim.push_back(fpt_symbol);
  plottypes_1dim.push_back(fpt_alpha_shade);
  plottypes_1dim.push_back(fpt_alarm_box);
  plottypes_1dim.push_back(fpt_fill_cell);
  plottypes_1dim.push_back(fpt_direction       );
  plottypes_1dim.push_back(fpt_frame);
  plottypes.push_back(plottypes_1dim);

  vector< miString> plottypes_2dim;
  plottypes_2dim.push_back(fpt_wind            );
  plottypes_2dim.push_back(fpt_vector          );
  plottypes_2dim.push_back(fpt_value);
  plottypes_2dim.push_back(fpt_frame);
  plottypes.push_back(plottypes_2dim);

  vector< miString> plottypes_3dim;
  plottypes_3dim.push_back(fpt_wind            );
  plottypes_3dim.push_back(fpt_vector          );
  plottypes_3dim.push_back(fpt_value);
  plottypes_3dim.push_back(fpt_wind_temp_fl    );
  plottypes_3dim.push_back(fpt_wind_value    );
  plottypes_3dim.push_back(fpt_frame);
  plottypes.push_back(plottypes_3dim);

  vector< miString> plottypes_4dim;
  plottypes_4dim.push_back(fpt_value);
  plottypes_4dim.push_back(fpt_frame);
  plottypes.push_back(plottypes_4dim);

  enabledOptions[fpt_contour] = " extreme line shading contour font";
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
bool PlotOptions::parsePlotOption( miString& optstr, PlotOptions& po,
    bool returnMergedOptionString){
  // defined keywords:
  //------------------------------------------
  // options1: off,isoline
  const miString key_options_1 = "options.1";
  // options1: off,isoline,shading
  const miString key_options_2 = "options.2";
  // colour:     main colour
  const miString key_colour= "colour";
  // colour:     main colour
  const miString key_colour_2= "colour_2";
  // tcolour:    text colour
  const miString key_tcolour= "tcolour";
  // lcolour:    line colour
  const miString key_lcolour= "lcolour";
  // lcolour:    line colour
  const miString key_lcolour_2= "lcolour_2";
  // fcolour:    fill colour
  const miString key_fcolour= "fcolour";
  // bcolour:    pattern colour
  const miString key_pcolour= "patterncolour";
  // bcolour:    border colour
  const miString key_bcolour= "bcolour";
  // colours:    list of colours
  const miString key_colours= "colours";
  // colours:    list of colours in palette
  const miString key_palettecolours= "palettecolours";
  // linetype:   linetype
  const miString key_linetype= "linetype";
  // linetype:   linetype
  const miString key_linetype_2= "linetype_2";
  // linetypes:  list of linetypes
  const miString key_linetypes= "linetypes";
  // linewidth:  linewidth
  const miString key_linewidth= "linewidth";
  // linewidth:  linewidth
  const miString key_linewidth_2= "linewidth_2";
  // linewidths: list of linewidths
  const miString key_linewidths= "linewidths";
  // patterns:  list of patterns
  const miString key_patterns= "patterns";
  // line.interval:
  const miString key_lineinterval= "line.interval";
  // line.interval:
  const miString key_lineinterval_2= "line.interval_2";
  // value.range: minValue
  const miString key_minvalue= "minvalue";
  // value.range: maxValue
  const miString key_maxvalue= "maxvalue";
  // value.range: minValue
  const miString key_minvalue_2= "minvalue_2";
  // value.range: maxValue
  const miString key_maxvalue_2= "maxvalue_2";
  // colourcut 0=off 1=on
  const miString key_colourcut= "colourcut";
  // line.values
  const miString key_linevalues= "line.values";
  // logarithmic line.values
  const miString key_loglinevalues= "log.line.values";
  // limits:
  const miString key_linevalues_2= "line.values_2";
  // logarithmic line.values
  const miString key_loglinevalues_2= "log.line.values_2";
  // limits:
  const miString key_limits= "limits";
  // values:
  const miString key_values= "values";
  // extreme (min,max)
  const miString key_extremeType= "extreme.type";
  const miString key_extremeSize= "extreme.size";
  const miString key_extremeRadius= "extreme.radius";
  const miString key_extremeLimits= "extreme.limits";
  // contour line smoothing
  const miString key_lineSmooth= "line.smooth";
  // field smoothing
  const miString key_fieldSmooth= "field.smooth";
  // plot frame around complete field area  ( 0=off 1=on)
  const miString key_frame= "frame";
  // zero line drawing (-1=no_option 0=off 1=on)
  const miString key_zeroLine= "zero.line";
  // labels on isolines (0=off 1=on)
  const miString key_valueLabel= "value.label";
  // rel. label size
  const miString key_labelSize= "label.size";
  // show grid values (-1=no_option 0=off 1=on)
  const miString key_gridValue= "grid.value";
  // show grid lines (0=off N=density)
  const miString key_gridLines= "grid.lines";
  // show max grid lines (0=no limit N=maximum, skip if more)
  const miString key_gridLinesMax= "grid.lines.max";
  // field plottype:
  const miString key_plottype= "plottype";
  // discontinuous field
  const miString key_rotatevectors= "rotate.vectors";
  // discontinuous field
  const miString key_discontinuous= "discontinuous";
  // table
  const miString key_table= "table";
  // alpha shading
  const miString key_alpha= "alpha";
  // repeat palette
  const miString key_repeat= "repeat";
  // class specifications
  const miString key_classes= "classes";
  // base value
  const miString key_basevalue= "base";
  // base value
  const miString key_basevalue_2= "base_2";
  // (vector) density
  const miString key_density= "density";
  // (vector) density - auto*factor
  const miString key_densityfactor= "density.factor";
  // vector unit
  const miString key_vectorunit= "vector.unit";
  // vector unit name
  const miString key_vectorunitname= "vector.unit.name";
  // forecast length
  const miString key_forecastLength= "forecast.length";
  // forecast value min
  const miString key_forecastValueMin= "forecast.value.min";
  // forecast value max
  const miString key_forecastValueMax= "forecast.value.max";
  // undefMasking and options
  const miString key_undefMasking=   "undef.masking";
  const miString key_undefColour=    "undef.colour";
  const miString key_undefLinewidth= "undef.linewidth";
  const miString key_undefLinetype=  "undef.linetype";
  // polyStyle
  const miString key_polystyle= "polystyle";
  // arrowStyle
  const miString key_arrowstyle= "arrowstyle";
  // h_alignment
  const miString key_h_alignment= "halign";
  // v_alignment
  const miString key_v_alignment= "valign";
  // alignment for plotted numbers
  const miString key_alignX= "alignX";
  // alignment for plotted numbers
  const miString key_alignY= "alignY";
  // fontname
  const miString key_fontname= "font";
  // fontface
  const miString key_fontface= "face";
  // fontsize
  const miString key_fontsize= "fontsize";
  // value precision
  const miString key_precision= "precision";
  // dinesion (scalar=1, vector=2)
  const miString key_dimension= "dim";
  // plot enabled
  const miString key_enabled= "enabled";
  //field description used for plotting
  const miString key_fdescr= "fdesc";
  //field names used for plotting
  const miString key_fname= "fname";
  //plot in overlay buffer
  const miString key_overlay="overlay";
  //contour shape
  const miString key_contourShape="contourShape";
  //Shape filename for output
  const miString key_shapefilename="shapefilename";
  //unit obsolete, use units
  const miString key_unit="unit";
  //units
  const miString key_units="units";
  //anti-aliasing
  const miString key_antialiasing="antialiasing";

  //------------------------------------------


  vector<miString> tokens,etokens,stokens;
  miString key,value;
  int i,j,n,m,l;
  Colour c;
  Linetype linetype;

  miString origStr;

  //float lw;
  bool result=true;

  tokens= optstr.split('"','"');
  n= tokens.size();

  for (i=0; i<n; i++){

    etokens= tokens[i].split("=");
    l= etokens.size();
    if (l>1){
      key= etokens[0];
      value= etokens[1];
//      cerr << "Key:"<<key<< " Value:"<<value<<endl;
      if (value[0]=='\'' && value[value.length()-1]=='\'')
        value= value.substr(1,value.length()-2);

//      if (key==key_fplottype_obsolete && po.plottype== fpt_contour){
//        key=key_fplottype;
////        cerr <<"New key:"<<key<<endl;
//      }

      if (key==key_colour){
        po.colours.clear();
        if(value=="off"){
          po.options_1= false;
        } else {
          po.options_1= true;
          c= Colour(value);
          po.textcolour= c;
          po.linecolour= c;
          po.fillcolour= c;
          po.bordercolour= c;
          po.colours.push_back(c);
        }

      } else if (key==key_colour_2){
        if(value=="off"){
          po.options_2= false;
        } else {
          po.options_2= true;
          c= Colour(value);
          po.linecolour_2= c;
          po.colours.push_back(c);
        }

      } else if (key==key_tcolour){
        c= Colour(value);
        po.textcolour= c;

      } else if (key==key_fcolour){
        c= Colour(value);
        po.fillcolour= c;

      } else if (key==key_pcolour){
        c= Colour(value);
        po.fillcolour= c;

      } else if (key==key_bcolour){
        c= Colour(value);
        po.bordercolour= c;

      } else if (key==key_colours){
        po.colours.clear();
        // 	if(value=="off"){
        // 	  po.options_1= false;
        // 	} else {
        po.options_1= true;
        stokens= value.split(',');
        m= stokens.size();
        for (j=0; j<m; j++){
          c= Colour(stokens[j]);
          po.colours.push_back(c);
        }
        //	}

      } else if (key==key_palettecolours){
        po.palettecolours.clear();
        po.palettecolours_cold.clear();
        if(value!="off")
          po.contourShading=1;
        po.palettename=value;
        stokens= value.split(',');
        m= stokens.size();
        if(m>2){
          for (j=0; j<m; j++){
            c= Colour(stokens[j]);
            po.palettecolours.push_back(c);
          }
        } else {
          if(m>0){
            vector<miString> ntoken=stokens[0].split(";");
            ColourShading cs(ntoken[0]);
            if(ntoken.size()==2 && ntoken[1].isInt()){
              po.palettecolours = cs.getColourShading(atoi(ntoken[1].cStr()));
            }
            else {
              po.palettecolours = cs.getColourShading();
            }
          }
          if(m>1){
            vector<miString> ntoken=stokens[1].split(";");
            ColourShading cs(ntoken[0]);
            if(ntoken.size()==2 && ntoken[1].isInt())
              po.palettecolours_cold = cs.getColourShading(atoi(ntoken[1].cStr()));
            else
              po.palettecolours_cold = cs.getColourShading();
          }
        }

      } else if (key==key_linetype){
        po.linetype = Linetype(value);
        po.linetypes.clear();
        po.linetypes.push_back(value);

      } else if (key==key_linetype_2){
        po.linetype_2 = Linetype(value);
        po.linetypes.push_back(value);

      } else if (key==key_linetypes){
        stokens= value.split(',');
        m= stokens.size();
        for (j=0; j<m; j++){
          po.linetypes.push_back(Linetype(stokens[j]));
        }

      } else if (key==key_linewidth){
        if (value.isInt()){
          po.linewidth= atoi(value.cStr());
          po.linewidths.clear();
          po.linewidths.push_back(atoi(value.cStr()));
        }
        else result=false;

      } else if (key==key_linewidth_2){
        if (value.isInt()){
          po.linewidth_2= atoi(value.cStr());
          po.linewidths.push_back(atoi(value.cStr()));
        }
        else result=false;

      } else if (key==key_linewidths){
        po.linewidths.clear();
        po.linewidths= po.intVector(value);
        if (po.linewidths.size()==0) result= false;

      } else if (key==key_patterns){
        if(value!="off") {
          po.patternname = value;
          po.contourShading=1;
        }
        if(value.contains(","))
          po.patterns = value.split(',');
        else{
          po.patterns = Pattern::getPatternInfo(value);
        }

      } else if (key==key_lineinterval){
        if (value.isNumber()) {
          po.lineinterval= atof(value.cStr());
          po.linevalues.clear();
          po.loglinevalues.clear();
        } else {
          result=false;
        }

      } else if (key==key_lineinterval_2){
        if (value.isNumber()) {
          po.lineinterval_2= atof(value.cStr());
          po.linevalues_2.clear();
          po.loglinevalues_2.clear();
        } else {
          result=false;
        }

      } else if (key==key_minvalue){
        if (value.isNumber()){
          po.minvalue = atof(value.cStr());
        }

      } else if (key==key_maxvalue){
        if (value.isNumber()){
          po.maxvalue = atof(value.cStr());
        }

      } else if (key==key_minvalue_2){
        if (value.isNumber()) {
          po.minvalue_2 = atof(value.cStr());
        }

      } else if (key==key_maxvalue_2){
        if (value.isNumber()){
          po.maxvalue_2 = atof(value.cStr());
        }

      } else if (key==key_colourcut){
        if (value.isNumber())
          po.colourcut = atoi(value.cStr());

      } else if (key==key_linevalues){
        po.linevalues = po.autoExpandFloatVector(value);
        m= po.linevalues.size();
        if (m==0) result= false;
        for (j=1; j<m; j++)
          if (po.linevalues[j-1]>=po.linevalues[j]) result= false;

      } else if (key==key_linevalues_2){
        po.linevalues_2 = po.autoExpandFloatVector(value);
        m= po.linevalues_2.size();
        if (m==0) result= false;
        for (j=1; j<m; j++)
          if (po.linevalues_2[j-1]>=po.linevalues_2[j]) result= false;

      } else if (key==key_loglinevalues){
        po.loglinevalues = po.floatVector(value);
        m= po.loglinevalues.size();
        if (m==0) result= false;
        for (j=1; j<m; j++)
          if (po.loglinevalues[j-1]>=po.loglinevalues[j]) result= false;

      } else if (key==key_loglinevalues_2){
        po.loglinevalues_2 = po.floatVector(value);
        m= po.loglinevalues_2.size();
        if (m==0) result= false;
        for (j=1; j<m; j++)
          if (po.loglinevalues_2[j-1]>=po.loglinevalues_2[j]) result= false;

      } else if (key==key_limits){
        po.limits = po.autoExpandFloatVector(value);
        if (po.limits.size()==0) result= false;

      } else if (key==key_values){
        po.values= po.floatVector(value);
        if (po.values.size()==0) result= false;

      } else if (key==key_extremeType){
        // this version: should be "L+H" or "C+W"...
        po.extremeType= value;

      } else if (key==key_extremeSize){
        if (value.isNumber())
          po.extremeSize= atof(value.cStr());
        else result=false;

      } else if (key==key_extremeRadius){
        if (value.isNumber())
          po.extremeRadius= atof(value.cStr());
        else result=false;

      } else if (key==key_extremeLimits){
        po.extremeLimits = po.autoExpandFloatVector(value);
        if (po.extremeLimits.size()==0) result= false;

      } else if (key==key_lineSmooth){
        if (value.isInt())
          po.lineSmooth= atoi(value.cStr());
        else result=false;

      } else if (key==key_fieldSmooth){
        if (value.isInt())
          po.fieldSmooth= atoi(value.cStr());
        else result=false;

      } else if (key==key_frame){
        if (value.isInt()) {
          po.frame= atoi(value.cStr());
          if (po.frame<0) po.frame=0;
          if (po.frame> 1) po.frame= 1;
        } else result=false;

      } else if (key==key_zeroLine){
        if (value.isInt()) {
          po.zeroLine= atoi(value.cStr());
          if (po.zeroLine<-1) po.zeroLine=-1;
          if (po.zeroLine> 1) po.zeroLine= 1;
        } else result=false;

      } else if (key==key_valueLabel){
        if (value.isInt()) {
          po.valueLabel= atoi(value.cStr());
          if (po.valueLabel<0) po.valueLabel= 0;
          if (po.valueLabel>1) po.valueLabel= 1;
        } else result=false;

      } else if (key==key_labelSize){
        if (value.isNumber()) {
          po.labelSize= atof(value.cStr());
          if (po.labelSize<0.2) po.labelSize= 0.2;
          if (po.labelSize>5.0) po.labelSize= 5.0;
        } else result=false;

      } else if (key==key_gridValue){
        if (value.isInt()) {
          po.gridValue= atoi(value.cStr());
          if (po.gridValue<-1) po.gridValue=-1;
          if (po.gridValue> 1) po.gridValue= 1;
        } else result=false;

      } else if (key==key_gridLines){
        if (value.isInt()) {
          po.gridLines= atoi(value.cStr());
          if (po.gridLines<0) po.gridLines= 0;
        } else result=false;

      } else if (key==key_gridLinesMax){
        if (value.isInt()) {
          po.gridLinesMax= atoi(value.cStr());
          if (po.gridLinesMax<0) po.gridLinesMax= 0;
        } else result=false;

      } else if (key==key_plottype){
        value= value.downcase();
        if (value==fpt_contour         || value==fpt_value ||
            value==fpt_alpha_shade     || value==fpt_symbol ||
            value==fpt_alarm_box       || value==fpt_fill_cell        ||
            value==fpt_wind            || value==fpt_vector          ||
            value==fpt_wind_temp_fl    || value==fpt_wind_value ||
            value==fpt_direction       || value==fpt_frame ) {
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
#ifdef DEBUGPRINT
            cerr<<"........diPlotOptions::parsePlotOption po.plottype ="<< value <<endl;

#endif
        } else {
          result= false;
        }

      } else if (key==key_rotatevectors){
        if (value.isInt())
          po.rotateVectors= atoi(value.cStr());
        else result=false;

      } else if (key==key_discontinuous){
        if (value.isInt())
          po.discontinuous= atoi(value.cStr());
        else result=false;

      } else if (key==key_table){
        if (value.isInt())
          po.table= atoi(value.cStr());
        else result=false;

      } else if (key==key_alpha){
        if (value.isInt())
          po.alpha= atoi(value.cStr());
        else result=false;

      } else if (key==key_repeat){
        if (value.isInt())
          po.repeat= atoi(value.cStr());
        else result=false;

      } else if (key==key_classes){
        po.classSpecifications= value;
      } else if (key==key_basevalue){
        if (value.isNumber())
          po.base= atof(value.cStr());
        else result=false;

      } else if (key==key_basevalue_2){
        if (value.isNumber())
          po.base_2= atof(value.cStr());
        else result=false;

      } else if (key==key_density){
        if (value.isInt()) {
          po.density= atoi(value.cStr());
        } else {
          po.density=0;
          size_t pos1 = value.find_first_of("(")+1;
          size_t pos2 = value.find_first_of(")");
          if ( pos2 > pos1 ) {
          miString tmp = value.substr(pos1,(pos2-pos1));
          if (tmp.isNumber() ) {
            po.densityFactor= atof(tmp.c_str());
          }
          }
        }

      } else if (key==key_densityfactor){
        if (value.isNumber()) {
          po.densityFactor= atof(value.cStr());
        } else {
          result=false;
        }

      } else if (key==key_vectorunit){
        if (value.isNumber())
          po.vectorunit= atof(value.cStr());
        else result=false;

      } else if (key==key_vectorunitname){
        po.vectorunitname= value;
        po.vectorunitname.remove('"');

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
        if (value.isInt())
          po.undefMasking= atoi(value.cStr());
        else result=false;

      } else if (key==key_undefColour){
        c= Colour(value);
        po.undefColour= c;

      } else if (key==key_undefLinewidth){
        if (value.isInt())
          po.undefLinewidth= atoi(value.cStr());
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
        po.alignX= atoi(value.cStr());

      } else if (key==key_alignY){
        po.alignY= atoi(value.cStr());

      } else if (key==key_fontname){
        po.fontname= value;

      } else if (key==key_fontface){
        po.fontface= value;

      } else if (key==key_fontsize){
        if (value.isNumber())
          po.fontsize= atof(value.cStr());
        else result=false;

      } else if (key==key_precision){
        if (value.isNumber())
          po.precision= atof(value.cStr());
        else result=false;

      } else if (key==key_dimension){
        if (value.isInt())
          po.dimension= value.toInt();
        else result=false;

      } else if (key==key_enabled){
        po.enabled= (value == "true");
      } else if (key==key_fdescr){
        po.fdescr=value.split(":");
      } else if (key==key_fname){
        po.fname=value;
      } else if (key==key_overlay){
        po.overlay=atoi(value.cStr());
      } else if (key==key_contourShape){
        po.contourShape = (value == "true");
      } else if (key==key_shapefilename){
        po.shapefilename=value.cStr();
      } else if (key==key_unit || key==key_units){
        po.unit=value.cStr();
      } else if (key==key_antialiasing){
        po.antialiasing=(value == "true");

      } else {
       origStr += " " + key + "=" + value;
      }
    } else {
      origStr += " " + etokens[0];
    }

  }

  if ( returnMergedOptionString ) {
    optstr = origStr + " " + po.toString();
  }

  return result;
}

// update static fieldplotoptions
bool PlotOptions::updateFieldPlotOptions(const miString& name,
    const miString& optstr)
{
#ifdef DEBUGPRINT
  cerr<<":::::::::PlotOptions::updateFieldPlotOptions"<<endl;
  cerr<<":::::::::name: "<< name << "   *******  optstr: "<<optstr<<endl;
#endif
  miString tmpOpt = optstr;
  return parsePlotOption(tmpOpt,fieldPlotOptions[name]);
}

// fill a fieldplotoption from static map, and substitute values
// from a string containing plotoptions
bool PlotOptions::fillFieldPlotOptions(miString name,
    miString& optstr,
    PlotOptions& po)
{
  removeSuffix(name);

  map<miString,PlotOptions>::iterator p;
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
vector<int> PlotOptions::intVector(const miString& str) const {
  vector<int> v;
  bool error= false;
  vector<miString> stokens= str.split(',');
  int m= stokens.size();
  for (int j=0; j<m; j++){
    if (stokens[j].isInt())
      v.push_back(atoi(stokens[j].c_str()));
    else
      error= true;
  }
  if (error) v.clear();
  return v;
}


// fill in values in a float vector
vector<float> PlotOptions::floatVector(const miString& str) const {
  vector<float> v;
  bool error= false;
  vector<miString> stokens= str.split(',');
  int m= stokens.size();
  for (int j=0; j<m; j++){
    if (stokens[j].isNumber())
      v.push_back(atof(stokens[j].c_str()));
    else
      error= true;
  }
  if (error) v.clear();
  return v;
}

// fill in values and "..." (1,2,3,...10) in a float vector
vector<float> PlotOptions::autoExpandFloatVector(const miString& str) const {
  vector<float> v;
  bool error= false;

  vector<miString> stokens= str.split(',');
  int m= stokens.size();
  int k;
  float val,delta,step;

  for (int j=0; j<m; j++){
    if (stokens[j].isNumber()){
      v.push_back(atof(stokens[j].cStr()));
    } else if (stokens[j].substr(0,2)==".." && v.size()>1){
      // add a whole interval (10,20,...50)
      stokens[j].replace("."," ");
      stokens[j].trim();
      if (stokens[j].isNumber()){
        val= atof(stokens[j].cStr());
        k= v.size();
        delta= v[k-1]-v[k-2];
        // keep previous step, example above: 10,20,30,40,50
        if (delta>0.0){
          for (step=v[k-1]+delta; step<=val; step+=delta)
            v.push_back(step);
        } else error= true;
      } else error= true;
    } else error= true;
  }

  if (error) v.clear();  // v.size()==0 means an error

  return v;
}

void PlotOptions::removeSuffix(miString& name)
{

  int n = suffix.size();
  for(int i=0; i<n; i++)
    name.replace(suffix[i],"");

}


void PlotOptions::getAllFieldOptions(vector<miString> fieldNames,
    map<miString,miString>& fieldoptions)
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


miString PlotOptions::toString()
{

  Linetype defaultLinetype= Linetype::getDefaultLinetype();
  if (linetype.name.empty())      linetype= defaultLinetype;
  if (undefLinetype.name.empty()) undefLinetype= defaultLinetype;

  ostringstream ostr;

  if(!options_1){
    ostr << " colour=off";
  } else{
    ostr << " colour="         << linecolour.Name();
  }

  ostr <<" plottype="<<plottype;
  ostr << " linetype="      << linetype.name
      << " linewidth="     << linewidth
      << " base=" << base;

  if(minvalue>-fieldUndef) {
    ostr << " minvalue=" << minvalue;
  }
  if(maxvalue < fieldUndef) {
    ostr << " maxvalue=" << maxvalue;
  }

  if (frame>=0) ostr << " frame=" << frame;
  if (zeroLine>=0) ostr << " zero.line=" << zeroLine;

  if ( colours.size() == 3  ) {
    ostr << " colours=" << colours[0].Name() + "," + colours[1].Name() + "," + colours[2].Name();
  }

  if(dimension == 1){

    if (linevalues.size()==0 && loglinevalues.size()==0) {
      ostr << " line.interval=" << lineinterval;
    } else if (linevalues.size() != 0 ) {
      miString str;
      str.join(linevalues,",");
      ostr << " line.values=" << str;
    } else if (loglinevalues.size() != 0 ) {
      miString str;
      str.join(loglinevalues,",");
      ostr << " logline.values=" << str;
    }

    ostr << " extreme.type="   << extremeType
        << " extreme.size="   << extremeSize
        << " extreme.radius=" << extremeRadius;
    if ( palettecolours.size() > 0 ) {
      ostr << " palettecolours="<<palettename;
    } else {
      ostr << " palettecolours=off";
    }

    if (patterns.size() >0 ) {
      ostr << " patterns="<< patternname;
    } else {
      ostr << " patterns=off";
    }

    ostr << " table="<<table
        << " repeat="<<repeat
        << " value.label="    << valueLabel;
  }

  if(plottype == fpt_vector || plottype == fpt_direction || plottype == fpt_wind
      || plottype == fpt_wind_temp_fl || plottype == fpt_wind_value ) {
    ostr << " vector.unit="<< vectorunit
        << " vector.unit.name="<< vectorunitname;
  }

  if (discontinuous==0) {
    ostr << " line.smooth="   << lineSmooth;
    ostr << " field.smooth="  << fieldSmooth;
  }

  ostr << " label.size=" << labelSize
      << " grid.lines="      << gridLines
      << " grid.lines.max="  << gridLinesMax
      << " undef.masking="   << undefMasking
      << " undef.colour="    << undefColour.Name()
      << " undef.linewidth=" << undefLinewidth
      << " undef.linetype="  << undefLinetype.name;

  if (gridValue>=0) ostr << " grid.value=" << gridValue;

  if(!options_2){
    ostr << " colour_2=off";
  } else{
    ostr << " colour_2="         << linecolour_2.Name()
                   << " line.interval_2="         << lineinterval_2
                   << " linewidth_2="     << linewidth_2
                   << " base_2=" << base_2;

    if (linevalues_2.size()==0 && loglinevalues_2.size()==0) {
      ostr << " line.interval_2=" << lineinterval_2;
    } else if (linevalues_2.size() != 0 ) {
      miString str;
      str.join(linevalues_2,",");
      ostr << " line.values_2=" << str;
    } else if (loglinevalues_2.size() != 0 ) {
      miString str;
      str.join(loglinevalues_2,",");
      ostr << " logline.values_2=" << str;
    }

    if(minvalue_2>-fieldUndef) {
      ostr << " minvalue_2=" << minvalue_2;
    }
    if(maxvalue_2 < fieldUndef) {
      ostr << " maxvalue_2=" << maxvalue_2;
    }
  }

  ostr << " dim="  << dimension;

  if(!unit.empty()) {
    ostr << " unit="  << unit;
  }

  if( precision > 0 ) {
    ostr << " precision="  << precision;
  }

  if (antialiasing)
    ostr << " antialiasing=" << antialiasing;

  //   ost << " font="  << fontname
  //    << " face="  << fontface
  //    << " fontsize="  << fontsize
  //    << " alignX="  << alignX
  //    << " alignY="  << alignY;

  return ostr.str();

}



bool PlotOptions::getFieldPlotOptions(const miString& name, PlotOptions& po)
{

  map<miString,PlotOptions>::iterator p;
  if ((p=fieldPlotOptions.find(name))
      != fieldPlotOptions.end()){
    po= p->second;

  } else {
    fieldPlotOptions[name]= po;
  }
  return true;

}
