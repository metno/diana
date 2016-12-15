/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013 met.no

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

#include "diPolyContouring.h"

#include "diGLPainter.h"
#include "diImageGallery.h"
#include "diUtilities.h"
#include "polyStipMasks.h"
#include "util/plotoptions_util.h"

#include <QPolygonF>
#include <QString>

#include <boost/make_shared.hpp>

#include <cmath>

#define MILOGGER_CATEGORY "diana.PolyContouring"
#include <miLogger/miLogging.h>

const float UNDEF_VALUE = 1e30;

inline bool isUndefined(float v)
{
  return isnan(v) or v >= UNDEF_VALUE or v < -UNDEF_VALUE;
}

static inline int rounded_div(float value, float unit)
{
    const int i = int(value / unit);
    if (value >= 0)
        return i+1;
    else
        return i;
}

inline bool skip_level_below0(const PlotOptions& po)
{
  return po.zeroLine==0 || !po.linevalues.empty() || !po.loglinevalues.empty();
}

inline bool skip_level_above0(const PlotOptions& po)
{
  return po.zeroLine==0 && po.linevalues.empty() && po.loglinevalues.empty();
}

class DianaArrayIndex {
public:
  DianaArrayIndex(size_t nx, size_t ny, size_t x0, size_t y0, size_t x1, size_t y1, size_t step=1);

  size_t size_x() const
    { return mSX; }

  size_t size_y() const
    { return mSY; }

  size_t index(size_t ix, size_t iy) const
    {  return (mY0 + iy*mStep)*mNX + (mX0+ix*mStep); }

  size_t operator()(size_t ix, size_t iy) const
    { return index(ix, iy); }

private:
  size_t mNX, mNY, mStep, mX0, mY0, mSX, mSY;
};

DianaArrayIndex::DianaArrayIndex(size_t nx, size_t ny, size_t x0, size_t y0, size_t x1, size_t y1, size_t step)
  : mNX(nx)
  , mNY(ny)
  , mStep(std::max((size_t)1, step))
  , mX0(mStep * (x0/mStep))
  , mY0(mStep * (y0/mStep))
  , mSX((std::min(x1+mStep-1, mNX)-mX0)/mStep)
  , mSY((std::min(y1+mStep-1, mNY)-mY0)/mStep)
{
}

// ########################################################################

class DianaPositionsSimple : public DianaPositions {
public:
  virtual contouring::point_t position(size_t ix, size_t iy) const
    { return contouring::point_t(ix, iy); }
};

class DianaPositionsList : public DianaPositions {
public:
  DianaPositionsList(const DianaArrayIndex& index, const float* xpos, const float* ypos)
    : mIndex(index), mXpos(xpos), mYpos(ypos) { }
  
  contouring::point_t position(size_t ix, size_t iy) const
    { const size_t i = mIndex(ix, iy); return contouring::point_t(mXpos[i], mYpos[i]); }

private:
  const DianaArrayIndex& mIndex;
  const float *mXpos, *mYpos;
};

class DianaPositionsFormula : public DianaPositions {
public:
  DianaPositionsFormula(const float* coeff)
    : cxy(coeff) { }
  
  contouring::point_t position(size_t ix, size_t iy) const
    { const float x = cxy[0]+cxy[1]*ix+cxy[2]*iy;
      const float y = cxy[3]+cxy[4]*ix+cxy[5]*iy;
      return contouring::point_t(x, y); }

private:
  const float *cxy;
};

// ########################################################################

DianaLevelList::DianaLevelList(const std::vector<float>& levels)
  : mLevels(levels)
{
}

DianaLevelList::DianaLevelList()
{
}

DianaLevelList::DianaLevelList(float lstart, float lstop, float lstep)
{
  for (float l=lstart; l<=lstop; l+=lstep)
    mLevels.push_back(l);
}

float DianaLevelList::value_for_level(contouring::level_t l) const
{
  return l != UNDEF_LEVEL ? mLevels[l] : UNDEF_VALUE;
}

contouring::level_t DianaLevelList::level_for_value(float value) const
{
  if (isUndefined(value))
    return UNDEF_LEVEL;
  return std::lower_bound(mLevels.begin(), mLevels.end(), value) - mLevels.begin();
}

// ------------------------------------------------------------------------

