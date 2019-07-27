/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2019 met.no

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

#include "diana_config.h"

//#define DEBUGPRINT

#include "diFieldPlot.h"

#include "diContouring.h"
#include "diField/VcrossUtil.h"
#include "diFieldPlotManager.h"
#include "diGLPainter.h"
#include "diGlUtilities.h"
#include "diImageGallery.h"
#include "diPlotOptions.h"
#include "diPolyContouring.h"
#include "diRasterAlarmBox.h"
#include "diRasterAlpha.h"
#include "diRasterFillCell.h"
#include "diRasterGrid.h"
#include "diRasterRGB.h"
#include "diRasterUndef.h"
#include "diRasterUtil.h"
#include "diStaticPlot.h"
#include "diUtilities.h"
#include "util/misc_util.h"
#include "util/plotoptions_util.h"
#include "util/string_util.h"

#include <mi_fieldcalc/MetConstants.h>
#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>

#include <sstream>

#define MILOGGER_CATEGORY "diana.FieldPlot"
#include <miLogger/miLogging.h>

#if defined(DEBUG_UNDEF)
#define IF_DEBUG_UNDEF(x) x
#else
#define IF_DEBUG_UNDEF(x) /* */
#endif

using namespace std;
using namespace miutil;

const int MaxWindsAuto = 40;
const int MaxArrowsAuto = 55;

// ========================================================================

FieldPlot::FieldPlot(FieldPlotManager* fieldplotm)
    : fieldplotm_(fieldplotm)
    , praster(false)
    , pshade(false)
    , vectorAnnotationSize(0)
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
  diutil::delete_all_and_clear(tmpfields);
  if (fieldplotm_)
    fieldplotm_->freeFields(fields);
  fields.clear();
}

std::string FieldPlot::getEnabledStateKey() const
{
  std::ostringstream ost;
  ost << cmd_->field.model << ':' << cmd_->field.name() << ':' << cmd_->field.reftime;
  return ost.str();
}

const Area& FieldPlot::getFieldArea() const
{
  if (fields.size() && fields[0])
    return fields[0]->area;
  else
    return getStaticPlot()->getMapArea(); // return master-area
}

//  return area for existing field
bool FieldPlot::getRealFieldArea(Area& a) const
{
  if (not checkFields(1))
    return false;
  a = fields[0]->area;
  return true;
}

int FieldPlot::getLevel() const
{
  if (not checkFields(1))
    return 0;
  return fields[0]->level;
}

void FieldPlot::changeTime(const miutil::miTime& mapTime)
{
  const bool update = (ftime.undef() || (ftime != mapTime && cmd_->time.empty()) || fields.empty());
  if (update && fieldplotm_ != 0) {
    std::vector<Field*> fv;
    fieldplotm_->makeFields(cmd_, mapTime, fv);
    setData(fv, mapTime);
  }
}

bool FieldPlot::hasData()
{
  return !fields.empty();
}

void FieldPlot::getAnnotation(string& s, Colour& c) const
{
  if (poptions.options_1)
    c = poptions.linecolour;
  else
    c = poptions.fillcolour;

  s = getPlotName();
}

bool FieldPlot::prepare(const std::string& fname, const FieldPlotCommand_cp& cmd)
{
  // merge current plotOptions (from pin) with plotOptions from setup
  miutil::KeyValue_v opts;
  fieldplotm_->getFieldPlotOptions(fname, poptions, opts);
  diutil::insert_all(opts, cmd->options());
  setPlotInfo(opts);

  // FIXME merging options should be done earlier, we should no longer modify the plot command here
  FieldPlotCommand_p cmd_opts = std::make_shared<FieldPlotCommand>(*cmd);
  cmd_opts->clearOptions();
  cmd_opts->addOptions(ooptions);
  ooptions = cmd->options();
  cmd_ = cmd_opts;

  if (poptions.maxDiagonalInMeters > -1) {
    const double diagonal = getStaticPlot()->getRequestedarea().getDiagonalInMeters();
    METLIBS_LOG_INFO("requestedarea.getDiagonalInMeters():" << diagonal << "  max:" << poptions.maxDiagonalInMeters);
    if (diagonal > poptions.maxDiagonalInMeters)
      return false;
  }

  praster = (plottype() == fpt_alpha_shade || plottype() == fpt_rgb || plottype() == fpt_alarm_box || plottype() == fpt_fill_cell);
  pshade = praster || (poptions.contourShading > 0 && (plottype() == fpt_contour || plottype() == fpt_contour1 || plottype() == fpt_contour2));

  return true;
}

const miutil::miTime& FieldPlot::getAnalysisTime() const
{
  static const miutil::miTime UNDEF;
  if (!fields.empty())
    return fields[0]->analysisTime;
  else
    return UNDEF;
}

plottimes_t FieldPlot::getFieldTimes() const
{
  return fieldplotm_->getFieldTime(std::vector<FieldPlotCommand_cp>(1, cmd_), false);
}

miutil::miTime FieldPlot::getReferenceTime() const
{
  return fieldplotm_->getFieldReferenceTime(cmd_);
}

//  set list of field-pointers, update datatime
void FieldPlot::setData(const vector<Field*>& vf, const miTime& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(vf.size()) << LOGVAL(t.isoTime()));

  clearFields();

  fields = vf;
  ftime = t;

  if (fields.empty()) {
    setPlotName("");
  } else {
    const Field* f0 = fields[0];
    setPlotName(f0->fulltext);
    vcross::Values_p p0 = f0->palette;
    if (p0 && p0->npoint() > 2) {
      poptions.palettecolours.clear();
      bool alpha = (p0->npoint() == 4);
      for (size_t i = 0; i<p0->nlevel(); ++i) {
        unsigned char r = p0->value(0,i);
        unsigned char g = p0->value(1,i);
        unsigned char b = p0->value(2,i);
        unsigned char a = 255;
        if (alpha)
          a = p0->value(3,i);
        poptions.palettecolours.push_back(Colour(r,g,b,a));
      }
    }
  }
}

namespace {
struct aTable {
  std::string colour;
  std::string pattern;
  std::string text;
};
} // namespace

void FieldPlot::getTableAnnotations(vector<string>& annos)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return;

  if (!isEnabled())
    return;

  if (!(plottype() == fpt_fill_cell || plottype() == fpt_contour
        || plottype() == fpt_contour1 || plottype() == fpt_contour2))
    return;

  if (poptions.table == 0 || (poptions.palettecolours.empty() && poptions.patterns.empty()))
    return;

  for (std::string& anno : annos) {
    if (miutil::contains(anno, "table")) {
      std::string unit = " " + poptions.legendunits;

      std::string endString;
      std::string startString;
      if (miutil::contains(anno, ",")) {
        size_t nn = anno.find_first_of(",");
        endString = anno.substr(nn);
        startString = anno.substr(0, nn);
      } else {
        startString = anno;
      }

      //if asking for spesific field
      if (miutil::contains(anno, "table=")) {
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
        int ndata = fields[0]->area.gridSize();
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
          ostr << "> " << min << unit;
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
          ostr << "> " << min << unit;
          vtable[ncodes - 1].text = ostr.str();
        } else {
          for (size_t i = 0; i < ncodes; i++) {
            ostringstream ostr;
            ostr << classSpec[i];
            vtable[i].text = ostr.str();
          }
        }

      } else {
        float max = poptions.base + poptions.lineinterval * (ncodes - 1);
        if (max < poptions.maxvalue) {
          ostringstream ostr;
          ostr << "> " << max << unit;
          vtable[ncodes + ncold - 1].text = ostr.str();
        }
        float min = poptions.base - poptions.lineinterval * (int(ncold) - 1);
        if (min > poptions.minvalue) {
          ostringstream ostr;
          ostr << "< " << min << unit;
          vtable[0].text = ostr.str();
        }

        //cold colours
        max = poptions.base;
        for (int i = ncold - 1; i > 0; i--) {
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
          min = poptions.base + poptions.lineinterval;
        }

        size_t i = 1;
        if (ncold > 0) {
          i = ncold;
          min -= poptions.lineinterval;
        }
        for (; i < ncodes + ncold - 1; i++) {
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

      annos.push_back(str);
    }
  }
}

