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

//#define DEBUGPRINT

#include "diFieldPlot.h"

#include "diContouring.h"
#include "diPolyContouring.h"
#include "diFontManager.h"
#include "diColour.h"
#include <diImageGallery.h>
#include <diPlotOptions.h>
#include <diColourShading.h>

#include <puTools/miStringFunctions.h>

#include <GL/gl.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

#define MILOGGER_CATEGORY "diana.FieldPlot"
#define M_TIME
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

const int MaxWindsAuto = 40;
const int MaxArrowsAuto = 55;

FieldPlot::FieldPlot() :
    overlay(false), pshade(false), pundefined(false), vectorAnnotationSize(0), imagedata(
        NULL), previrs(1)
{
  METLIBS_LOG_SCOPE();
}

FieldPlot::~FieldPlot()
{
  METLIBS_LOG_SCOPE();
  clearFields();
}

void FieldPlot::clearFields()
{
  METLIBS_LOG_SCOPE();
  if (imagedata)
    delete[] imagedata;
  imagedata = NULL;
  for (size_t i = 0; i < tmpfields.size(); i++)
    delete tmpfields[i];
  tmpfields.clear();

  fields.clear();
}

const Area& FieldPlot::getFieldArea()
{
  if (fields.size() && fields[0])
    return fields[0]->area;
  else
    return getStaticPlot()->getMapArea(); // return master-area
}

//  return area for existing field
bool FieldPlot::getRealFieldArea(Area& a)
{
  if (not checkFields(1))
    return false;
  a = fields[0]->area;
  return true;
}

// check if current data from plottime
bool FieldPlot::updateNeeded(string& pin)
{
  if (ftime.undef() || (ftime != getStaticPlot()->getTime() && !miutil::contains(pinfo, " time="))
      || fields.size() == 0) {
    pin = pinfo;
    return true;
  }
  return false;
}

// check if current has same level
bool FieldPlot::updatePinNeeded(const std::string& pin)
{
  if (pinfo == pin)
    return false;

  pinfo = pin;
  return true;
}

void FieldPlot::getFieldAnnotation(string& s, Colour& c)
{
  if (poptions.options_1)
    c = poptions.linecolour;
  else
    c = poptions.fillcolour;

  s = getPlotName();
}

// Extract plotting-parameters from PlotInfo.
bool FieldPlot::prepare(const std::string& fname, const std::string& pin)
{
  //merge current plotOptions (from pin) with plotOptions form setup
  pinfo = pin;
  poptions.fillFieldPlotOptions(fname, pinfo, poptions);
  plottype = poptions.plottype;
  if (poptions.enabled == false)
    setEnabled(false);

  if (poptions.maxDiagonalInMeters > -1) {
    METLIBS_LOG_INFO(
      " FieldPlot::prepare: requestedarea.getDiagonalInMeters():"<<getStaticPlot()->getRequestedarea().getDiagonalInMeters()<<"  max:"<<poptions.maxDiagonalInMeters);
    if (getStaticPlot()->getRequestedarea().getDiagonalInMeters() > poptions.maxDiagonalInMeters)
      return false;
  }

#ifdef DEBUGPRINT
  if (plottype==fpt_contour) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotContour");
  else if (plottype==fpt_contour2) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotContour2");
  else if (plottype==fpt_wind) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotWind");
  else if (plottype==fpt_wind_temp_fl) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotWindAndValue");
  else if (plottype==fpt_wind_value) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotWindAndValue");
  else if (plottype==fpt_value) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotValue");
  else if (plottype==fpt_symbol) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotValue");
  else if (plottype==fpt_vector) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotVector");
  else if (plottype==fpt_direction) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotDirection");
  else if (plottype==fpt_alpha_shade) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotAlpha_shade");
  else if (plottype==fpt_alarm_box) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotAlarmBox");
  else if (plottype==fpt_fill_cell) METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"plotFillCell");
  else METLIBS_LOG_DEBUG("FieldPlot "<<fname<<" : "<<"ERROR");
#endif

  pshade = (plottype == fpt_alpha_shade || plottype == fpt_alarm_box
      || plottype == fpt_fill_cell);

  if (plottype == fpt_contour && poptions.contourShading > 0)
    pshade = true;

  pundefined = (poptions.undefMasking > 0 and plottype != fpt_contour2);

  return true;
}

//  set list of field-pointers, update datatime
bool FieldPlot::setData(const vector<Field*>& vf, const miTime& t)
{
  METLIBS_LOG_DEBUG(LOGVAL(vf.size()) << LOGVAL(t.isoTime()));

  clearFields();

  fields = vf;
  ftime = t;




  if (fields.empty()) {
    setPlotName("");
  } else {
    setPlotName(fields[0]->fulltext);
    analysisTime = fields[0]->analysisTime;
    if ( fields[0]->palette && fields[0]->palette->npoint() > 2) {
      vector<Colour> palette;
      bool alpha = (fields[0]->palette->npoint() == 4);
      for ( size_t i = 0; i<fields[0]->palette->nlevel(); ++i ) {
        unsigned char r = fields[0]->palette->value(0,i);
        unsigned char g = fields[0]->palette->value(1,i);
        unsigned char b = fields[0]->palette->value(2,i);
        unsigned char a = 255;
        if ( alpha )
          a = fields[0]->palette->value(3,i);
        Colour c1= Colour(r,g,b,a);
        palette.push_back(c1);
      }
      poptions.palettecolours = palette;
    }
  }
  return true;
}


struct aTable {
  std::string colour;
  std::string pattern;
  std::string text;
};

bool FieldPlot::getTableAnnotations(vector<string>& anno)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  int nanno = anno.size();
  for (int j = 0; j < nanno; j++) {
    if (miutil::contains(anno[j], "table")) {
      if (!isEnabled() || poptions.table == 0
          || (poptions.palettecolours.size() == 0
              && poptions.patterns.size() == 0))
        continue;;

      std::string unit = " " + poptions.legendunits;

      std::string endString;
      std::string startString;
      if (miutil::contains(anno[j], ",")) {
        size_t nn = anno[j].find_first_of(",");
        endString = anno[j].substr(nn);
        startString = anno[j].substr(0, nn);
      } else {
        startString = anno[j];
      }

      //if asking for spesific field
      if (miutil::contains(anno[j], "table=")) {
        std::string name = startString.substr(
            startString.find_first_of("=") + 1);
        if (name[0] == '"')
          miutil::remove(name, '"');
        miutil::trim(name);
        if (!miutil::contains(fields[0]->fieldText, name))
          continue;
      }

      std::string str = "table=\"";
      if (poptions.legendtitle.empty()) {
        str += fields[0]->fieldText;
      } else {
        str += poptions.legendtitle;
      }

      //find min/max if repeating colours
      float cmin = fieldUndef;
      float cmax = -fieldUndef;
      if (!poptions.repeat) {
        cmin = poptions.base;
      } else {
        int ndata = fields[0]->nx * fields[0]->ny;
        for (int i = 0; i < ndata; ++i) {
          if (fields[0]->data[i] != fieldUndef) {
            if (cmin > fields[0]->data[i])
              cmin = fields[0]->data[i];
            if (cmax < fields[0]->data[i])
              cmax = fields[0]->data[i];
          }
        }
      }

      aTable table;
      vector<aTable> vtable;

      size_t ncolours = poptions.palettecolours.size();
      size_t ncold = poptions.palettecolours_cold.size();
      size_t npatterns = poptions.patterns.size();
      size_t nlines = poptions.linevalues.size();
      size_t nloglines = poptions.loglinevalues.size();
      size_t ncodes = (ncolours > npatterns ? ncolours : npatterns);

      if (cmin > poptions.base)
        ncold = 0;

      vector<std::string> classSpec;
      classSpec = miutil::split(poptions.classSpecifications, ",");

      // if class specification is given, do not plot more entries than class specifications
      if (classSpec.size() && ncodes > classSpec.size()) {
        ncodes = classSpec.size();
      }

      if (nlines > 0 && nlines - 1 < ncodes)
        ncodes = nlines;

      for (int i = ncold - 1; i >= 0; i--) {
        table.colour = poptions.palettecolours_cold[i].Name();
        if (npatterns > 0) {
          int ii = (npatterns - 1) - i % npatterns;
          table.pattern = poptions.patterns[ii];
        }
        vtable.push_back(table);
      }

      for (size_t i = 0; i < ncodes; i++) {
        if (ncolours > 0) {
          int ii = i % ncolours;
          table.colour = poptions.palettecolours[ii].Name();
        } else {
          table.colour = poptions.fillcolour.Name();
        }
        if (npatterns > 0) {
          int ii = i % npatterns;
          table.pattern = poptions.patterns[ii];
        }
        vtable.push_back(table);
      }

      if (poptions.discontinuous == 1 && classSpec.size()
          && poptions.lineinterval > 0.99 && poptions.lineinterval < 1.01) {
        for (size_t i = 0; i < ncodes; i++) {
          vector<std::string> tstr = miutil::split(classSpec[i], ":");
          if (tstr.size() > 1) {
            vtable[i].text = tstr[1];
          } else {
            vtable[i].text = tstr[0];
          }
        }

      } else if (nlines > 0) {
        if (!classSpec.size()) {
          for (size_t i = 0; i < ncodes - 1; i++) {
            float min = poptions.linevalues[i];
            float max = poptions.linevalues[i + 1];
            ostringstream ostr;
            ostr << min << " - " << max << unit;
            vtable[i].text = ostr.str();
          }
          float min = poptions.linevalues[ncodes - 1];
          ostringstream ostr;
          ostr << ">" << min << unit;
          vtable[ncodes - 1].text = ostr.str();
        } else {
          for (size_t i = 0; i < ncodes; i++) {
            ostringstream ostr;
            ostr << classSpec[i];
            vtable[i].text = ostr.str();
          }
        }

      } else if (nloglines > 0) {
        if (!classSpec.size()) {
          vector<float> vlog;
          for (size_t n = 0; n < ncodes; n++) {
            float slog = powf(10.0, n);
            for (size_t i = 0; i < nloglines; i++) {
              vlog.push_back(slog * poptions.loglinevalues[i]);
            }
          }
          for (size_t i = 0; i < ncodes - 1; i++) {
            float min = vlog[i];
            float max = vlog[i + 1];
            ostringstream ostr;
            ostr << min << " - " << max << unit;
            vtable[i].text = ostr.str();
          }
          float min = vlog[ncodes - 1];
          ostringstream ostr;
          ostr << ">" << min << unit;
          vtable[ncodes - 1].text = ostr.str();
        } else {
          for (size_t i = 0; i < ncodes; i++) {
            ostringstream ostr;
            ostr << classSpec[i];
            vtable[i].text = ostr.str();
          }
        }

      } else {

        //cold colours
        float max = poptions.base;
        float min = max;
        for (int i = ncold - 1; i > -1; i--) {
          min = max - poptions.lineinterval;
          ostringstream ostr;
          if (fabs(min) < poptions.lineinterval / 10)
            min = 0.;
          if (fabs(max) < poptions.lineinterval / 10)
            max = 0.;
          ostr << min << " - " << max << unit;
          max = min;
          vtable[i].text = ostr.str();
          if (max < poptions.minvalue) {
            vector<aTable>::iterator it = vtable.begin();
            it += i;
            vtable.erase(vtable.begin(), it);
            ncold -= i;
            break;
          }
        }

        //colours
        if (cmin > poptions.base) {
          float step = ncodes * poptions.lineinterval;
          min = int((cmin - poptions.base) / step) * step + poptions.base;
          if (cmax + cmin > 2 * (min + step))
            min += step;
        } else {
          min = poptions.base;
        }
        for (size_t i = ncold; i < ncodes + ncold; i++) {
          max = min + poptions.lineinterval;
          ostringstream ostr;
          ostr << min << " - " << max << unit;
          min = max;
          vtable[i].text = ostr.str();
          if (min > poptions.maxvalue) {
            vector<aTable>::iterator it = vtable.begin();
            it += i;
            vtable.erase(it, vtable.end());
            break;
          }
        }
      }

      int n = vtable.size();
      for (int i = n - 1; i > -1; i--) {
        str += ";";
        str += vtable[i].colour;
        str += ";";
        str += vtable[i].pattern;
        str += ";";
        str += vtable[i].text;
      }
      str += "\"";
      str += endString;

      anno.push_back(str);

    }
  }
  return true;
}

bool FieldPlot::getDataAnnotations(vector<string>& anno)
{
  METLIBS_LOG_DEBUG("getDataAnnotations:"<<anno.size());

  if (not checkFields(1))
    return false;

  int nanno = anno.size();
  for (int j = 0; j < nanno; j++) {

    std::string lg = "";
    int k;
    if ((k = anno[j].find("$lg=")) != string::npos) {
      lg = anno[j].substr(k, 7) + " ";
    }

    if (miutil::contains(anno[j], "$referencetime")) {
      if (fields.size() && fields[0]) {
        std::string refString = analysisTime.format("%Y%m%d %H");
        miutil::replace(anno[j], "$referencetime", refString);
      }
    }
    if (miutil::contains(anno[j], "$forecasthour")) {
      if (fields.size() && fields[0]) {
        ostringstream ost;
        ost << fields[0]->forecastHour;
        miutil::replace(anno[j], "$forecasthour", ost.str());
      }
    }
    if (miutil::contains(anno[j], "$currenttime")) {
      if (fields.size() && fields[0]) {
        miutil::replace(anno[j], "$currenttime", fields[0]->timetext);
      }
    }
    if (miutil::contains(anno[j], "$validtime")) {
      if (fields.size() && fields[0]) {
        miutil::replace(anno[j], "$validtime",
            fields[0]->validFieldTime.format("%Y%m%d %A %H" + lg));
      }
    }
    if (miutil::contains(anno[j], "$model")) {
      if (fields.size() && fields[0]) {
        miutil::replace(anno[j], "$model", fields[0]->modelName);
      }
    }
    if (miutil::contains(anno[j], "$idnum")) {
      if (fields.size() && fields[0]) {
        miutil::replace(anno[j], "$idnum", fields[0]->idnumtext);
      }
    }
    if (miutil::contains(anno[j], "$level")) {
      if (fields.size() && fields[0]) {
        miutil::replace(anno[j], "$level", fields[0]->leveltext);
      }
    }

    if (miutil::contains(anno[j], "arrow") && vectorAnnotationSize > 0.
        && not vectorAnnotationText.empty()) {
      if (miutil::contains(anno[j], "arrow="))
        continue;

      std::string endString;
      std::string startString;
      if (miutil::contains(anno[j], ",")) {
        size_t nn = anno[j].find_first_of(",");
        endString = anno[j].substr(nn);
        startString = anno[j].substr(0, nn);
      } else {
        startString = anno[j];
      }
      //       //if asking for spesific field
      //       if(miutil::contains(anno[j], "arrow=")){
      //         std::string name = startString.substr(startString.find_first_of("=")+1);
      //         if( name[0]=='"' )
      // 	  miutil::remove(name, '"');
      //         miutil::trim(name);
      //         if(!fields[0]->miutil::contains(fieldText, name)) continue;
      //       }

      std::string str = "arrow=" + miutil::from_number(vectorAnnotationSize)
          + ",tcolour=" + poptions.linecolour.Name() + endString;
      anno.push_back(str);
      str = "text=\" " + vectorAnnotationText + "\"" + ",tcolour="
          + poptions.linecolour.Name() + endString;
      anno.push_back(str);

    }
  }

  getTableAnnotations(anno);

  return true;
}