DianaLevelList10::DianaLevelList10(const std::vector<float>& levels, size_t count)
{
  for (size_t i = 0; i<std::min(count, levels.size()); ++i) {
    float l = levels[i];
    if (l <= 0)
      continue;
    if (not mLevels.empty() and (l >= BASE*mLevels.front() or l <= mLevels.back()))
      break;
    mLevels.push_back(l);
  }
  const size_t nLevels = mLevels.size(); // remember number of specified levels
  if (nLevels > 0) {
    for (size_t f=BASE; mLevels.size()<count; f *= BASE) {
      for (size_t i=0; i<nLevels && mLevels.size()<count; ++i)
        mLevels.push_back(f * mLevels[i]);
    }
  }
}

//------------------------------------------------------------------------

DianaLevelLog::DianaLevelLog(const std::vector<float>& levels)
{
  for (size_t i=0; i<levels.size(); ++i) {
     float l = levels[i];
    if (l > BASE or (l>0 and l<1)) {
      const float logBl = log(l) / log(BASE), flogBl = logBl - floor(logBl);
      l = pow(BASE, flogBl);
    }
    if (l >= 1 and l < BASE and (mLevels.empty() or l > mLevels.back()))
      mLevels.push_back(l);
    else
      METLIBS_LOG_WARN("illegal log.line.values element @" << i << '=' << levels[i]);
  }
}

contouring::level_t DianaLevelLog::level_for_value(float value) const
{
  if (isUndefined(value) or value < 0 or mLevels.empty())
    return UNDEF_LEVEL;
  if (value == 0)
    return UNDEF_LEVEL;
  const double logBv = log(value)/log(BASE), ilogBv = floor(logBv);
  const double v_0_B = std::pow(BASE, logBv - ilogBv);
  const contouring::level_t l = DianaLevelList::level_for_value(v_0_B);
  return ilogBv*nlevels() + l;
}

float DianaLevelLog::value_for_level(contouring::level_t l) const
{
  if (l == UNDEF_LEVEL)
    return UNDEF_VALUE;
  const int ilogBv = floor(l / double(nlevels())), lvl = abs(l) % nlevels();
  return std::pow(double(BASE), ilogBv) * mLevels[lvl];
}

//------------------------------------------------------------------------

DianaLevelStep::DianaLevelStep(float step, float off)
  : mStep(step)
  , mOff(off)
  , mMin(1)
  , mMax(0)
  , mHaveMin(false)
  , mHaveMax(false)
{
}

void DianaLevelStep::set_limits(float mini, float maxi)
{
  mMin = mini;
  mMax = maxi;
  mHaveMin = not isUndefined(mMin);
  mHaveMax = not isUndefined(mMax);
  if (mHaveMin and mHaveMax and mMin > mMax)
    mHaveMin = mHaveMax = false;
}

contouring::level_t DianaLevelStep::level_for_value(float value) const
{
  if (isUndefined(value))
    return UNDEF_LEVEL;
  // invalid combinations of mMin and mMax are caught in set_limits
  if (mHaveMin and value < mMin)
    value = mMin - 0.5*mStep;
  if (mHaveMax and value > mMax)
    value = mMax + 0.5*mStep;
  return rounded_div(value - mOff, mStep);
}

float DianaLevelStep::value_for_level(contouring::level_t l) const
{
  return l != UNDEF_LEVEL ? l*mStep + mOff : UNDEF_VALUE;
}

// ########################################################################

contouring::point_t DianaFieldBase::line_point(contouring::level_t level, size_t x0, size_t y0, size_t x1, size_t y1) const
{
    const float v0 = value(x0, y0);
    const float v1 = value(x1, y1);
    const float vl = mLevels.value_for_level(level);
    const float c = (vl-v0)/(v1-v0);

    const contouring::point_t p0 = position(x0, y0);
    const contouring::point_t p1 = position(x1, y1);
    const float x = (1-c)*p0.x + c*p1.x;
    const float y = (1-c)*p0.y + c*p1.y;
    return contouring::point_t(x, y);
}

// ########################################################################

class DianaField : public DianaFieldBase {
public:
  DianaField(const DianaArrayIndex& index, const float* data, const DianaLevels& levels, const DianaPositions& positions)
    : DianaFieldBase(levels, positions)
    , mIndex(index), mData(data)
    { }
  
  virtual size_t nx() const
    { return mIndex.size_x(); }
  
  virtual size_t ny() const
    { return mIndex.size_y(); }

protected:
  virtual float value(size_t ix, size_t iy) const
    { return mData[mIndex(ix, iy)]; }

private:
  const DianaArrayIndex& mIndex;
  const float *mData;
};