void FieldPlot::getDataAnnotations(vector<string>& annos)
{
  METLIBS_LOG_SCOPE(LOGVAL(annos.size()));

  if (not checkFields(1))
    return;

  for (std::string& anno : annos) {

    std::string lg;
    size_t k = anno.find("$lg=");
    if (k != string::npos) {
      lg = anno.substr(k, 7) + " ";
    }

    if (miutil::contains(anno, "$referencetime")) {
      std::string refString = getAnalysisTime().format("%Y%m%d %H", "", true);
      miutil::replace(anno, "$referencetime", refString);
    }
    if (miutil::contains(anno, "$forecasthour")) {
      ostringstream ost;
      ost << fields[0]->forecastHour;
      miutil::replace(anno, "$forecasthour", ost.str());
    }
    miutil::replace(anno, "$currenttime", fields[0]->timetext);
    if (miutil::contains(anno, "$validtime")) {
      const std::string vtime = fields[0]->validFieldTime.format("%Y%m%d %A %H" + lg, "", true);
      miutil::replace(anno, "$validtime", vtime);
    }
    miutil::replace(anno, "$model", fields[0]->modelName);
    miutil::replace(anno, "$idnum", fields[0]->idnumtext);
    miutil::replace(anno, "$level", fields[0]->leveltext);

    if (miutil::contains(anno, "arrow") && vectorAnnotationSize > 0
        && not vectorAnnotationText.empty())
    {
      if (miutil::contains(anno, "arrow="))
        continue;

      std::string endString;
      std::string startString;
      if (miutil::contains(anno, ",")) {
        size_t nn = anno.find_first_of(",");
        endString = anno.substr(nn);
        startString = anno.substr(0, nn);
      } else {
        startString = anno;
      }

      std::string str = "arrow=" + miutil::from_number(vectorAnnotationSize)
          + ",tcolour=" + poptions.linecolour.Name() + endString;
      annos.push_back(str);
      str = "text=\" " + vectorAnnotationText + "\"" + ",tcolour="
          + poptions.linecolour.Name() + endString;
      annos.push_back(str);
    }
  }

  getTableAnnotations(annos);
}

void FieldPlot::plot(DiGLPainter* gl, PlotOrder zorder)
{
  if (!isEnabled() || fields.size() < 1)
    return;

  // possibly smooth all "single" fields
  for (size_t i = 0; i < fields.size(); i++) {
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
  poptions.bordercolour = getStaticPlot()->notBackgroundColour(poptions.bordercolour);
  poptions.linecolour   = getStaticPlot()->notBackgroundColour(poptions.linecolour);
  for (unsigned int i = 0; i < poptions.colours.size(); i++)
    poptions.colours[i] = getStaticPlot()->notBackgroundColour(poptions.colours[i]);

  if (poptions.antialiasing)
    gl->Enable(DiGLPainter::gl_MULTISAMPLE);
  else
    gl->Disable(DiGLPainter::gl_MULTISAMPLE);

  if (zorder == PO_SHADE_BACKGROUND) {
    // should be below all real fields
    if (poptions.gridLines > 0)
      plotGridLines(gl);
    if (poptions.gridValue > 0)
      plotNumbers(gl);

    plotUndefined(gl);
  } else if (zorder == PO_SHADE) {
    if (getShadePlot())
      plotMe(gl, zorder);
  } else if (zorder == PO_LINES) {
    if (!getShadePlot())
      plotMe(gl, zorder);
  } else if (zorder == PO_OVERLAY) {
    plotMe(gl, zorder);
  }
}

bool FieldPlot::plotMe(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(getModelName()));

  if (poptions.use_stencil || poptions.update_stencil) {
    // Enable the stencil test for masking the field to be plotted or
    // updating the stencil.
    gl->Enable(DiGLPainter::gl_STENCIL_TEST);

    if (poptions.use_stencil) {
      // Test the stencil buffer values against a value of 0.
      gl->StencilFunc(DiGLPainter::gl_EQUAL, 0, 0x01);
      // Do not update the stencil buffer when plotting.
      gl->StencilOp(DiGLPainter::gl_KEEP, DiGLPainter::gl_KEEP, DiGLPainter::gl_KEEP);
    }
  }

  bool ok = false;

  if (plottype() == fpt_contour1)
    ok = plotContour(gl);
  else if (plottype() == fpt_contour || plottype() == fpt_contour2)
    ok = plotContour2(gl, zorder);
  else if (plottype() == fpt_wind)
    ok = plotWind(gl);
  else if (plottype() == fpt_wind_temp_fl)
    ok = plotWindAndValue(gl, true);
  else if (plottype() == fpt_wind_value)
    ok = plotWindAndValue(gl, false);
  else if (plottype() == fpt_value)
    ok = plotValue(gl);
  else if (plottype() == fpt_symbol)
    ok = plotValue(gl);
  else if (plottype() == fpt_vector)
    ok = plotVector(gl);
  else if (plottype() == fpt_direction)
    ok = plotDirection(gl);
  else if (praster)
    ok = plotRaster(gl);
  else if (plottype() == fpt_frame)
    ok = plotFrameOnly(gl);

  if (poptions.use_stencil || poptions.update_stencil)
    gl->Disable(DiGLPainter::gl_STENCIL_TEST);

  return ok;
}

std::vector<float*> FieldPlot::doPrepareVectors(float* x, float* y, bool direction)
{
  METLIBS_LOG_SCOPE();

  const bool rotateVectors = poptions.rotateVectors;

  vector<float*> uv;
  float *u = 0, *v = 0;

  int nf = tmpfields.size();

  const Projection& mapP = getStaticPlot()->getMapArea().P();
  if (!direction && (!rotateVectors || fields[0]->area.P() == mapP)) {
    u = fields[0]->data;
    v = fields[1]->data;
    diutil::delete_all_and_clear(tmpfields);
  } else if (nf == 2 && tmpfields[0]->numSmoothed == fields[0]->numSmoothed && tmpfields[0]->area.P() == mapP) {
    u = tmpfields[0]->data;
    v = tmpfields[1]->data;
  } else {
    if (nf == 0) {
      tmpfields.push_back(new Field());
      tmpfields.push_back(new Field());
    }
    *(tmpfields[0]) = *(fields[0]);
    *(tmpfields[1]) = *(fields[direction ? 0 : 1]);
    u = tmpfields[0]->data;
    v = tmpfields[1]->data;
    int npos = fields[0]->area.gridSize();
    bool ok = false;
    if (direction) {
      for (int i = 0; i < npos; i++)
        v[i] = 1.0f;
      bool turn = fields[0]->turnWaveDirection;
      ok = getStaticPlot()->gc.getDirectionVectors(getStaticPlot()->getMapArea(), turn, npos, x, y, u, v);
    } else {
      ok = getStaticPlot()->gc.getVectors(tmpfields[0]->area, getStaticPlot()->getMapProjection(), npos, x, y, u, v);
    }
    if (!ok)
      return uv;
    tmpfields[0]->area.setP(mapP);
    tmpfields[1]->area.setP(mapP);
  }
  uv.push_back(u);
  uv.push_back(v);
  return uv;
}