bool FieldPlot::plot()
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(getModelName()));

  int n = fields.size();

  if (!isEnabled() || n < 1)
    return false;

  // possibly smooth all "single" fields
  for (int i = 0; i < n; i++) {
    if (fields[i] && fields[i]->data) {
      if (fields[i]->numSmoothed < poptions.fieldSmooth) {
        int nsmooth = poptions.fieldSmooth - fields[i]->numSmoothed;
        fields[i]->smooth(nsmooth);
      } else if (fields[i]->numSmoothed > poptions.fieldSmooth) {
        METLIBS_LOG_ERROR(
            "field smoothed too much !!! numSmoothed = " << fields[i]->numSmoothed << ", fieldSmooth=" << poptions.fieldSmooth);
      }
    }
  }

  // avoid background colour
  if (poptions.bordercolour == getStaticPlot()->getBackgroundColour())
    poptions.bordercolour = getStaticPlot()->getBackContrastColour();
  if (poptions.linecolour == getStaticPlot()->getBackgroundColour())
    poptions.linecolour = getStaticPlot()->getBackContrastColour();
  for (unsigned int i = 0; i < poptions.colours.size(); i++)
    if (poptions.colours[i] == getStaticPlot()->getBackgroundColour())
      poptions.colours[i] = getStaticPlot()->getBackContrastColour();

  if (poptions.antialiasing)
    glEnable(GL_MULTISAMPLE);
  else
    glDisable(GL_MULTISAMPLE);

  // should be below all real fields
  if (poptions.gridLines > 0)
    plotGridLines();
  if (poptions.gridValue > 0)
    plotNumbers();

  if (poptions.use_stencil || poptions.update_stencil) {
    // Enable the stencil test for masking the field to be plotted or
    // updating the stencil.
    glEnable(GL_STENCIL_TEST);

    if (poptions.use_stencil) {
      // Test the stencil buffer values against a value of 0.
      glStencilFunc(GL_EQUAL, 0, 0x01);
      // Do not update the stencil buffer when plotting.
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
  }

  bool ok = false;

  if (plottype == fpt_contour)
    ok = plotContour();
  else if (plottype == fpt_contour2)
    ok = plotContour2();
  else if (plottype == fpt_wind)
    ok = plotWind();
  else if (plottype == fpt_wind_temp_fl)
    ok = plotWindAndValue(true);
  else if (plottype == fpt_wind_value)
    ok = plotWindAndValue(false);
  else if (plottype == fpt_value)
    ok = plotValue();
  else if (plottype == fpt_symbol)
    ok = plotValue();
  else if (plottype == fpt_vector)
    ok = plotVector();
  else if (plottype == fpt_direction)
    ok = plotDirection();
  else if (plottype == fpt_alpha_shade)
    ok = plotAlpha_shade();
  else if (plottype == fpt_alarm_box)
    ok = plotAlarmBox();
  else if (plottype == fpt_fill_cell)
    ok = plotFillCellExt();
  else if (plottype == fpt_frame)
    ok = plotFrameOnly();

  if (poptions.use_stencil || poptions.update_stencil)
    glDisable(GL_STENCIL_TEST);

  return ok;
}

vector<float*> FieldPlot::prepareVectors(float* x, float* y, bool rotateVectors)
{
  METLIBS_LOG_SCOPE();

  vector<float*> uv;
  float *u = 0, *v = 0;

  int nf = tmpfields.size();

  if (!rotateVectors || fields[0]->area.P() == getStaticPlot()->getMapArea().P()) {
    u = fields[0]->data;
    v = fields[1]->data;
    if (nf == 2) {
      delete tmpfields[0];
      delete tmpfields[1];
      tmpfields.clear();
    }
  } else if (nf == 2 && tmpfields[0]->numSmoothed == fields[0]->numSmoothed
      && tmpfields[0]->area.P() == getStaticPlot()->getMapArea().P()) {
    u = tmpfields[0]->data;
    v = tmpfields[1]->data;
  } else {
    if (nf == 0) {
      Field* utmp = new Field();
      Field* vtmp = new Field();
      tmpfields.push_back(utmp);
      tmpfields.push_back(vtmp);
    }
    *(tmpfields[0]) = *(fields[0]);
    *(tmpfields[1]) = *(fields[1]);
    u = tmpfields[0]->data;
    v = tmpfields[1]->data;
    int npos = fields[0]->nx * fields[0]->ny;
    if (!getStaticPlot()->gc.getVectors(tmpfields[0]->area, getStaticPlot()->getMapArea(), npos, x, y, u, v)) {
      return uv;
    }
    tmpfields[0]->area.setP(getStaticPlot()->getMapArea().P());
    tmpfields[1]->area.setP(getStaticPlot()->getMapArea().P());
  }
  uv.push_back(u);
  uv.push_back(v);
  return uv;
}

vector<float*> FieldPlot::prepareDirectionVectors(float* x, float* y)
{
  METLIBS_LOG_SCOPE();

  vector<float*> uv;

  float *u = 0, *v = 0;

  //tmpfields: fields in current projection
  int nf = tmpfields.size();

  if (nf == 2 && tmpfields[0]->numSmoothed == fields[0]->numSmoothed
      && tmpfields[0]->area.P() == getStaticPlot()->getMapArea().P()) {
    //use fields in current projection
    u = tmpfields[0]->data;
    v = tmpfields[1]->data;
  } else {
    if (nf == 0) {
      Field* utmp = new Field();
      Field* vtmp = new Field();
      tmpfields.push_back(utmp);
      tmpfields.push_back(vtmp);
    }
    //calc fields in current projection
    *(tmpfields[0]) = *(fields[0]);
    *(tmpfields[1]) = *(fields[0]);
    u = tmpfields[0]->data;
    v = tmpfields[1]->data;
    int npos = fields[0]->nx * fields[0]->ny;
    for (int i = 0; i < npos; i++)
      v[i] = 1.0f;

    bool turn = fields[0]->turnWaveDirection;
    if (!getStaticPlot()->gc.getDirectionVectors(getStaticPlot()->getMapArea(), turn, npos, x, y, u, v)) {
      return uv;
    }

    tmpfields[0]->area.setP(getStaticPlot()->getMapArea().P());
    tmpfields[1]->area.setP(getStaticPlot()->getMapArea().P());
  }
  uv.push_back(u);
  uv.push_back(v);
  return uv;
}

void FieldPlot::setAutoStep(float* x, float* y, int& ixx1, int ix2, int& iyy1,
    int iy2, int maxElementsX, int& step, float& dist)
{
  int i, ix, iy;
  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;
  int ix1 = ixx1;
  int iy1 = iyy1;

  // Use all grid point to make average step, not only current rectangle.
  // This ensures that different tiles have the same vector density
  if (poptions.density == -1) {
    ix1 = nx / 4;
    iy1 = ny / 4;
    ix2 = nx - nx / 4;
    iy2 = ny - ny / 4;
  }

  if (nx < 3 || ny < 3) {
    step = 1;
    dist = 1.;
    return;
  }

  int mx = ix2 - ix1;
  int my = iy2 - iy1;
  int ixstep = (mx > 5) ? mx / 5 : 1;
  int iystep = (my > 5) ? my / 5 : 1;

  if (mx > 5) {
    if (ix1 < 2)
      ix1 = 2;
    if (ix2 > nx - 3)
      ix2 = nx - 3;
  }
  if (my > 5) {
    if (iy1 < 2)
      iy1 = 2;
    if (iy2 > ny - 3)
      iy2 = ny - 3;
  }

  float dx, dy;
  float adx = 0.0, ady = 0.0;

  mx = (ix2 - ix1 + ixstep - 1) / ixstep;
  my = (iy2 - iy1 + iystep - 1) / iystep;

  for (iy = iy1; iy < iy2; iy += iystep) {
    for (ix = ix1; ix < ix2; ix += ixstep) {
      i = iy * nx + ix;
      if (x[i] == HUGE_VAL || y[i] == HUGE_VAL)
        continue;
      if (x[i + 1] == HUGE_VAL || y[i + 1] == HUGE_VAL)
        continue;
      if (x[i + nx] == HUGE_VAL || y[i + nx] == HUGE_VAL)
        continue;
      dx = x[i + 1] - x[i];
      dy = y[i + 1] - y[i];
      adx += sqrtf(dx * dx + dy * dy);
      dx = x[i + nx] - x[i];
      dy = y[i + nx] - y[i];
      ady += sqrtf(dx * dx + dy * dy);
    }
  }

  int mxmy = mx * my > 25 ? mx * my : 25;
  adx /= float(mxmy);
  ady /= float(mxmy);

  dist = (adx + ady) * 0.5;

  // automatic wind/vector density if step<1
  if (step < 1) {
    // 40 winds or 55 arrows if 1000 pixels
    float numElements = float(maxElementsX) * getStaticPlot()->getPhysWidth() / 1000.;
    float elementSize = getStaticPlot()->getPlotSize().width() / numElements;
    step = int(elementSize / dist + 0.75);
    if (step < 1)
      step = 1;
  }

  dist *= float(step);

  if (step > poptions.densityFactor && poptions.densityFactor > 0) {
    step /= (poptions.densityFactor);
  }

  //adjust ix1,iy1 to make sure that same grid points are used when panning
  ixx1 = int(ixx1 / step) * step;
  iyy1 = int(iyy1 / step) * step;

}

int FieldPlot::xAutoStep(float* x, float* y, int& ixx1, int ix2, int iy,
    float sdist)
{
  const int nx = fields[0]->nx;
  if (nx < 3)
    return 1;

  int ix1 = ixx1;

  // Use all grid point to make average step, not only current rectangle.
  // This ensures that different tiles have the same vector density
  if (poptions.density == -1) {
    ix1 = nx / 4;
    ix2 = nx - nx / 4;
  }

  int mx = ix2 - ix1;
  const int ixstep = (mx > 5) ? mx / 5 : 1;

  if (mx > 5) {
    if (ix1 < 2)
      ix1 = 2;
    if (ix2 > nx - 3)
      ix2 = nx - 3;
  }

  float adx = 0.0f;

  mx = (ix2 - ix1 + ixstep - 1) / ixstep;

  for (int ix = ix1; ix < ix2; ix += ixstep) {
    const int i = iy * nx + ix;
    if (x[i] == HUGE_VAL || y[i] == HUGE_VAL || x[i + 1] == HUGE_VAL
        || y[i + 1] == HUGE_VAL) {
      continue;
    }
    const float dx = x[i + 1] - x[i];
    const float dy = y[i + 1] - y[i];
    adx += sqrtf(dx * dx + dy * dy);
  }

  adx /= float(mx);

  int xstep;
  if (adx > 0) {
    xstep = int(sdist / adx + 0.75);
    if (xstep < 1)
      xstep = 1;
  } else {
    xstep = nx;
  }

  //adjust ix1 to make sure that same grid points are used when panning
  ixx1 = int(ixx1 / xstep) * xstep;

  return xstep;
}

bool FieldPlot::getGridPoints(float* &x, float* &y, int& ix1, int& ix2, int& iy1, int& iy2, int factor, bool boxes, bool cached) const
{
  if (not getStaticPlot()->gc.getGridPoints(fields[0]->area, fields[0]->gridResolutionX * factor, fields[0]->gridResolutionY * factor,
          getStaticPlot()->getMapArea(), getStaticPlot()->getMapSize(),
          boxes, fields[0]->nx / factor, fields[0]->ny / factor, &x, &y, ix1, ix2, iy1, iy2, cached))
    return false;
  if (ix1 > ix2 || iy1 > iy2)
    return false;
  return true;
}

bool FieldPlot::getPoints(int n, float* x, float* y) const
{
  return getStaticPlot()->gc.getPoints(fields[0]->area.P(), getStaticPlot()->getMapArea().P(), n, x, y);
}

// plot vector field as wind arrows
// Fields u(0) v(1), optional- colorfield(2)
bool FieldPlot::plotWind()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(2))
    return false;
  const bool colourwind = (fields.size() == 3);
  const float* colourdata = 0;
  if (colourwind) {
    if (not fields[2] or not fields[2]->data)
      return false;
    colourdata = fields[2]->data;
  }

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  // convert windvectors to correct projection
  const vector<float*> uv = prepareVectors(x, y, poptions.rotateVectors);
  if (uv.size() != 2)
    return false;
  const float *u = uv[0];
  const float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, step, sdist);

  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  float* limits = 0;
  const GLfloat* rgb = 0;
  int nlim = poptions.limits.size();
  int ncol = poptions.colours.size();

  const int n_rgb_default = 4;
  const GLfloat rgb_default[n_rgb_default * 3] = { 0.5, 0.5, 0.5, 0, 0, 0, 0, 1,
      1, 1, 0, 0 };

  if (colourwind) {
    if (ncol >= 2) {
      GLfloat* rgb_tmp = new GLfloat[ncol * 3];
      for (int i = 0; i < ncol; i++) {
        rgb_tmp[i * 3 + 0] = poptions.colours[i].fR();
        rgb_tmp[i * 3 + 1] = poptions.colours[i].fG();
        rgb_tmp[i * 3 + 2] = poptions.colours[i].fB();
      }
      rgb = rgb_tmp;
    } else {
      ncol = n_rgb_default;
      rgb = rgb_default;
    }

    if (nlim >= 1) {

      if (nlim > ncol - 1)
        nlim = ncol - 1;
      if (ncol > nlim + 1)
        ncol = nlim + 1;
      limits = new float[nlim];
      for (int i = 0; i < nlim; i++) {
        limits[i] = poptions.limits[i];
      }

    } else {

      // default, should be handled when reading setup, if allowed...
      const int maxdef = 4;
      nlim = maxdef - 1;

      float fmin = fieldUndef, fmax = -fieldUndef;
      for (int i = 0; i < nx * ny; ++i) {
        const float cv = colourdata[i];
        if (cv != fieldUndef) {
          if (fmin > cv)
            fmin = cv;
          if (fmax < cv)
            fmax = cv;
        }
      }
      if (fmin > fmax)
        return false;

      limits = new float[nlim];
      float dlim = (fmax - fmin) / float(ncol);

      for (int i = 0; i < nlim; i++) {
        limits[i] = fmin + dlim * float(i + 1);
      }
    }
  }

  const float unitlength = poptions.vectorunit / 10;
  const float flagl = sdist * 0.85 / unitlength;
  const float flagstep = flagl / 10.;
  const float flagw = flagl * 0.35;
  const float hflagw = 0.6;
  // for arrow tip
  const float afac = -1.5, sfac = afac * 0.5;

  vector<float> vx, vy; // keep vertices for 50-knot flags
  vector<int> vc;    // keep the colour too

  ix1 -= step;
  if (ix1 < 0)
    ix1 = 0;
  iy1 -= step;
  if (iy1 < 0)
    iy1 = 0;
  ix2 += (step + 1);
  if (ix2 > nx)
    ix2 = nx;
  iy2 += (step + 1);
  if (iy2 > ny)
    iy2 = ny;

  Rectangle ms = getStaticPlot()->getMapSize();
  ms.setExtension(flagl);

  Projection geoProj;
  geoProj.setGeographic();
  const Projection& projection = getStaticPlot()->getMapArea().P();

  glLineWidth(poptions.linewidth + 0.1);  // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  glBegin(GL_LINES);

  for (int iy = iy1; iy < iy2; iy += step) {
    const int xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (int ix = ix1; ix < ix2; ix += xstep) {
      const int i = iy * nx + ix;

      //If southern hemisphere, turn the feathers
      float xx = x[i];
      float yy = y[i];
      projection.convertToGeographic(1, &xx, &yy, geoProj);
      const int sign = (yy < 0) ? -1 : 1;

      float gx = x[i];
      float gy = y[i];
      if (u[i] != fieldUndef && v[i] != fieldUndef && ms.isnear(gx, gy)) {
        float ff = sqrtf(u[i] * u[i] + v[i] * v[i]);
        if (ff > 0.00001 && (!colourwind || colourdata[i] != fieldUndef)) {

          const float gu = u[i] / ff;
          const float gv = v[i] / ff;

          ff *= 3600.0 / 1852.0;

          // find no. of 50,10 and 5 knot flags
          int n50, n10, n05;
          if (ff < 182.49) {
            n05 = int(ff * 0.2 + 0.5);
            n50 = n05 / 10;
            n05 -= n50 * 10;
            n10 = n05 / 2;
            n05 -= n10 * 2;
          } else if (ff < 190.) {
            n50 = 3;
            n10 = 3;
            n05 = 0;
          } else if (ff < 205.) {
            n50 = 4;
            n10 = 0;
            n05 = 0;
          } else if (ff < 225.) {
            n50 = 4;
            n10 = 1;
            n05 = 0;
          } else {
            n50 = 5;
            n10 = 0;
            n05 = 0;
          }

          int l = 0;
          if (colourwind) {
            while (l < nlim && colourdata[i] > limits[l])
              l++;
            glColor3fv(&rgb[l * 3]);
          }

          const float dx = flagstep * gu;
          const float dy = flagstep * gv;
          const float dxf = -sign * flagw * gv - dx;
          const float dyf = sign * flagw * gu - dy;

          if (poptions.arrowstyle == arrow_wind_arrow) {
            // arrow (drawn as two lines)
            vx.push_back(gx);
            vy.push_back(gy);
            vx.push_back(gx + afac * dx + sfac * dy);
            vy.push_back(gy + afac * dy - sfac * dx);
            vx.push_back(gx + afac * dx - sfac * dy);
            vy.push_back(gy + afac * dy + sfac * dx);
          }
          // direction
          glVertex2f(gx, gy);
          gx = gx - flagl * gu;
          gy = gy - flagl * gv;
          glVertex2f(gx, gy);

          // 50-knot flags, store for plot below
          if (n50 > 0) {
            for (int n = 0; n < n50; n++) {
              if (colourwind)
                vc.push_back(l);
              vx.push_back(gx);
              vy.push_back(gy);
              gx += dx * 2.;
              gy += dy * 2.;
              vx.push_back(gx + dxf);
              vy.push_back(gy + dyf);
              vx.push_back(gx);
              vy.push_back(gy);
            }
            gx += dx;
            gy += dy;
          }
          // 10-knot flags
          for (int n = 0; n < n10; n++) {
            glVertex2f(gx, gy);
            glVertex2f(gx + dxf, gy + dyf);
            gx += dx;
            gy += dy;
          }
          // 5-knot flag
          if (n05 > 0) {
            if (n50 + n10 == 0) {
              gx += dx;
              gy += dy;
            }
            glVertex2f(gx, gy);
            glVertex2f(gx + hflagw * dxf, gy + hflagw * dyf);
          }
        }
      }
    }
  }
  glEnd();

  getStaticPlot()->UpdateOutput();

  // draw 50-knot flags
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  const int vi = vx.size();
  int j = 0;
  if (vi >= 3) {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < vi; i += 3) {
      if (colourwind) {
        glColor3fv(&rgb[vc[j++] * 3]);
      }
      glVertex2f(vx[i], vy[i]);
      glVertex2f(vx[i + 1], vy[i + 1]);
      glVertex2f(vx[i + 2], vy[i + 2]);
    }
    glEnd();
  }

  getStaticPlot()->UpdateOutput();

  if (rgb != rgb_default)
    delete[] rgb;
  delete[] limits;

  glDisable(GL_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

/*
 ROUTINE:   FieldPlot::plotValue
 PURPOSE:   plot field value as number or symbol
 ALGORITHM:
 */

bool FieldPlot::plotValue()
{
  METLIBS_LOG_SCOPE();

  if (fields.size() > 1)
    return plotValues();

  if (not checkFields(1))
    return false;

  // plot symbol
  ImageGallery ig;
  map<int, std::string> classImages;
  if (poptions.plottype == fpt_symbol && poptions.discontinuous == 1
      && (not poptions.classSpecifications.empty())) {
    vector<int> classValues;
    vector<std::string> classNames;
    vector<std::string> classSpec = miutil::split(poptions.classSpecifications,
        ",");
    int nc = classSpec.size();
    for (int i = 0; i < nc; i++) {
      vector<std::string> vstr = miutil::split(classSpec[i], ":");
      if (vstr.size() > 2) {
        int value = atoi(vstr[0].c_str());
        classValues.push_back(value);
        classNames.push_back(vstr[1]);
        classImages[value] = vstr[2];
      }
    }
  }

  int i, ix, iy;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  float* field = fields[0]->data;
  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, step, sdist);
  int xstep = step;

  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;
  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  bool smallvalues = false;
  if (poptions.precision == 0) {
    int npos = nx * ny;
    int ii = 0;
    while (ii < npos && (fabsf(field[ii]) == fieldUndef || fabsf(field[ii]) < 1)) {
      ++ii;
    }
    smallvalues = (ii == npos);

    if (smallvalues) {
      xstep *= 2;
    }
  } else {
    xstep *= poptions.precision;
  }

  float gx, gy;
  float flagl = sdist * 0.85;

  ix1 -= step;
  if (ix1 < 0)
    ix1 = 0;
  iy1 -= step;
  if (iy1 < 0)
    iy1 = 0;
  ix2 += (step + 1);
  if (ix2 > nx)
    ix2 = nx;
  iy2 += (step + 1);
  if (iy2 > ny)
    iy2 = ny;

  glColor3ubv(poptions.linecolour.RGB());

  Rectangle ms = getStaticPlot()->getMapSize();
  ms.setExtension(flagl);

  float fontsize = 10. * poptions.labelSize;
  getStaticPlot()->getFontPack()->set("BITMAPFONT", poptions.fontface, fontsize);

  float chx, chy;
  getStaticPlot()->getFontPack()->getCharSize('0', chx, chy);
  chy *= 0.75;

  for (iy = iy1; iy < iy2; iy += step) {
    xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (ix = ix1; ix < ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];
      if (field[i] != fieldUndef && ms.isnear(gx, gy)
          && field[i] > poptions.minvalue && field[i] < poptions.maxvalue) {
        float value = (field[i]);

        if (poptions.colours.size() > 2) {
          if (value < poptions.base) {
            glColor3ubv(poptions.colours[0].RGB());
          } else if (value > poptions.base) {
            glColor3ubv(poptions.colours[2].RGB());
          } else {
            glColor3ubv(poptions.colours[1].RGB());
          }
        }

        if (!classImages.empty()) { //plot symbol
          if (classImages.count(int(value))) {
            ig.plotImage(classImages[int(value)], gx, gy, true,
                poptions.labelSize * 0.25);
          }
        } else { // plot value
          ostringstream ostr;
          if (smallvalues) {
            ostr.setf(ios::scientific);
            ostr.precision(1);
          } else {
            ostr.setf(ios::fixed);
            ostr.precision(poptions.precision);
          }
          ostr << value;
          std::string str = ostr.str();
          getStaticPlot()->getFontPack()->drawStr(str.c_str(), gx - chx / 2, gy - chy / 2, 0.0);
        }
      }
    }
  }
  glEnd();

  getStaticPlot()->UpdateOutput();

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

