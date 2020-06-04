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

#include "diFieldRenderer.h"

#include "diana_config.h"

#include "diContouring.h"
#include "diField/diField.h"
#include "diGLPainter.h"
#include "diGridConverter.h"
#include "diImageGallery.h"
#include "diPolyContouring.h"
#include "diRasterAlarmBox.h"
#include "diRasterAlpha.h"
#include "diRasterFillCell.h"
#include "diRasterGrid.h"
#include "diRasterRGB.h"
#include "diRasterUndef.h"
#include "diRasterUtil.h"
#include "diUtilities.h"
#include "util/plotoptions_util.h"

#include <mi_fieldcalc/math_util.h>
#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.FieldRenderer"
#include <miLogger/miLogging.h>

namespace {
const int MaxWindsAuto = 40;
const int MaxArrowsAuto = 55;
} // namespace

struct FieldRenderer::GridPoints
{
  int ix1, ix2, iy1, iy2;
  Points_cp points;
  const float* x() const { return points->x; }
  const float* y() const { return points->y; }
};

FieldRenderer::FieldRenderer()
    : is_raster_(false)
    , is_shade_(false)
{
}

FieldRenderer::~FieldRenderer() {}

bool FieldRenderer::hasVectorAnnotation()
{
  return plottype() == fpt_vector; // FIXME also needs data, successful getGridPoints, ...
}

void FieldRenderer::getVectorAnnotation(float& size, std::string& text)
{
  if (hasVectorAnnotation() && !fields_.empty()) {
    GridPoints gp;
    if (getGridPoints(gp)) {
      int step = poptions_.density;
      setAutoStep(gp.x(), gp.y(), gp.ix1, gp.ix2, gp.iy1, gp.iy2, MaxArrowsAuto, step, size);
      text = miutil::from_number(poptions_.vectorunit) + poptions_.vectorunitname;
      return;
    }
  }

  size = 0;
  text.clear();
}

void FieldRenderer::clearData()
{
  METLIBS_LOG_SCOPE();
  fields_.clear();
}

void FieldRenderer::setData(const Field_pv& fields, const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE(LOGVAL(fields.size()) << LOGVAL(t.isoTime()));

  clearData();
  fields_ = fields;
  ftime_ = t;
}

void FieldRenderer::setGridConverter(GridConverter* gc)
{
  gc_ = gc;
}

void FieldRenderer::setBackgroundColour(const Colour& bg)
{
  backgroundcolour_ = bg;
}

void FieldRenderer::setPlotOptions(const PlotOptions& po)
{
  poptions_ = po;

  const std::string& pt = plottype();
  is_raster_ = (pt == fpt_alpha_shade || pt == fpt_rgb || pt == fpt_alarm_box || pt == fpt_fill_cell);
  is_shade_ = is_raster_ || (poptions_.contourShading > 0 && (pt == fpt_contour || pt == fpt_contour1 || pt == fpt_contour2));
}

bool FieldRenderer::isShadePlot() const
{
  return (is_shade_ || poptions_.plot_under);
}

void FieldRenderer::setMap(const PlotArea& pa)
{
  pa_ = pa;
}

std::set<PlotOrder> FieldRenderer::plotLayers()
{
  std::set<PlotOrder> layers;
  if (poptions_.gridLines > 0 || poptions_.gridValue > 0 || (poptions_.undefMasking > 0 && checkFields(1) && !fields_[0]->allDefined()))
    layers.insert(PO_SHADE_BACKGROUND);
  if (isShadePlot())
    layers.insert(PO_SHADE);
  else
    layers.insert(PO_LINES);
  return layers;
}

void FieldRenderer::plot(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_TIME();
  if (fields_.empty())
    return;

  if (poptions_.antialiasing)
    gl->Enable(DiGLPainter::gl_MULTISAMPLE);
  else
    gl->Disable(DiGLPainter::gl_MULTISAMPLE);

  if (zorder == PO_SHADE_BACKGROUND) {
    // should be below all real fields
    if (poptions_.gridLines > 0)
      plotGridLines(gl);
    if (poptions_.gridValue > 0)
      plotNumbers(gl);

    plotUndefined(gl);
  } else if (zorder == PO_SHADE) {
    if (isShadePlot())
      plotMe(gl, zorder);
  } else if (zorder == PO_LINES) {
    if (!isShadePlot())
      plotMe(gl, zorder);
  } else if (zorder == PO_OVERLAY) {
    plotMe(gl, zorder);
  }
}

bool FieldRenderer::plotMe(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_TIME();

  if (poptions_.use_stencil || poptions_.update_stencil) {
    METLIBS_LOG_DEBUG("using stencil");
    // Enable the stencil test for masking the field to be plotted or
    // updating the stencil.
    gl->Enable(DiGLPainter::gl_STENCIL_TEST);

    if (poptions_.use_stencil) {
      // Test the stencil buffer values against a value of 0.
      gl->StencilFunc(DiGLPainter::gl_EQUAL, 0, 0x01);
      // Do not update the stencil buffer when plotting.
      gl->StencilOp(DiGLPainter::gl_KEEP, DiGLPainter::gl_KEEP, DiGLPainter::gl_KEEP);
    }
  }

  bool ok = false;

  if (plottype() == fpt_contour1) {
    ok = plotContour(gl);
  } else if (plottype() == fpt_contour || plottype() == fpt_contour2) {
    ok = plotContour2(gl, zorder);
  } else if (plottype() == fpt_wind) {
    ok = plotWind(gl);
  } else if (plottype() == fpt_wind_temp_fl) {
    ok = plotWindAndValue(gl, true);
  } else if (plottype() == fpt_wind_value) {
    ok = plotWindAndValue(gl, false);
  } else if (plottype() == fpt_value) {
    ok = plotValue(gl);
  } else if (plottype() == fpt_symbol) {
    ok = plotValue(gl);
  } else if (plottype() == fpt_vector) {
    ok = plotVector(gl);
  } else if (plottype() == fpt_direction) {
    ok = plotDirection(gl);
  } else if (is_raster_) {
    ok = plotRaster(gl);
  } else if (plottype() == fpt_frame) {
    ok = plotFrameOnly(gl);
  }

  if (poptions_.use_stencil || poptions_.update_stencil)
    gl->Disable(DiGLPainter::gl_STENCIL_TEST);

  return ok;
}

std::vector<float*> FieldRenderer::doPrepareVectors(const float* x, const float* y, bool direction)
{
  METLIBS_LOG_SCOPE();

  const bool rotateVectors = poptions_.rotateVectors;

  std::vector<float*> uv;
  float *u = 0, *v = 0;

  int nf = tmpfields_.size();

  const Projection& mapP = pa_.getMapProjection();
  if (!direction && (!rotateVectors || fields_[0]->area.P() == mapP)) {
    u = fields_[0]->data;
    v = fields_[1]->data;
    tmpfields_.clear();
  } else if (nf == 2 && tmpfields_[0]->numSmoothed == fields_[0]->numSmoothed && tmpfields_[0]->area.P() == mapP) {
    u = tmpfields_[0]->data;
    v = tmpfields_[1]->data;
  } else {
    if (nf == 0) {
      tmpfields_.push_back(std::make_shared<Field>());
      tmpfields_.push_back(std::make_shared<Field>());
    }
    *(tmpfields_[0]) = *(fields_[0]);
    *(tmpfields_[1]) = *(fields_[direction ? 0 : 1]);
    u = tmpfields_[0]->data;
    v = tmpfields_[1]->data;
    int npos = fields_[0]->area.gridSize();
    bool ok = false;
    if (direction) {
      std::fill(v, v + npos, 1.0f);
      bool turn = fields_[0]->turnWaveDirection;
      ok = gc_->getDirectionVectors(pa_.getMapArea(), turn, npos, x, y, u, v);
    } else {
      ok = gc_->getVectors(tmpfields_[0]->area, pa_.getMapProjection(), npos, x, y, u, v);
    }
    if (!ok)
      return uv;
    tmpfields_[0]->area.setP(mapP);
    tmpfields_[1]->area.setP(mapP);
  }
  uv.push_back(u);
  uv.push_back(v);
  return uv;
}