// ########################################################################

DianaLines::DianaLines(const PlotOptions& poptions, const DianaLevels& levels)
  : mPlotOptions(poptions)
  , mLevels(levels)
  , mPaintMode(UNDEFINED | FILL | LINES_LABELS)
  , mUseOptions2(false)
  , mClassValues(diutil::parseClassValues(mPlotOptions))
{
}

DianaLines::~DianaLines()
{
}

void DianaLines::paint()
{
  if ((mPaintMode & (UNDEFINED | FILL)) != 0)
    paint_polygons();
  if ((mPaintMode & (UNDEFINED | LINES_LABELS)) != 0) {
    paint_lines();
    if (mPlotOptions.valueLabel)
      paint_labels();
  }
}

void DianaLines::setLineForLevel(contouring::level_t li)
{
  if (li == DianaLevels::UNDEF_LEVEL) {
    setLine(mPlotOptions.undefColour, mPlotOptions.undefLinetype, mPlotOptions.undefLinewidth);
  } else if (mUseOptions2) {
    setLine(mPlotOptions.linecolour_2, mPlotOptions.linetype_2, mPlotOptions.linewidth_2);
  } else {
    const Colour* lc;
    // "three colours" option
    if (mPlotOptions.colours.size() == 3) {
      const int cidx = (li < 0) ? 0 : ((li > 0) ? 2 : 1);
      lc = &mPlotOptions.colours[cidx];
    } else {
      lc = &mPlotOptions.linecolour;
    }
    setLine(*lc, mPlotOptions.linetype, mPlotOptions.linewidth);
  }
}

void DianaLines::paint_polygons()
{
  METLIBS_LOG_TIME(LOGVAL(mPlotOptions.undefMasking));
  const PlotOptions& po = mPlotOptions;

  const int nclasses = mClassValues.size();
  const int ncolours = po.palettecolours.size();
  const int ncolours_cold = po.palettecolours_cold.size();
  const int npatterns = po.patterns.size();
  const bool no_fill = (mPaintMode & FILL) == 0;
  const bool skip_level_0 = skip_level_below0(po);
  const bool skip_level_1 = skip_level_above0(po);
  const bool skip_undef = (mPaintMode & UNDEFINED) == 0 || po.undefMasking != 1;
  const contouring::level_t level_min = mLevels.level_for_value(po.minvalue),
      level_max = mLevels.level_for_value(po.maxvalue);
  const bool skip_min = level_min != DianaLevels::UNDEF_LEVEL;
  const bool skip_max = level_max != DianaLevels::UNDEF_LEVEL;
  const std::string NOPATTERN;

  for (level_points_m::const_iterator it = m_polygons.begin(); it != m_polygons.end(); ++it) {
    const contouring::level_t li = it->first;
    METLIBS_LOG_TIME(LOGVAL(li));

    if (li == DianaLevels::UNDEF_LEVEL) {
      if (skip_undef)
        continue;
      setFillColour(mPlotOptions.undefColour);
    } else {
      if (no_fill || (skip_min && li < level_min) || (skip_max && li >= level_max)
          || (skip_level_0 && li == 0) || (skip_level_1 && li == 1))
        continue;
      if (mPlotOptions.discontinuous == 1) {
        int cidx;
        if (nclasses) {
          cidx = 0;
          while (cidx < nclasses && mClassValues[cidx] != li)
            cidx++;
          if (cidx == nclasses)
            continue;
        } else {
          cidx = diutil::find_index(true/*always repeat*/, ncolours, li - 1);
        }
        if (cidx < ncolours)
          setFillColour(mPlotOptions.palettecolours[cidx]);
        else
          setFillColour(mPlotOptions.fillcolour);
        if (cidx < npatterns)
          setFillPattern(mPlotOptions.patterns[cidx]);
        else
          setFillPattern(NOPATTERN);
      } else {
        if (li <= 0 and ncolours_cold) {
          const int idx = diutil::find_index(mPlotOptions.repeat, ncolours_cold, -li);
          setFillColour(mPlotOptions.palettecolours_cold[idx]);
        } else if (ncolours) {
          if (li <= 0 and not mPlotOptions.loglinevalues.empty())
            continue;
          const int idx = diutil::find_index(mPlotOptions.repeat, ncolours, li - 1);
          setFillColour(mPlotOptions.palettecolours[idx]);
        } else {
          setFillColour(mPlotOptions.fillcolour);
        }
        if (li >= 0 and npatterns) {
          const int pidx = diutil::find_index(mPlotOptions.repeat, npatterns, li - 1);
          setFillPattern(mPlotOptions.patterns[pidx]);
        } else {
          setFillPattern(NOPATTERN);
        }
      }
    }

    drawPolygons(it->second);
  }
}