/*
 ROUTINE:   FieldPlot::plotWindAndValue
 PURPOSE:   plot vector field as wind arrows and third field as number.
 ALGORITHM: Fields u(0) v(1) number(2)
 */

bool FieldPlot::plotWindAndValue(bool flightlevelChart)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(3))
    return false;

  float *t = fields[2]->data;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  // convert windvectors to correct projection
  vector<float*> uv = prepareVectors(x, y, poptions.rotateVectors);
  if (uv.size() != 2)
    return false;
  float *u = uv[0];
  float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, step, sdist);
  int xstep = step;

  int i, ix, iy;
  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  float unitlength = poptions.vectorunit / 10;
  int n50, n10, n05;
  float ff, gu, gv, gx, gy, dx, dy, dxf, dyf;
  float flagl = sdist * 0.85 / unitlength;
  float flagstep = flagl / 10.;
  float flagw = flagl * 0.35;
  float hflagw = 0.6;

  vector<float> vx, vy; // keep vertices for 50-knot flags

  ix1 -= step;
  if (ix1 < 0)
    ix1 = 0;
  iy1 -= step;
  if (iy1 < 0)
    iy1 = 0;
  ix2 += (step + 1);
  if (ix2 > nx)
    ix2 = nx;
  iy2 += (step + 1);
  if (iy2 > ny)
    iy2 = ny;

  Rectangle ms = getStaticPlot()->getMapSize();
  ms.setExtension(flagl);
  float fontsize = 7. * poptions.labelSize;

  getStaticPlot()->getFontPack()->set("BITMAPFONT", poptions.fontface, fontsize);

  float chx, chy;
  getStaticPlot()->getFontPack()->getStringSize("ps00", chx, chy);

  getStaticPlot()->getFontPack()->getCharSize('0', chx, chy);

  // the real height for numbers 0-9 (width is ok)
  chy *= 0.75;

  // bit matrix used to avoid numbers plotted across wind and numbers
  const int nbitwd = sizeof(int) * 8;
  float bres = 1.0 / (chx * 0.5);
  float bx = ms.x1 - flagl * 2.5;
  float by = ms.y1 - flagl * 2.5;
  if (bx >= 0.0f)
    bx = float(int(bx * bres)) / bres;
  else
    bx = float(int(bx * bres - 1.0f)) / bres;
  if (by >= 0.0f)
    by = float(int(by * bres)) / bres;
  else
    by = float(int(by * bres - 1.0f)) / bres;
  int nbx = int((ms.x2 + flagl * 2.5 - bx) * bres) + 3;
  int nby = int((ms.y2 + flagl * 2.5 - by) * bres) + 3;
  int nbmap = (nbx * nby + nbitwd - 1) / nbitwd;
  int *bmap = new int[nbmap];
  for (i = 0; i < nbmap; i++)
    bmap[i] = 0;
  int m, ib, jb, ibit, iwrd, nb;
  float xb[20][2], yb[20][2];

  vector<int> vxstep;

  glLineWidth(poptions.linewidth + 0.1);  // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  // plot wind............................................

  glBegin(GL_LINES);

  //Wind arrows are adjusted to lat=10 and Lon=10 if
  //poptions.density!=auto and proj=geographic
  bool adjustToLatLon = poptions.density > 0
      && fields[0]->area.P().isGeographic() && step > 0;
  if (adjustToLatLon)
    iy1 = (iy1 / step) * step;
  for (iy = iy1; iy < iy2; iy += step) {
    xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    if (adjustToLatLon) {
      xstep = (xstep / step) * step;
      if (xstep == 0)
        xstep = step;
      ix1 = (ix1 / xstep) * xstep;
    }
    vxstep.push_back(xstep);
    for (ix = ix1; ix < ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];
      if (u[i] != fieldUndef && v[i] != fieldUndef && ms.isnear(gx, gy)
          && t[i] > poptions.minvalue && t[i] < poptions.maxvalue) {
        ff = sqrtf(u[i] * u[i] + v[i] * v[i]);
        if (ff > 0.00001) {

          gu = u[i] / ff;
          gv = v[i] / ff;

          ff *= 3600.0 / 1852.0;

          // find no. of 50,10 and 5 knot flags
          if (ff < 182.49) {
            n05 = int(ff * 0.2 + 0.5);
            n50 = n05 / 10;
            n05 -= n50 * 10;
            n10 = n05 / 2;
            n05 -= n10 * 2;
          } else if (ff < 190.) {
            n50 = 3;
            n10 = 3;
            n05 = 0;
          } else if (ff < 205.) {
            n50 = 4;
            n10 = 0;
            n05 = 0;
          } else if (ff < 225.) {
            n50 = 4;
            n10 = 1;
            n05 = 0;
          } else {
            n50 = 5;
            n10 = 0;
            n05 = 0;
          }

          dx = flagstep * gu;
          dy = flagstep * gv;
          dxf = -flagw * gv - dx;
          dyf = flagw * gu - dy;

          nb = 0;
          xb[nb][0] = gx;
          yb[nb][0] = gy;

          // direction
          glVertex2f(gx, gy);
          gx = gx - flagl * gu;
          gy = gy - flagl * gv;
          glVertex2f(gx, gy);

          xb[nb][1] = gx;
          yb[nb++][1] = gy;

          // 50-knot flags, store for plot below
          if (n50 > 0) {
            for (int n = 0; n < n50; n++) {
              xb[nb][0] = gx;
              yb[nb][0] = gy;
              vx.push_back(gx);
              vy.push_back(gy);
              gx += dx * 2.;
              gy += dy * 2.;
              vx.push_back(gx + dxf);
              vy.push_back(gy + dyf);
              vx.push_back(gx);
              vy.push_back(gy);
              xb[nb][1] = gx + dxf;
              yb[nb++][1] = gy + dyf;
              xb[nb][0] = gx + dxf;
              yb[nb][0] = gy + dyf;
              xb[nb][1] = gx;
              yb[nb++][1] = gy;
            }
            gx += dx;
            gy += dy;
          }

          // 10-knot flags
          for (int n = 0; n < n10; n++) {
            glVertex2f(gx, gy);
            glVertex2f(gx + dxf, gy + dyf);
            xb[nb][0] = gx;
            yb[nb][0] = gy;
            xb[nb][1] = gx + dxf;
            yb[nb++][1] = gy + dyf;
            gx += dx;
            gy += dy;
          }
          // 5-knot flag
          if (n05 > 0) {
            if (n50 + n10 == 0) {
              gx += dx;
              gy += dy;
            }
            glVertex2f(gx, gy);
            glVertex2f(gx + hflagw * dxf, gy + hflagw * dyf);
            xb[nb][0] = gx;
            yb[nb][0] = gy;
            xb[nb][1] = gx + hflagw * dxf;
            yb[nb++][1] = gy + hflagw * dyf;
          }

          // mark used space (lines) in bitmap
          for (int n = 0; n < nb; n++) {
            dx = xb[n][1] - xb[n][0];
            dy = yb[n][1] - yb[n][0];
            if (fabsf(dx) > fabsf(dy))
              m = int(fabsf(dx) * bres) + 2;
            else
              m = int(fabsf(dy) * bres) + 2;
            dx /= float(m - 1);
            dy /= float(m - 1);
            gx = xb[n][0];
            gy = yb[n][0];
            for (i = 0; i < m; i++) {
              ib = int((gx - bx) * bres);
              jb = int((gy - by) * bres);
              ibit = jb * nbx + ib;
              iwrd = ibit / nbitwd;
              ibit = ibit % nbitwd;
              bmap[iwrd] = bmap[iwrd] | (1 << ibit);
              gx += dx;
              gy += dy;
            }
          }

        }
      }
    }
  }
  glEnd();

  getStaticPlot()->UpdateOutput();

  // draw 50-knot flags
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  int vi = vx.size();
  if (vi >= 3) {
    glBegin(GL_TRIANGLES);
    for (i = 0; i < vi; i++)
      glVertex2f(vx[i], vy[i]);
    glEnd();
  }

  getStaticPlot()->UpdateOutput();

  // plot numbers.................................................

  //-----------------------------------------------------------------------
  //
  //  -----------  Default:  (* = grid point)
  //  |  3   0  |  wind in quadrant 0 => number in quadrant 3   (pos. 6)
  //  |    *    |  wind in quadrant 1 => number in quadrant 0   (pos. 0)
  //  |  2   1  |  wind in quadrant 2 => number in quadrant 1   (pos. 2)
  //  -----------  wind in quadrant 3 => number in quadrant 2   (pos. 4)
  //
  //  -----------  If the default position can not be used:
  //  |  6 7 0  |  look for an o.k. position in 8 different positions
  //  |  5 * 1  | (all 'busy'=> use the least busy position)
  //  |  4 3 2  |  if wind is not plotted (not wind or undefined)
  //  -----------  the number is plotted at the grid point (*, pos. 8)
  //
  //-----------------------------------------------------------------------
  //

  float hchy = chy * 0.5;
  float d = chx * 0.5;

  float adx[9] = { d, d, d, 0.0f, -d, -d, -d, 0.0f, 0.0f };
  float cdx[9] = { 0.0f, 0.0f, 0.0f, -0.5f, -1.0f, -1.0f, -1.0f, -0.5f, -0.5f };
  float ady[9] = { d, -d, -d - chy, -d - chy, -d - chy, -hchy, d, d, -hchy };

  int j, ipos0, ipos1, ipos, ib1, ib2, jb1, jb2, mused, nused, value;
  float x1, x2, y1, y2, w, h;
  int ivx = 0;

  for (iy = iy1; iy < iy2; iy += step) {
    xstep = vxstep[ivx++];
    for (ix = ix1; ix < ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];
      if (t[i] != fieldUndef && getStaticPlot()->getMapSize().isinside(gx, gy)
          && t[i] > poptions.minvalue && t[i] < poptions.maxvalue) {

        if (u[i] != fieldUndef && v[i] != fieldUndef)
          ff = sqrtf(u[i] * u[i] + v[i] * v[i]);
        else
          ff = 0.0f;
        if (ff > 0.00001) {
          if (u[i] >= 0.0f && v[i] >= 0.0f)
            ipos0 = 2;
          else if (u[i] >= 0.0f)
            ipos0 = 4;
          else if (v[i] >= 0.0f)
            ipos0 = 0;
          else
            ipos0 = 6;
          m = 8;
        } else {
          ipos0 = 8;
          m = 9;
        }

        value = (t[i] >= 0.0f) ? int(t[i] + 0.5f) : int(t[i] - 0.5f);
        if (poptions.colours.size() > 2) {
          if (value < poptions.base) {
            glColor4ubv(poptions.colours[0].RGBA());
          } else if (value > poptions.base) {
            glColor4ubv(poptions.colours[2].RGBA());
          } else {
            glColor4ubv(poptions.colours[1].RGBA());
          }
        }

        ostringstream ostr;
        if (flightlevelChart) {
          if (value <= 0) {
            ostr << -value;
          } else {
            ostr << "ps" << value;
          }
        } else {
          ostr << value;
        }

        std::string str = ostr.str();
        getStaticPlot()->getFontPack()->getStringSize(str.c_str(), w, h);
        //###########################################################################
        //        x1= gx + adx[ipos0] + cdx[ipos0]*w;
        //        y1= gy + ady[ipos0];
        //        x2= x1 + w;
        //        y2= y1 + chy;
        //        glBegin(GL_LINE_LOOP);
        //        glVertex2f(x1,y1);
        //        glVertex2f(x2,y1);
        //        glVertex2f(x2,y2);
        //        glVertex2f(x1,y2);
        //        glEnd();
        //###########################################################################

        mused = nbx * nby;
        ipos1 = ipos0;

        for (j = 0; j < m; j++) {
          ipos = (ipos0 + j) % m;
          x1 = gx + adx[ipos] + cdx[ipos] * w;
          y1 = gy + ady[ipos];
          x2 = x1 + w;
          y2 = y1 + chy;
          ib1 = int((x1 - bx) * bres);
          ib2 = int((x2 - bx) * bres) + 1;
          jb1 = int((y1 - by) * bres);
          jb2 = int((y2 - by) * bres) + 1;
          nused = 0;
          for (jb = jb1; jb < jb2; jb++) {
            for (ib = ib1; ib < ib2; ib++) {
              ibit = jb * nbx + ib;
              iwrd = ibit / nbitwd;
              ibit = ibit % nbitwd;
              if (bmap[iwrd] & (1 << ibit))
                nused++;
            }
          }
          if (nused < mused) {
            mused = nused;
            ipos1 = ipos;
            if (nused == 0)
              break;
          }
        }

        x1 = gx + adx[ipos1] + cdx[ipos1] * w;
        y1 = gy + ady[ipos1];
        x2 = x1 + w;
        y2 = y1 + chy;
        if (getStaticPlot()->getMapSize().isinside(x1, y1) && getStaticPlot()->getMapSize().isinside(x2, y2)) {
          getStaticPlot()->getFontPack()->drawStr(str.c_str(), x1, y1, 0.0);
          // mark used space for number (line around)
          ib1 = int((x1 - bx) * bres);
          ib2 = int((x2 - bx) * bres) + 1;
          jb1 = int((y1 - by) * bres);
          jb2 = int((y2 - by) * bres) + 1;
          for (jb = jb1; jb < jb2; jb++) {
            for (ib = ib1; ib < ib2; ib++) {
              ibit = jb * nbx + ib;
              iwrd = ibit / nbitwd;
              ibit = ibit % nbitwd;
              bmap[iwrd] = bmap[iwrd] | (1 << ibit);
            }
          }
        }

      }
    }
  }

  getStaticPlot()->UpdateOutput();

  delete[] bmap;

  glDisable(GL_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

/*
 ROUTINE:   FieldPlot::plotValues
 PURPOSE:   plot the values of three, four or five fields:
 field1  |       field1      |         field1  field4
 field3  | field3     field4 |  field3  -----
 field2  |      field2       |          field2 field5
 colours are set by poptions.textcolour or poptions.colours
 minvalue and maxvalue refer to field1
 */
bool FieldPlot::plotValues()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(0))
    return false;

  const size_t nfields = fields.size();

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, 22, step, sdist);
  int xstep = step;

  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  ix1 -= step;
  if (ix1 < 0)
    ix1 = 0;
  iy1 -= step;
  if (iy1 < 0)
    iy1 = 0;
  ix2 += (step + 1);
  if (ix2 > nx)
    ix2 = nx;
  iy2 += (step + 1);
  if (iy2 > ny)
    iy2 = ny;

  float fontsize = 8. * poptions.labelSize;

  getStaticPlot()->getFontPack()->set("BITMAPFONT", poptions.fontface, fontsize);

  float chx, chy;
  getStaticPlot()->getFontPack()->getCharSize('0', chx, chy);
  float hchy = chy * 0.5;

  //-----------------------------------------------------------------------
  //  -----------  Plotting numbers in position 0 - 8
  //  |  6 7 0  |  3 numbers: pos 7, 3, 8
  //  |  5 8 1  |  4 numbers: pos 7, 3, 5, 1
  //  |  4 3 2  |  5 numbers: pos 7, 3, 5, 0, 2 and line in pos 8
  //  -----------
  //
  //-----------------------------------------------------------------------
  //

  float xshift = chx * 2;
  float yshift = chx * 0.5;

  if (nfields == 3) {
    yshift *= 1.9;
  }

  float adx[9] = { xshift, xshift, xshift, 0, -xshift, -xshift, -xshift, 0, 0 };
  float cdx[9] = { 0.0, 0.0, 0.0, -0.5, -1.0, -1.0, -1.0, -0.5, -0.5 };
  float ady[9] = { yshift, -yshift, -yshift - chy, -yshift - chy, -yshift - chy,
      -hchy, yshift, yshift, -hchy };

  int position[5];
  position[0] = 7;
  position[1] = 3;
  if (nfields == 3) {
    position[2] = 8;
  } else {
    position[2] = 5;
  }
  if (nfields == 4) {
    position[3] = 1;
  } else {
    position[3] = 0;
  }
  position[4] = 2;

  Colour col[5];
  for (size_t i = 0; i < nfields; i++) {
    if (poptions.colours.size() > i) {
      col[i] = poptions.colours[i];
    } else {
      col[i] = poptions.textcolour;
    }
  }

  //Wind arrows are adjusted to lat=10 and Lon=10 if
  //poptions.density!=auto and proj=geographic
  vector<int> vxstep;
  bool adjustToLatLon = poptions.density && fields[0]->area.P().isGeographic()
      && step > 0;
  if (adjustToLatLon)
    iy1 = (iy1 / step) * step;
  for (int iy = iy1; iy < iy2; iy += step) {
    xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    if (adjustToLatLon) {
      xstep = (xstep / step) * step;
      if (xstep == 0)
        xstep = step;
      ix1 = (ix1 / xstep) * xstep;
    }
    vxstep.push_back(xstep);
  }

  float x1, x2, y1, y2, w, h;
  int ivx = 0;
  for (int iy = iy1; iy < iy2; iy += step) {
    xstep = vxstep[ivx++];
    for (int ix = ix1; ix < ix2; ix += xstep) {
      int i = iy * nx + ix;
      float gx = x[i];
      float gy = y[i];

      if (fields[0]->data[i] > poptions.minvalue
          && fields[0]->data[i] < poptions.maxvalue) {

        for (size_t j = 0; j < nfields; j++) {
          float * fieldData = fields[j]->data;
          if (fieldData[i] != fieldUndef && getStaticPlot()->getMapSize().isinside(gx, gy)) {
            int value =
                (fieldData[i] >= 0.0f) ? int(fieldData[i] + 0.5f) :
                    int(fieldData[i] - 0.5f);
            ostringstream ostr;
            ostr << value;
            std::string str = ostr.str();
            getStaticPlot()->getFontPack()->getStringSize(str.c_str(), w, h);

            int ipos1 = position[j];
            x1 = gx + adx[ipos1] + cdx[ipos1] * w;
            y1 = gy + ady[ipos1];
            x2 = x1 + w;
            y2 = y1 + chy;
            if (getStaticPlot()->getMapSize().isinside(x1, y1) && getStaticPlot()->getMapSize().isinside(x2, y2)) {
              glColor3ubv(col[j].RGB());
              getStaticPlot()->getFontPack()->drawStr(str.c_str(), x1, y1, 0.0);
            }
          }
        }

        // ---
        if (nfields == 4 || nfields == 5) {
          ostringstream ostr;
          ostr << "----";
          std::string str = ostr.str();
          getStaticPlot()->getFontPack()->getStringSize(str.c_str(), w, h);

          x1 = gx + adx[8] + cdx[8] * w;
          y1 = gy + ady[8];
          x2 = x1 + w;
          y2 = y1 + chy;
          if (getStaticPlot()->getMapSize().isinside(x1, y1) && getStaticPlot()->getMapSize().isinside(x2, y2)) {
            getStaticPlot()->getFontPack()->drawStr(str.c_str(), x1, y1, 0.0);
          }
        }

      }
    }
  }

  getStaticPlot()->UpdateOutput();

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  glDisable(GL_LINE_STIPPLE);

  return true;
}