std::vector<float*> FieldRenderer::prepareVectors(const float* x, const float* y)
{
  return doPrepareVectors(x, y, false);
}

std::vector<float*> FieldRenderer::prepareDirectionVectors(const float* x, const float* y)
{
  return doPrepareVectors(x, y, true);
}

void FieldRenderer::setAutoStep(const float* x, const float* y, int& ixx1, int ix2, int& iyy1, int iy2, int maxElementsX, int& step, float& dist)
{
  int i, ix, iy;
  const int nx = fields_[0]->area.nx;
  const int ny = fields_[0]->area.ny;
  int ix1 = ixx1;
  int iy1 = iyy1;

  // Use all grid point to make average step, not only current rectangle.
  // This ensures that different tiles have the same vector density
  if (poptions_.density == -1) {
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
    float numElements = float(maxElementsX) * pa_.getPhysSize().x() / 1000.;
    float elementSize = pa_.getPlotSize().width() / numElements;
    step = int(elementSize / dist + 0.75);
    if (step < 1) {
      step = 1;
    } else {

      if (step > poptions_.densityFactor && poptions_.densityFactor > 0) {
        step /= (poptions_.densityFactor);
      }
    }
  }

  dist *= float(step);

  // adjust ix1,iy1 to make sure that same grid points are used when panning
  ixx1 = int(ixx1 / step) * step;
  iyy1 = int(iyy1 / step) * step;
}