std::vector<float*> FieldPlot::prepareVectors(float* x, float* y)
{
  return doPrepareVectors(x, y, false);
}

std::vector<float*> FieldPlot::prepareDirectionVectors(float* x, float* y)
{
  return doPrepareVectors(x, y, true);
}

void FieldPlot::setAutoStep(float* x, float* y, int& ixx1, int ix2, int& iyy1,
    int iy2, int maxElementsX, int& step, float& dist)
{
  int i, ix, iy;
  const int nx = fields[0]->area.nx;
  const int ny = fields[0]->area.ny;
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
      adx += miutil::absval(dx, dy);
      dx = x[i + nx] - x[i];
      dy = y[i + nx] - y[i];
      ady += miutil::absval(dx, dy);
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
    if (step < 1) {
      step = 1;
    } else {

      if (step > poptions.densityFactor && poptions.densityFactor > 0) {
        step /= (poptions.densityFactor);
      }
    }
  }

  dist *= float(step);

  //adjust ix1,iy1 to make sure that same grid points are used when panning
  ixx1 = int(ixx1 / step) * step;
  iyy1 = int(iyy1 / step) * step;

}

int FieldPlot::xAutoStep(float* x, float* y, int& ixx1, int ix2, int iy,
    float sdist)
{
  const int nx = fields[0]->area.nx;
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
    adx += miutil::absval(dx, dy);
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

bool FieldPlot::getGridPoints(float* &x, float* &y, int& ix1, int& ix2, int& iy1, int& iy2, int factor, bool boxes) const
{
  const GridArea f0area = fields[0]->area.scaled(factor);
  if (not getStaticPlot()->gc.getGridPoints(f0area, getStaticPlot()->getMapArea(), getStaticPlot()->getMapSize(),
          boxes, &x, &y, ix1, ix2, iy1, iy2))
    return false;
  if (ix1 > ix2 || iy1 > iy2)
    return false;
  return true;
}

bool FieldPlot::getPoints(int n, float* x, float* y) const
{
  return getStaticPlot()->ProjToMap(fields[0]->area.P(), n, x, y);
}

// plot vector field as wind arrows
// Fields u(0) v(1), optional- colorfield(2)
bool FieldPlot::plotWind(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(2))
    return false;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  // convert windvectors to correct projection
  const vector<float*> uv = prepareVectors(x, y);
  if (uv.size() != 2)
    return false;
  const float *u = uv[0];
  const float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, step, sdist);

  const int nx = fields[0]->area.nx;
  const int ny = fields[0]->area.ny;

  if (poptions.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  diutil::ColourLimits colours;
  if (fields.size() == 3 && fields[2] && fields[2]->data) {
    float mini, maxi;
    if (!diutil::mini_maxi(fields[2]->data, nx * ny, fieldUndef, mini, maxi))
      return false;
    colours.initialize(poptions, mini, maxi);
  }

  const float unitlength = poptions.vectorunit / 10;
  const float flagl = sdist * 0.85 / unitlength;

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

  const Rectangle msex = diutil::extendedRectangle(getStaticPlot()->getMapSize(), flagl);

  const Projection& projection = getStaticPlot()->getMapArea().P();

  gl->setLineStyle(poptions.linecolour, poptions.linewidth, false);

  for (int iy = iy1; iy < iy2; iy += step) {
    const int xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (int ix = ix1; ix < ix2; ix += xstep) {
      const int i = iy * nx + ix;

      if (u[i] == fieldUndef || v[i] == fieldUndef || !msex.isinside(x[i], y[i]))
        continue;

      if (colours) {
        const float c = fields[2]->data[i];
        if (c == fieldUndef)
          continue;
        else
          colours.setColour(gl, c);
      }

      // If southern hemisphere, turn the feathers
      float xx = x[i], yy = y[i];
      projection.convertToGeographic(1, &xx, &yy);
      const int turnBarbs = (yy < 0) ? -1 : 1;
      gl->drawWindArrow(miutil::ms2knots(u[i]), miutil::ms2knots(v[i]), x[i], y[i], flagl, poptions.arrowstyle == arrow_wind_arrow, turnBarbs);
    }
  }
  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

/*
 ROUTINE:   FieldPlot::plotValue
 PURPOSE:   plot field value as number or symbol
 ALGORITHM:
 */

bool FieldPlot::plotValue(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (fields.size() > 1)
    return plotValues(gl);

  if (not checkFields(1))
    return false;

  // plot symbol
  std::map<float, std::string> classImages;
  if (plottype() == fpt_symbol) {
    std::vector<float> classValues;
    std::vector<std::string> classNames;
    unsigned int maxlen = 0;
    diutil::parseClasses(poptions, classValues, classNames, classImages, maxlen);
  }

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  const float* field = fields[0]->data;
  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, step, sdist);

  const int nx = fields[0]->area.nx;
  const int ny = fields[0]->area.ny;
  if (poptions.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  bool smallvalues = false;
  if (poptions.precision == 0) {
    int npos = nx * ny;
    int ii = 0;
    while (ii < npos && (fabsf(field[ii]) == fieldUndef || fabsf(field[ii]) < 1)) {
      ++ii;
    }
    smallvalues = (ii == npos);
  }

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

  gl->setColour(poptions.linecolour, false);

  const Rectangle msex = diutil::extendedRectangle(getStaticPlot()->getMapSize(), flagl);

  gl->setFont(diutil::BITMAPFONT, poptions.fontface, 10. * poptions.labelSize);

  float chx, chy;
  gl->getCharSize('0', chx, chy);
  chy *= 0.75;

  ImageGallery ig;
  for (int iy = iy1; iy < iy2; iy += step) {
    const int xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (int ix = ix1; ix < ix2; ix += xstep) {
      const int i = iy * nx + ix;
      const float gx = x[i], gy = y[i], value = field[i];
      if (value != fieldUndef && msex.isinside(gx, gy) && value >= poptions.minvalue && value <= poptions.maxvalue) {
        if (poptions.colours.size() > 2) {
          if (value < poptions.base) {
            gl->setColour(poptions.colours[0], false);
          } else if (value > poptions.base) {
            gl->setColour(poptions.colours[2], false);
          } else {
            gl->setColour(poptions.colours[1], false);
          }
        }

        if (!classImages.empty()) { //plot symbol
          std::map<float, std::string>::const_iterator it = classImages.find(value);
          if (it != classImages.end()) {
            ig.plotImage(gl, getStaticPlot()->plotArea(), it->second, gx, gy, true, poptions.labelSize * 0.25);
          }
        } else { // plot value
          QString ost;
          if (smallvalues)
            ost = QString::number(value, 'g', 1);
          else
            ost = QString::number(value, 'f', poptions.precision);
          gl->drawText(ost, gx - chx / 2, gy - chy / 2, 0.0);
        }
      }
    }
  }

  if (poptions.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

/*
 ROUTINE:   FieldPlot::plotWindAndValue
 PURPOSE:   plot vector field as wind arrows and third field as number.
 ALGORITHM: Fields u(0) v(1) number(2)
 */

bool FieldPlot::plotWindAndValue(DiGLPainter* gl, bool flightlevelChart)
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
  vector<float*> uv = prepareVectors(x, y);
  if (uv.size() != 2)
    return false;
  float *u = uv[0];
  float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxWindsAuto, step, sdist);
  int xstep;

  int i, ix, iy;
  const int nx = fields[0]->area.nx;
  const int ny = fields[0]->area.ny;

  if (poptions.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  float unitlength = poptions.vectorunit / 10;
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

  const Rectangle& ms = getStaticPlot()->getMapSize();
  const Rectangle msex = diutil::extendedRectangle(ms, flagl);
  float fontsize = 7. * poptions.labelSize;

  gl->setFont(diutil::BITMAPFONT, poptions.fontface, fontsize);

  float chx, chy;
  gl->getTextSize("ps00", chx, chy);
  gl->getCharSize('0', chx, chy);

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
  if (nbmap<0 || nbmap > 1000000)
    return false;
  int *bmap = new int[nbmap];
  for (i = 0; i < nbmap; i++)
    bmap[i] = 0;
  int m, ib, jb, ibit, iwrd, nb;
  float xb[20][2], yb[20][2];

  vector<int> vxstep;

  gl->setLineStyle(poptions.linecolour, poptions.linewidth, false);

  // plot wind............................................

  gl->Begin(DiGLPainter::gl_LINES);

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
      if (u[i] != fieldUndef && v[i] != fieldUndef && msex.isinside(gx, gy) && t[i] >= poptions.minvalue && t[i] <= poptions.maxvalue) {
        ff = miutil::absval(u[i], v[i]);
        if (ff > 0.00001) {

          gu = u[i] / ff;
          gv = v[i] / ff;

          ff = miutil::ms2knots(ff);

          const vcross::util::WindArrowFeathers waf = vcross::util::countFeathers(ff);

          dx = flagstep * gu;
          dy = flagstep * gv;
          dxf = -flagw * gv - dx;
          dyf = flagw * gu - dy;

          nb = 0;
          xb[nb][0] = gx;
          yb[nb][0] = gy;

          // direction
          gl->Vertex2f(gx, gy);
          gx = gx - flagl * gu;
          gy = gy - flagl * gv;
          gl->Vertex2f(gx, gy);

          xb[nb][1] = gx;
          yb[nb++][1] = gy;

          // 50-knot flags, store for plot below
          if (waf.n50 > 0) {
            for (int n = 0; n < waf.n50; n++) {
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
          for (int n = 0; n < waf.n10; n++) {
            gl->Vertex2f(gx, gy);
            gl->Vertex2f(gx + dxf, gy + dyf);
            xb[nb][0] = gx;
            yb[nb][0] = gy;
            xb[nb][1] = gx + dxf;
            yb[nb++][1] = gy + dyf;
            gx += dx;
            gy += dy;
          }
          // 5-knot flag
          if (waf.n05 > 0) {
            if (waf.n50 + waf.n10 == 0) {
              gx += dx;
              gy += dy;
            }
            gl->Vertex2f(gx, gy);
            gl->Vertex2f(gx + hflagw * dxf, gy + hflagw * dyf);
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
  gl->End();

  // draw 50-knot flags
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
  int vi = vx.size();
  if (vi >= 3) {
    gl->Begin(DiGLPainter::gl_TRIANGLES);
    for (i = 0; i < vi; i++)
      gl->Vertex2f(vx[i], vy[i]);
    gl->End();
  }

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
          && t[i] >= poptions.minvalue && t[i] <= poptions.maxvalue) {

        if (u[i] != fieldUndef && v[i] != fieldUndef)
          ff = miutil::absval(u[i], v[i]);
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
            gl->setColour(poptions.colours[0]);
          } else if (value > poptions.base) {
            gl->setColour(poptions.colours[2]);
          } else {
            gl->setColour(poptions.colours[1]);
          }
        }

        QString str;
        if (flightlevelChart) {
          if (value <= 0) {
            str = QString::number(-value);
          } else {
            str = "ps" + QString::number(value);
          }
        } else {
          str = QString::number(value);
        }

        gl->getTextSize(str, w, h);

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
          gl->drawText(str, x1, y1, 0.0);
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

  delete[] bmap;

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

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
bool FieldPlot::plotValues(DiGLPainter* gl)
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
  int xstep;

  const int nx = fields[0]->area.nx;
  const int ny = fields[0]->area.ny;

  if (poptions.frame) {
    plotFrame(gl, nx, ny, x, y);
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

  gl->setFont(diutil::BITMAPFONT, poptions.fontface, fontsize);

  float chx, chy;
  gl->getCharSize('0', chx, chy);
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

      if (fields[0]->data[i] >= poptions.minvalue
          && fields[0]->data[i] <= poptions.maxvalue) {

        for (size_t j = 0; j < nfields; j++) {
          float * fieldData = fields[j]->data;
          if (fieldData[i] != fieldUndef && getStaticPlot()->getMapSize().isinside(gx, gy)) {
            int value =
                (fieldData[i] >= 0.0f) ? int(fieldData[i] + 0.5f) :
                    int(fieldData[i] - 0.5f);
            const QString str = QString::number(value);
            gl->getTextSize(str, w, h);

            int ipos1 = position[j];
            x1 = gx + adx[ipos1] + cdx[ipos1] * w;
            y1 = gy + ady[ipos1];
            x2 = x1 + w;
            y2 = y1 + chy;
            if (getStaticPlot()->getMapSize().isinside(x1, y1) && getStaticPlot()->getMapSize().isinside(x2, y2)) {
              gl->setColour(col[j], false);
              gl->drawText(str, x1, y1, 0.0);
            }
          }
        }

        // ---
        if (nfields == 4 || nfields == 5) {
          const QString str("----");
          gl->getTextSize(str, w, h);

          x1 = gx + adx[8] + cdx[8] * w;
          y1 = gy + ady[8];
          x2 = x1 + w;
          y2 = y1 + chy;
          if (getStaticPlot()->getMapSize().isinside(x1, y1) && getStaticPlot()->getMapSize().isinside(x2, y2)) {
            gl->drawText(str, x1, y1, 0.0);
          }
        }
      }
    }
  }

  if (poptions.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  return true;
}

//  plot vector field as arrows (wind,sea current,...)
bool FieldPlot::plotVector(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE(LOGVAL(fields.size()));

  if (not checkFields(2))
    return false;

  const float* colourfield = (fields.size() == 3 && fields[2] && fields[2]->data) ? fields[2]->data : 0;

  float arrowlength = 0;
  const bool ok = plotArrows(gl, &FieldPlot::prepareVectors,
      colourfield, arrowlength);

  // for annotations .... should probably be resized if very small or large...
  vectorAnnotationSize = arrowlength;
  vectorAnnotationText = miutil::from_number(poptions.vectorunit)
      + poptions.vectorunitname;

  return ok;
}

/* plot true north direction field as arrows (wave,...) */
bool FieldPlot::plotDirection(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;
  const float* colourfield = (fields.size() == 2 && fields[1] && fields[1]->data) ? fields[1]->data : 0;

  float arrowlength_dummy = 0;
  return plotArrows(gl, &FieldPlot::prepareDirectionVectors,
      colourfield, arrowlength_dummy);
}

//  plot vector field as arrows (wind,sea current,...)
bool FieldPlot::plotArrows(DiGLPainter* gl, prepare_vectors_t pre_vec,
    const float* colourfield, float& arrowlength)
{
  METLIBS_LOG_SCOPE(LOGVAL(fields.size()));

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  // convert vectors to correct projection
  vector<float*> uv = (this->*pre_vec)(x, y);
  if (uv.size() != 2)
    return false;
  float *u = uv[0];
  float *v = uv[1];

  int step = poptions.density;
  float sdist;
  setAutoStep(x, y, ix1, ix2, iy1, iy2, MaxArrowsAuto, step, sdist);
  int xstep;

  const int nx = fields[0]->area.nx;
  const int ny = fields[0]->area.ny;

  if (poptions.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  diutil::ColourLimits colours;
  if (colourfield) {
    float mini, maxi;
    if (!diutil::mini_maxi(colourfield, nx * ny, fieldUndef, mini, maxi))
      return false;
    colours.initialize(poptions, mini, maxi);
  }

  arrowlength = sdist;
  const float unitlength = poptions.vectorunit;
  const float scale = arrowlength / unitlength;

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

  const Rectangle msex = diutil::extendedRectangle(getStaticPlot()->getMapSize(), arrowlength * 1.5);

  gl->setLineStyle(poptions.linecolour, poptions.linewidth, false);

  for (int iy = iy1; iy < iy2; iy += step) {
    xstep = xAutoStep(x, y, ix1, ix2, iy, sdist);
    for (int ix = ix1; ix < ix2; ix += xstep) {
      const int i = iy * nx + ix;

      const float gx = x[i], gy = y[i];
      if (u[i] == fieldUndef || v[i] == fieldUndef || (u[i] == 0 && v[i] == 0) || !msex.isinside(gx, gy))
        continue;

      if (colourfield && colours) {
        const float c = colourfield[i];
        if (c == fieldUndef)
          continue;
        colours.setColour(gl, c);
      }

      gl->drawArrow(gx, gy, gx + scale * u[i], gy + scale * v[i], 0);
    }
  }

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

//  plot scalar field as contour lines
bool FieldPlot::plotContour(DiGLPainter* gl)
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

  const int rnx = fields[0]->area.nx / factor, rny = fields[0]->area.ny / factor;

  // Create a resampled data array to pass to the contour function.
  float *data;
  if (factor != 1) {
    METLIBS_LOG_INFO("Resampled field from" << LOGVAL(fields[0]->area.nx) << LOGVAL(fields[0]->area.ny)
        << " to" << LOGVAL(rnx) << LOGVAL(rny));
    data = new float[rnx * rny];
    int i = 0;
    for (int iy = 0; iy < rny; ++iy) {
      int j = 0;
      for (int ix = 0; ix < rnx; ++ix) {
        data[(i * rnx) + j] = fields[0]->data[(iy * fields[0]->area.nx * factor) + (ix * factor)];
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

  if (poptions.frame)
    plotFrame(gl, rnx, rny, x, y);

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

    gl->setFont(poptions.fontname, poptions.fontface, fontsize);
    gl->getCharSize('0', chxlab, chylab);

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
      gl->Disable(DiGLPainter::gl_MULTISAMPLE);

    METLIBS_LOG_TIME("contour");
    res = contour(rnx, rny, data, x, y, ipart, 2, NULL, xylim, idraw, zrange,
        zstep, zoff, nlines, rlines, ncol, icol, ntyp, ityp, nwid, iwid, nlim,
        rlim, idraw2, zrange2, zstep2, zoff2, nlines2, rlines2, ncol2, icol2,
        ntyp2, ityp2, nwid2, iwid2, nlim2, rlim2, ismooth, labfmt, chxlab,
        chylab, ibcol, ibmap, lbmap, kbmap, nxbmap, nybmap, rbmap, gl, poptions,
        fields[0]->area, fieldUndef, getModelName(), fields[0]->name,
        ftime.hour());

    if (poptions.antialiasing)
      gl->Enable(DiGLPainter::gl_MULTISAMPLE);
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
      gl->setColour(poptions.linecolour, false);
    } else {
      gl->setColour(poptions.linecolour_2, false);
    }

    if (poptions.minvalue_2 > -fieldUndef || poptions.maxvalue_2 < fieldUndef) {
      zrange2[0] = poptions.minvalue_2;
      zrange2[1] = poptions.maxvalue_2;
    }

    if (poptions.linewidths.size() == 1) {
      gl->LineWidth(poptions.linewidth);
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
      gl->LineStipple(poptions.linetype.factor, poptions.linetype.bmap);
      gl->Enable(DiGLPainter::gl_LINE_STIPPLE);
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
        chylab, ibcol, ibmap, lbmap, kbmap, nxbmap, nybmap, rbmap, gl, poptions,
        fields[0]->area, fieldUndef, getModelName(), fields[0]->name,
        ftime.hour());

    //restore contour shading
    poptions.contourShading = contourShading;
  }

  if (poptions.extremeType != "None" && poptions.extremeType != "Ingen"
      && !poptions.extremeType.empty()) {
    markExtreme(gl);
  }


  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (!res)
    METLIBS_LOG_ERROR("contour error");

  if (poptions.update_stencil)
    plotFrameStencil(gl, rnx, rny, x, y);

  if (factor != 1)
    delete[] data;

  return true;
}

//  plot scalar field as contour lines using new contour algorithm
bool FieldPlot::plotContour2(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_SCOPE();

  int paintMode;
  if (zorder == PO_SHADE_BACKGROUND) {
    if (poptions.undefMasking <= 0)
      return true;
    paintMode = DianaLines::UNDEFINED;
  } else if (zorder == PO_SHADE) {
    if (!getShadePlot())
      return true;
    paintMode = DianaLines::FILL | DianaLines::LINES_LABELS;
  } else if (zorder == PO_LINES) {
    if (getShadePlot())
      return true;
    paintMode = DianaLines::LINES_LABELS;
  } else if (zorder == PO_OVERLAY) {
    paintMode = DianaLines::UNDEFINED | DianaLines::FILL | DianaLines::LINES_LABELS;
  } else {
    return true;
  }

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
  const int nx = fields[0]->area.nx;
  const int ny = fields[0]->area.ny;
  if (ix1 >= nx || ix2 < 0 || iy1 >= ny || iy2 < 0)
    return false;

  if (poptions.frame)
    plotFrame(gl, nx, ny, x, y);

  if (poptions.valueLabel)
    gl->setFont(poptions.fontname, poptions.fontface, 10 * poptions.labelSize);

  {
    METLIBS_LOG_TIME("contour2");
    if (not poly_contour(nx, ny, ix1, iy1, ix2, iy2, fields[0]->data, x, y, gl,
            poptions, fieldUndef, paintMode))
      METLIBS_LOG_ERROR("contour2 error");
  }
  if (poptions.options_2) {
    METLIBS_LOG_TIME("contour2 options_2");
    if (not poly_contour(nx, ny, ix1, iy1, ix2, iy2, fields[0]->data, x, y, gl,
            poptions, fieldUndef, paintMode, true))
      METLIBS_LOG_ERROR("contour2 options_2 error");
  }
  if (poptions.extremeType != "None" && poptions.extremeType != "Ingen"
      && !poptions.extremeType.empty())
    markExtreme(gl);

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (poptions.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

bool FieldPlot::centerOnGridpoint() const
{
  return plottype() == fpt_alpha_shade
      || plottype() == fpt_rgb
      || plottype() == fpt_alarm_box
      || plottype() == fpt_fill_cell;
}

bool FieldPlot::plotRaster(DiGLPainter* gl)
{
  METLIBS_LOG_TIME();

  if (not checkFields(1))
    return false;

  METLIBS_LOG_DEBUG(LOGVAL(getModelName()));

  float *x = 0, *y = 0;
  int nx = fields[0]->area.nx, ny = fields[0]->area.ny;
  if (poptions.frame || poptions.update_stencil) {
    const bool boxes = centerOnGridpoint();
    int ix1, ix2, iy1, iy2;
    if (!getGridPoints(x, y, ix1, ix2, iy1, iy2, 1, boxes) || !x || !y)
      x = y = 0;
    if (boxes) {
      // extend frame
      nx += 1;
      ny += 1;
    }
  }
  if (x && y && poptions.frame) {
    gl->setLineStyle(poptions.bordercolour, poptions.linewidth, false);
    plotFrame(gl, nx, ny, x, y);
  }

  QImage target;
  if (plottype() == fpt_rgb) {
    if (checkFields(3)) {
      RasterRGB raster(getStaticPlot()->plotArea(), fields, poptions);
      target = raster.rasterPaint();
    }
  } else if (plottype() == fpt_alpha_shade) {
    RasterAlpha raster(getStaticPlot()->plotArea(), fields[0], poptions);
    target = raster.rasterPaint();
  } else if (plottype() == fpt_fill_cell) {
    RasterFillCell raster(getStaticPlot()->plotArea(), fields[0], poptions);
    target = raster.rasterPaint();
  } else if (plottype() == fpt_alarm_box) {
    RasterAlarmBox raster(getStaticPlot()->plotArea(), fields[0], poptions);
    target = raster.rasterPaint();
  }
  if (!target.isNull())
    gl->drawScreenImage(QPointF(0,0), target);

  if (x && y && poptions.update_stencil) {
    plotFrameStencil(gl, nx, ny, x, y);
  }

  return true;
}

bool FieldPlot::plotFrameOnly(DiGLPainter* gl)
{
  if (not checkFields(1))
    return false;

  int nx = fields[0]->area.nx;
  int ny = fields[0]->area.ny;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  if (poptions.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  if (poptions.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

QPolygonF FieldPlot::makeFramePolygon(int nx, int ny, const float *x, const float *y) const
{
  QPolygonF frame;
  for (int ix = 0; ix < nx; ix++) {
    const int iy = 0;
    const int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      frame << QPointF(x[i], y[i]);
    }
  }
  for (int iy = 1; iy < ny; iy++) {
    const int ix = nx - 1;
    const int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      frame << QPointF(x[i], y[i]);
    }
  }
  for (int ix = nx - 2; ix >= 0; ix--) {
    const int iy = ny - 1;
    const int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      frame << QPointF(x[i], y[i]);
    }
  }
  for (int iy = ny - 2; iy > 0; iy--) {
    const int ix = 0;
    const int i = iy * nx + ix;
    if (x[i] != HUGE_VAL && y[i] != HUGE_VAL) {
      frame << QPointF(x[i], y[i]);
    }
  }
  return frame;
}

// plot frame for complete field area
void FieldPlot::plotFrame(DiGLPainter* gl, int nx, int ny, const float *x, const float *y)
{
  METLIBS_LOG_SCOPE(LOGVAL(nx) << LOGVAL(ny));

  if (fields.empty() || !fields[0])
    return;

  const QPolygonF frame = makeFramePolygon(nx, ny, x, y);

  // If the frame value is 2 or 3 then fill the frame with the background colour.
  if ((poptions.frame & 2) != 0) {
    gl->setColour(getStaticPlot()->getBackgroundColour(), true);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->drawPolygon(frame);
  }
  if ((poptions.frame & 1) != 0) {
    gl->setLineStyle(poptions.bordercolour, poptions.linewidth, true);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
    gl->drawPolyline(frame);
  }
}

void FieldPlot::plotFrameStencil(DiGLPainter* gl, int nx, int ny, const float *x, const float *y)
{
  // Fill the frame in the stencil buffer with values.
  gl->ColorMask(DiGLPainter::gl_FALSE, DiGLPainter::gl_FALSE, DiGLPainter::gl_FALSE, DiGLPainter::gl_FALSE);
  gl->DepthMask(DiGLPainter::gl_FALSE);

  // Fill the frame in the stencil buffer with non-zero values.
  gl->StencilFunc(DiGLPainter::gl_ALWAYS, 1, 0x01);
  gl->StencilOp(DiGLPainter::gl_KEEP, DiGLPainter::gl_KEEP, DiGLPainter::gl_REPLACE);
  gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

  const QPolygonF frame = makeFramePolygon(nx, ny, x, y);
#if 0
  gl->drawPolygon(DiGLPainter::gl_POLYGON); // does not update stencil in DiPaintGLPainter
#else
  gl->Begin(DiGLPainter::gl_POLYGON);
  for (int i=0; i<frame.size(); ++i) {
    const QPointF& p = frame.at(i);
    gl->Vertex2f(p.x(), p.y());
  }
  gl->End();
#endif

  gl->ColorMask(DiGLPainter::gl_TRUE, DiGLPainter::gl_TRUE, DiGLPainter::gl_TRUE, DiGLPainter::gl_TRUE);
  gl->DepthMask(DiGLPainter::gl_TRUE);
}

/*
 Mark extremepoints in a field with L/H (Low/High), C/W (Cold/Warm) or value
 */
bool FieldPlot::markExtreme(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  int nx = fields[0]->area.nx;
  int ny = fields[0]->area.ny;


  // convert gridpoints to correct projection
  float *x, *y;
  int ix1, ix2, iy1, iy2;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  if (ix1 < 1)
    ix1 = 1;
  if (ix2 > nx - 2)
    ix2 = nx - 2;
  if (iy1 < 1)
    iy1 = 1;
  if (iy2 > ny - 2)
    iy2 = ny - 2;

  //find avg. dist. between grid points
  int ixstp = std::max((ix2 - ix1) / 5, 1);
  int iystp = std::max((iy2 - iy1) / 5, 1);
  float avgdist = 0;
  int navg = 0;
  for (int iy = iy1; iy < iy2; iy += iystp) {
    for (int ix = ix1; ix < ix2; ix += ixstp) {
      int i = iy * nx + ix;
      float dx = x[i + 1] - x[i];
      float dy = y[i + 1] - y[i];
      avgdist += miutil::absval(dx, dy);
      dx = x[i + nx] - x[i];
      dy = y[i + nx] - y[i];
      avgdist += miutil::absval(dx, dy);
      navg += 2;
    }
  }
  if (navg > 0)
    avgdist = avgdist / float(navg);
  else
    avgdist = 1.;

  ix2++;
  iy2++;

  QString pmarks[2];
  bool extremeString = false;
  enum { NONE, MIN, MAX, BOTH } extremeValue = NONE;
  const vector<std::string> vextremetype = miutil::split(miutil::to_upper(poptions.extremeType), "+");
  if (vextremetype.size() > 0) {
    if (!miutil::contains(vextremetype[0], "VALUE")) {
      pmarks[0] = QString::fromStdString(vextremetype[0]);
      extremeString = true;
    }
    if (vextremetype.size() > 1 && !miutil::contains(vextremetype[1], "VALUE")) {
      pmarks[1] = QString::fromStdString(vextremetype[1]);
      extremeString = true;
    }
    if (vextremetype.back() == "MINVALUE") {
      extremeValue = MIN;
    } else if (vextremetype.back() == "MAXVALUE") {
      extremeValue = MAX;
      pmarks[1] = pmarks[0];
    } else if (vextremetype.back() == "VALUE") {
      extremeValue = BOTH;
    }
  }

  gl->setColour(poptions.linecolour, false);

  const float fontsizeHuge = 28. * poptions.extremeSize;
  const float fontsizeBig = 18. * poptions.extremeSize;
  gl->setFont(poptions.fontname, poptions.fontface, fontsizeHuge);

  float chrx, chry;
  gl->getCharSize('L', chrx, chry);
  chry *= 0.75; // approx. real height for the character (width is ok)
  const float size = std::max(chry, chrx);

  const float radius = 3.0 * size * poptions.extremeRadius;
  const int nscan = std::max(int(radius / avgdist), 2);
  const float rg = float(nscan), rg2 = miutil::square(rg);

  // for scan of surrounding positions
  const int mscan = nscan * 2 + 1;
  std::unique_ptr<int[]> iscan(new int[mscan]);
  iscan[nscan] = nscan;
  for (int j = 1; j <= nscan; j++) {
    float dx = sqrtf(rg2 - miutil::square(j));
    int i = int(dx + 0.5);
    iscan[nscan - j] = i;
    iscan[nscan + j] = i;
  }

  // for test of gradients in the field
  int igrad[4][6];
  const int nscan45degrees = int(sqrtf(rg2 * 0.5) + 0.5);

  for (int k = 0; k < 4; k++) {
    int i = 0, j = 0;
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
  const int near[8] = {-nx - 1, -nx, -nx + 1, -1, 1, nx - 1, nx, nx + 1};

  const Rectangle msex = diutil::extendedRectangle(getStaticPlot()->getMapSize(), -size * 0.5);

  for (int iy = iy1; iy < iy2; iy++) {
    for (int ix = ix1; ix < ix2; ix++) {
      const int p = iy * nx + ix;
      const float fpos = fields[0]->data[p];
      if (fpos == fieldUndef)
        continue;
      if (!(fpos < poptions.maxvalue && fpos > poptions.minvalue))
        continue;

      const float gx = x[p];
      const float gy = y[p];
      if (!msex.isinside(gx, gy))
        continue;

      {
        // first check the nearest gridpoints
        float fmin = fieldUndef;
        float fmax = -fieldUndef;
        for (int j = 0; j < 8; j++) {
          const float f = fields[0]->data[p + near[j]];
          if (fmin > f)
            fmin = f;
          if (fmax < f)
            fmax = f;
        }

        if (fmax != fieldUndef && ((fmin >= fpos && extremeValue != MAX) || (fmax <= fpos && extremeValue != MIN)) && fmin != fmax) {
          // scan a larger area

          bool ok = true;
          const bool is_minimum = (fmin >= fpos); // minimum at pos ix,iy
          vector<int> pequal;

          const int jbeg = std::max(iy - nscan, 0);
          const int jend = std::min(iy + nscan + 1, ny);
          for (int j = jbeg; j < jend && ok; ++j) {
            const int n = iscan[nscan + iy - j];
            const int ibeg = std::max(ix - n, 0);
            const int iend = std::min(ix + n + 1, nx);
            for (int i = ibeg; i < iend; i++) {
              const int ppp = j * nx + i;
              const float f = fields[0]->data[ppp];
              if (f != fieldUndef) {
                if ((is_minimum && f < fpos) || (!is_minimum && f > fpos))
                  ok = false;
                else if (f == fpos)
                  pequal.push_back(ppp);
              }
            }
          }

          if (ok) {
            int ibest, jbest;
            const int n = pequal.size();
            if (n < 2) {
              ibest = ix;
              jbest = iy;
            } else {
              // more than one pos with the same extreme value,
              // select the one with smallest surrounding
              // gradients in the field (seems to work...)
              float gbest = fieldUndef;
              ibest = jbest = -1;
              for (int l = 0; l < n; l++) {
                const int pp = pequal[l];
                int j = pp / nx;
                int i = pp - j * nx;
                float fgrad = 0;
                int ngrad = 0;
                for (int k = 0; k < 4; k++) {
                  int i1 = i + igrad[k][0];
                  int j1 = j + igrad[k][1];
                  int i2 = i + igrad[k][2];
                  int j2 = j + igrad[k][3];
                  if (i1 >= 0 && j1 >= 0 && i1 < nx && j1 < ny && i2 >= 0 && j2 >= 0 && i2 < nx && j2 < ny) {
                    const float f1 = fields[0]->data[pp + igrad[k][4]];
                    const float f2 = fields[0]->data[pp + igrad[k][5]];
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
                // mark extreme point
                if (!extremeString) {
                  const QString str = QString::number(fpos, 'f', poptions.precision);
                  gl->drawText(str, gx - chrx * 0.5, gy - chry * 0.5, 0.0);
                } else {
                  gl->drawText(pmarks[is_minimum ? 0 : 1], gx - chrx * 0.5, gy - chry * 0.5, 0.0);
                  if (extremeValue != NONE) {
                    gl->setFontSize(fontsizeBig);
                    const QString str = QString::number(fpos, 'f', poptions.precision);
                    gl->drawText(str, gx - chrx * (-0.6), gy - chry * 0.8, 0.0);
                    gl->setFontSize(fontsizeHuge);
                  }
              }
            }
          }
        }
      }
    }
  }

  return true;
}

// draw the grid lines in some density
bool FieldPlot::plotGridLines(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2))
    return false;

  ix2++;
  iy2++;

  const int step = poptions.gridLines;
  if (step <= 0 || (poptions.gridLinesMax > 0
          && ((ix2 - ix1) / step > poptions.gridLinesMax
              || (iy2 - iy1) / step > poptions.gridLinesMax)))
  {
    return false;
  }

  Colour bc = poptions.bordercolour;
  bc.setF(Colour::alpha, 0.5);
  gl->setLineStyle(bc, 1);

  ix1 = int(ix1 / step) * step;
  iy1 = int(iy1 / step) * step;

  const int nx = fields[0]->area.nx;

  diutil::PolylinePainter pp(gl);
  for (int ix = ix1; ix < ix2; ix += step) {
    for (int iy = iy1; iy < iy2; iy++)
      pp.add(x, y, diutil::index(nx, ix, iy));
    pp.draw();
  }

  for (int iy = iy1; iy < iy2; iy += step) {
    for (int ix = ix1; ix < ix2; ix++)
      pp.add(x, y, diutil::index(nx, ix, iy));
    pp.draw();
  }

  return true;
}

// show areas with undefined field values
bool FieldPlot::plotUndefined(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (poptions.undefMasking <= 0)
    return false;

  if (not checkFields(1))
    return false;
  if (plottype() == fpt_contour || plottype() == fpt_contour2) {
    plotContour2(gl, PO_SHADE_BACKGROUND);
    return true;
  }

  if (praster) {
    RasterUndef raster(getStaticPlot()->plotArea(), fields, poptions);
    QImage target = raster.rasterPaint();
    if (!target.isNull())
      gl->drawScreenImage(QPointF(0, 0), target);
    return true;
  }

  const bool center_on_gridpoint = centerOnGridpoint()
      || plottype() == fpt_contour1; // old contour does some tricks with undefined values

  const int nx_fld = fields[0]->area.nx, ny_fld = fields[0]->area.ny;
  const int nx_pts = nx_fld + (center_on_gridpoint ? 1 : 0);

  // convert gridpoints to correct projection
  int ix1, ix2, iy1, iy2;
  float *x, *y;
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2, 1, center_on_gridpoint))
    return false;
  if (ix1 >= nx_fld || ix2 < 0 || iy1 >= ny_fld || iy2 < 0)
    return false;

  const diutil::is_undef undef_f0(fields[0]->data, nx_fld);

  const Colour undefC = getStaticPlot()->notBackgroundColour(poptions.undefColour);

  if (poptions.undefMasking == 1) {
    // filled undefined areas
    gl->setColour(undefC, false);
    gl->ShadeModel(DiGLPainter::gl_FLAT);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

    for (int iy = iy1; iy <= iy2; iy++) {
      int ix = ix1;
      while (ix <= ix2) {
        // find start of undef
        while (ix <= ix2 && !undef_f0(ix, iy))
          ix += 1;
        if (ix > ix2)
          break;

        int ixx = ix;
        // find end of undef
        while (ix <= ix2 && undef_f0(ix, iy))
          ix += 1;

        // fill
        gl->Begin(DiGLPainter::gl_QUAD_STRIP);
        for (; ixx <= ix; ++ixx) {
          int idx0 = diutil::index(nx_pts, ixx, iy), idx1 = idx0 + nx_pts;
          gl->Vertex2f(x[idx1], y[idx1]);
          gl->Vertex2f(x[idx0], y[idx0]);
        }
        gl->End();
      }
    }
  } else {
    // grid lines around undefined cells
    gl->setLineStyle(undefC, poptions.undefLinewidth, poptions.undefLinetype, false);

    // line is where at least one of two neighbouring cells is undefined
    diutil::PolylinePainter pp(gl);

    // horizontal lines
    for (int iy = iy1; iy <= iy2; iy++) {
      int ix = ix1;
      while (ix <= ix2) {
        while (ix <= ix2 && (!undef_f0(ix, iy) && (iy == iy1 || !undef_f0(ix, iy-1))))
          ix += 1;
        if (ix >= ix2)
          break;

        int idx = diutil::index(nx_pts, ix, iy);
        pp.add(x, y, idx);
        do {
          ix += 1;
          idx += 1;
          pp.add(x, y, idx);
        } while (ix < ix2 && !(!undef_f0(ix, iy) && (iy == iy1 || !undef_f0(ix, iy-1))));
        pp.draw();
      }
    }

    // vertical lines
    for (int ix = ix1; ix <= ix2; ix++) {
      int iy = iy1;
      while (iy <= iy2) {
        while (iy <= iy2 && (!undef_f0(ix, iy) && (ix == ix1 || !undef_f0(ix-1, iy))))
          iy += 1;
        if (iy >= iy2)
          break;

        int idx = diutil::index(nx_pts, ix, iy);
        pp.add(x, y, idx);
        do {
          iy += 1;
          idx += nx_pts;
          pp.add(x, y, idx);
        } while (iy < iy2 && !(!undef_f0(ix, iy) && (ix == ix1 || !undef_f0(ix-1, iy))));
        pp.draw();
      }
    }
  }

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  return true;
}

/*
 plot field values as numbers in each gridpoint
 skip plotting if too many (or too small) numbers
 */
bool FieldPlot::plotNumbers(DiGLPainter* gl)
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
  gl->setFont(diutil::BITMAPFONT, diutil::F_BOLD, fontsize);

  float chx, chy;
  gl->getCharSize('0', chx, chy);
  // the real height for numbers 0-9 (width is ok)
  chy *= 0.75;

  float *field = fields[0]->data;
  float gx, gy, w, h;
  float hh = chy * 0.5;
  float ww = chx * 0.5;
  int iprec = 0;
  if (poptions.precision > 0) {
    iprec = poptions.precision;
  }
  QString str;

  gl->setLineStyle(poptions.linecolour, 1, false);

  for (iy = iy1; iy < iy2; iy++) {
    for (ix = ix1; ix < ix2; ix++) {
      i = iy * fields[0]->area.nx + ix;
      gx = x[i];
      gy = y[i];

      if (field[i] != fieldUndef) {
        str = QString::number(field[i], 'f', iprec);
        gl->getTextSize(str, w, h);
        w *= 0.5;
      } else {
        str = "X";
        w = ww;
      }

      if (getStaticPlot()->getMapSize().isinside(gx - w, gy - hh)
          && getStaticPlot()->getMapSize().isinside(gx + w, gy + hh))
        gl->drawText(str, gx - w, gy - hh, 0.0);
    }
  }

  // draw lines/boxes at borders between gridpoints..............................

  // convert gridpoints to correct projection
  if (not getGridPoints(x, y, ix1, ix2, iy1, iy2, 1, true))
    return false;

  const int nx = fields[0]->area.nx + 1;

  ix2++;
  iy2++;

  diutil::PolylinePainter pp(gl);
  for (int ix = ix1; ix < ix2; ix++) {
    for (int iy = iy1; iy < iy2; iy++)
      pp.add(x, y, diutil::index(nx, ix, iy));
    pp.draw();
  }
  for (int iy = iy1; iy < iy2; iy++) {
    for (int ix = ix1; ix < ix2; ix++)
      pp.add(x, y, diutil::index(nx, ix, iy));
    pp.draw();
  }

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
  if (plottype() == fpt_wind)
    nf = 2;
  if (plottype() == fpt_vector)
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
  if (count > 1) {
    const int gridsize0 = fields[0]->area.gridSize();
    for (size_t i = 1; i < count; ++i) { // start from 1
      if (fields[i]->area.gridSize() != gridsize0)
        return false;
    }
  }
  return true;
}