//  plot vector field as arrows (wind,sea current,...)
bool FieldPlot::plotVector()
{
  METLIBS_LOG_SCOPE();

  if (fields.size() == 3)
    return plotVectorColour();

  if (not checkFields(2))
    return false;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  // convert vectors to correct projection
  vector<float*> uv = prepareVectors(x, y, poptions.rotateVectors);
  if (uv.size() != 2)
    return false;
  float *u = uv[0];
  float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, step, sdist);
  int xstep = step;

  int i, ix, iy;
  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  float unitlength = poptions.vectorunit;

  // length if abs(vector) = unitlength
  float arrowlength = sdist;

  float scale = arrowlength / unitlength;

  // for annotations .... should probably be resized if very small or large...
  vectorAnnotationSize = arrowlength;
  vectorAnnotationText = miutil::from_number(unitlength)
      + poptions.vectorunitname;

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float gx, gy, dx, dy;

  ix1 -= step;
  if (ix1 < 0)
    ix1 = 0;
  iy1 -= step;
  if (iy1 < 0)
    iy1 = 0;
  ix2 += (step + 1);
  if (ix2 > nx)
    ix2 = nx;
  iy2 += (step + 1);
  if (iy2 > ny)
    iy2 = ny;

  Rectangle ms = getStaticPlot()->getMapSize();
  ms.setExtension(arrowlength * 1.5);

  glLineWidth(poptions.linewidth + 0.1);  // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  glBegin(GL_LINES);

  for (iy = iy1; iy < iy2; iy += step) {
    xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (ix = ix1; ix < ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];
      if (u[i] != fieldUndef && v[i] != fieldUndef
          && (u[i] != 0.0f || v[i] != 0.0f) && ms.isnear(gx, gy)) {
        dx = scale * u[i];
        dy = scale * v[i];
        // direction
        glVertex2f(gx, gy);
        gx = gx + dx;
        gy = gy + dy;
        glVertex2f(gx, gy);

        // arrow (drawn as two lines)
        glVertex2f(gx, gy);
        glVertex2f(gx + afac * dx + sfac * dy, gy + afac * dy - sfac * dx);
        glVertex2f(gx, gy);
        glVertex2f(gx + afac * dx - sfac * dy, gy + afac * dy + sfac * dx);
      }
    }
  }
  glEnd();

  getStaticPlot()->UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

// PURPOSE:   plot vector field as arrows (wind,sea current,...)
// ALGORITHM: Fields u(0) v(1) colorfield(2)
bool FieldPlot::plotVectorColour()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(3))
    return false;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  // convert vectors to correct projection
  vector<float*> uv = prepareVectors(x, y, poptions.rotateVectors);
  if (uv.size() != 2)
    return false;
  float *u = uv[0];
  float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, step, sdist);
  int xstep = step;

  int i, ix, iy, l;
  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  float* limits = 0;
  GLfloat* rgb = 0;

  int nlim = poptions.limits.size();
  int ncol = poptions.colours.size();

  if (nlim >= 1 && ncol >= 2) {

    if (nlim > ncol - 1)
      nlim = ncol - 1;
    if (ncol > nlim + 1)
      ncol = nlim + 1;
    limits = new float[nlim];
    rgb = new GLfloat[ncol * 3];
    for (i = 0; i < nlim; i++)
      limits[i] = poptions.limits[i];
    for (i = 0; i < ncol; i++) {
      rgb[i * 3 + 0] = poptions.colours[i].fR();
      rgb[i * 3 + 1] = poptions.colours[i].fG();
      rgb[i * 3 + 2] = poptions.colours[i].fB();
    }

  } else {

    // default, should be handled when reading setup, if allowed...
    const int maxdef = 4;
    float rgbdef[maxdef][3] = { { 0.5, 0.5, 0.5 }, { 0, 0, 0 }, { 0, 1, 1 }, {
        1, 0, 0 } };

    ncol = maxdef;
    nlim = maxdef - 1;

    float fmin = fieldUndef, fmax = -fieldUndef;
    for (i = 0; i < nx * ny; ++i) {
      if (fields[2]->data[i] != fieldUndef) {
        if (fmin > fields[2]->data[i])
          fmin = fields[2]->data[i];
        if (fmax < fields[2]->data[i])
          fmax = fields[2]->data[i];
      }
    }
    if (fmin > fmax)
      return false;

    limits = new float[nlim];
    rgb = new GLfloat[ncol * 3];
    float dlim = (fmax - fmin) / float(ncol);
    for (i = 0; i < nlim; i++)
      limits[i] = fmin + dlim * float(i + 1);
    for (i = 0; i < ncol; i++) {
      rgb[i * 3 + 0] = rgbdef[i][0];
      rgb[i * 3 + 1] = rgbdef[i][1];
      rgb[i * 3 + 2] = rgbdef[i][2];
    }

  }

  float unitlength = poptions.vectorunit;

  // length if abs(vector) = unitlength
  float arrowlength = sdist;

  float scale = arrowlength / unitlength;

  // for annotations .... should probably be resized if very small or large...
  vectorAnnotationSize = arrowlength;
  vectorAnnotationText = miutil::from_number(unitlength)
      + poptions.vectorunitname;

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float gx, gy, dx, dy;

  ix1 -= step;
  if (ix1 < 0)
    ix1 = 0;
  iy1 -= step;
  if (iy1 < 0)
    iy1 = 0;
  ix2 += (step + 1);
  if (ix2 > nx)
    ix2 = nx;
  iy2 += (step + 1);
  if (iy2 > ny)
    iy2 = ny;

  Rectangle ms = getStaticPlot()->getMapSize();
  ms.setExtension(arrowlength * 1.5);

  glLineWidth(poptions.linewidth + 0.1);  // +0.1 to avoid MesaGL coredump

  glBegin(GL_LINES);

  for (iy = iy1; iy < iy2; iy += step) {
    xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (ix = ix1; ix < ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];

      if (u[i] != fieldUndef && v[i] != fieldUndef
          && (u[i] != 0.0f || v[i] != 0.0f) && fields[2]->data[i] != fieldUndef
          && ms.isnear(gx, gy)) {

        l = 0;
        while (l < nlim && fields[2]->data[i] > limits[l])
          l++;
        glColor3fv(&rgb[l * 3]);

        dx = scale * u[i];
        dy = scale * v[i];

        // direction
        glVertex2f(gx, gy);
        gx = gx + dx;
        gy = gy + dy;
        glVertex2f(gx, gy);

        // arrow (drawn as two lines)
        glVertex2f(gx, gy);
        glVertex2f(gx + afac * dx + sfac * dy, gy + afac * dy - sfac * dx);
        glVertex2f(gx, gy);
        glVertex2f(gx + afac * dx - sfac * dy, gy + afac * dy + sfac * dx);
      }
    }
  }
  glEnd();

  getStaticPlot()->UpdateOutput();

  delete[] rgb;
  delete[] limits;

  glDisable(GL_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

/* plot true north direction field as arrows (wave,...) */
bool FieldPlot::plotDirection()
{
  METLIBS_LOG_SCOPE();

  if (fields.size() == 2)
    return plotDirectionColour();

  if (not checkFields(1))
    return false;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  // convert directions to vectors in correct projection
  vector<float*> uv = prepareDirectionVectors(x, y);
  if (uv.size() != 2)
    return false;
  float *u = uv[0];
  float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, step, sdist);
  int xstep = step;

  int i, ix, iy;
  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  // length if abs(vector) = 1
  float arrowlength = sdist;
  float unitlength = poptions.vectorunit;

  float scale = arrowlength / unitlength;

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float gx, gy, dx, dy;

  ix1 -= step;
  if (ix1 < 0)
    ix1 = 0;
  iy1 -= step;
  if (iy1 < 0)
    iy1 = 0;
  ix2 += (step + 1);
  if (ix2 > nx)
    ix2 = nx;
  iy2 += (step + 1);
  if (iy2 > ny)
    iy2 = ny;

  Rectangle ms = getStaticPlot()->getMapSize();
  ms.setExtension(arrowlength * 1.0);

  glLineWidth(poptions.linewidth + 0.1);  // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  glBegin(GL_LINES);

  for (iy = iy1; iy < iy2; iy += step) {
    xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (ix = ix1; ix < ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];
      if (u[i] != fieldUndef && v[i] != fieldUndef && ms.isnear(gx, gy)) {
        dx = scale * u[i];
        dy = scale * v[i];

        // direction
        glVertex2f(gx, gy);
        gx = gx + dx;
        gy = gy + dy;
        glVertex2f(gx, gy);

        // arrow (drawn as two lines)
        glVertex2f(gx, gy);
        glVertex2f(gx + afac * dx + sfac * dy, gy + afac * dy - sfac * dx);
        glVertex2f(gx, gy);
        glVertex2f(gx + afac * dx - sfac * dy, gy + afac * dy + sfac * dx);
      }
    }
  }
  glEnd();

  getStaticPlot()->UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

/*
 PURPOSE:   plot true north direction field as arrows (wave,...)
 * ALGORITHM: Fields: dd(0) colourfield(1)
 */