int FieldRenderer::xAutoStep(const float* x, const float* y, int& ixx1, int ix2, int iy, float sdist)
{
  const int nx = fields_[0]->area.nx;
  if (nx < 3)
    return 1;

  int ix1 = ixx1;

  // Use all grid point to make average step, not only current rectangle.
  // This ensures that different tiles have the same vector density
  if (poptions_.density == -1) {
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
    if (x[i] == HUGE_VAL || y[i] == HUGE_VAL || x[i + 1] == HUGE_VAL || y[i + 1] == HUGE_VAL) {
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

  // adjust ix1 to make sure that same grid points are used when panning
  ixx1 = int(ixx1 / xstep) * xstep;

  return xstep;
}

bool FieldRenderer::getGridPoints(GridPoints& gp, int factor, bool boxes) const
{
  const GridArea f0area = fields_[0]->area.scaled(factor);
  gp.points = gc_->getGridPoints(f0area, pa_.getMapArea(), pa_.getMapSize(), boxes, gp.ix1, gp.ix2, gp.iy1, gp.iy2);
  return (gp.points && gp.ix1 <= gp.ix2 && gp.iy1 <= gp.iy2);
}

bool FieldRenderer::getPoints(int n, float* x, float* y) const
{
  return pa_.getMapArea().P().convertPoints(fields_[0]->area.P(), n, x, y);
}

// plot vector field as wind arrows
// Fields u(0) v(1), optional- colorfield(2)
bool FieldRenderer::plotWind(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(2))
    return false;

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = gp.x(), *y = gp.y();

  // convert windvectors to correct projection
  const std::vector<float*> uv = prepareVectors(x, y);
  if (uv.size() != 2)
    return false;
  const float* u = uv[0];
  const float* v = uv[1];

  int step = poptions_.density;
  float sdist;
  setAutoStep(x, y, gp.ix1, gp.ix2, gp.iy1, gp.iy2, MaxWindsAuto, step, sdist);

  const int nx = fields_[0]->area.nx;
  const int ny = fields_[0]->area.ny;

  if (poptions_.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  diutil::ColourLimits colours;
  if (fields_.size() == 3 && fields_[2] && fields_[2]->data) {
    float mini, maxi;
    if (!diutil::mini_maxi(fields_[2]->data, nx * ny, fieldUndef, mini, maxi))
      return false;
    colours.initialize(poptions_, mini, maxi);
  }

  const float unitlength = poptions_.vectorunit / 10;
  const float flagl = sdist * 0.85 / unitlength;

  gp.ix1 -= step;
  if (gp.ix1 < 0)
    gp.ix1 = 0;
  gp.iy1 -= step;
  if (gp.iy1 < 0)
    gp.iy1 = 0;
  gp.ix2 += (step + 1);
  if (gp.ix2 > nx)
    gp.ix2 = nx;
  gp.iy2 += (step + 1);
  if (gp.iy2 > ny)
    gp.iy2 = ny;

  const Rectangle msex = diutil::extendedRectangle(pa_.getMapSize(), flagl);

  const Projection& projection = pa_.getMapArea().P();

  gl->setLineStyle(poptions_.linecolour, poptions_.linewidth, false);

  for (int iy = gp.iy1; iy < gp.iy2; iy += step) {
    const int xstep = xAutoStep(x, y, gp.ix1, gp.ix2, iy, sdist);
    for (int ix = gp.ix1; ix < gp.ix2; ix += xstep) {
      const int i = iy * nx + ix;

      if (u[i] == fieldUndef || v[i] == fieldUndef || !msex.isinside(x[i], y[i]))
        continue;

      if (colours) {
        const float c = fields_[2]->data[i];
        if (c == fieldUndef)
          continue;
        else
          colours.setColour(gl, c);
      }

      // If southern hemisphere, turn the feathers
      float xx = x[i], yy = y[i];
      projection.convertToGeographic(1, &xx, &yy);
      const int turnBarbs = (yy < 0) ? -1 : 1;
      const float KNOT = 3600.0 / 1852.0;
      const float gu = u[i] * KNOT, gv = v[i] * KNOT;
      gl->drawWindArrow(gu, gv, x[i], y[i], flagl, poptions_.arrowstyle == arrow_wind_arrow, turnBarbs);
    }
  }
  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (poptions_.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

/*
 ROUTINE:   FieldRenderer::plotValue
 PURPOSE:   plot field value as number or symbol
 ALGORITHM:
 */

bool FieldRenderer::plotValue(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (fields_.size() > 1)
    return plotValues(gl);

  if (not checkFields(1))
    return false;

  // plot symbol
  std::map<float, std::string> classImages;
  if (plottype() == fpt_symbol) {
    std::vector<float> classValues;
    std::vector<std::string> classNames;
    unsigned int maxlen = 0;
    diutil::parseClasses(poptions_, classValues, classNames, classImages, maxlen);
  }

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = gp.x(), *y = gp.y();

  const float* field = fields_[0]->data;
  int step = poptions_.density;
  float sdist;
  setAutoStep(x, y, gp.ix1, gp.ix2, gp.iy1, gp.iy2, MaxWindsAuto, step, sdist);

  const int nx = fields_[0]->area.nx;
  const int ny = fields_[0]->area.ny;
  if (poptions_.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  bool smallvalues = false;
  if (poptions_.precision == 0) {
    int npos = nx * ny;
    int ii = 0;
    while (ii < npos && (fabsf(field[ii]) == fieldUndef || fabsf(field[ii]) < 1)) {
      ++ii;
    }
    smallvalues = (ii == npos);
  }

  float flagl = sdist * 0.85;

  gp.ix1 -= step;
  if (gp.ix1 < 0)
    gp.ix1 = 0;
  gp.iy1 -= step;
  if (gp.iy1 < 0)
    gp.iy1 = 0;
  gp.ix2 += (step + 1);
  if (gp.ix2 > nx)
    gp.ix2 = nx;
  gp.iy2 += (step + 1);
  if (gp.iy2 > ny)
    gp.iy2 = ny;

  gl->setColour(poptions_.linecolour, false);

  const Rectangle msex = diutil::extendedRectangle(pa_.getMapSize(), flagl);

  gl->setFont(diutil::BITMAPFONT, poptions_.fontface, 10. * poptions_.labelSize);

  float chx, chy;
  gl->getCharSize('0', chx, chy);
  chy *= 0.75;

  ImageGallery ig;
  for (int iy = gp.iy1; iy < gp.iy2; iy += step) {
    const int xstep = xAutoStep(x, y, gp.ix1, gp.ix2, iy, sdist);
    for (int ix = gp.ix1; ix < gp.ix2; ix += xstep) {
      const int i = iy * nx + ix;
      const float gx = x[i], gy = y[i], value = field[i];
      if (value != fieldUndef && msex.isinside(gx, gy) && value >= poptions_.minvalue && value <= poptions_.maxvalue) {
        if (poptions_.colours.size() > 2) {
          if (value < poptions_.base) {
            gl->setColour(poptions_.colours[0], false);
          } else if (value > poptions_.base) {
            gl->setColour(poptions_.colours[2], false);
          } else {
            gl->setColour(poptions_.colours[1], false);
          }
        }

        if (!classImages.empty()) { // plot symbol
          std::map<float, std::string>::const_iterator it = classImages.find(value);
          if (it != classImages.end()) {
            ig.plotImage(gl, pa_, it->second, gx, gy, true, poptions_.labelSize * 0.25);
          }
        } else { // plot value
          QString ost;
          if (smallvalues)
            ost = QString::number(value, 'g', 1);
          else
            ost = QString::number(value, 'f', poptions_.precision);
          gl->drawText(ost, gx - chx / 2, gy - chy / 2, 0.0);
        }
      }
    }
  }

  if (poptions_.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

/*
 ROUTINE:   FieldRenderer::plotWindAndValue
 PURPOSE:   plot vector field as wind arrows and third field as number.
 ALGORITHM: Fields u(0) v(1) number(2)
 */

bool FieldRenderer::plotWindAndValue(DiGLPainter* gl, bool flightlevelChart)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(3))
    return false;

  float* t = fields_[2]->data;

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = gp.x(), *y = gp.y();

  // convert windvectors to correct projection
  std::vector<float*> uv = prepareVectors(x, y);
  if (uv.size() != 2)
    return false;
  float* u = uv[0];
  float* v = uv[1];

  int step = poptions_.density;
  float sdist;
  setAutoStep(x, y, gp.ix1, gp.ix2, gp.iy1, gp.iy2, MaxWindsAuto, step, sdist);
  int xstep;

  int i, ix, iy;
  const int nx = fields_[0]->area.nx;
  const int ny = fields_[0]->area.ny;

  if (poptions_.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  float unitlength = poptions_.vectorunit / 10;
  int n50, n10, n05;
  float ff, gu, gv, gx, gy, dx, dy, dxf, dyf;
  float flagl = sdist * 0.85 / unitlength;
  float flagstep = flagl / 10.;
  float flagw = flagl * 0.35;
  float hflagw = 0.6;

  std::vector<float> vx, vy; // keep vertices for 50-knot flags

  gp.ix1 -= step;
  if (gp.ix1 < 0)
    gp.ix1 = 0;
  gp.iy1 -= step;
  if (gp.iy1 < 0)
    gp.iy1 = 0;
  gp.ix2 += (step + 1);
  if (gp.ix2 > nx)
    gp.ix2 = nx;
  gp.iy2 += (step + 1);
  if (gp.iy2 > ny)
    gp.iy2 = ny;

  const Rectangle& ms = pa_.getMapSize();
  const Rectangle msex = diutil::extendedRectangle(ms, flagl);
  float fontsize = 7. * poptions_.labelSize;

  gl->setFont(diutil::BITMAPFONT, poptions_.fontface, fontsize);

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
  if (nbmap < 0 || nbmap > 1000000)
    return false;
  int* bmap = new int[nbmap];
  for (i = 0; i < nbmap; i++)
    bmap[i] = 0;
  int m, ib, jb, ibit, iwrd, nb;
  float xb[20][2], yb[20][2];

  std::vector<int> vxstep;

  gl->setLineStyle(poptions_.linecolour, poptions_.linewidth, false);

  // plot wind............................................

  gl->Begin(DiGLPainter::gl_LINES);

  // Wind arrows are adjusted to lat=10 and Lon=10 if
  // poptions_.density!=auto and proj=geographic
  bool adjustToLatLon = poptions_.density > 0 && fields_[0]->area.P().isGeographic() && step > 0;
  if (adjustToLatLon)
    gp.iy1 = (gp.iy1 / step) * step;
  for (iy = gp.iy1; iy < gp.iy2; iy += step) {
    xstep = xAutoStep(x, y, gp.ix1, gp.ix2, iy, sdist);
    if (adjustToLatLon) {
      xstep = (xstep / step) * step;
      if (xstep == 0)
        xstep = step;
      gp.ix1 = (gp.ix1 / xstep) * xstep;
    }
    vxstep.push_back(xstep);
    for (ix = gp.ix1; ix < gp.ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];
      if (u[i] != fieldUndef && v[i] != fieldUndef && msex.isinside(gx, gy) && t[i] >= poptions_.minvalue && t[i] <= poptions_.maxvalue) {
        ff = miutil::absval(u[i], v[i]);
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
          gl->Vertex2f(gx, gy);
          gx = gx - flagl * gu;
          gy = gy - flagl * gv;
          gl->Vertex2f(gx, gy);

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
          if (n05 > 0) {
            if (n50 + n10 == 0) {
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

  float adx[9] = {d, d, d, 0.0f, -d, -d, -d, 0.0f, 0.0f};
  float cdx[9] = {0.0f, 0.0f, 0.0f, -0.5f, -1.0f, -1.0f, -1.0f, -0.5f, -0.5f};
  float ady[9] = {d, -d, -d - chy, -d - chy, -d - chy, -hchy, d, d, -hchy};

  int j, ipos0, ipos1, ipos, ib1, ib2, jb1, jb2, mused, nused, value;
  float x1, x2, y1, y2, w, h;
  int ivx = 0;

  for (iy = gp.iy1; iy < gp.iy2; iy += step) {
    xstep = vxstep[ivx++];
    for (ix = gp.ix1; ix < gp.ix2; ix += xstep) {
      i = iy * nx + ix;
      gx = x[i];
      gy = y[i];
      if (t[i] != fieldUndef && pa_.getMapSize().isinside(gx, gy) && t[i] >= poptions_.minvalue && t[i] <= poptions_.maxvalue) {

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
        if (poptions_.colours.size() > 2) {
          if (value < poptions_.base) {
            gl->setColour(poptions_.colours[0]);
          } else if (value > poptions_.base) {
            gl->setColour(poptions_.colours[2]);
          } else {
            gl->setColour(poptions_.colours[1]);
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
        if (pa_.getMapSize().isinside(x1, y1) && pa_.getMapSize().isinside(x2, y2)) {
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

  if (poptions_.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

/*
 ROUTINE:   FieldRenderer::plotValues
 PURPOSE:   plot the values of three, four or five fields:
 field1  |       field1      |         field1  field4
 field3  | field3     field4 |  field3  -----
 field2  |      field2       |          field2 field5
 colours are set by poptions_.textcolour or poptions_.colours
 minvalue and maxvalue refer to field1
 */
bool FieldRenderer::plotValues(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(0))
    return false;

  const size_t nfields = fields_.size();

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = gp.x(), *y = gp.y();

  int step = poptions_.density;
  float sdist;
  setAutoStep(x, y, gp.ix1, gp.ix2, gp.iy1, gp.iy2, 22, step, sdist);
  int xstep;

  const int nx = fields_[0]->area.nx;
  const int ny = fields_[0]->area.ny;

  if (poptions_.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  gp.ix1 -= step;
  if (gp.ix1 < 0)
    gp.ix1 = 0;
  gp.iy1 -= step;
  if (gp.iy1 < 0)
    gp.iy1 = 0;
  gp.ix2 += (step + 1);
  if (gp.ix2 > nx)
    gp.ix2 = nx;
  gp.iy2 += (step + 1);
  if (gp.iy2 > ny)
    gp.iy2 = ny;

  float fontsize = 8. * poptions_.labelSize;

  gl->setFont(diutil::BITMAPFONT, poptions_.fontface, fontsize);

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

  float adx[9] = {xshift, xshift, xshift, 0, -xshift, -xshift, -xshift, 0, 0};
  float cdx[9] = {0.0, 0.0, 0.0, -0.5, -1.0, -1.0, -1.0, -0.5, -0.5};
  float ady[9] = {yshift, -yshift, -yshift - chy, -yshift - chy, -yshift - chy, -hchy, yshift, yshift, -hchy};

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
    if (poptions_.colours.size() > i) {
      col[i] = poptions_.colours[i];
    } else {
      col[i] = poptions_.textcolour;
    }
  }

  // Wind arrows are adjusted to lat=10 and Lon=10 if
  // poptions_.density!=auto and proj=geographic
  std::vector<int> vxstep;
  bool adjustToLatLon = poptions_.density && fields_[0]->area.P().isGeographic() && step > 0;
  if (adjustToLatLon)
    gp.iy1 = (gp.iy1 / step) * step;
  for (int iy = gp.iy1; iy < gp.iy2; iy += step) {
    xstep = xAutoStep(x, y, gp.ix1, gp.ix2, iy, sdist);
    if (adjustToLatLon) {
      xstep = (xstep / step) * step;
      if (xstep == 0)
        xstep = step;
      gp.ix1 = (gp.ix1 / xstep) * xstep;
    }
    vxstep.push_back(xstep);
  }

  float x1, x2, y1, y2, w, h;
  int ivx = 0;
  for (int iy = gp.iy1; iy < gp.iy2; iy += step) {
    xstep = vxstep[ivx++];
    for (int ix = gp.ix1; ix < gp.ix2; ix += xstep) {
      int i = iy * nx + ix;
      float gx = x[i];
      float gy = y[i];

      if (fields_[0]->data[i] >= poptions_.minvalue && fields_[0]->data[i] <= poptions_.maxvalue) {

        for (size_t j = 0; j < nfields; j++) {
          float* fieldData = fields_[j]->data;
          if (fieldData[i] != fieldUndef && pa_.getMapSize().isinside(gx, gy)) {
            int value = (fieldData[i] >= 0.0f) ? int(fieldData[i] + 0.5f) : int(fieldData[i] - 0.5f);
            const QString str = QString::number(value);
            gl->getTextSize(str, w, h);

            int ipos1 = position[j];
            x1 = gx + adx[ipos1] + cdx[ipos1] * w;
            y1 = gy + ady[ipos1];
            x2 = x1 + w;
            y2 = y1 + chy;
            if (pa_.getMapSize().isinside(x1, y1) && pa_.getMapSize().isinside(x2, y2)) {
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
          if (pa_.getMapSize().isinside(x1, y1) && pa_.getMapSize().isinside(x2, y2)) {
            gl->drawText(str, x1, y1, 0.0);
          }
        }
      }
    }
  }

  if (poptions_.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  return true;
}

//  plot vector field as arrows (wind,sea current,...)
bool FieldRenderer::plotVector(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE(LOGVAL(fields_.size()));

  if (not checkFields(2))
    return false;

  const float* colourfield = (fields_.size() == 3 && fields_[2] && fields_[2]->data) ? fields_[2]->data : 0;

  float arrowlength = 0;
  return plotArrows(gl, &FieldRenderer::prepareVectors, colourfield, arrowlength);
}

/* plot true north direction field as arrows (wave,...) */
bool FieldRenderer::plotDirection(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;
  const float* colourfield = (fields_.size() == 2 && fields_[1] && fields_[1]->data) ? fields_[1]->data : 0;

  float arrowlength_dummy = 0;
  return plotArrows(gl, &FieldRenderer::prepareDirectionVectors, colourfield, arrowlength_dummy);
}

//  plot vector field as arrows (wind,sea current,...)
bool FieldRenderer::plotArrows(DiGLPainter* gl, prepare_vectors_t pre_vec, const float* colourfield, float& arrowlength)
{
  METLIBS_LOG_SCOPE(LOGVAL(fields_.size()));

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = gp.x(), *y = gp.y();

  // convert vectors to correct projection
  std::vector<float*> uv = (this->*pre_vec)(x, y);
  if (uv.size() != 2)
    return false;
  float* u = uv[0];
  float* v = uv[1];

  int step = poptions_.density;
  float sdist;
  setAutoStep(x, y, gp.ix1, gp.ix2, gp.iy1, gp.iy2, MaxArrowsAuto, step, sdist);
  int xstep;

  const int nx = fields_[0]->area.nx;
  const int ny = fields_[0]->area.ny;

  if (poptions_.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  diutil::ColourLimits colours;
  if (colourfield) {
    float mini, maxi;
    if (!diutil::mini_maxi(colourfield, nx * ny, fieldUndef, mini, maxi))
      return false;
    colours.initialize(poptions_, mini, maxi);
  }

  arrowlength = sdist;
  const float unitlength = poptions_.vectorunit;
  const float scale = arrowlength / unitlength;

  gp.ix1 -= step;
  if (gp.ix1 < 0)
    gp.ix1 = 0;
  gp.iy1 -= step;
  if (gp.iy1 < 0)
    gp.iy1 = 0;
  gp.ix2 += (step + 1);
  if (gp.ix2 > nx)
    gp.ix2 = nx;
  gp.iy2 += (step + 1);
  if (gp.iy2 > ny)
    gp.iy2 = ny;

  const Rectangle msex = diutil::extendedRectangle(pa_.getMapSize(), arrowlength * 1.5);

  gl->setLineStyle(poptions_.linecolour, poptions_.linewidth, false);

  for (int iy = gp.iy1; iy < gp.iy2; iy += step) {
    xstep = xAutoStep(x, y, gp.ix1, gp.ix2, iy, sdist);
    for (int ix = gp.ix1; ix < gp.ix2; ix += xstep) {
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

  if (poptions_.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

//  plot scalar field as contour lines
bool FieldRenderer::plotContour(DiGLPainter* gl)
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
  float rlines[poptions_.linevalues.size()];
  int idraw2, nlines2, nlim2;
  int ncol2, icol2[mmmUsed], ntyp2, ityp2[mmmUsed], nwid2, iwid2[mmmUsed];
  float zrange2[2], zstep2 = 0, zoff2 = 0, rlines2[mmmUsed], rlim2[mmm];
  int ibmap, lbmap, kbmap[mmm], nxbmap, nybmap;
  float rbmap[4];

  bool res = true;

  // Resampling disabled
  int factor = 1; // resamplingFactor(nx, ny);
  if (factor < 2)
    factor = 1;

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp, factor)) {
    METLIBS_LOG_ERROR("getGridPoints returned false");
    return false;
  }
  const float *x = gp.x(), *y = gp.y();

  const int rnx = fields_[0]->area.nx / factor, rny = fields_[0]->area.ny / factor;

  // Create a resampled data array to pass to the contour function.
  float* data;
  if (factor != 1) {
    METLIBS_LOG_INFO("Resampled field from" << LOGVAL(fields_[0]->area.nx) << LOGVAL(fields_[0]->area.ny) << " to" << LOGVAL(rnx) << LOGVAL(rny));
    data = new float[rnx * rny];
    int i = 0;
    for (int iy = 0; iy < rny; ++iy) {
      int j = 0;
      for (int ix = 0; ix < rnx; ++ix) {
        data[(i * rnx) + j] = fields_[0]->data[(iy * fields_[0]->area.nx * factor) + (ix * factor)];
        ++j;
      }
      ++i;
    }
  } else
    data = fields_[0]->data;

  if (gp.ix1 >= gp.ix2 || gp.iy1 >= gp.iy2)
    return false;
  if (gp.ix1 >= rnx || gp.ix2 < 0 || gp.iy1 >= rny || gp.iy2 < 0)
    return false;

  if (poptions_.frame)
    plotFrame(gl, rnx, rny, x, y);

  ipart[0] = gp.ix1;
  ipart[1] = gp.ix2;
  ipart[2] = gp.iy1;
  ipart[3] = gp.iy2;

  xylim[0] = pa_.getMapSize().x1;
  xylim[1] = pa_.getMapSize().x2;
  xylim[2] = pa_.getMapSize().y1;
  xylim[3] = pa_.getMapSize().y2;

  if (poptions_.valueLabel == 0)
    labfmt[0] = 0;
  else
    labfmt[0] = -1;
  labfmt[1] = 0;
  labfmt[2] = 0;
  ibcol = -1;

  if (labfmt[0] != 0) {
    float fontsize = 10. * poptions_.labelSize;

    gl->setFont(poptions_.fontname, poptions_.fontface, fontsize);
    gl->getCharSize('0', chxlab, chylab);

    // the real height for numbers 0-9 (width is ok)
    chylab *= 0.75;
  } else {
    chxlab = chylab = 1.0;
  }

  zstep = poptions_.lineinterval;
  zoff = poptions_.base;

  if (poptions_.linevalues.size() > 0) {
    nlines = poptions_.linevalues.size();
    if (nlines > mmmUsed)
      nlines = mmmUsed;
    for (int ii = 0; ii < nlines; ii++) {
      rlines[ii] = poptions_.linevalues[ii];
    }
    idraw = 3;
  } else if (poptions_.loglinevalues.size() > 0) {
    nlines = poptions_.loglinevalues.size();
    if (nlines > mmmUsed)
      nlines = mmmUsed;
    for (int ii = 0; ii < nlines; ii++) {
      rlines[ii] = poptions_.loglinevalues[ii];
    }
    idraw = 4;
  } else {
    nlines = 0;
    idraw = 1;
    if (poptions_.zeroLine == 0) {
      idraw = 2;
    }
  }

  zrange[0] = +1.;
  zrange[1] = -1.;
  zrange2[0] = +1.;
  zrange2[1] = -1.;

  if (poptions_.minvalue > -fieldUndef || poptions_.maxvalue < fieldUndef) {
    zrange[0] = poptions_.minvalue;
    zrange[1] = poptions_.maxvalue;
  }

  ncol = 1;
  icol[0] = -1; // -1: set colour below
  // otherwise index in poptions_.colours[]
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

  ismooth = poptions_.lineSmooth;
  if (ismooth < 0)
    ismooth = 0;

  ibmap = 0;
  lbmap = 0;
  nxbmap = 0;
  nybmap = 0;

  if (poptions_.contourShading == 0 && !poptions_.options_1)
    idraw = 0;

  // Plot colour shading
  if (poptions_.contourShading != 0) {

    int idraw2 = 0;

    if (poptions_.antialiasing)
      gl->Disable(DiGLPainter::gl_MULTISAMPLE);

    METLIBS_LOG_TIME("contour");
    res = contour(rnx, rny, data, x, y, ipart, 2, NULL, xylim, idraw, zrange, zstep, zoff, nlines, rlines, ncol, icol, ntyp, ityp, nwid, iwid, nlim, rlim,
                  idraw2, zrange2, zstep2, zoff2, nlines2, rlines2, ncol2, icol2, ntyp2, ityp2, nwid2, iwid2, nlim2, rlim2, ismooth, labfmt, chxlab, chylab,
                  ibcol, ibmap, lbmap, kbmap, nxbmap, nybmap, rbmap, gl, poptions_, fields_[0]->area, fieldUndef,
                  "diana" /*only used for writing shape files */, fields_[0]->name, ftime_.hour());

    if (poptions_.antialiasing)
      gl->Enable(DiGLPainter::gl_MULTISAMPLE);
  }

  // Plot contour lines
  if (!poptions_.options_1)
    idraw = 0;

  if (!poptions_.options_2) {
    idraw2 = 0;
  } else {
    zstep2 = poptions_.lineinterval_2;
    zoff2 = poptions_.base_2;
    if (poptions_.linevalues_2.size() > 0) {
      nlines2 = poptions_.linevalues_2.size();
      if (nlines2 > mmmUsed)
        nlines2 = mmmUsed;
      for (int ii = 0; ii < nlines2; ii++) {
        rlines2[ii] = poptions_.linevalues_2[ii];
      }
      idraw2 = 3;
    } else if (poptions_.loglinevalues_2.size() > 0) {
      nlines2 = poptions_.loglinevalues_2.size();
      if (nlines2 > mmmUsed)
        nlines2 = mmmUsed;
      for (int ii = 0; ii < nlines2; ii++) {
        rlines2[ii] = poptions_.loglinevalues_2[ii];
      }
      idraw2 = 4;
    } else {
      idraw2 = 1;
      if (poptions_.zeroLine == 0) {
        idraw2 = 2;
      }
    }
  }

  if (idraw > 0 || idraw2 > 0) {

    if (poptions_.colours.size() > 1) {
      if (idraw > 0 && idraw2 > 0) {
        icol[0] = 0;
        icol2[0] = 1;
      } else {
        ncol = poptions_.colours.size();
        if (ncol > mmmUsed)
          ncol = mmmUsed;
        for (int i = 0; i < ncol; ++i)
          icol[i] = i;
      }
    } else if (idraw > 0) {
      gl->setColour(poptions_.linecolour, false);
    } else {
      gl->setColour(poptions_.linecolour_2, false);
    }

    if (poptions_.minvalue_2 > -fieldUndef || poptions_.maxvalue_2 < fieldUndef) {
      zrange2[0] = poptions_.minvalue_2;
      zrange2[1] = poptions_.maxvalue_2;
    }

    if (poptions_.linewidths.size() == 1) {
      gl->LineWidth(poptions_.linewidth);
    } else {
      if (idraw2 > 0) { // two set of plot options
        iwid[0] = 0;
        iwid2[0] = 1;
      } else { // one set of plot options, different lines
        nwid = poptions_.linewidths.size();
        if (nwid > mmmUsed)
          nwid = mmmUsed;
        for (int i = 0; i < nwid; ++i)
          iwid[i] = i;
      }
    }

    if (poptions_.linetypes.size() == 1 && poptions_.linetype.stipple) {
      gl->LineStipple(poptions_.linetype.factor, poptions_.linetype.bmap);
      gl->Enable(DiGLPainter::gl_LINE_STIPPLE);
    } else {
      if (idraw2 > 0) { // two set of plot options
        ityp[0] = 0;
        ityp2[0] = 1;
      } else { // one set of plot options, different lines
        ntyp = poptions_.linetypes.size();
        if (ntyp > mmmUsed)
          ntyp = mmmUsed;
        for (int i = 0; i < ntyp; ++i)
          ityp[i] = i;
      }
    }

    if (!poptions_.options_1)
      idraw = 0;

    // turn off contour shading
    bool contourShading = poptions_.contourShading;
    poptions_.contourShading = 0;

    res = contour(rnx, rny, data, x, y, ipart, 2, NULL, xylim, idraw, zrange, zstep, zoff, nlines, rlines, ncol, icol, ntyp, ityp, nwid, iwid, nlim, rlim,
                  idraw2, zrange2, zstep2, zoff2, nlines2, rlines2, ncol2, icol2, ntyp2, ityp2, nwid2, iwid2, nlim2, rlim2, ismooth, labfmt, chxlab, chylab,
                  ibcol, ibmap, lbmap, kbmap, nxbmap, nybmap, rbmap, gl, poptions_, fields_[0]->area, fieldUndef,
                  "diana" /* only used for writing shape files */, fields_[0]->name, ftime_.hour());

    // restore contour shading
    poptions_.contourShading = contourShading;
  }

  if (poptions_.extremeType != "None" && poptions_.extremeType != "Ingen" && !poptions_.extremeType.empty()) {
    markExtreme(gl);
  }

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (!res)
    METLIBS_LOG_ERROR("contour error");

  if (poptions_.update_stencil)
    plotFrameStencil(gl, rnx, rny, x, y);

  if (factor != 1)
    delete[] data;

  return true;
}

//  plot scalar field as contour lines using new contour algorithm
bool FieldRenderer::plotContour2(DiGLPainter* gl, PlotOrder zorder)
{
  METLIBS_LOG_TIME();

  int paintMode;
  if (zorder == PO_SHADE_BACKGROUND) {
    if (poptions_.undefMasking <= 0)
      return true;
    paintMode = DianaLines::UNDEFINED;
  } else if (zorder == PO_SHADE) {
    if (!isShadePlot())
      return true;
    paintMode = DianaLines::FILL | DianaLines::LINES_LABELS;
  } else if (zorder == PO_LINES) {
    if (isShadePlot())
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
  GridPoints gp;
  if (!getGridPoints(gp)) {
    METLIBS_LOG_INFO("getGridPoints problem");
    return false;
  }
  const float *x = gp.x(), *y = gp.y();

  if (gp.ix1 >= gp.ix2 || gp.iy1 >= gp.iy2)
    return false;
  const int nx = fields_[0]->area.nx;
  const int ny = fields_[0]->area.ny;
  if (gp.ix1 >= nx || gp.ix2 < 0 || gp.iy1 >= ny || gp.iy2 < 0)
    return false;

  if (poptions_.frame)
    plotFrame(gl, nx, ny, x, y);

  if (poptions_.valueLabel)
    gl->setFont(poptions_.fontname, poptions_.fontface, 10 * poptions_.labelSize);

  {
    if (not poly_contour(nx, ny, gp.ix1, gp.iy1, gp.ix2, gp.iy2, fields_[0]->data, x, y, gl, poptions_, fieldUndef, paintMode))
      METLIBS_LOG_ERROR("contour2 error");
  }
  if (poptions_.options_2) {
    if (not poly_contour(nx, ny, gp.ix1, gp.iy1, gp.ix2, gp.iy2, fields_[0]->data, x, y, gl, poptions_, fieldUndef, paintMode, true))
      METLIBS_LOG_ERROR("contour2 options_2 error");
  }
  if (poptions_.extremeType != "None" && poptions_.extremeType != "Ingen" && !poptions_.extremeType.empty())
    markExtreme(gl);

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);

  if (poptions_.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

bool FieldRenderer::plotRaster(DiGLPainter* gl)
{
  METLIBS_LOG_TIME();

  if (not checkFields(1))
    return false;

  const float *x = 0, *y = 0;
  GridPoints gp; // put here to keep in scope while in use
  int nx = fields_[0]->area.nx+1, ny = fields_[0]->area.ny+1;
  if (poptions_.frame || poptions_.update_stencil) {
    if (getGridPoints(gp, 1, true)) {
      x = gp.x();
      y = gp.y();
    }
  }
  if (x && y && poptions_.frame) {
    gl->setLineStyle(poptions_.bordercolour, poptions_.linewidth, false);
    plotFrame(gl, nx, ny, x, y);
  }

  QImage target;
  if (plottype() == fpt_rgb) {
    if (checkFields(3)) {
      RasterRGB raster(pa_, fields_, poptions_);
      target = raster.rasterPaint();
    }
  } else if (plottype() == fpt_alpha_shade) {
    RasterAlpha raster(pa_, fields_[0], poptions_);
    target = raster.rasterPaint();
  } else if (plottype() == fpt_fill_cell) {
    RasterFillCell raster(pa_, fields_[0], poptions_);
    target = raster.rasterPaint();
  } else if (plottype() == fpt_alarm_box) {
    RasterAlarmBox raster(pa_, fields_[0], poptions_);
    target = raster.rasterPaint();
  }
  if (!target.isNull())
    gl->drawScreenImage(QPointF(0, 0), target);

  if (x && y && poptions_.update_stencil) {
    plotFrameStencil(gl, nx, ny, x, y);
  }

  return true;
}

bool FieldRenderer::plotFrameOnly(DiGLPainter* gl)
{
  if (not checkFields(1))
    return false;

  int nx = fields_[0]->area.nx;
  int ny = fields_[0]->area.ny;

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = gp.x(), *y = gp.y();

  if (poptions_.frame) {
    plotFrame(gl, nx, ny, x, y);
  }

  if (poptions_.update_stencil)
    plotFrameStencil(gl, nx, ny, x, y);

  return true;
}

QPolygonF FieldRenderer::makeFramePolygon(int nx, int ny, const float* x, const float* y) const
{
  int lx = nx, ly = ny;
  // FIXME adjusting the frame boundaries should be done from the function calling plotFrame(Stencil)
  if (plottype() == fpt_contour || plottype() == fpt_contour2) {
    lx -= 1;
    ly -= 1;
  }
  diutil::PolylineBuilder frame;
  frame.reserve(1 + 2 * (lx + ly));
  for (int ix = 0; ix < lx; ix++)
    frame.addValid(x, y, diutil::index(nx, ix, 0));
  for (int iy = 1; iy < ly; iy++)
    frame.addValid(x, y, diutil::index(nx, lx - 1, iy));
  for (int ix = lx - 2; ix >= 0; ix--)
    frame.addValid(x, y, diutil::index(nx, ix, ly - 1));
  for (int iy = ly - 2; iy > 0; iy--)
    frame.addValid(x, y, diutil::index(nx, 0, iy));
  frame.addValid(x, y, 0);
  return frame.polyline();
}

// plot frame for complete field area
void FieldRenderer::plotFrame(DiGLPainter* gl, int nx, int ny, const float* x, const float* y)
{
  METLIBS_LOG_TIME(LOGVAL(nx) << LOGVAL(ny));

  if (fields_.empty() || !fields_[0])
    return;

  const QPolygonF frame = makeFramePolygon(nx, ny, x, y);

  // If the frame value is 2 or 3 then fill the frame with the background colour.
  if ((poptions_.frame & 2) != 0) {
    gl->setColour(backgroundcolour_, true);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);
    gl->drawPolygon(frame);
  }
  if ((poptions_.frame & 1) != 0) {
    gl->setLineStyle(poptions_.bordercolour, poptions_.linewidth, true);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_LINE);
    gl->drawPolyline(frame);
  }
}

void FieldRenderer::plotFrameStencil(DiGLPainter* gl, int nx, int ny, const float* x, const float* y)
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
  for (int i = 0; i < frame.size(); ++i) {
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
bool FieldRenderer::markExtreme(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  int nx = fields_[0]->area.nx;
  int ny = fields_[0]->area.ny;

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = gp.x(), *y = gp.y();

  if (gp.ix1 < 1)
    gp.ix1 = 1;
  if (gp.ix2 > nx - 2)
    gp.ix2 = nx - 2;
  if (gp.iy1 < 1)
    gp.iy1 = 1;
  if (gp.iy2 > ny - 2)
    gp.iy2 = ny - 2;

  // find avg. dist. between grid points
  int ixstp = std::max((gp.ix2 - gp.ix1) / 5, 1);
  int iystp = std::max((gp.iy2 - gp.iy1) / 5, 1);
  float avgdist = 0;
  int navg = 0;
  for (int iy = gp.iy1; iy < gp.iy2; iy += iystp) {
    for (int ix = gp.ix1; ix < gp.ix2; ix += ixstp) {
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

  gp.ix2++;
  gp.iy2++;

  QString pmarks[2];
  bool extremeString = false;
  enum { NONE, MIN, MAX, BOTH } extremeValue = NONE;
  const std::vector<std::string> vextremetype = miutil::split(miutil::to_upper(poptions_.extremeType), "+");
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

  gl->setColour(poptions_.linecolour, false);

  const float fontsizeHuge = 28. * poptions_.extremeSize;
  const float fontsizeBig = 18. * poptions_.extremeSize;
  gl->setFont(poptions_.fontname, poptions_.fontface, fontsizeHuge);

  float chrx, chry;
  gl->getCharSize('L', chrx, chry);
  chry *= 0.75; // approx. real height for the character (width is ok)
  const float size = std::max(chry, chrx);

  const float radius = 3.0 * size * poptions_.extremeRadius;
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

  const Rectangle ems = diutil::extendedRectangle(pa_.getMapSize(), -size * 0.5);

  for (int iy = gp.iy1; iy < gp.iy2; iy++) {
    for (int ix = gp.ix1; ix < gp.ix2; ix++) {
      const int p = iy * nx + ix;
      const float fpos = fields_[0]->data[p];
      if (fpos == fieldUndef)
        continue;
      if (!(fpos < poptions_.maxvalue && fpos > poptions_.minvalue))
        continue;

      const float gx = x[p];
      const float gy = y[p];
      if (!ems.isinside(gx, gy))
        continue;

      {
        // first check the nearest gridpoints
        float fmin = fieldUndef;
        float fmax = -fieldUndef;
        for (int j = 0; j < 8; j++) {
          const float f = fields_[0]->data[p + near[j]];
          if (fmin > f)
            fmin = f;
          if (fmax < f)
            fmax = f;
        }

        if (fmax != fieldUndef && ((fmin >= fpos && extremeValue != MAX) || (fmax <= fpos && extremeValue != MIN)) && fmin != fmax) {
          // scan a larger area

          bool ok = true;
          const bool is_minimum = (fmin >= fpos); // minimum at pos ix,iy
          std::vector<int> pequal;

          const int jbeg = std::max(iy - nscan, 0);
          const int jend = std::min(iy + nscan + 1, ny);
          for (int j = jbeg; j < jend && ok; ++j) {
            const int n = iscan[nscan + iy - j];
            const int ibeg = std::max(ix - n, 0);
            const int iend = std::min(ix + n + 1, nx);
            for (int i = ibeg; i < iend; i++) {
              const int ppp = j * nx + i;
              const float f = fields_[0]->data[ppp];
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
                    const float f1 = fields_[0]->data[pp + igrad[k][4]];
                    const float f2 = fields_[0]->data[pp + igrad[k][5]];
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
                const QString str = QString::number(fpos, 'f', poptions_.precision);
                gl->drawText(str, gx - chrx * 0.5, gy - chry * 0.5, 0.0);
              } else {
                gl->drawText(pmarks[is_minimum ? 0 : 1], gx - chrx * 0.5, gy - chry * 0.5, 0.0);
                if (extremeValue != NONE) {
                  gl->setFontSize(fontsizeBig);
                  const QString str = QString::number(fpos, 'f', poptions_.precision);
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
bool FieldRenderer::plotGridLines(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (not checkFields(1))
    return false;

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = x, *y = y;

  gp.ix2++;
  gp.iy2++;

  const int step = poptions_.gridLines;
  if (step <= 0 || (poptions_.gridLinesMax > 0 && ((gp.ix2 - gp.ix1) / step > poptions_.gridLinesMax || (gp.iy2 - gp.iy1) / step > poptions_.gridLinesMax))) {
    return false;
  }

  Colour bc = poptions_.bordercolour;
  bc.setF(Colour::alpha, 0.5);
  gl->setLineStyle(bc, 1);

  gp.ix1 = int(gp.ix1 / step) * step;
  gp.iy1 = int(gp.iy1 / step) * step;

  const int nx = fields_[0]->area.nx;

  diutil::PolylinePainter pp(gl);
  for (int ix = gp.ix1; ix < gp.ix2; ix += step) {
    for (int iy = gp.iy1; iy < gp.iy2; iy++)
      pp.add(x, y, diutil::index(nx, ix, iy));
    pp.draw();
  }

  for (int iy = gp.iy1; iy < gp.iy2; iy += step) {
    for (int ix = gp.ix1; ix < gp.ix2; ix++)
      pp.add(x, y, diutil::index(nx, ix, iy));
    pp.draw();
  }

  return true;
}

// show areas with undefined field values
void FieldRenderer::plotUndefined(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (poptions_.undefMasking <= 0)
    return;

  if (not checkFields(1)) // FIXME wrong, if NONE_DEFINED, field[0]->data may be nullptr
    return;

  if (fields_[0]->allDefined())
    return;

  if (plottype() == fpt_contour || plottype() == fpt_contour2) {
    plotContour2(gl, PO_SHADE_BACKGROUND);
    return;
  }

  if (is_raster_ && !fields_[0]->allDefined()) {
    RasterUndef raster(pa_, fields_, poptions_);
    QImage target = raster.rasterPaint();
    if (!target.isNull())
      gl->drawScreenImage(QPointF(0, 0), target);
    return;
  }

  const int nx_fld = fields_[0]->area.nx, ny_fld = fields_[0]->area.ny;
  const int nx_pts = nx_fld + 1;

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp, 1, true))
    return;
  if (gp.ix1 >= nx_fld || gp.ix2 < 0 || gp.iy1 >= ny_fld || gp.iy2 < 0)
    return;
  const float *x = gp.x(), *y = gp.y();

  const diutil::is_undef undef_f0(fields_[0]->data, nx_fld);

  const Colour undefC = poptions_.undefColour;

  if (poptions_.undefMasking == 1) {
    // filled undefined areas
    gl->setColour(undefC, false);
    gl->ShadeModel(DiGLPainter::gl_FLAT);
    gl->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK, DiGLPainter::gl_FILL);

    for (int iy = gp.iy1; iy <= gp.iy2; iy++) {
      int ix = gp.ix1;
      while (ix <= gp.ix2) {
        // find start of undef
        while (ix <= gp.ix2 && !undef_f0(ix, iy))
          ix += 1;
        if (ix > gp.ix2)
          break;

        int ixx = ix;
        // find end of undef
        while (ix <= gp.ix2 && undef_f0(ix, iy))
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
    gl->setLineStyle(undefC, poptions_.undefLinewidth, poptions_.undefLinetype, false);

    // line is where at least one of two neighbouring cells is undefined
    diutil::PolylinePainter pp(gl);

    // horizontal lines
    for (int iy = gp.iy1; iy <= gp.iy2; iy++) {
      int ix = gp.ix1;
      while (ix <= gp.ix2) {
        while (ix <= gp.ix2 && (!undef_f0(ix, iy) && (iy == gp.iy1 || !undef_f0(ix, iy - 1))))
          ix += 1;
        if (ix >= gp.ix2)
          break;

        int idx = diutil::index(nx_pts, ix, iy);
        pp.add(x, y, idx);
        do {
          ix += 1;
          idx += 1;
          pp.add(x, y, idx);
        } while (ix < gp.ix2 && !(!undef_f0(ix, iy) && (iy == gp.iy1 || !undef_f0(ix, iy - 1))));
        pp.draw();
      }
    }

    // vertical lines
    for (int ix = gp.ix1; ix <= gp.ix2; ix++) {
      int iy = gp.iy1;
      while (iy <= gp.iy2) {
        while (iy <= gp.iy2 && (!undef_f0(ix, iy) && (ix == gp.ix1 || !undef_f0(ix - 1, iy))))
          iy += 1;
        if (iy >= gp.iy2)
          break;

        int idx = diutil::index(nx_pts, ix, iy);
        pp.add(x, y, idx);
        do {
          iy += 1;
          idx += nx_pts;
          pp.add(x, y, idx);
        } while (iy < gp.iy2 && !(!undef_f0(ix, iy) && (ix == gp.ix1 || !undef_f0(ix - 1, iy))));
        pp.draw();
      }
    }
  }

  gl->Disable(DiGLPainter::gl_LINE_STIPPLE);
}

/*
 plot field values as numbers in each gridpoint
 skip plotting if too many (or too small) numbers
 */
bool FieldRenderer::plotNumbers(DiGLPainter* gl)
{
  METLIBS_LOG_SCOPE();

  if (fields_.size() != 1)
    return false;
  if (not checkFields(1))
    return false;

  int i, ix, iy;

  // convert gridpoints to correct projection
  GridPoints gp;
  if (!getGridPoints(gp))
    return false;
  const float *x = gp.x(), *y = gp.y();

  int autostep = poptions_.density;
  float dist;
  setAutoStep(x, y, gp.ix1, gp.ix2, gp.iy1, gp.iy2, 25, autostep, dist);
  if (autostep > 1)
    return false;

  gp.ix2++;
  gp.iy2++;

  float fontsize = 16.;
  gl->setFont(diutil::BITMAPFONT, diutil::F_BOLD, fontsize);

  float chx, chy;
  gl->getCharSize('0', chx, chy);
  // the real height for numbers 0-9 (width is ok)
  chy *= 0.75;

  float* field = fields_[0]->data;
  float gx, gy, w, h;
  float hh = chy * 0.5;
  float ww = chx * 0.5;
  int iprec = 0;
  if (poptions_.precision > 0) {
    iprec = poptions_.precision;
  }
  QString str;

  gl->setLineStyle(poptions_.linecolour, 1, false);

  for (iy = gp.iy1; iy < gp.iy2; iy++) {
    for (ix = gp.ix1; ix < gp.ix2; ix++) {
      i = iy * fields_[0]->area.nx + ix;
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

      if (pa_.getMapSize().isinside(gx - w, gy - hh) && pa_.getMapSize().isinside(gx + w, gy + hh))
        gl->drawText(str, gx - w, gy - hh, 0.0);
    }
  }

  // draw lines/boxes at borders between gridpoints..............................

  // convert gridpoints to correct projection
  if (!getGridPoints(gp, 1, true))
    return false;
  x = gp.x();
  y = gp.y();

  const int nx = fields_[0]->area.nx + 1;

  gp.ix2++;
  gp.iy2++;

  diutil::PolylinePainter pp(gl);
  for (int ix = gp.ix1; ix < gp.ix2; ix++) {
    for (int iy = gp.iy1; iy < gp.iy2; iy++)
      pp.add(x, y, diutil::index(nx, ix, iy));
    pp.draw();
  }
  for (int iy = gp.iy1; iy < gp.iy2; iy++) {
    for (int ix = gp.ix1; ix < gp.ix2; ix++)
      pp.add(x, y, diutil::index(nx, ix, iy));
    pp.draw();
  }

  return true;
}

bool FieldRenderer::checkFields(size_t count) const
{
  if (count == 0) {
    if (fields_.empty())
      return false;
    count = fields_.size();
  } else if (fields_.size() < count) {
    return false;
  }
  for (size_t i = 0; i < count; ++i) {
    if (not(fields_[i] and fields_[i]->data))
      return false;
  }
  if (count > 1) {
    const int gridsize0 = fields_[0]->area.gridSize();
    for (size_t i = 1; i < count; ++i) { // start from 1
      if (fields_[i]->area.gridSize() != gridsize0)
        return false;
    }
  }
  return true;
}