void DianaLines::paint_lines()
{
  const PlotOptions& po = mPlotOptions;
  const bool no_lines = (mPaintMode & LINES_LABELS) == 0;
  const bool skip_undef = (mPaintMode & UNDEFINED) == 0 || !po.undefMasking;
  const bool skip_level_0 = skip_level_above0(po);
  for (level_points_m::const_iterator it = m_lines.begin(); it != m_lines.end(); ++it) {
    const contouring::level_t li = it->first;
    if (li == DianaLevels::UNDEF_LEVEL) {
      if (skip_undef)
        continue;
    } else {
      if (no_lines || (skip_level_0 && li == 0))
        continue;
    }
    setLineForLevel(li);
    for (point_vv::const_iterator itP = it->second.begin(); itP != it->second.end(); ++itP)
      drawLine(*itP);
  }
}

void DianaLines::paint_labels()
{
  const bool skip_level_0 = skip_level_above0(mPlotOptions);
  for (level_points_m::const_iterator it = m_lines.begin(); it != m_lines.end(); ++it) {
    const contouring::level_t li = it->first;
    if (li == DianaLevels::UNDEF_LEVEL || (skip_level_0 && li == 0))
      continue;
    setLineForLevel(li);
    for (point_vv::const_iterator itP = it->second.begin(); itP != it->second.end(); ++itP)
      drawLabels(*itP, li);
  }
}

void DianaLines::add_contour_line(contouring::level_t li, const contouring::points_t& cpoints, bool closed)
{
  if (li == DianaLevels::UNDEF_LEVEL && (mPlotOptions.undefMasking != 2 || (mPaintMode & UNDEFINED) == 0))
    return;
  if (li != DianaLevels::UNDEF_LEVEL && (!mPlotOptions.options_1 || (mPaintMode & LINES_LABELS) == 0))
    return;

  point_v points(cpoints.begin(), cpoints.end());
  if (closed)
    points.push_back(cpoints.front());
  m_lines[li].push_back(points);
}

void DianaLines::add_contour_polygon(contouring::level_t level, const contouring::points_t& cpoints)
{
  const PlotOptions& po = mPlotOptions;
  if (level == DianaLevels::UNDEF_LEVEL) {
    if (po.undefMasking != 1 || (mPaintMode & UNDEFINED) == 0)
      return;
  } else {
    if ((mPaintMode & FILL) == 0 || (po.alpha == 0))
      return;
    if (po.palettecolours.empty() && po.palettecolours_cold.empty() && mClassValues.empty() && po.patterns.empty())
      return;
  }

  point_v points(cpoints.begin(), cpoints.end());
  m_polygons[level].push_back(points);
}

// ########################################################################

class DianaGLLines : public DianaLines
{
public:
  DianaGLLines(DiGLPainter* gl, const PlotOptions& poptions,
      const DianaLevels& levels)
    : DianaLines(poptions, levels), mGL(gl) { }

protected:
  void paint_polygons();
  void paint_lines();

  void setLine(const Colour& colour, const Linetype& linetype, int linewidth);
  void setFillColour(const Colour& colour);
  void setFillPattern(const std::string& pattern);
  void drawLine(const point_v& lines);
  void drawPolygons(const point_vv& polygons);
  void drawLabels(const point_v& points, contouring::level_t li);

private:
  DiGLPainter* mGL;
};

void DianaGLLines::paint_polygons()
{
  METLIBS_LOG_TIME(LOGVAL(mPlotOptions.undefMasking));
  mGL->ShadeModel(DiGLPainter::gl_FLAT);
  mGL->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK,DiGLPainter::gl_FILL);
  mGL->Enable(DiGLPainter::gl_BLEND);
  mGL->BlendFunc(DiGLPainter::gl_SRC_ALPHA, DiGLPainter::gl_ONE_MINUS_SRC_ALPHA);

  DianaLines::paint_polygons();

  mGL->PolygonMode(DiGLPainter::gl_FRONT_AND_BACK,DiGLPainter::gl_LINE);
  mGL->Disable(DiGLPainter::gl_BLEND);
  mGL->ShadeModel(DiGLPainter::gl_FLAT);
  mGL->EdgeFlag(DiGLPainter::gl_TRUE);
  mGL->Disable(DiGLPainter::gl_POLYGON_STIPPLE);
}