bool FieldPlot::plotDirectionColour()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(2))
    return false;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  // convert directions to vectors in correct projection
  vector<float*> uv = prepareDirectionVectors(x, y);
  if (uv.size() != 2)
    return false;
  float *u = uv[0];
  float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, step, sdist);
  int xstep = step;

  int i, ix, iy, l;
  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  float* limits = 0;
  GLfloat* rgb = 0;

  int nlim = poptions.limits.size();
  int ncol = poptions.colours.size();

  if (nlim >= 1 && ncol >= 2) {

    if (nlim > ncol - 1)
      nlim = ncol - 1;
    if (ncol > nlim + 1)
      ncol = nlim + 1;
    limits = new float[nlim];
    rgb = new GLfloat[ncol * 3];
    for (i = 0; i < nlim; i++)
      limits[i] = poptions.limits[i];
    for (i = 0; i < ncol; i++) {
      rgb[i * 3 + 0] = poptions.colours[i].fR();
      rgb[i * 3 + 1] = poptions.colours[i].fG();
      rgb[i * 3 + 2] = poptions.colours[i].fB();
    }

  } else {

    // default, should be handled when reading setup, if allowed...
    const int maxdef = 4;
    float rgbdef[maxdef][3] = { { 0.5, 0.5, 0.5 }, { 0, 0, 0 }, { 0, 1, 1 }, {
        1, 0, 0 } };

    ncol = maxdef;
    nlim = maxdef - 1;

    float fmin = fieldUndef, fmax = -fieldUndef;
    for (i = 0; i < nx * ny; ++i) {
      if (fields[1]->data[i] != fieldUndef) {
        if (fmin > fields[1]->data[i])
          fmin = fields[1]->data[i];
        if (fmax < fields[1]->data[i])
          fmax = fields[1]->data[i];
      }
    }
    if (fmin > fmax)
      return false;

    limits = new float[nlim];
    rgb = new GLfloat[ncol * 3];
    float dlim = (fmax - fmin) / float(ncol);
    for (i = 0; i < nlim; i++)
      limits[i] = fmin + dlim * float(i + 1);
    for (i = 0; i < ncol; i++) {
      rgb[i * 3 + 0] = rgbdef[i][0];
      rgb[i * 3 + 1] = rgbdef[i][1];
      rgb[i * 3 + 2] = rgbdef[i][2];
    }

  }
  //##############################################################
  //  METLIBS_LOG_DEBUG("--------------");
  //  METLIBS_LOG_DEBUG(" nlim,ncol: "<<nlim<<" "<<ncol);
  //  for (i=0; i<nlim; i++)
  //    METLIBS_LOG_DEBUG("   RGB: "<<rgb[i*3+0]<<" "<<rgb[i*3+1]<<" "<<rgb[i*3+2]
  //        <<"  lim= "<<limits[i]);
  //  i=nlim;
  //  METLIBS_LOG_DEBUG("   RGB: "<<rgb[i*3+0]<<" "<<rgb[i*3+1]<<" "<<rgb[i*3+2]);
  //##############################################################

  // length if abs(vector) = 1
  float arrowlength = sdist;
  float unitlength = poptions.vectorunit;
  float scale = arrowlength / unitlength;

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float gx, gy, dx, dy;

  ix1 -= step;
  if (ix1 < 0)
    ix1 = 0;
  iy1 -= step;
  if (iy1 < 0)
    iy1 = 0;
  ix2 += (step + 1);
  if (ix2 > nx)
    ix2 = nx;
  iy2 += (step + 1);
  if (iy2 > ny)
    iy2 = ny;

  Rectangle ms = getStaticPlot()->getMapSize();
  ms.setExtension(arrowlength * 1.0);

  glLineWidth(poptions.linewidth + 0.1);  // +0.1 to avoid MesaGL coredump

  glBegin(GL_LINES);

  for (iy = iy1; iy < iy2; iy += step) {
    xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (ix = ix1; ix < ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];
      if (u[i] != fieldUndef && v[i] != fieldUndef
          && fields[1]->data[i] != fieldUndef && ms.isnear(gx, gy)) {

        l = 0;
        while (l < nlim && fields[1]->data[i] > limits[l])
          l++;
        glColor3fv(&rgb[l * 3]);

        dx = scale * u[i];
        dy = scale * v[i];

        // direction
        glVertex2f(gx, gy);
        gx = gx + dx;
        gy = gy + dy;
        glVertex2f(gx, gy);

        // arrow (drawn as two lines)
        glVertex2f(gx, gy);
        glVertex2f(gx + afac * dx + sfac * dy, gy + afac * dy - sfac * dx);
        glVertex2f(gx, gy);
        glVertex2f(gx + afac * dx - sfac * dy, gy + afac * dy + sfac * dx);
      }
    }
  }
  glEnd();

  getStaticPlot()->UpdateOutput();

  delete[] rgb;
  delete[] limits;

  glDisable(GL_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

//  plot scalar field as contour lines
bool FieldPlot::plotContour()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  int ipart[4];

  const int mmm = 2;
  const int mmmUsed = 256;

  float xylim[4], chxlab, chylab;
  int ismooth, labfmt[3], ibcol;
  int idraw, nlines, nlim;
  int ncol, icol[mmmUsed], ntyp, ityp[mmmUsed], nwid, iwid[mmmUsed];
  float zrange[2], zstep, zoff, rlim[mmm];
  float rlines[poptions.linevalues.size()];
  int idraw2, nlines2, nlim2;
  int ncol2, icol2[mmmUsed], ntyp2, ityp2[mmmUsed], nwid2, iwid2[mmmUsed];
  float zrange2[2], zstep2 = 0, zoff2 = 0, rlines2[mmmUsed], rlim2[mmm];
  int ibmap, lbmap, kbmap[mmm], nxbmap, nybmap;
  float rbmap[4];

  int ix1, ix2, iy1, iy2;
  bool res = true;
  float *x = 0, *y = 0;

  //Resampling disabled
  int factor = 1; //resamplingFactor(nx, ny);

  if (factor < 2)
    factor = 1;

  // convert gridpoints to correct projection
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2, factor)) {
    METLIBS_LOG_ERROR("getGridPoints returned false");
    return false;
  }

  const int rnx = fields[0]->nx / factor, rny = fields[0]->ny / factor;

  // Create a resampled data array to pass to the contour function.
  float *data;
  if (factor != 1) {
    METLIBS_LOG_INFO("Resampled field from" << LOGVAL(fields[0]->nx) << LOGVAL(fields[0]->ny)
        << " to" << LOGVAL(rnx) << LOGVAL(rny));
    data = new float[rnx * rny];
    int i = 0;
    for (int iy = 0; iy < rny; ++iy) {
      int j = 0;
      for (int ix = 0; ix < rnx; ++ix) {
        data[(i * rnx) + j] = fields[0]->data[(iy * fields[0]->nx * factor) + (ix * factor)];
        ++j;
      }
      ++i;
    }
  } else
    data = fields[0]->data;

  if (ix1 >= ix2 || iy1 >= iy2)
    return false;
  if (ix1 >= rnx || ix2 < 0 || iy1 >= rny || iy2 < 0)
    return false;

  if (getColourMode() && poptions.frame)
    plotFrame(rnx, rny, x, y);

  ipart[0] = ix1;
  ipart[1] = ix2;
  ipart[2] = iy1;
  ipart[3] = iy2;

  xylim[0] = getStaticPlot()->getMapSize().x1;
  xylim[1] = getStaticPlot()->getMapSize().x2;
  xylim[2] = getStaticPlot()->getMapSize().y1;
  xylim[3] = getStaticPlot()->getMapSize().y2;

  if (poptions.valueLabel == 0)
    labfmt[0] = 0;
  else
    labfmt[0] = -1;
  labfmt[1] = 0;
  labfmt[2] = 0;
  ibcol = -1;

  if (labfmt[0] != 0) {
    float fontsize = 10. * poptions.labelSize;

    getStaticPlot()->getFontPack()->set(poptions.fontname, poptions.fontface, fontsize);
    getStaticPlot()->getFontPack()->getCharSize('0', chxlab, chylab);

    // the real height for numbers 0-9 (width is ok)
    chylab *= 0.75;
  } else {
    chxlab = chylab = 1.0;
  }

  zstep = poptions.lineinterval;
  zoff = poptions.base;

  if (poptions.linevalues.size() > 0) {
    nlines = poptions.linevalues.size();
    if (nlines > mmmUsed)
      nlines = mmmUsed;
    for (int ii = 0; ii < nlines; ii++) {
      rlines[ii] = poptions.linevalues[ii];
    }
    idraw = 3;
  } else if (poptions.loglinevalues.size() > 0) {
    nlines = poptions.loglinevalues.size();
    if (nlines > mmmUsed)
      nlines = mmmUsed;
    for (int ii = 0; ii < nlines; ii++) {
      rlines[ii] = poptions.loglinevalues[ii];
    }
    idraw = 4;
  } else {
    nlines = 0;
    idraw = 1;
    if (poptions.zeroLine == 0) {
      idraw = 2;
    }
  }

  zrange[0] = +1.;
  zrange[1] = -1.;
  zrange2[0] = +1.;
  zrange2[1] = -1.;

  if (poptions.minvalue > -fieldUndef || poptions.maxvalue < fieldUndef) {
    zrange[0] = poptions.minvalue;
    zrange[1] = poptions.maxvalue;
  }

  ncol = 1;
  icol[0] = -1; // -1: set colour below
  // otherwise index in poptions.colours[]
  ntyp = 1;
  ityp[0] = -1;
  nwid = 1;
  iwid[0] = -1;
  nlim = 0;
  rlim[0] = 0.;

  nlines2 = 0;
  ncol2 = 1;
  icol2[0] = -1;
  ntyp2 = 1;
  ityp2[0] = -1;
  nwid2 = 1;
  iwid2[0] = -1;
  nlim2 = 0;
  rlim2[0] = 0.;

  ismooth = poptions.lineSmooth;
  if (ismooth < 0)
    ismooth = 0;

  ibmap = 0;
  lbmap = 0;
  nxbmap = 0;
  nybmap = 0;

  if (poptions.contourShading == 0 && !poptions.options_1)
    idraw = 0;

  //Plot colour shading
  if (poptions.contourShading != 0) {

    int idraw2 = 0;

    if (poptions.antialiasing)
      glDisable(GL_MULTISAMPLE);

    METLIBS_LOG_TIME("contour");
    res = contour(rnx, rny, data, x, y, ipart, 2, NULL, xylim, idraw, zrange,
        zstep, zoff, nlines, rlines, ncol, icol, ntyp, ityp, nwid, iwid, nlim,
        rlim, idraw2, zrange2, zstep2, zoff2, nlines2, rlines2, ncol2, icol2,
        ntyp2, ityp2, nwid2, iwid2, nlim2, rlim2, ismooth, labfmt, chxlab,
        chylab, ibcol, ibmap, lbmap, kbmap, nxbmap, nybmap, rbmap, getStaticPlot()->getFontPack(), poptions,
        getStaticPlot()->psoutput, fields[0]->area, fieldUndef, getModelName(), fields[0]->name,
        ftime.hour());

    if (poptions.antialiasing)
      glEnable(GL_MULTISAMPLE);
  }

  //Plot contour lines
  if (!poptions.options_1)
    idraw = 0;

  if (!poptions.options_2) {
    idraw2 = 0;
  } else {
    zstep2 = poptions.lineinterval_2;
    zoff2 = poptions.base_2;
    if (poptions.linevalues_2.size() > 0) {
      nlines2 = poptions.linevalues_2.size();
      if (nlines2 > mmmUsed)
        nlines2 = mmmUsed;
      for (int ii = 0; ii < nlines2; ii++) {
        rlines2[ii] = poptions.linevalues_2[ii];
      }
      idraw2 = 3;
    } else if (poptions.loglinevalues_2.size() > 0) {
      nlines2 = poptions.loglinevalues_2.size();
      if (nlines2 > mmmUsed)
        nlines2 = mmmUsed;
      for (int ii = 0; ii < nlines2; ii++) {
        rlines2[ii] = poptions.loglinevalues_2[ii];
      }
      idraw2 = 4;
    } else {
      idraw2 = 1;
      if (poptions.zeroLine == 0) {
        idraw2 = 2;
      }
    }
  }

  if (idraw > 0 || idraw2 > 0) {

    if (getColourMode()) {
      if (poptions.colours.size() > 1) {
        if (idraw > 0 && idraw2 > 0) {
          icol[0] = 0;
          icol2[0] = 1;
        } else {
          ncol = poptions.colours.size();
          if (ncol > mmmUsed)
            ncol = mmmUsed;
          for (int i = 0; i < ncol; ++i)
            icol[i] = i;
        }
      } else if (idraw > 0) {
        glColor3ubv(poptions.linecolour.RGB());
      } else {
        glColor3ubv(poptions.linecolour_2.RGB());
      }
    } else if (!getColourMode()) {
      // drawing in overlay ... NOT USED !!!!!!!!!!!!!!
      glIndexi(poptions.linecolour.Index());
    }

    if (poptions.minvalue_2 > -fieldUndef || poptions.maxvalue_2 < fieldUndef) {
      zrange2[0] = poptions.minvalue_2;
      zrange2[1] = poptions.maxvalue_2;
    }

    if (poptions.linewidths.size() == 1) {
      glLineWidth(poptions.linewidth);
    } else {
      if (idraw2 > 0) { // two set of plot options
        iwid[0] = 0;
        iwid2[0] = 1;
      } else {      // one set of plot options, different lines
        nwid = poptions.linewidths.size();
        if (nwid > mmmUsed)
          nwid = mmmUsed;
        for (int i = 0; i < nwid; ++i)
          iwid[i] = i;
      }
    }

    if (poptions.linetypes.size() == 1 && poptions.linetype.stipple) {
      glLineStipple(poptions.linetype.factor, poptions.linetype.bmap);
      glEnable(GL_LINE_STIPPLE);
    } else {
      if (idraw2 > 0) { // two set of plot options
        ityp[0] = 0;
        ityp2[0] = 1;
      } else {      // one set of plot options, different lines
        ntyp = poptions.linetypes.size();
        if (ntyp > mmmUsed)
          ntyp = mmmUsed;
        for (int i = 0; i < ntyp; ++i)
          ityp[i] = i;
      }
    }

    if (!poptions.options_1)
      idraw = 0;

    //turn off contour shading
    bool contourShading = poptions.contourShading;
    poptions.contourShading = 0;

    res = contour(rnx, rny, data, x, y, ipart, 2, NULL, xylim, idraw, zrange,
        zstep, zoff, nlines, rlines, ncol, icol, ntyp, ityp, nwid, iwid, nlim,
        rlim, idraw2, zrange2, zstep2, zoff2, nlines2, rlines2, ncol2, icol2,
        ntyp2, ityp2, nwid2, iwid2, nlim2, rlim2, ismooth, labfmt, chxlab,
        chylab, ibcol, ibmap, lbmap, kbmap, nxbmap, nybmap, rbmap, getStaticPlot()->getFontPack(), poptions,
        getStaticPlot()->psoutput, fields[0]->area, fieldUndef, getModelName(), fields[0]->name,
        ftime.hour());

    //restore contour shading
    poptions.contourShading = contourShading;
  }

  if (poptions.extremeType != "None" && poptions.extremeType != "Ingen"
      && !poptions.extremeType.empty()) {
    markExtreme();
  }

  getStaticPlot()->UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  if (!res)
    METLIBS_LOG_ERROR("contour error");

  if (poptions.update_stencil)
    plotFrameStencil(rnx, rny, x, y);

  if (factor != 1)
    delete[] data;

  return true;
}

//  plot scalar field as contour lines using new contour algorithm
bool FieldPlot::plotContour2()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1)) {
    METLIBS_LOG_ERROR("no fields or no field data");
    return false;
  }

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x = 0, *y = 0;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2)) {
    METLIBS_LOG_ERROR("getGridPoints returned false");
    return false;
  }

  if (ix1 >= ix2 || iy1 >= iy2)
    return false;
  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;
  if (ix1 >= nx || ix2 < 0 || iy1 >= ny || iy2 < 0)
    return false;

  if (getColourMode() && poptions.frame)
    plotFrame(nx, ny, x, y);

  if (poptions.valueLabel)
    getStaticPlot()->getFontPack()->set(poptions.fontname, poptions.fontface, 10 * poptions.labelSize);

  {
    METLIBS_LOG_TIME("contour2");
    if (not poly_contour(nx, ny, ix1, iy1, ix2, iy2, fields[0]->data, x, y, getStaticPlot()->getFontPack(),
        poptions, fieldUndef))
      METLIBS_LOG_ERROR("contour2 error");
  }
  if (poptions.extremeType != "None" && poptions.extremeType != "Ingen"
      && !poptions.extremeType.empty())
    markExtreme();

  getStaticPlot()->UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

// plot scalar field as boxes with filled with patterns (cloud)
//not used, do not work properly
bool FieldPlot::plotBox_pattern()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  int i, ix, iy, i1, i2;

  // convert gridbox corners to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  const int nx = fields[0]->nx, nxc = nx;
  const int ny = fields[0]->ny;
  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  const int npattern = 4;
  //float clim[npattern+1] = { 30., 50., 70., 90., 0.9e+35 };
  float clim[npattern + 1] = { 25., 50., 75., 95., 0.9e+35 };
  int step[npattern] = { 16, 8, 4, 1 };
  int mark[npattern] = { 2, 2, 2, 1 };
  GLubyte pattern[npattern][128], bit = 1;
  int istart, jstart, j, b, w;
  float cmin, cmax;

  for (int n = 0; n < npattern; n++) {
    for (i = 0; i < 128; i++)
      pattern[n][i] = 0;
    for (jstart = 0; jstart < mark[n]; jstart++) {
      for (istart = 0; istart < mark[n]; istart++) {
        for (j = jstart; j < 32; j += step[n]) {
          for (i = istart; i < 32; i += step[n]) {
            b = i + j * 32;
            w = b / 8;
            b = 7 - (b - w * 8);
            pattern[n][w] = pattern[n][w] | (bit << b);
          }
        }
      }
    }
  }

  glShadeModel(GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_POLYGON_STIPPLE);

  glColor3ubv(poptions.linecolour.RGB());

  ix2++;
  iy2++;

  for (iy = iy1; iy < iy2; iy++) {
    i2 = ix1 - 1;

    while (i2 < ix2 - 1) {

      i1 = i2 + 1;

      // find start of cloud area
      while (i1 < ix2
          && (fields[0]->data[iy * nx + i1] < clim[0]
              || fields[0]->data[iy * nx + i1] == fieldUndef))
        i1++;
      if (i1 < ix2) {

        int n = 1;
        while (n < npattern && fields[0]->data[iy * nx + i1] > clim[n])
          n++;
        n--;
        cmin = clim[n];
        cmax = clim[n + 1];

        i2 = i1 + 1;
        // find end of cloud area
        while (i2 < ix2 && fields[0]->data[iy * nx + i2] >= cmin
            && fields[0]->data[iy * nx + i2] < cmax)
          i2++;
        i2++;

        glPolygonStipple(&pattern[n][0]);
        glBegin(GL_QUAD_STRIP);

        for (ix = i1; ix < i2; ix++) {
          glVertex2f(x[(iy + 1) * nxc + ix], y[(iy + 1) * nxc + ix]);
          glVertex2f(x[iy * nxc + ix], y[iy * nxc + ix]);
        }
        glEnd();
        i2 -= 2;
        getStaticPlot()->UpdateOutput();
      } else
        i2 = nx;

    }
  }

  getStaticPlot()->UpdateOutput();

  glDisable(GL_POLYGON_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);
  return true;
}

// plot scalar field with RGBA (RGB=constant)
bool FieldPlot::plotBox_alpha_shade()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  int ix, iy, i1, i2;
  const int nx = fields[0]->nx;
  const int ny = fields[0]->ny;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  float cmin, cmax;

  //cmin=0.;  cmax=100.;

  if (poptions.minvalue > -fieldUndef && poptions.maxvalue < fieldUndef) {
    cmin = poptions.minvalue;
    cmax = poptions.maxvalue;
  } else {
    //##### not nice in timeseries... !!!!!!!!!!!!!!!!!!!!!
    cmin = fieldUndef;
    cmax = -fieldUndef;
    for (int i = 0; i < nx * ny; ++i) {
      if (fields[0]->data[i] != fieldUndef) {
        if (cmin > fields[0]->data[i])
          cmin = fields[0]->data[i];
        if (cmax < fields[0]->data[i])
          cmax = fields[0]->data[i];
      }
    }
  }

  glShadeModel(GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float red = poptions.linecolour.fR(), green = poptions.linecolour.fG(), blue =
      poptions.linecolour.fB(), alpha;

  ix2++;
  iy2++;

  for (iy = iy1; iy < iy2; iy++) {
    i2 = ix1 - 1;

    while (i2 < ix2 - 1) {

      i1 = i2 + 1;

      // find start of cloud area
      while (i1 < ix2
          && (fields[0]->data[iy * nx + i1] < cmin
              || fields[0]->data[iy * nx + i1] == fieldUndef))
        i1++;
      if (i1 < ix2) {

        i2 = i1 + 1;
        // find end of cloud area
        while (i2 < ix2 && fields[0]->data[iy * nx + i2] >= cmax
            && fields[0]->data[iy * nx + i2] != fieldUndef)
          i2++;

        i2++;
        glBegin(GL_QUAD_STRIP);

        for (ix = i1; ix < i2; ix++) {
          glVertex2f(x[(iy + 1) * nx + ix], y[(iy + 1) * nx + ix]);
          if (ix > i1) {
            alpha = (fields[0]->data[iy * nx + ix] - cmin) / (cmax - cmin);
            //if (fields[0]->data[iy*nx+ix-1]==fieldUndef) alpha=0.;
            //if (alpha < 0.) alpha=0.;
            //if (alpha > 1.) alpha=1.;
            glColor4f(red, green, blue, alpha);
          }
          glVertex2f(x[iy * nx + ix], y[iy * nx + ix]);
        }
        glEnd();
        i2 -= 2;
      } else
        i2 = nx;

    }
  }

  getStaticPlot()->UpdateOutput();

  glDisable(GL_BLEND);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

//  plot some scalar field values with RGBA (RGB=constant)
bool FieldPlot::plotAlarmBox()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2, 1, true))
    return false;

  int ix, iy, i1, i2;
  const int nx = fields[0]->nx, nxc = nx + 1;
  const int ny = fields[0]->ny;

  // vmin,vmax: ok range, without alarm !!!
  float vmin = -fieldUndef;
  float vmax = fieldUndef;

  int sf = poptions.forecastLength.size();
  int s1 = poptions.forecastValueMin.size();
  int s2 = poptions.forecastValueMax.size();

  // rather print error message than do something wrong

  if (sf > 1) {

    int fc = fields[0]->forecastHour; // forecast hour

    if (fc < poptions.forecastLength[0])
      return false;
    if (fc > poptions.forecastLength[sf - 1])
      return false;

    int i = 1;
    while (i < sf && fc > poptions.forecastLength[i])
      i++;
    if (i == sf)
      i = sf - 1;

    float r1 = float(poptions.forecastLength[i] - fc)
        / float(poptions.forecastLength[i] - poptions.forecastLength[i - 1]);
    float r2 = 1. - r1;

    if (sf == s1 && s2 == 0) {

      vmin = r1 * poptions.forecastValueMin[i - 1]
          + r2 * poptions.forecastValueMin[i];

    } else if (sf == s2 && s1 == 0) {

      vmax = r1 * poptions.forecastValueMax[i - 1]
          + r2 * poptions.forecastValueMax[i];

    } else if (sf == s1 && sf == s2) {

      vmin = r1 * poptions.forecastValueMin[i - 1]
          + r2 * poptions.forecastValueMin[i];
      vmax = r1 * poptions.forecastValueMax[i - 1]
          + r2 * poptions.forecastValueMax[i];

    } else {
      METLIBS_LOG_ERROR("ERROR in setup!");
      return false;
    }

  } else if (sf == 0 && s1 == 0 && s2 == 0) {

    vmin = poptions.minvalue;
    vmax = poptions.maxvalue;

  } else {

    METLIBS_LOG_ERROR("ERROR in setup!");
    return false;

  }

  if (poptions.frame) {
    plotFrame(nxc, ny + 1, x, y);
  }

  glShadeModel(GL_FLAT);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glColor3ubv(poptions.linecolour.RGB());

  ix2++;
  iy2++;

  for (iy = iy1; iy < iy2; iy++) {
    i2 = ix1 - 1;
    while (i2 < ix2 - 1) {

      i1 = i2 + 1;

      // find start of alarm area
      while (i1 < ix2
          && (fields[0]->data[iy * nx + i1] == fieldUndef
              || (fields[0]->data[iy * nx + i1] < vmin
                  || fields[0]->data[iy * nx + i1] > vmax)))
        i1++;
      if (i1 < ix2) {

        i2 = i1 + 1;
        // find end of alarm area
        while (i2 < ix2 && fields[0]->data[iy * nx + i2] != fieldUndef
            && fields[0]->data[iy * nx + i2] >= vmin
            && fields[0]->data[iy * nx + i2] <= vmax)
          i2++;

        i2++;
        glBegin(GL_QUAD_STRIP);

        for (ix = i1; ix < i2; ix++) {

          glVertex2f(x[(iy + 1) * nxc + ix], y[(iy + 1) * nxc + ix]);
          glVertex2f(x[iy * nxc + ix], y[iy * nxc + ix]);
        }
        glEnd();
        i2 -= 2;
      } else
        i2 = nx;

    }
  }

  getStaticPlot()->UpdateOutput();

  if (poptions.update_stencil)
    plotFrameStencil(nxc, ny + 1, x, y);

  return true;
}

bool FieldPlot::plotFillCellExt()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;
  // projection is not equal...
  if (!getStaticPlot()->getMapArea().P().isAlmostEqual(fields[0]->area.P())) {
    return plotFillCell();
  }
  // plotPixmap does not work when plotting fields
  // in geographic projection....
  if (fields[0]->area.P().isGeographic()) {
    return plotFillCell();
  }

  return plotPixmap();
}

unsigned char * FieldPlot::createRGBAImage(Field * field)
{
  METLIBS_LOG_SCOPE();

  int nx = field->nx;
  int ny = field->ny;
  Colour pixelColor;
  unsigned char * cimage = new unsigned char[4 * nx * ny];
  for (int iy = 0; iy < ny; iy++)
    for (int ix = 0; ix < nx; ix++) {
      // The index for bitmap
      int newi = (iy * nx + ix) * 4;
      // Index in field
      int oldi = ix + (iy * nx);
      float value = field->data[oldi];
      if (value == fieldUndef){
        cimage[newi + 0] = 0;
        cimage[newi + 1] = 0;
        cimage[newi + 2] = 0;
        cimage[newi + 3] = 0;

        continue;
      }
      if (value >= poptions.minvalue && value <= poptions.maxvalue) {

        // set pixel value
        // Don't divide by poptions.palettecolours(_cold).size() if repeat not set.
        int index = 0;
        if (poptions.linevalues.size() == 0) {

          if (value > poptions.base) {
            if (poptions.repeat) {
              index = int((value - poptions.base) / poptions.lineinterval)
                  % poptions.palettecolours.size();
            } else {
              // Just divide by lineintervall...
              index = int((value - poptions.base) / poptions.lineinterval);
            }
            if (index > poptions.palettecolours.size() - 1)
              index = poptions.palettecolours.size() - 1;
            if (index < 0)
              index = 0;
            pixelColor.set(poptions.palettecolours[index].R(),
                poptions.palettecolours[index].G(),
                poptions.palettecolours[index].B(),
                poptions.palettecolours[index].A());
          } else if (poptions.palettecolours_cold.size() != 0) {
            float base = poptions.lineinterval
                * poptions.palettecolours_cold.size() - poptions.base;
            if (poptions.repeat) {
              index = int((value + base) / poptions.lineinterval)
                  % poptions.palettecolours_cold.size();
            } else {
              index = int((value + base) / poptions.lineinterval);
            }
            index = poptions.palettecolours_cold.size() - 1 - index;
            if (index > poptions.palettecolours_cold.size() - 1)
              index = poptions.palettecolours_cold.size() - 1;
            if (index < 0)
              index = 0;
            pixelColor.set(poptions.palettecolours_cold[index].R(),
                poptions.palettecolours_cold[index].G(),
                poptions.palettecolours_cold[index].B(),
                poptions.palettecolours_cold[index].A());
          } else {
            // Use index 0...
            pixelColor.set(poptions.palettecolours[index].R(),
                poptions.palettecolours[index].G(),
                poptions.palettecolours[index].B(),
                poptions.palettecolours[index].A());
          }
        } else {
          std::vector<float>::const_iterator it = poptions.linevalues.begin();
          while (*it < value && it != poptions.linevalues.end()) {
            it++;
          }
          if (it == poptions.linevalues.begin())
            continue; //less than first limit
          index = it - poptions.linevalues.begin() - 1;
          pixelColor.set(poptions.palettecolours[index].R(),
              poptions.palettecolours[index].G(),
              poptions.palettecolours[index].B(),
              poptions.palettecolours[index].A());
        }
        cimage[newi + 0] = pixelColor.R();
        cimage[newi + 1] = pixelColor.G();
        cimage[newi + 2] = pixelColor.B();
        cimage[newi + 3] = pixelColor.A();
      }

    }
  return cimage;
}

unsigned char * FieldPlot::resampleImage(int& currwid, int& currhei,
    int& bmStartx, int& bmStarty, float& scalex, float& scaley, int& nx,
    int& ny)
{

  METLIBS_LOG_SCOPE(LOGVAL(scalex));

  unsigned char * cimage;
  int irs = 1;            // resample-size

  GLint maxdims[2];      // find OpenGL maximums
  glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxdims);
  int maxww = maxdims[0];
  int maxhh = maxdims[1];
  int orignx = nx;
  if (currwid > maxww || currhei > maxhh) {

    if ((currwid - maxww) > (currhei - maxhh)) {
      irs = (currwid / maxww) + 1;
    } else {
      irs = (currhei / maxhh) + 1;
    }

    currwid /= irs;
    currhei /= irs;
    bmStartx /= irs;
    bmStarty /= irs;
    nx /= irs;
    ny /= irs;
    scalex *= irs;
    scaley *= irs;

    // check if correct resampling already available..
    if (irs != previrs) {
      METLIBS_LOG_DEBUG(" resampling image:" << irs);

      previrs = irs;
      cimage = new unsigned char[4 * nx * ny];
      for (int iy = 0; iy < ny; iy++)
        for (int ix = 0; ix < nx; ix++) {
          int newi = (iy * nx + ix) * 4;
          int oldi = (irs * iy * orignx + irs * ix) * 4;
          cimage[newi + 0] = imagedata[oldi + 0];
          cimage[newi + 1] = imagedata[oldi + 1];
          cimage[newi + 2] = imagedata[oldi + 2];
          cimage[newi + 3] = imagedata[oldi + 3];
        }
      if (imagedata) {
        delete[] imagedata;
        imagedata = cimage;
      }
    }
    // Point to resampled data
    cimage = imagedata;
  } else {
    // No resampling: use original image
    cimage = imagedata;
  }

  return cimage;

}

bool FieldPlot::plotPixmap()
{

  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  // How to deal with palettecolours_cold
  // For now, don't create one
  if (poptions.palettecolours.size() == 0) {
    poptions.palettecolours = ColourShading::getColourShading("standard");
  }

  int factor = resamplingFactor(fields[0]->nx, fields[0]->ny);

  if (factor < 2)
    factor = 1;

  // convert all gridpoints only if needed by plotFrame or update_stencil
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (  poptions.update_stencil || poptions.frame ) {
    if (not getGridPoints(x, y, ix1, ix2, iy1, iy2, factor, true, true))
      return false;
  }

  const int rnx = fields[0]->nx / factor, rny = fields[0]->ny / factor;

  glLineWidth(poptions.linewidth);
  glColor3ubv(poptions.bordercolour.RGB());
  if (poptions.frame) {
    plotFrame(rnx + 1, rny + 1, x, y);
  }

  if (poptions.alpha < 255) {
    for (size_t i = 0; i < poptions.palettecolours.size(); i++) {
      poptions.palettecolours[i].set(Colour::alpha,
          (unsigned char) poptions.alpha);
    }
    for (size_t i = 0; i < poptions.palettecolours_cold.size(); i++) {
      poptions.palettecolours_cold[i].set(Colour::alpha,
          (unsigned char) poptions.alpha);
    }
  }

  // From plotFillCell ends.

  //Member variables, used in values().
  //Corners of total image (map coordinates)
  float xmin = 0.;
  float ymin = 0.;
  if (not getPoints(1, &xmin, &ymin))
    return false;
  float xmax = fields[0]->nx * fields[0]->gridResolutionX;
  float ymax = fields[0]->ny * fields[0]->gridResolutionY;

  if (not getPoints(1, &xmax, &ymax))
    return false;

  // scaling
  float scalex = getStaticPlot()->getPhysWidth() / getStaticPlot()->getPlotSize().width();
  float scaley = getStaticPlot()->getPhysHeight() / getStaticPlot()->getPlotSize().height();

  // Corners of image shown (map coordinates)
  // special care of lat/lon projection global
  // xmin is wrong for these projections...
  // handled in plotfillcellext.

  float grStartx = (getStaticPlot()->getMapSize().x1 > xmin) ? getStaticPlot()->getMapSize().x1 : xmin;
  float grStarty = (getStaticPlot()->getMapSize().y1 > ymin) ? getStaticPlot()->getMapSize().y1 : ymin;

  // Corners of total image (image coordinates)
  float x1 = getStaticPlot()->getMapSize().x1;
  float y1 = getStaticPlot()->getMapSize().y1;
  if (!getStaticPlot()->gc.getPoints(getStaticPlot()->getMapArea().P(), fields[0]->area.P(), 1, &x1, &y1))
    return false;
  float x2 = getStaticPlot()->getMapSize().x2;
  float y2 = getStaticPlot()->getMapSize().y2;
  if (!getStaticPlot()->gc.getPoints(getStaticPlot()->getMapArea().P(), fields[0]->area.P(), 1, &x2, &y2))
    return false;
  x1 /= fields[0]->gridResolutionX;
  x2 /= fields[0]->gridResolutionX;
  y1 /= fields[0]->gridResolutionY;
  y2 /= fields[0]->gridResolutionY;

  // Corners of image shown (image coordinates)
  int bmStartx = (getStaticPlot()->getMapSize().x1 > xmin) ? int(x1) : 0;
  int bmStarty = (getStaticPlot()->getMapSize().y1 > ymin) ? int(y1) : 0;
  int bmStopx = (getStaticPlot()->getMapSize().x2 < xmax) ? int(x2) : (fields[0]->nx - 1);
  int bmStopy = (getStaticPlot()->getMapSize().y2 < ymax) ? int(y2) : (fields[0]->ny - 1);

  // lower left corner of displayed image part, in map coordinates
  // (part of lower left pixel may well be outside screen)
  float xstart = bmStartx * fields[0]->gridResolutionX;
  float ystart = bmStarty * fields[0]->gridResolutionY;
  if (not getPoints(1, &xstart, &ystart))
    return false;

  //Strange, but needed
  float bmxmove = (getStaticPlot()->getMapSize().x1 > xmin) ? (xstart - grStartx) * scalex : 0;
  float bmymove = (getStaticPlot()->getMapSize().y1 > ymin) ? (ystart - grStarty) * scaley : 0;

  // update scaling with ratio image to map (was map to screen pixels)
  scalex *= fields[0]->gridResolutionX;
  scaley *= fields[0]->gridResolutionY;

  // width of image (pixels)
  int currwid = bmStopx - bmStartx + 1;  // use pixels in image
  int currhei = bmStopy - bmStarty + 1;  // use pixels in image

  /*
   create an original image from the field if not created before...
   */

  if (imagedata == NULL) {
    // Create image from field
    imagedata = createRGBAImage(fields[0]);
  }

  /*
   If rasterimage wider than OpenGL-maxsizes: For now, temporarily resample image..
   cImage: Pointer to imagedata, either sat_image or resampled data
   */
  unsigned char * cimage = resampleImage(currwid, currhei, bmStartx, bmStarty,
      scalex, scaley, fields[0]->nx, fields[0]->ny);

  // always needed (if not, slow oper...) ??????????????
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // assure valid raster position after OpenGL transformations
  grStartx += getStaticPlot()->getPlotSize().width() * 0.0001;
  grStarty += getStaticPlot()->getPlotSize().height() * 0.0001;

  glPixelZoom(scalex, scaley);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, bmStarty); //pixels
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, bmStartx); //pixels
  glPixelStorei(GL_UNPACK_ROW_LENGTH, fields[0]->nx); //pixels on image
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glRasterPos2f(grStartx, grStarty); //glcoord.

  //Strange, but needed
  //if (bmxmove<0. || bmymove<0.) glBitmap(0,0,0.,0.,bmxmove,bmymove,NULL);
  glBitmap(0, 0, 0., 0., bmxmove, bmymove, NULL);
  glDrawPixels((GLint) currwid, (GLint) currhei, GL_RGBA, GL_UNSIGNED_BYTE,
      cimage);
  //Reset gl
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glDisable(GL_BLEND);

  getStaticPlot()->UpdateOutput();
  //From plotFillCell
  if (poptions.update_stencil) {
    plotFrameStencil(rnx + 1, rny + 1, x, y);
  }
  // From plotFillCell ends.

  return true;
}