void DianaGLLines::paint_lines()
{
  mGL->ShadeModel(DiGLPainter::gl_FLAT);
  DianaLines::paint_lines();
}

void DianaGLLines::setFillColour(const Colour& colour)
{
  if (mPlotOptions.alpha < 255)
    mGL->Color4ub(colour.R(), colour.G(), colour.B(), mPlotOptions.alpha);
  else
    mGL->setColour(colour);
}

void DianaGLLines::setFillPattern(const std::string& pattern)
{
  if (!pattern.empty()) {
    ImageGallery ig;
    const GLubyte* p = ig.getPattern(pattern);
    if (p) {
      mGL->Enable(DiGLPainter::gl_POLYGON_STIPPLE);
      mGL->PolygonStipple(p);
      return;
    }
  }
  // no fill pattern, or p == 0
  mGL->Disable(DiGLPainter::gl_POLYGON_STIPPLE);
  // mGL->PolygonStipple(solid); // "solid" is from polyStipMasks.h
}

void DianaGLLines::setLine(const Colour& colour, const Linetype& linetype, int linewidth)
{
  mGL->setLineStyle(colour, linewidth, linetype);
}

void DianaGLLines::drawLine(const point_v& points)
{
  QPolygonF line;
  for (point_v::const_iterator it = points.begin(); it != points.end(); ++it)
    line << QPointF(it->x, it->y);
  mGL->drawPolyline(line);
}

void DianaGLLines::drawPolygons(const point_vv& polygons)
{
  // METLIBS_LOG_TIME();
  for (point_vv::const_iterator itL = polygons.begin(); itL != polygons.end(); ++itL) {
    QPolygonF polygon;
    for (point_v::const_iterator p = itL->begin(); p != itL->end(); ++p)
      polygon << QPointF(p->x, p->y);
    mGL->drawPolygon(polygon);
  }
}

void DianaGLLines::drawLabels(const point_v& points, contouring::level_t li)
{
  if (points.size() < 10)
    return;

  const QString lbl = QString::number(mLevels.value_for_level(li));

  float lbl_w = 0, lbl_h = 0;
  if (not mGL->getTextSize(lbl, lbl_w, lbl_h))
    return;
  if (lbl_h <= 0 or lbl_w <= 0)
    return;
  const float lbl_w2 = lbl_w * lbl_w;

  size_t idx = int(0.1*(1 + (std::abs(li+100000) % 5))) * points.size();
  for (; idx + 1 < points.size(); idx += 5) {
    contouring::point_t p0 = points.at(idx), p1;
    const int idx0 = idx;
    for (idx += 1; idx < points.size(); ++idx) {
      p1 = points.at(idx);
      const float dy = p1.y - p0.y, dx = p1.x - p0.x;
      if (dx*dx + dy*dy >= lbl_w2)
        break;
    }
    if (idx >= points.size())
      break;

    if (p1.x < p0.x)
      std::swap(p0, p1);
    const float angle_deg = atan2f(p1.y - p0.y, p1.x - p0.x) * 180. / M_PI;

    // check that line is somewhat straight under label
    size_t idx2 = idx0 + 1;
    for (; idx2 < idx; ++idx2) {
      const contouring::point_t p2 = points.at(idx2);
      const float a = atan2f(p2.y - p0.y, p2.x - p0.x) * 180. / M_PI;
      if (std::abs(a - angle_deg) > 15)
        break;
    }
    if (idx2 < idx)
      continue;

    // label angle seems ok, find position
#if 0
    // centered on line
    const float f = lbl_h/(2*lbl_w), tx = p0.x + (p1.y-p0.y)*f, ty = p0.y - (p1.x-p0.x)*f;
#else
    // sitting on top of line
    const float tx = p0.x, ty = p0.y;
#endif
    mGL->drawText(lbl, tx, ty, angle_deg);
    idx += 10*(idx - idx0);
  }
}

// ########################################################################