bool FieldPlot::plotFillCell()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  vector<Colour> palette;
  // How to deal with palettecolours_cold
  // For now, dont create one
  if (poptions.palettecolours.size() == 0) {
    poptions.palettecolours = ColourShading::getColourShading("standard");
  }

  int nx = fields[0]->nx;
  int ny = fields[0]->ny;
  int rnx, rny;

  // convert gridbox corners to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;

  int factor = resamplingFactor(nx, ny);
  if (factor < 2)
    factor = 1;

  rnx = nx / factor;
  rny = ny / factor;
  getGridPoints(x, y, ix1, ix2, iy1, iy2, factor, true, true); // no check here as ix2 needs to be "fixed"

  // Make sure not to wrap data when plotting data with geo proj on geo map (ECMWF data)
  if (ix2 == nx - 1)
    ix2--;

  if (ix1 > ix2 || iy1 > iy2)
    return false;

  glLineWidth(poptions.linewidth);
  glColor3ubv(poptions.bordercolour.RGB());
  if (poptions.frame)
    plotFrame(rnx + 1, rny + 1, x, y);

  //auto -> 0
  if (poptions.density == 0) {
    poptions.density = 10;
  }

  //  float dx = poptions.density*(0.1) * (x[1]-x[0]);
  //  float dy = poptions.density*(0.1) * (y[nx]-y[0]);

  if (poptions.alpha < 255) {
    for (size_t i = 0; i < poptions.palettecolours.size(); i++) {
      poptions.palettecolours[i].set(Colour::alpha,
          (unsigned char) poptions.alpha);
    }
    for (size_t i = 0; i < poptions.palettecolours_cold.size(); i++) {
      poptions.palettecolours_cold[i].set(Colour::alpha,
          (unsigned char) poptions.alpha);
    }
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  for (int iy = iy1; iy <= iy2; iy++) {
    for (int ix = ix1; ix <= ix2; ix++) {

      float value = fields[0]->data[ix * factor + (iy * (nx) * factor)];
      if (value == fieldUndef)
        continue;
      if (value >= poptions.minvalue && value <= poptions.maxvalue) {

        float x1 = x[iy * (rnx + 1) + ix];
        float x2 = x[iy * (rnx + 1) + (ix + 1)];
        float x3 = x[(iy + 1) * (rnx + 1) + (ix + 1)];
        float x4 = x[(iy + 1) * (rnx + 1) + ix];
        float y1 = y[iy * (rnx + 1) + ix];
        float y2 = y[(iy) * (rnx + 1) + (ix + 1)];
        float y3 = y[(iy + 1) * (rnx + 1) + (ix + 1)];
        float y4 = y[(iy + 1) * (rnx + 1) + (ix)];

        // set fillcolor of cell
        // Don't divide by poptions.palettecolours(_cold).size() if repeat not set.
        if (poptions.linevalues.size() == 0) {
          int index = 0;
          if (value > poptions.base) {
            if (poptions.repeat) {
              index = int((value - poptions.base) / poptions.lineinterval)
                      % poptions.palettecolours.size();
            } else {
              // Just divide by lineintervall...
              index = int((value - poptions.base) / poptions.lineinterval);
            }
            if (index > poptions.palettecolours.size() - 1)
              index = poptions.palettecolours.size() - 1;
            if (index < 0)
              index = 0;
            glColor4ubv(poptions.palettecolours[index].RGBA());
          } else if (poptions.palettecolours_cold.size() != 0) {
            float base = poptions.lineinterval
                * poptions.palettecolours_cold.size() - poptions.base;
            if (poptions.repeat) {
              index = int((value + base) / poptions.lineinterval)
                      % poptions.palettecolours_cold.size();
            } else {
              index = int((value + base) / poptions.lineinterval);
            }
            index = poptions.palettecolours_cold.size() - 1 - index;
            if (index > poptions.palettecolours_cold.size() - 1)
              index = poptions.palettecolours_cold.size() - 1;
            if (index < 0)
              index = 0;
            glColor4ubv(poptions.palettecolours_cold[index].RGBA());
          } else {
            // Use index 0...
            glColor4ubv(poptions.palettecolours[index].RGBA());
          }
        } else {
          std::vector<float>::const_iterator it = poptions.linevalues.begin();
          while (*it < value && it != poptions.linevalues.end()) {
            it++;
          }
          if (it == poptions.linevalues.begin())
            continue; //less than first limit
          glColor4ubv(
              poptions.palettecolours[it - poptions.linevalues.begin() - 1].RGBA());
        }

        // lower-left corner of gridcell
        glVertex2f(x1, y1);
        // lower-right corner of gridcell
        glVertex2f(x2, y2);
        // upper-right corner of gridcell
        glVertex2f(x3, y3);
        // upper-left corner of gridcell
        glVertex2f(x4, y4);
      }
    }
  }
  glEnd();
  glDisable(GL_BLEND);

  getStaticPlot()->UpdateOutput();

  if (poptions.update_stencil) {
    if (factor >= 2)
      plotFrameStencil(rnx + 1, rny + 1, x, y);
    else
      plotFrameStencil(nx + 1, ny + 1, x, y);
  }
  return true;
}

// plot scalar field with RGBA (RGB=constant)
bool FieldPlot::plotAlpha_shade()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  int ix, iy;
  int nx = fields[0]->nx;
  int ny = fields[0]->ny;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  float cmin, cmax;

  if (poptions.minvalue != -fieldUndef && poptions.maxvalue != fieldUndef) {
    cmin = poptions.minvalue;
    cmax = poptions.maxvalue;
  } else {
    //##### not nice in timeseries... !!!!!!!!!!!!!!!!!!!!!
    cmin = fieldUndef;
    cmax = -fieldUndef;
    for (int i = 0; i < nx * ny; ++i) {
      if (fields[0]->data[i] != fieldUndef) {
        if (cmin > fields[0]->data[i])
          cmin = fields[0]->data[i];
        if (cmax < fields[0]->data[i])
          cmax = fields[0]->data[i];
      }
    }
  }
  glShadeModel(GL_SMOOTH);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  float red = poptions.linecolour.fR(), green = poptions.linecolour.fG(), blue =
      poptions.linecolour.fB(), alpha;

  ix2++;

  for (iy = iy1; iy < iy2; iy++) {
    glBegin(GL_QUAD_STRIP);
    for (ix = ix1; ix < ix2; ix++) {

      alpha = (fields[0]->data[(iy + 1) * nx + ix] - cmin) / (cmax - cmin);
      if (fields[0]->data[(iy + 1) * nx + ix] == fieldUndef)
        alpha = 0.;
      //if (alpha < 0.) alpha=0.;
      //if (alpha > 1.) alpha=1.;
      glColor4f(red, green, blue, alpha);
      glVertex2f(x[(iy + 1) * nx + ix], y[(iy + 1) * nx + ix]);

      alpha = (fields[0]->data[iy * nx + ix] - cmin) / (cmax - cmin);
      if (fields[0]->data[iy * nx + ix] == fieldUndef)
        alpha = 0.;
      //if (alpha < 0.) alpha=0.;
      //if (alpha > 1.) alpha=1.;
      glColor4f(red, green, blue, alpha);
      glVertex2f(x[iy * nx + ix], y[iy * nx + ix]);
    }
    glEnd();
  }

  getStaticPlot()->UpdateOutput();

  glDisable(GL_BLEND);
  glShadeModel(GL_FLAT);

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

bool FieldPlot::plotFrameOnly()
{
  if (not checkFields(1))
    return false;

  int nx = fields[0]->nx;
  int ny = fields[0]->ny;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  if (poptions.frame) {
    plotFrame(nx, ny, x, y);
  }

  if (poptions.update_stencil)
    plotFrameStencil(nx, ny, x, y);

  return true;
}

// plot frame for complete field area
void FieldPlot::plotFrame(const int nx, const int ny, float *x, float *y)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(nx) << LOGVAL(ny));

  if (!getColourMode())
    return;

  if (fields.empty())
    return;
  if (!fields[0])
    return;

  // If the frame value is 2 or 3 then fill the frame with the background colour.
  if ((poptions.frame & 2) != 0) {
    plotFilledFrame(nx, ny, x, y);
    // Only plot a frame if the lowest bit is set (frame=3).
    if ((poptions.frame & 1) == 0)
      return;
  }

  glColor3ubv(poptions.bordercolour.RGB());
  //glLineWidth(1);
  glLineWidth(poptions.linewidth);

  int ix, iy;

  glBegin(GL_LINE_STRIP);
  iy = 0;
  for (ix = 0; ix < nx; ix++) {
    int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      glVertex2f(x[i], y[i]);
    }
  }
  glEnd();

  glBegin(GL_LINE_STRIP);
  ix = nx - 1;
  for (iy = 1; iy < ny; iy++) {
    int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      glVertex2f(x[i], y[i]);
    }
  }
  glEnd();

  glBegin(GL_LINE_STRIP);
  iy = ny - 1;
  for (ix = nx - 1; ix >= 0; ix--) {
    int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      glVertex2f(x[i], y[i]);
    }
  }
  glEnd();

  glBegin(GL_LINE_STRIP);
  ix = 0;
  for (iy = ny - 1; iy > 0; iy--) {
    int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      glVertex2f(x[i], y[i]);
    }
  }
  glEnd();

  // glDisable(GL_LINE_STIPPLE);

  getStaticPlot()->UpdateOutput();

  return;
}

void FieldPlot::plotFrameStencil(const int nx, const int ny, float *x, float *y)
{
  const int ix1 = 0;
  const int ix2 = nx;
  const int iy1 = 0;
  const int iy2 = ny;
  int ix, iy;

  // Fill the frame in the stencil buffer with values.
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);

  // Fill the frame in the stencil buffer with non-zero values.
  glStencilFunc(GL_ALWAYS, 1, 0x01);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glBegin(GL_POLYGON);
  iy = iy1;
  for (ix = ix1; ix < ix2; ix++) {
    int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      glVertex2f(x[i], y[i]);
    }
  }

  ix = ix2 - 1;
  for (iy = iy1 + 1; iy < iy2; iy++) {
    int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      glVertex2f(x[i], y[i]);
    }
  }

  iy = iy2 - 1;
  for (ix = ix2 - 1; ix >= ix1; ix--) {
    int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      glVertex2f(x[i], y[i]);
    }
  }

  ix = ix1;
  for (iy = iy2 - 1; iy > iy1; iy--) {
    int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      glVertex2f(x[i], y[i]);
    }
  }

  glEnd();

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
}

void FieldPlot::plotFilledFrame(const int nx, const int ny, float *x, float *y)
{
  const int ix1 = 0;
  const int ix2 = nx;
  const int iy1 = 0;
  const int iy2 = ny;
  int ix, iy;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glColor4ub(getStaticPlot()->getBackgroundColour().R(), getStaticPlot()->getBackgroundColour().G(), getStaticPlot()->getBackgroundColour().B(),
      getStaticPlot()->getBackgroundColour().A());
  glDisable(GL_BLEND);

  for (iy = iy1; iy < iy2 - 1; iy++) {
    glBegin(GL_QUAD_STRIP);
    for (ix = ix1; ix < ix2; ix++) {
      int i = (iy * nx) + ix;
      int j = ((iy + 1) * nx) + ix;
      glVertex2f(x[i], y[i]);
      glVertex2f(x[j], y[j]);
    }
    glEnd();
  }
}

/*
 Mark extremepoints in a field with L/H (Low/High), C/W (Cold/Warm) or value
 */