boost::shared_ptr<DianaLevels> dianaLevelsForPlotOptions(const PlotOptions& poptions, float fieldUndef)
{
  if (not poptions.linevalues.empty()) {
    return boost::make_shared<DianaLevelList>(poptions.linevalues);
  } else if (not poptions.loglinevalues.empty()) {
    // selected line values (the first values in rlines)
    // are drawn, the following vales drawn are the
    // previous multiplied by 10 and so on
    // (nlines=2 rlines=0.1,0.3 => 0.1,0.3,1,3,10,30,...)
    // (or the line at value=zoff)
    return boost::make_shared<DianaLevelList10>(poptions.loglinevalues, poptions.palettecolours.size());
  } else {
    // equally spaced lines (value)
    boost::shared_ptr<DianaLevelStep> ls
        = boost::make_shared<DianaLevelStep>(poptions.lineinterval, poptions.base);
    if (poptions.minvalue > -fieldUndef or poptions.maxvalue < fieldUndef)
      ls->set_limits(poptions.minvalue, poptions.maxvalue);
    return ls;
  }
}

//! same as dianaLevelsForPlotOptions except that it uses options with "_2" at the end of the name
boost::shared_ptr<DianaLevels> dianaLevelsForPlotOptions_2(const PlotOptions& poptions, float fieldUndef)
{
  if (not poptions.linevalues_2.empty()) {
    return boost::make_shared<DianaLevelList>(poptions.linevalues_2);
  } else if (not poptions.loglinevalues_2.empty()) {
    return boost::make_shared<DianaLevelList10>(poptions.loglinevalues_2, poptions.loglinevalues_2.size());
  } else {
    boost::shared_ptr<DianaLevelStep> ls
        = boost::make_shared<DianaLevelStep>(poptions.lineinterval_2, poptions.base_2);
    if (poptions.minvalue_2 > -fieldUndef or poptions.maxvalue_2 < fieldUndef)
      ls->set_limits(poptions.minvalue_2, poptions.maxvalue_2);
    return ls;
  }
}

// ########################################################################

// same parameters as diContouring::contour, most of them ignored
bool poly_contour(int nx, int ny, int ix0, int iy0, int ix1, int iy1,
    const float z[], const float xz[], const float yz[],
    DiGLPainter* gl, const PlotOptions& poptions, float fieldUndef,
    int paintMode, bool use_options_2)
{
  DianaLevels_p levels;
  if (use_options_2) {
    // no second ("_2") option set for fill colours and undefined areas
    paintMode &= ~(DianaLines::UNDEFINED|DianaLines::FILL);
    levels = dianaLevelsForPlotOptions_2(poptions, fieldUndef);
  } else {
    levels = dianaLevelsForPlotOptions  (poptions, fieldUndef);
  }

  const int blockPaintMode = (paintMode & DianaLines::FILL);
  if (!gl->isPrinting() && blockPaintMode) {
    const int BLOCK = 32*std::max(1, poptions.lineSmooth);
    METLIBS_LOG_TIME("contour fill blocks " << BLOCK);
    for (int ixx0 = ix0; ixx0 < ix1; ixx0 += BLOCK) {
      int ixx1 = std::min(ix1, ixx0 + BLOCK+1);
      for (int iyy0 = iy0; iyy0 < iy1; iyy0 += BLOCK) {
        int iyy1 = std::min(iy1, iyy0 + BLOCK+1);

        const DianaArrayIndex index(nx, ny, ixx0, iyy0, ixx1, iyy1, poptions.lineSmooth);
        DianaPositions_p positions = boost::make_shared<DianaPositionsList>(index, xz, yz);

        const DianaField df(index, z, *levels, *positions);
        DianaGLLines dl(gl, poptions, *levels);
        dl.setPaintMode(blockPaintMode);
        dl.setUseOptions2(use_options_2);

        try {
          contouring::run(df, dl);
          dl.paint();
        } catch (contouring::too_many_levels& tml) {
          METLIBS_LOG_WARN(tml.what());
        }
      }
    }
    paintMode &= ~DianaLines::FILL;
  }

  const DianaArrayIndex index(nx, ny, ix0, iy0, ix1, iy1, poptions.lineSmooth);
  DianaPositions_p positions = boost::make_shared<DianaPositionsList>(index, xz, yz);

  const DianaField df(index, z, *levels, *positions);
  DianaGLLines dl(gl, poptions, *levels);
  dl.setPaintMode(paintMode);
  dl.setUseOptions2(use_options_2);

  { METLIBS_LOG_TIME("contouring");
    try {
      contouring::run(df, dl);
    } catch (contouring::too_many_levels& tml) {
      METLIBS_LOG_WARN(tml.what());
    }
  }

  { METLIBS_LOG_TIME("painting");
    dl.paint();
  }

  return true;
}