bool FieldPlot::markExtreme()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  int nx = fields[0]->nx;
  int ny = fields[0]->ny;

  int ix1, ix2, iy1, iy2;

  // convert gridpoints to correct projection
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  int ix, iy, n, i, j, k;
  float dx, dy, avgdist;

  //find avg. dist. between grid points
  int i1 = (ix1 > 1) ? ix1 : 1;
  int i2 = (ix2 < nx - 2) ? ix2 : nx - 2;
  int j1 = (iy1 > 1) ? iy1 : 1;
  int j2 = (iy2 < ny - 2) ? iy2 : ny - 2;
  int ixstp = (i2 - i1) / 5;
  if (ixstp < 1)
    ixstp = 1;
  int iystp = (j2 - j1) / 5;
  if (iystp < 1)
    iystp = 1;
  avgdist = 0.;
  n = 0;
  for (iy = j1; iy < j2; iy += iystp) {
    for (ix = i1; ix < i2; ix += ixstp) {
      i = iy * nx + ix;
      dx = x[i + 1] - x[i];
      dy = y[i + 1] - y[i];
      avgdist += sqrtf(dx * dx + dy * dy);
      dx = x[i + nx] - x[i];
      dy = y[i + nx] - y[i];
      avgdist += sqrtf(dx * dx + dy * dy);
      n += 2;
    }
  }
  if (n > 0)
    avgdist = avgdist / float(n);
  else
    avgdist = 1.;

  //
  if (ix1 < 1)
    ix1 = 1;
  if (ix2 > nx - 2)
    ix2 = nx - 2;
  if (iy1 < 1)
    iy1 = 1;
  if (iy2 > ny - 2)
    iy2 = ny - 2;
  ix2++;
  iy2++;

  std::string pmarks[2];
  //int bextremevalue=0;
  bool extremeString = false;
  std::string extremeValue = "none";
  std::string extremeType = miutil::to_upper(poptions.extremeType);
  vector<std::string> vextremetype = miutil::split(extremeType, "+");
  if (vextremetype.size() > 0 && !miutil::contains(vextremetype[0], "VALUE")) {
    pmarks[0] = vextremetype[0];
    extremeString = true;
  }
  if (vextremetype.size() > 1 && !miutil::contains(vextremetype[1], "VALUE")) {
    pmarks[1] = vextremetype[1];
    extremeString = true;
  }
  if (vextremetype.size() > 0
      && vextremetype[vextremetype.size() - 1] == "MINVALUE") {
    extremeValue = "min";
  } else if (vextremetype.size() > 0
      && vextremetype[vextremetype.size() - 1] == "MAXVALUE") {
    extremeValue = "max";
    pmarks[1] = pmarks[0];
  } else if (vextremetype.size() > 0
      && vextremetype[vextremetype.size() - 1] == "VALUE") {
    extremeValue = "both";
  }

  glColor3ubv(poptions.linecolour.RGB());

  float fontsize = 28. * poptions.extremeSize;
  getStaticPlot()->getFontPack()->set(poptions.fontname, poptions.fontface, fontsize);

  float chrx, chry;
  getStaticPlot()->getFontPack()->getCharSize('L', chrx, chry);
  // approx. real height for the character (width is ok)
  chry *= 0.75;
  float size = chry < chrx ? chrx : chry;

  float radius = 3.0 * size * poptions.extremeRadius;
  //int nscan = int(radius/avgdist+0.9);
  int nscan = int(radius / avgdist);
  if (nscan < 2)
    nscan = 2;
  float rg = float(nscan);

  // for scan of surrounding positions
  int mscan = nscan * 2 + 1;
  int *iscan = new int[mscan];
  float rg2 = rg * rg;
  iscan[nscan] = nscan;
  for (j = 1; j <= nscan; j++) {
    dy = float(j);
    dx = sqrtf(rg2 - dy * dy);
    i = int(dx + 0.5);
    iscan[nscan - j] = i;
    iscan[nscan + j] = i;
  }

  // for test of gradients in the field
  int igrad[4][6];
  int nscan45degrees = int(sqrtf(rg2 * 0.5) + 0.5);

  for (k = 0; k < 4; k++) {
    i = j = 0;
    if (k == 0)
      j = nscan;
    else if (k == 1)
      i = j = nscan45degrees;
    else if (k == 2)
      i = nscan;
    else {
      i = nscan45degrees;
      j = -nscan45degrees;
    }
    igrad[k][0] = -i;
    igrad[k][1] = -j;
    igrad[k][2] = i;
    igrad[k][3] = j;
    igrad[k][4] = -j * nx - i;
    igrad[k][5] = j * nx + i;
  }

  // the nearest grid points
  int near[8] = { -nx - 1, -nx, -nx + 1, -1, 1, nx - 1, nx, nx + 1 };

  float gx, gy, fpos, fmin, fmax, f, f1, f2, gbest, fgrad;
  int p, pp, l, iend, jend, ibest, jbest, ngrad, etype;
  bool ok;

  Rectangle ms = getStaticPlot()->getMapSize();
  ms.setExtension(-size * 0.5);

  for (iy = iy1; iy < iy2; iy++) {
    for (ix = ix1; ix < ix2; ix++) {
      p = iy * nx + ix;
      gx = x[p];
      gy = y[p];
      if (fields[0]->data[p] != fieldUndef && ms.isnear(gx, gy)) {
        fpos = fields[0]->data[p];

        // first check the nearest gridpoints
        fmin = fieldUndef;
        fmax = -fieldUndef;
        for (j = 0; j < 8; j++) {
          f = fields[0]->data[p + near[j]];
          if (fmin > f)
            fmin = f;
          if (fmax < f)
            fmax = f;
        }

        if (fmax != fieldUndef
            && ((fmin >= fpos && extremeValue != "max")
                || (fmax <= fpos && extremeValue != "min")) && fmin != fmax) {
          // scan a larger area

          j = (iy - nscan > 0) ? iy - nscan : 0;
          jend = (iy + nscan + 1 < ny) ? iy + nscan + 1 : ny;
          ok = true;
          etype = -1;
          vector<int> pequal;

          if (fmin >= fpos) {
            // minimum at pos ix,iy
            etype = 0;
            while (j < jend && ok) {
              n = iscan[nscan + iy - j];
              i = (ix - n > 0) ? ix - n : 0;
              iend = (ix + n + 1 < nx) ? ix + n + 1 : nx;
              for (; i < iend; i++) {
                f = fields[0]->data[j * nx + i];
                if (f != fieldUndef) {
                  if (f < fpos)
                    ok = false;
                  else if (f == fpos)
                    pequal.push_back(j * nx + i);
                }
              }
              j++;
            }
          } else {
            // maximum at pos ix,iy
            etype = 1;
            while (j < jend && ok) {
              n = iscan[nscan + iy - j];
              i = (ix - n > 0) ? ix - n : 0;
              iend = (ix + n + 1 < nx) ? ix + n + 1 : nx;
              for (; i < iend; i++) {
                f = fields[0]->data[j * nx + i];
                if (f != fieldUndef) {
                  if (f > fpos)
                    ok = false;
                  else if (f == fpos)
                    pequal.push_back(j * nx + i);
                }
              }
              j++;
            }
          }

          if (ok) {
            n = pequal.size();
            if (n < 2) {
              ibest = ix;
              jbest = iy;
            } else {
              // more than one pos with the same extreme value,
              // select the one with smallest surrounding
              // gradients in the field (seems to work...)
              gbest = fieldUndef;
              ibest = jbest = -1;
              for (l = 0; l < n; l++) {
                pp = pequal[l];
                j = pp / nx;
                i = pp - j * nx;
                fgrad = 0.;
                ngrad = 0;
                for (k = 0; k < 4; k++) {
                  i1 = i + igrad[k][0];
                  j1 = j + igrad[k][1];
                  i2 = i + igrad[k][2];
                  j2 = j + igrad[k][3];
                  if (i1 >= 0 && j1 >= 0 && i1 < nx && j1 < ny && i2 >= 0
                      && j2 >= 0 && i2 < nx && j2 < ny) {
                    f1 = fields[0]->data[pp + igrad[k][4]];
                    f2 = fields[0]->data[pp + igrad[k][5]];
                    if (f1 != fieldUndef && f2 != fieldUndef) {
                      fgrad += fabsf(f1 - f2);
                      ngrad++;
                    }
                  }
                }
                if (ngrad > 0) {
                  fgrad /= float(ngrad);
                  if (fgrad < gbest) {
                    gbest = fgrad;
                    ibest = i;
                    jbest = j;
                  } else if (fgrad == gbest) {
                    gbest = fieldUndef;
                    ibest = jbest = -1;
                  }
                }
              }
            }

            if (ibest == ix && jbest == iy) {
              if (fpos < poptions.maxvalue && fpos > poptions.minvalue) {
                // mark extreme point
                if (!extremeString) {
                  ostringstream ostr;
                  ostr.setf(ios::fixed);
                  ostr.precision(poptions.precision);
                  ostr << fpos;
                  getStaticPlot()->getFontPack()->drawStr(ostr.str().c_str(), gx - chrx * 0.5,
                      gy - chry * 0.5, 0.0);
                } else {
                  getStaticPlot()->getFontPack()->drawStr(pmarks[etype].c_str(), gx - chrx * 0.5,
                      gy - chry * 0.5, 0.0);
                  if (extremeValue != "none") {
                    float fontsize = 18. * poptions.extremeSize;
                    getStaticPlot()->getFontPack()->set(poptions.fontname, poptions.fontface, fontsize);
                    ostringstream ostr;
                    ostr.setf(ios::fixed);
                    ostr.precision(poptions.precision);
                    ostr << fpos;
                    getStaticPlot()->getFontPack()->drawStr(ostr.str().c_str(), gx - chrx * (-0.6),
                        gy - chry * 0.8, 0.0);
                  }
                  float fontsize = 28. * poptions.extremeSize;
                  getStaticPlot()->getFontPack()->set(poptions.fontname, poptions.fontface, fontsize);

                }

                //#######################################################################
                //		glBegin(GL_LINE_LOOP);
                //		glVertex2f(gx-chrx[etype]*0.5,gy-chry[etype]*0.5);
                //		glVertex2f(gx+chrx[etype]*0.5,gy-chry[etype]*0.5);
                //		glVertex2f(gx+chrx[etype]*0.5,gy+chry[etype]*0.5);
                //		glVertex2f(gx-chrx[etype]*0.5,gy+chry[etype]*0.5);
                //		glEnd();
                //#######################################################################
              }
            }
          }
        }
      }
    }
  }

  delete[] iscan;

  return true;
}

// draw the grid lines in some density
bool FieldPlot::plotGridLines()
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  int nx = fields[0]->nx;

  int ix1, ix2, iy1, iy2;

  // convert gridpoints to correct projection
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  ix2++;
  iy2++;

  int step = poptions.gridLines;
  int maxlines = poptions.gridLinesMax;

  if (maxlines > 0) {
    if ((ix2 - ix1) / step > maxlines || (iy2 - iy1) / step > maxlines)
      return false;
  }

  glColor4f(poptions.bordercolour.fR(), poptions.bordercolour.fG(),
      poptions.bordercolour.fB(), 0.5);
  glLineWidth(1.0);

  int ix, iy;
  ix1 = int(ix1 / step) * step;
  iy1 = int(iy1 / step) * step;
  int i;
  for (ix = ix1; ix < ix2; ix += step) {
    glBegin(GL_LINE_STRIP);
    for (iy = iy1; iy < iy2; iy++) {
      i = iy * nx + ix;
      glVertex2f(x[i], y[i]);
    }
    glEnd();
  }
  for (iy = iy1; iy < iy2; iy += step) {
    glBegin(GL_LINE_STRIP);
    for (ix = ix1; ix < ix2; ix++) {
      i = iy * nx + ix;
      glVertex2f(x[i], y[i]);
    }
    glEnd();
  }

  getStaticPlot()->UpdateOutput();

  return true;
}

// show areas with undefined field values
bool FieldPlot::plotUndefined()
{
  METLIBS_LOG_SCOPE();

  if (not isEnabled())
    return false;
  if (not checkFields(1))
    return false;

  int nx = fields[0]->nx;
  int ny = fields[0]->ny;

  int ix1, ix2, iy1, iy2;

  // convert gridpoints to correct projection
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2, 1, true))
    return false;
  if (ix1 >= nx || ix2 < 0 || iy1 >= ny || iy2 < 0)
    return false;

  int nxc = nx + 1;

  if (poptions.undefMasking == 1) {
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  } else {
    glLineWidth(poptions.undefLinewidth + 0.1);
    if (poptions.undefLinetype.bmap != 0xFFFF) {
      glLineStipple(1, poptions.undefLinetype.bmap);
      glEnable(GL_LINE_STIPPLE);
    }
  }

  if (poptions.undefColour == getStaticPlot()->getBackgroundColour())
    poptions.undefColour = getStaticPlot()->getBackContrastColour();

  glColor3ubv(poptions.undefColour.RGB());

  int ix, iy, ixbgn, ixend, iybgn, iyend;

  ix2++;
  iy2++;

  for (iy = iy1; iy < iy2; iy++) {

    ix = ix1;

    while (ix < ix2) {

      while (ix < ix2 && fields[0]->data[iy * nx + ix] != fieldUndef)
        ix++;

      if (ix < ix2) {

        ixbgn = ix++;
        while (ix < ix2 && fields[0]->data[iy * nx + ix] == fieldUndef)
          ix++;
        ixend = ix;

        if (poptions.undefMasking == 1) {

          glBegin(GL_QUAD_STRIP);
          for (ix = ixbgn; ix <= ixend; ix++) {
            glVertex2f(x[(iy + 1) * nxc + ix], y[(iy + 1) * nxc + ix]);
            glVertex2f(x[iy * nxc + ix], y[iy * nxc + ix]);
          }
          glEnd();

        } else {

          // drawing lines
          // here only drawing lines in grid x direction (y dir. below),
          // avoiding plot of many short lines that are connected
          // (but still drawing many "double" lines...)

          for (int iyy = iy; iyy <= iy + 1; iyy++) {
            glBegin(GL_LINE_STRIP);
            for (ix = ixbgn; ix <= ixend; ix++)
              glVertex2f(x[iyy * nxc + ix], y[iyy * nxc + ix]);
            glEnd();
          }
        }
        ix = ixend + 1;
      }
    }
  }

  if (poptions.undefMasking > 1) {

    // linedrawing in grid y direction (undefMasking>1)

    for (ix = ix1; ix < ix2; ix++) {

      iy = iy1;

      while (iy < iy2) {

        while (iy < iy2 && fields[0]->data[iy * nx + ix] != fieldUndef)
          iy++;

        if (iy < iy2) {

          iybgn = iy++;
          while (iy < iy2 && fields[0]->data[iy * nx + ix] == fieldUndef)
            iy++;
          iyend = iy;

          for (int ixx = ix; ixx <= ix + 1; ixx++) {
            glBegin(GL_LINE_STRIP);
            for (iy = iybgn; iy <= iyend; iy++)
              glVertex2f(x[iy * nxc + ixx], y[iy * nxc + ixx]);
            glEnd();
          }
          iy = iyend + 1;
        }
      }
    }
  }

  getStaticPlot()->UpdateOutput();

  glDisable(GL_LINE_STIPPLE);

  return true;
}

/*
 plot field values as numbers in each gridpoint
 skip plotting if too many (or too small) numbers
 */
bool FieldPlot::plotNumbers()
{
  METLIBS_LOG_SCOPE();

  if (fields.size() != 1)
    return false;
  if (not checkFields(1))
    return false;

  int i, ix, iy;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  int autostep;
  float dist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, 25, autostep, dist);
  if (autostep > 1)
    return false;

  ix2++;
  iy2++;

  float fontsize = 16.;
  //getStaticPlot()->getFontPack()->set("BITMAPFONT",poptions.fontface,fontsize);
  getStaticPlot()->getFontPack()->set("BITMAPFONT", "bold", fontsize);

  float chx, chy;
  getStaticPlot()->getFontPack()->getCharSize('0', chx, chy);
  // the real height for numbers 0-9 (width is ok)
  chy *= 0.75;

  float *field = fields[0]->data;
  float gx, gy, w, h;
  float hh = chy * 0.5;
  float ww = chx * 0.5;
  int iprec = 0;
  if (poptions.precision > 0) {
    iprec = poptions.precision;
  } else {
    iprec = -int(log10(fields[0]->storageScaling));
    if (iprec < 0)
      iprec = 0;
  }
  std::string str;

  glColor3ubv(poptions.linecolour.RGB());

  for (iy = iy1; iy < iy2; iy++) {
    for (ix = ix1; ix < ix2; ix++) {
      i = iy * fields[0]->nx + ix;
      gx = x[i];
      gy = y[i];

      if (field[i] != fieldUndef) {
        ostringstream ostr;
        ostr << setprecision(iprec) << setiosflags(ios::fixed) << field[i];
        str = ostr.str();
        getStaticPlot()->getFontPack()->getStringSize(str.c_str(), w, h);
        w *= 0.5;
      } else {
        str = "X";
        w = ww;
      }

      if (getStaticPlot()->getMapSize().isinside(gx - w, gy - hh)
          && getStaticPlot()->getMapSize().isinside(gx + w, gy + hh))
        getStaticPlot()->getFontPack()->drawStr(str.c_str(), gx - w, gy - hh, 0.0);
    }
  }

  getStaticPlot()->UpdateOutput();

  // draw lines/boxes at borders between gridpoints..............................

  // convert gridpoints to correct projection
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2, 1, true))
    return false;

  const int nx = fields[0]->nx + 1;

  glLineWidth(1.0);

  ix2++;
  iy2++;

  for (ix = ix1; ix < ix2; ix++) {
    glBegin(GL_LINE_STRIP);
    for (iy = iy1; iy < iy2; iy++) {
      glVertex2f(x[iy * nx + ix], y[iy * nx + ix]);
    }
    glEnd();
  }
  for (iy = iy1; iy < iy2; iy++) {
    glBegin(GL_LINE_STRIP);
    for (ix = ix1; ix < ix2; ix++) {
      i = iy * nx + ix;
      glVertex2f(x[iy * nx + ix], y[iy * nx + ix]);
    }
    glEnd();
  }

  getStaticPlot()->UpdateOutput();
  return true;
}

std::string FieldPlot::getModelName()
{
  if (checkFields(1))
    return fields[0]->modelName;
  else
    return std::string();
}

std::string FieldPlot::getTrajectoryFieldName()
{
  std::string str;
  unsigned int nf = 0;
  if (plottype == fpt_wind)
    nf = 2;
  if (plottype == fpt_vector)
    nf = 2;

  if (nf >= 2 && fields.size() >= nf) {
    bool ok = true;
    for (unsigned int i = 0; i < nf; i++) {
      if (!fields[i])
        ok = false;
      else if (!fields[i]->data)
        ok = false;
    }
    if (ok)
      str = fields[0]->fieldText;
  }
  return str;
}

bool FieldPlot::checkFields(size_t count) const
{
  if (count == 0) {
    if (fields.empty())
      return false;
    count = fields.size();
  } else if (fields.size() < count) {
    return false;
  }
  for (size_t i = 0; i < count; ++i) {
    if (not (fields[i] and fields[i]->data))
      return false;
  }
  return true;
}

bool FieldPlot::fieldsOK()
{
  return checkFields(0);
}

int FieldPlot::resamplingFactor(int nx, int ny) const
{
  float cx[2], cy[2];
  cx[0] = fields[0]->area.R().x1;
  cy[0] = fields[0]->area.R().y1;
  cx[1] = fields[0]->area.R().x2;
  cy[1] = fields[0]->area.R().y2;
  getPoints(2, cx, cy);

  double xs = getStaticPlot()->getPlotSize().width() / fabs(cx[1] - cx[0]);
  double ys = getStaticPlot()->getPlotSize().height() / fabs(cy[1] - cy[0]);
  double resamplingF = min(xs, ys);
  return int(resamplingF);
}
