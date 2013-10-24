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

/*
 NOTES: Heavily based on old fortran code (1987-2001, A.Foss)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diVcrossPlot.h"

#include "diVcrossAxis.h"
#include "diVcrossContour.h"
#include "diVcrossHardcopy.h"
#include "diVcrossOptions.h"
#include "diVcrossPaint.h"
#include "diVcrossUtil.h"
#include "diFontManager.h"

#include <qglobal.h>

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glText/glText.h>
#endif

#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>

#include <cmath>
#include <iterator>
#include <iomanip>

#define MILOGGER_CATEGORY "diana.VcrossPlot"
#include <miLogger/miLogging.h>

#if 0
void calc_ps_from_pp()
{
  if (vcoord == 12 && nps < 0) {
    nps = addPar1d(8);
    for (int i = 0; i < nPoint; i++)
      cdata1d[nps][i] = 1.5 * cdata2d[npp][i] - 0.5 * cdata2d[npp][nPoint + i];
  }
}
#endif

namespace /* anonymous */ {

void writeLonEWLatNS(std::ostream& out, float lonlat, char EN, char WS)
{
  out << std::setw(5) << std::setprecision(1) << std::setiosflags(std::ios::fixed)
      << std::abs(lonlat);
  if (lonlat >= 0)
    out << EN;
  else
    out << WS;
}

inline void writeLonEW(std::ostream& out, float lon)
{
  writeLonEWLatNS(out, lon, 'E', 'W');
}

inline void writeLatNS(std::ostream& out, float lat)
{
  writeLonEWLatNS(out, lat, 'N', 'S');
}

static const float LINE_GAP = 0.2;
static const float LINES_1 = 1 + LINE_GAP;
static const float LINES_2 = 2 + LINE_GAP;
static const float LINES_3 = 3 + LINE_GAP;
static const float CHARS_NUMBERS = 10;
static const float CHARS_DISTANCE = 5.5;
static const float CHARS_POS_LEFT  = 5.5;
static const float CHARS_POS_RIGHT = 1.9;
static const float CHARS_TIME = 2.5;

static const float MAX_FRAMETEXT = 0.4;

} // namespace anonymous

// ########################################################################

VcrossPlot::VcrossPlot(VcrossOptions* vcoptions)
  : fp(new FontManager()) // fontpack
  , mOptions(vcoptions)
  , mTotalSize(100, 100)
  , mFontSize(0)
  , mViewChanged(true)
  , mAxisX(new VcrossPlotDetail::Axis(true))
  , mAxisY(new VcrossPlotDetail::Axis(false))
{
  METLIBS_LOG_SCOPE();

  fp->parseSetup();
  setFont();
  fp->setFontFace(glText::F_NORMAL);
  fp->setScalingType(glText::S_FIXEDSIZE);

  hardcopy.reset(new VcrossHardcopy(fp.get()));
}

bool VcrossPlot::startPSoutput(const printOptions& po)
{
  return hardcopy->start(po);
}

void VcrossPlot::nextPSpage()
{
  hardcopy->startPSnewpage();
}

bool VcrossPlot::endPSoutput()
{
  return hardcopy->end();
}

VcrossPlot::~VcrossPlot()
{
  METLIBS_LOG_SCOPE();
}

void VcrossPlot::plotText()
{
  METLIBS_LOG_SCOPE();
  if (not mOptions->pText)
    return;

  float widthModel = 0, widthField = 0;
  BOOST_FOREACH(const Plot& plot, mPlots) {
    VcrossUtil::updateMaxStringWidth(fp.get(), widthModel, plot.modelName);
    VcrossUtil::updateMaxStringWidth(fp.get(), widthField, plot.fieldName);
  }

  const float xModel = mCharSize.width(), xField = xModel + widthModel + mCharSize.width();
  float yPlot = mTotalSize.height() - LINE_GAP*mCharSize.height();
  const float yCSName = yPlot, yStep = mCharSize.height() * LINES_1;

  BOOST_FOREACH(const Plot& plot, mPlots) {
    glColor3ubv(colourOrContrast(plot.poptions.linecolour).RGB());
    fp->drawStr(plot.modelName.c_str(), xModel, yPlot, 0);
    fp->drawStr(plot.fieldName.c_str(), xField, yPlot, 0);
    yPlot -= yStep;
  }

  { // show cross section name
    glColor3ubv(colourOrContrast(mOptions->frameColour).RGB());
    float csn_w, csn_h;
    fp->getStringSize(mCrossectionName.c_str(), csn_w, csn_h);
    fp->drawStr(mCrossectionName.c_str(), mTotalSize.width() - csn_w - mCharSize.width(), yCSName, 0);
  }
}

void VcrossPlot::viewSetWindow(int w, int h)
{
  METLIBS_LOG_SCOPE();

  mTotalSize = QSize(w, h);
  hardcopy->resetPage();
  mViewChanged = true;
}

void VcrossPlot::getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour)
{
  METLIBS_LOG_SCOPE();

  x1 = mAxisX->getPaintMin();
  y1 = mAxisX->getPaintMax();
  x2 = mAxisY->getPaintMin();
  y2 = mAxisY->getPaintMax();
  rubberbandColour = mContrastColour;
}

void VcrossPlot::viewZoomIn(int px1, int py1, int px2, int py2)
{
  METLIBS_LOG_SCOPE();

  py1 = mTotalSize.height() - py1;
  py2 = mTotalSize.height() - py2;

  const bool zx = mAxisX->zoomIn(px1, px2);
  const bool zy = mAxisY->zoomIn(py1, py2);
  mViewChanged |= (zx or zy);
}

void VcrossPlot::viewZoomOut()
{
  METLIBS_LOG_SCOPE();

  const bool zx = mAxisX->zoomOut();
  const bool zy = mAxisY->zoomOut();
  mViewChanged |= (zx or zy);
}

void VcrossPlot::viewPan(int pxmove, int pymove)
{
  const bool px = mAxisX->pan(pxmove);
  const bool py = mAxisY->pan(pymove);
  mViewChanged |= (px or py);
}

void VcrossPlot::viewStandard()
{
  METLIBS_LOG_SCOPE();

  float xMin = mAxisX->getDataMin(), xMax = mAxisX->getDataMax();
  if (mOptions->stdHorizontalArea) {
    const float range = (xMax - xMin) * 0.01;
    xMin += range * mOptions->minHorizontalArea;
    xMax = xMin + range * mOptions->maxHorizontalArea;
  }
  mAxisX->setValueRange(xMin, xMax);

  float yMin = mAxisY->getDataMin(), yMax = mAxisY->getDataMax();
  if (mOptions->stdVerticalArea) {
    const float range = (yMax - yMin) * 0.01;
    yMin += range * mOptions->minVerticalArea;
    yMax = yMin + range * mOptions->maxVerticalArea;
  }
  mAxisY->setValueRange(yMin, yMax);

  mViewChanged = true;
}

void VcrossPlot::clear()
{
  mPlots.clear();

  mCrossectionName.clear();
  mCrossectionPoints.clear();
  mCrossectionDistances.clear();

  mTimePoints.clear();
  mTimeDistances.clear();
}

void VcrossPlot::setHorizontalCross(const std::string& csName, const VcrossData::Cut::lonlat_t& ll)
{
  METLIBS_LOG_SCOPE();

  mCrossectionName = csName;
  mCrossectionPoints = ll;

  mCrossectionDistances.clear();
  mCrossectionDistances.reserve(mCrossectionPoints.size());
  mCrossectionDistances.push_back(0);
  for (size_t i=1; i<mCrossectionPoints.size(); ++i) {
    const LonLat &p0 = mCrossectionPoints.at(i-1), &p1 = mCrossectionPoints.at(i);
    mCrossectionDistances.push_back(mCrossectionDistances.back() + p0.distanceTo(p1));
  }
}

void VcrossPlot::setHorizontalTime(const std::vector<miutil::miTime>& times, int csPoint)
{
  METLIBS_LOG_SCOPE();

  mTimePoints = times;
  mTimeCSPoint = csPoint;

  mTimeDistances.clear();
  mTimeDistances.reserve(mTimePoints.size());
  if (not mTimePoints.empty()) {
    const miutil::miTime &t0 = mTimePoints.front();
    mTimeDistances.push_back(0);
    for (size_t i=1; i<mTimePoints.size(); ++i) {
      const miutil::miTime& t1 = mTimePoints.at(i);
      mTimeDistances.push_back(miutil::miTime::minDiff(t0, t1));
    }
  }
}

void VcrossPlot::setVerticalAxis(VcrossData::ZAxis::Quantity q)
{
  METLIBS_LOG_SCOPE();
  if (q == VcrossData::ZAxis::PRESSURE) {
    mAxisY->quantity = VcrossPlotDetail::Axis::PRESSURE;
    mAxisY->type     = VcrossPlotDetail::Axis::EXNER;
  } else {
    mAxisY->quantity = VcrossPlotDetail::Axis::HEIGHT;
    mAxisY->type     = VcrossPlotDetail::Axis::LINEAR;
  }
  METLIBS_LOG_DEBUG(LOGVAL(mAxisY->quantity) << LOGVAL(mAxisY->type));
}

void VcrossPlot::addPlot(const std::string& mn, const std::string& fn,
    VCPlotType type, const VcrossData::values_t& p0, const VcrossData::values_t& p1, VcrossData::ZAxisPtr zax,
    const PlotOptions& poptions)
{
  METLIBS_LOG_SCOPE();
  mPlots.push_back(Plot(mn, fn, type, p0, p1, zax, poptions));
}

void VcrossPlot::prepare()
{
  METLIBS_LOG_SCOPE();

  prepareAxes();
  mViewChanged = true;
}

void VcrossPlot::prepareAxes()
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> vs = miutil::split(miutil::to_lower(mOptions->verticalType), 0, "/");
  if (vs.size() == 1) {
    vs.push_back("x");
  } else if (vs.empty()) {
    vs.push_back("standard");
    vs.push_back("x");
  }

  if (mAxisY->quantity == VcrossPlotDetail::Axis::PRESSURE) {
    mAxisY->label = "hPa";
    if (vs[1] == "fl")
      mAxisY->label = "FL";
  } else if (mAxisY->quantity == VcrossPlotDetail::Axis::HEIGHT) {
    mAxisY->label = "m";
    if (vs[1] == "fl" || vs[1] == "ft")
      mAxisY->label = "Ft";
  }

  const bool timeGraph = isTimeGraph();

  float yax_min = 1e35, yax_max = -1e35;
  BOOST_FOREACH(const Plot& plot, mPlots) {
    METLIBS_LOG_DEBUG("next plot" << LOGVAL(plot.zax->mPoints) << LOGVAL(plot.zax->mLevels));
    for (int l=0; l<plot.zax->mLevels; ++l) {
      if (not timeGraph) {
        for (int p=0; p<plot.zax->mPoints; ++p) {
          const float zax_value = plot.zax->value(l, p);
          if (l==0 and p==0) METLIBS_LOG_DEBUG(LOGVAL(l) << LOGVAL(p) << LOGVAL(zax_value));
          VcrossUtil::minimaximize(yax_min, yax_max, zax_value);
        }
      } else {
        VcrossUtil::minimaximize(yax_min, yax_max, plot.zax->value(l, mTimeCSPoint));
      }
    }
  }
  if (mAxisY->quantity == VcrossPlotDetail::Axis::PRESSURE)
    std::swap(yax_min, yax_max);
  METLIBS_LOG_DEBUG(LOGVAL(yax_min) << LOGVAL(yax_max));
  mAxisY->setDataRange(yax_min, yax_max);
  mAxisY->setValueRange(yax_min, yax_max);

  float xax_min = 0, xax_max;
  if (not timeGraph)
    xax_max = mCrossectionDistances.back();
  else
    xax_max = mTimeDistances.back();
  METLIBS_LOG_DEBUG(LOGVAL(xax_min) << LOGVAL(xax_max));
    
  mAxisX->setDataRange (xax_min, xax_max);
  viewStandard();
}

void VcrossPlot::prepareView()
{
  METLIBS_LOG_SCOPE();

  calculateContrastColour();
  computeMaxPlotArea();
  prepareAxesForAspectRatio();

  mViewChanged = false;
}

void VcrossPlot::prepareAxesForAspectRatio()
{
  METLIBS_LOG_SCOPE();
  float v2h = mOptions->verHorRatio;
  if (isTimeGraph() or v2h <= 0) {
    METLIBS_LOG_DEBUG("no aspect ratio");
    // horizontal axis has time unit, vertical axis pressure or height; aspect ratio is meaningless
    mAxisX->setPaintRange(mPlotAreaMax.left(),   mPlotAreaMax.right());
    mAxisY->setPaintRange(mPlotAreaMax.bottom(), mPlotAreaMax.top());
    return;
  }

  const float rangeX = std::abs(mAxisX->getValueMax() - mAxisX->getValueMin());
  float rangeY = v2h * std::abs(mAxisY->getValueMax() - mAxisY->getValueMin());
  if (mAxisY->quantity == VcrossPlotDetail::Axis::PRESSURE)
    rangeY *= 10; // approximately 10m/hPa

  // m/pixel on x and y axis
  const float pmx = rangeX / mPlotAreaMax.width(), pmy = rangeY / mPlotAreaMax.height();
  METLIBS_LOG_DEBUG(LOGVAL(pmx) << LOGVAL(pmy) << LOGVAL(rangeX) << LOGVAL(rangeY));

  if (pmy > pmx) {
    // too high for plot area, reduce width
    const float mid = (mPlotAreaMax.left() + mPlotAreaMax.right())/2, w2 = mPlotAreaMax.width() * pmx / (pmy*2);
    mAxisX->setPaintRange(mid - w2, mid + w2);
    mAxisY->setPaintRange(mPlotAreaMax.bottom(), mPlotAreaMax.top());
  } else {
    // reduce height
    const float mid = (mPlotAreaMax.bottom() + mPlotAreaMax.top())/2, h2 = mPlotAreaMax.height() * pmy / (pmx*2);
    mAxisX->setPaintRange(mPlotAreaMax.left(), mPlotAreaMax.right());
    mAxisY->setPaintRange(mid + h2, mid - h2);
  }
  METLIBS_LOG_DEBUG(LOGVAL(mAxisX->getPaintMin()) << LOGVAL(mAxisX->getPaintMax())
      << LOGVAL(mAxisY->getPaintMin()) << LOGVAL(mAxisY->getPaintMax()));
}

void VcrossPlot::computeMaxPlotArea()
{
  METLIBS_LOG_SCOPE();
  
  float charsXleft = 0, charsXrght = 0, linesYbot = 0, linesYtop = 0;

  if (mOptions->pLevelNumbers or mOptions->pFrame) {
    charsXleft = charsXrght = (mOptions->pLevelNumbers) ? CHARS_NUMBERS : 0;
    linesYbot  = linesYtop = LINES_1;
  }
  if (not isTimeGraph()) {
    if (mOptions->pDistance) {
      linesYbot += LINES_1;
      VcrossUtil::maximize(charsXleft, CHARS_DISTANCE);
      charsXrght = charsXleft;
    }
    if (mOptions->pGeoPos)
      linesYbot += LINES_2;
    if (mOptions->pGeoPos) {
      VcrossUtil::maximize(charsXleft, CHARS_POS_LEFT);
      VcrossUtil::maximize(charsXrght, CHARS_POS_RIGHT);
    }
  } else /* timeGraph */ {
    // time graph: hour/date/forecast (not if text is off)
    if (mOptions->pText) {
      linesYbot += LINES_3;
      VcrossUtil::maximize(charsXleft, CHARS_TIME);
      charsXrght = charsXleft;
    }
    // time graph: only one position (and only one line needed)
    if (mOptions->pDistance or mOptions->pGeoPos)
      linesYbot += LINES_1;
  }

  if (mOptions->pText)
    linesYbot += mPlots.size() + LINE_GAP;

  if (mOptions->pPositionNames and not markName.empty())
    linesYtop += LINES_1;

  METLIBS_LOG_DEBUG(LOGVAL(charsXleft) << LOGVAL(charsXrght) << LOGVAL(linesYbot) << LOGVAL(linesYtop));

  // ----------------------------------------
  // calculate font size such that the text does not occupy more than MAX_FRAMETEXT of width or height
  mFontSize = 10.;
  fp->setVpSize(mTotalSize.width(), mTotalSize.height());
  fp->setGlSize(mTotalSize.width(), mTotalSize.height());
  updateCharSize();

  float framePixelsX = (charsXleft + charsXrght) * mCharSize.width(), framePixelsY = (linesYbot + linesYtop) * mCharSize.height();

  const float maxFramePixelsX = mTotalSize.width() * MAX_FRAMETEXT, maxFramePixelsY = mTotalSize.height() * MAX_FRAMETEXT;
  if (framePixelsX > maxFramePixelsX or framePixelsY > maxFramePixelsY) {
    const float bigFramePixelsX = framePixelsX, bigFramePixelsY = framePixelsY;
    VcrossUtil::minimize(framePixelsX, maxFramePixelsX);
    VcrossUtil::minimize(framePixelsY, maxFramePixelsY);
    mFontSize *= std::min(framePixelsX / bigFramePixelsX, framePixelsY / bigFramePixelsY);
    updateCharSize();
    framePixelsX = (charsXleft + charsXrght) * mCharSize.width();
    framePixelsY = (linesYbot  + linesYtop)  * mCharSize.height();
  }
  // ----------------------------------------

  mPlotAreaMax = QRectF(QPointF(charsXleft * mCharSize.width(), linesYtop * mCharSize.height()),
      QPointF(mTotalSize.width() - charsXrght * mCharSize.width(), mTotalSize.height() - linesYbot * mCharSize.height()));
  METLIBS_LOG_DEBUG(LOGVAL(mPlotAreaMax.x()) << LOGVAL(mPlotAreaMax.y())
      << LOGVAL(mPlotAreaMax.width()) << LOGVAL(mPlotAreaMax.height())
      << LOGVAL(mPlotAreaMax.bottom()) << LOGVAL(mPlotAreaMax.top()));
}

void VcrossPlot::updateCharSize()
{
  fp->setFontSize(mFontSize);
  float charPixelsX = 1, charPixelsY = 1;
  fp->getCharSize('M', charPixelsX, charPixelsY);
  mCharSize = QSizeF(charPixelsX, charPixelsY);
}

void VcrossPlot::calculateContrastColour()
{
  // contrast colour
  mBackColour = Colour(mOptions->backgroundColour);
  const int sum = mBackColour.R() + mBackColour.G() + mBackColour.B();
  if (sum > 255 * 3 / 2)
    mContrastColour.set(0, 0, 0);
  else
    mContrastColour.set(255, 255, 255);
}

int VcrossPlot::getNearestPos(int px)
{
  METLIBS_LOG_SCOPE();

  const float valueX = mAxisX->paint2value(px);
  int n = 0;
  if (mAxisX->legalValue(valueX)) {
    float d2 = 1e20;
    for (size_t i = 0; i < mCrossectionDistances.size(); i++) {
      const float dx = mCrossectionDistances.at(i) - px, dx2 = dx*dx;
      if (d2 > dx2) {
        d2 = dx2;
        n = i;
      }
    }
  }
  return n;
}

void VcrossPlot::plot()
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(mAxisY->quantity) << LOGVAL(mAxisY->type));

  if (mViewChanged)
    prepareView();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glClearColor(mBackColour.fR(), mBackColour.fG(), mBackColour.fB(), 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glLoadIdentity();
  glOrtho(0, mTotalSize.width(), mTotalSize.height(), 0, -1., 1.); // same as Qt coordinate system, top=small numbers
  fp->setVpSize(mTotalSize.width(), mTotalSize.height());
  fp->setGlSize(mTotalSize.width(), mTotalSize.height());
  updateCharSize();

  plotData();

  plotFrame();
  plotText();
  setFont();
  plotXLabels();
  plotTitle();
  hardcopy->UpdateOutput();

  plotLevels();
  plotSurfacePressure();
  plotVerticalGridLines();
  plotMarkerLines();
  plotVerticalMarkerLines();

  const std::vector<std::string> labels; // TODO
  plotAnnotations(labels);
}

void VcrossPlot::plotTitle()
{
  if (not mOptions->pPositionNames or markName.empty() or isTimeGraph())
    return;

#if 0
  float chystp = 1.5;
  float chx = chxdef;
  float chy = chydef;
//  float dchy = chy * 0.6;
  float chxt, chyt;

  int nn = markName.size();
  float y = yWindowmax - 1.25 * chy * chystp;
  fp->setFontSize(fontscale);
  chxt = 1.25 * chx;
  chyt = 1.25 * chy;
  glColor3ubv(colourOrContrast(mOptions->positionNamesColour).RGB());
  float xm, dxh;
  float xlen = 0., xmin = xWindowmax, xmax = xWindowmin;
  vector<int> vpos;
  vector<float> vdx, vxm, vx1, vx2;

  for (int n = 0; n < nn; n++) {
    float x = markNamePosMin[n];
    int i = int(x);
    if (i < 0)
      i = 0;
    if (i > nPoint - 2)
      i = nPoint - 2;
    float x1 = cdata1d[nxs][i] + (cdata1d[nxs][i + 1] - cdata1d[nxs][i]) * (x - float(i));
    x = markNamePosMax[n];
    i = int(x);
    if (i < 0)
      i = 0;
    if (i > nPoint - 2)
      i = nPoint - 2;
    float x2 = cdata1d[nxs][i] + (cdata1d[nxs][i + 1] - cdata1d[nxs][i]) * (x - float(i));
    if (x2 > xDatamin && x1 < xDatamax) {
      float dx,dy;
      fp->getStringSize(markName[n].c_str(), dx, dy);
      float dxh = dx * 0.5 + chxt;
      xm = (x1 + x2) * 0.5;
      if (x1 < xWindowmin + dxh)
        x1 = xWindowmin + dxh;
      if (x2 > xWindowmax - dxh)
        x2 = xWindowmax - dxh;
      if (x1 <= x2) {
        if (xm < x1)
          xm = x1;
        if (xm > x2)
          xm = x2;
        vpos.push_back(n);
        vdx.push_back(dx);
        vxm.push_back(xm);
        vx1.push_back(x1);
        vx2.push_back(x2);
        xlen += dxh * 2.;
        if (xmin > x1)
          xmin = x1;
        if (xmax < x2)
          xmax = x2;
      }
    }
  }

  nn = vpos.size();

  if (nn > 0) {

    if (xlen > xmax - xmin) {
      float s = (xmax - xmin) / xlen;
      if (s < 0.75)
        s = 0.75;
      chxt = chxt * s;
      chyt = chyt * s;
      fp->setFontSize(fontscale * s);
      for (int n = 0; n < nn; n++) {
        float dx, dy;
        fp->getStringSize(markName[n].c_str(), dx, dy);
        vdx[n] = dx;
      }
    }

    for (int n = 0; n < nn; n++) {
      float x = vxm[n];
      dxh = vdx[n] * 0.5 + chxt;
      xmin = vx1[n] - dxh;
      xmax = vx2[n] + dxh;
      int k1 = -1;
      int k2 = -1;
      for (int i = 0; i < nn; i++) {
        if (i != n) {
          float x1 = vxm[i] - vdx[i] * 0.5 - chxt;
          float x2 = vxm[i] + vdx[i] * 0.5 + chxt;
          if (vxm[i] < x && xmin < x2) {
            xmin = x2;
            k1 = i;
          }
          if (vxm[i] > x && xmax > x1) {
            xmax = x1;
            k2 = i;
          }
        }
      }
      float x1 = x - dxh;
      float x2 = x + dxh;
      if (xmin > x1 && k1 >= 0) {
        if (k1 < n)
          x1 = xmin;
      }
      if (xmax < x2 && k2 >= 0) {
        if (k2 < n)
          x2 = xmax;
      }
      if (x1 < vx1[n] - dxh)
        x1 = vx1[n] - dxh;
      if (x2 > vx2[n] + dxh)
        x2 = vx2[n] + dxh;
      if (x2 - x1 > dxh * 1.99) {
        x = (x1 + x2) * 0.5;
        vxm[n] = x;
        x -= vdx[n] * 0.5;
        fp->drawStr(markName[vpos[n]].c_str(), x, y, 0.0);
      }
    }
  }
#endif
}

void VcrossPlot::plotXLabels()
{
  METLIBS_LOG_SCOPE();
  const float lines_1 = LINES_1*mCharSize.height();

  if (not isTimeGraph()) {
    float labelY = mAxisY->getPaintMin() + lines_1;
    // Distance from reference position (default left end)
    if (mOptions->pDistance) {
      glColor3ubv(colourOrContrast(mOptions->distanceColour).RGB());
      float unit;
      std::string uname;
      if (miutil::to_lower(mOptions->distanceUnit) == "nm") {
        unit = 1852; // nautical mile
        uname = "nm";
      } else {
        unit = 1000; // kilometer
        uname = "km";
      }

      const float tickTopEnd = mAxisY->getPaintMax(), tickTopStart = tickTopEnd + 0.5*mCharSize.height();
      const float tickBotStart = mAxisY->getPaintMin(), tickBotEnd = tickBotStart - 0.5*mCharSize.height();
      float nextLabelX = mAxisX->getPaintMin() - 1;
      const int precision = (((mAxisX->getValueMax() - mAxisX->getValueMin()) / unit) > 100) ? 0 : 1;
      for (size_t i=0; i<mCrossectionDistances.size(); ++i) {
        const float distance = mCrossectionDistances.at(i); 
        const float tickX = mAxisX->value2paint(distance);
        
        if (mAxisX->legalPaint(tickX)) {
          glBegin(GL_LINES);
          glVertex2f(tickX, tickBotStart);
          glVertex2f(tickX, tickBotEnd);
          glVertex2f(tickX, tickTopStart);
          glVertex2f(tickX, tickTopEnd);
          glEnd();
          
          if (tickX >= nextLabelX) {
            std::ostringstream xostr;
            xostr << std::setprecision(precision) << std::setiosflags(std::ios::fixed)
                  << std::abs(distance / unit) << uname;
            const std::string xstr = xostr.str();
            const char* c_str = xstr.c_str();
            float labelW=0, labelH=0;
            fp->getStringSize(c_str, labelW, labelH);
            fp->drawStr(c_str, tickX - labelW/2, labelY, 0.0);
            nextLabelX += labelW + mCharSize.width();
          }
        }
      }
      labelY += lines_1;
    }
    // latitude,longitude
    if (mOptions->pGeoPos) {
      glColor3ubv(colourOrContrast(mOptions->geoposColour).RGB());
      float nextLabelX = mAxisX->getPaintMin() - 1;
      for (size_t i=0; i<mCrossectionDistances.size(); ++i) {
        const float distance = mCrossectionDistances.at(i); 
        const float tickX = mAxisX->value2paint(distance);
        if (mAxisX->legalPaint(tickX)) {
          if (tickX >= nextLabelX) {
            std::ostringstream xostr, yostr;
            writeLonEW(xostr, mCrossectionPoints.at(i).lonDeg());
            writeLatNS(yostr, mCrossectionPoints.at(i).latDeg());
            const std::string xstr = xostr.str(), ystr = yostr.str();
            const char *x_str = xstr.c_str(), *y_str = ystr.c_str();
            float labelWx=0, labelWy=0, labelH=0;
            fp->getStringSize(x_str, labelWx, labelH);
            fp->drawStr(x_str, tickX - labelWx/2, labelY, 0.0);
            fp->getStringSize(x_str, labelWy, labelH);
            fp->drawStr(y_str, tickX - labelWy/2, labelY + lines_1, 0.0);
            nextLabelX += std::min(labelWx, labelWy) + mCharSize.width();
          }
        }
      }
      labelY += LINES_2 * mCharSize.height();
    }
  } else {
#if 0
    // time graph
    if (mOptions->pText) {
      glColor3ubv(colourOrContrast(mOptions->textColour).RGB());
      // hour,day,month,hour_forecast along x axis (year in text below)
      vector<string> fctext(nPoint);
      vector<bool> showday(nPoint, false);
      unsigned int lmax = 0;
      for (int i = ip1; i <= ip2; i++) {
        ostringstream ostr;
        ostr << setiosflags(ios::showpos) << forecastHourSeries[i];
        string str = ostr.str();
        if (lmax < str.length())
          lmax = str.length();
        fctext[i] = str;
      }
      int ihrmin = validTimeSeries[ip1].hour();
      int ittmin = ip1;
      int ittday = ip1;
      float dxday = cdata1d[nxs][ip2] - cdata1d[nxs][ip1];
      float dxhr = cdata1d[nxs][ip2] - cdata1d[nxs][ip1];
      for (int i = ip1 + 1; i <= ip2; i++) {
        if (validTimeSeries[i].hour() < ihrmin) {
          ihrmin = validTimeSeries[i].hour();
          ittmin = i;
        }
        if (validTimeSeries[i].date() != validTimeSeries[ittday].date()) {
          float dx = cdata1d[nxs][i] - cdata1d[nxs][ittday];
          if (dxday > dx)
            dxday = dx;
          ittday = i;
          showday[i] = true;
        }
        float dx = cdata1d[nxs][i] - cdata1d[nxs][i - 1];
        if (dxhr > dx)
          dxhr = dx;
      }
      chxt = chx * 0.75;
      if (chxt * 3. > dxhr)
        chxt = dxhr / 3.;
      if (chxt * 6. > dxday)
        chxt = dxday / 6.;
      if (chxt * (lmax + 1) > dxday)
        chxt = dxday / (lmax + 1);
      if (ittmin == ip1)
        showday[ip1] = true;
      else if (cdata1d[nxs][ittmin] - cdata1d[nxs][ip1] > chxt * (lmax + 1))
        showday[ip1] = true;
      chyt = chxt * chy / chx;
      float y1 = y - chyt * chystp;
      float y2 = y - chyt * chystp * 2.;
      float y3 = y - chyt * chystp * 3.;
      y = y - chyt * chystp * 3.2;
      fp->setFontSize(fontscale * chyt / chy);
      for (int i = ip1; i <= ip2; i++) {
        float x = cdata1d[nxs][i];
        ostringstream ostr;
        ostr << setw(2) << setfill('0') << validTimeSeries[i].hour();
        string str = ostr.str();
        float dx, dy;
        fp->getStringSize(str.c_str(), dx, dy);
        fp->drawStr(str.c_str(), x - dx * 0.5, y1, 0.0);
        if (showday[i]) {
          ostringstream ostr2;
          ostr2 << validTimeSeries[i].day() << "/"
              << validTimeSeries[i].month();
          str = ostr2.str();
          fp->drawStr(str.c_str(), x - chxt, y2, 0.0);
        }
        fp->drawStr(fctext[i].c_str(), x - chxt, y3, 0.0);
      }
    }
    if (mOptions->pDistance or mOptions->pGeoPos) {
      y -= (chy * chystp);
      vector<std::string> vpstr, vpcol;
      int l = 0;
      if (mOptions->pDistance) {
        //----------------------------------------------------------
        int i = int(refPosition);
        if (i < 0)
          i = 0;
        if (i > nPoint - 2)
          i = nPoint - 2;
        float rpos = cdata1d[nxs][i] + (cdata1d[nxs][i + 1] - cdata1d[nxs][i])
                * (refPosition - float(i));
        //----------------------------------------------------------
        ostringstream xostr;
        xostr << "Distance=" << setprecision(1) << setiosflags(ios::fixed)
                << (cdata1d[nxs][i] - rpos) / 1000.;
        std::string xstr = xostr.str();
        miutil::trim(xstr);
        xstr += "km";
        l += xstr.length();
        vpstr.push_back(xstr);
        vpcol.push_back(mOptions->distanceColour);
      }
      // x,y coordinate
      // latitude,longitude
      if (mOptions->pGeoPos) {
        float glat = cdata1d[nlat][0];
        float glon = cdata1d[nlon][0];
        ostringstream ostr;
        ostr << "Lat=" << setprecision(1) << setiosflags(ios::fixed);
        writeLatNS(ostr, glat);
        ostr << "  Long=" << setprecision(1) << setiosflags(ios::fixed);
        writeLonEW(ostr, glon);
        const std::string str = ostr.str();
        l += str.length();
        vpstr.push_back(str);
        vpcol.push_back(mOptions->geoposColour);
      }
      l += (vpstr.size() - 1) * 2;
      chxt = chx;
      if (l * chx > cdata1d[nxs][ipd2] - cdata1d[nxs][ipd1])
        chxt = (cdata1d[nxs][ipd2] - cdata1d[nxs][ipd1]) / float(l);
      chyt = chxt * chy / chx;
      fp->setFontSize(fontscale * chyt / chy);
      float x = xPlotmin;
      for (unsigned int n = 0; n < vpstr.size(); n++) {
        glColor3ubv(colourOrContrast(vpcol[n]).RGB());
        fp->drawStr(vpstr[n].c_str(), x, y, 0.0);
        x += ((vpstr[n].length() + 2) * chxt);
      }
    }
#endif
  }
}

void VcrossPlot::plotLevels()
{
#if 0
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

  int npd = ipd2 - ipd1 + 1;

  // find lower and upper level inside current window
  // (contour plot: ilev1 - ilev2    wind plot: iwlev1 - iwlev2)
  int ilev1 = 0;
  int ilev2 = 1;
  for (int k = 0; k < numLev - 1; k++) {
    if (vlimitmax[k] < yPlotmin)
      ilev1 = k;
    if (vlimitmin[k] < yPlotmax)
      ilev2 = k + 1;
  }

  float xylim[4] = { xPlotmin, xPlotmax, yPlotmin, yPlotmax };

  Colour c;
  int k1,k2;

  for (int loop = 0; loop < 3; loop++) {
    float lwidth;
    std::string ltype;
    if (loop == 0 && mOptions->pUpperLevel) {
      k1 = k2 = numLev - 1;
      c = Colour(mOptions->upperLevelColour);
      lwidth = mOptions->upperLevelLinewidth;
      ltype = mOptions->upperLevelLinetype;
    } else if (loop == 1 && mOptions->pLowerLevel) {
      k1 = k2 = 0;
      c = Colour(mOptions->lowerLevelColour);
      lwidth = mOptions->lowerLevelLinewidth;
      ltype = mOptions->lowerLevelLinetype;
    } else if (loop == 2 && mOptions->pOtherLevels) {
      k1 = 1;
      k2 = numLev - 2;
      c = Colour(mOptions->otherLevelsColour);
      lwidth = mOptions->otherLevelsLinewidth;
      ltype = mOptions->otherLevelsLinetype;
    } else {
      k1 = k2 = -1;
    }
    if (k1 >= 0) {
      if (k1 < ilev1)
        k1 = ilev1;
      if (k2 > ilev2)
        k2 = ilev2;
      k2++;
    }
    if (k1 < k2) {
      if (c == backColour)
        c = contrastColour;
      glColor3ubv(c.RGB());
      glLineWidth(lwidth);
      Linetype linetype(ltype);
      if (linetype.stipple) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(linetype.factor, linetype.bmap);
      }
      if (vcoord != 4) {
        for (int k = k1; k < k2; k++)
          xyclip(npd, &cdata2d[nx][k * nPoint + ipd1], &cdata2d[ny][k * nPoint + ipd1], xylim);
      } else {
        // theta levels
        for (int k = k1; k < k2; k++) {
          int i = ipd1 - 1;
          while (i < ipd2) {
            i++;
            while (i <= ipd2 && cdata2d[ny][k * nPoint + i] == fieldUndef)
              i++;
            int ibgn = i;
            while (i <= ipd2 && cdata2d[ny][k * nPoint + i] != fieldUndef)
              i++;
            int iend = i;
            if (ibgn < iend - 1)
              xyclip(iend - ibgn, &cdata2d[nx][k * nPoint + ibgn],
                  &cdata2d[ny][k * nPoint + ibgn], xylim);
          }
        }
      }
      UpdateOutput();
      glDisable(GL_LINE_STIPPLE);
    }
  }
#endif
}

void VcrossPlot::plotSurfacePressure()
{
  if (not mOptions->pSurface)
    return;

#if 0
  // surface pressure (ps)
  // or approx. surface when isentropic levels (vcoord=4)
  // or sea bottom + sea surface elevation (vcoord=5)
  // or surface height (vcoord=11)

  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

//  int ip1 = (cdata1d[nxs][ipd1] < xPlotmin) ? ipd1 + 1 : ipd1;
//  int ip2 = (cdata1d[nxs][ipd2] > xPlotmax) ? ipd2 - 1 : ipd2;
  int npd = ipd2 - ipd1 + 1;

  float xylim[4] = { xPlotmin, xPlotmax, yPlotmin, yPlotmax };

  glColor3ubv(colourOrContrast(mOptions->surfaceColour).RGB());
  glLineWidth(mOptions->surfaceLinewidth);
  Linetype linetype(mOptions->surfaceLinetype);
  if (linetype.stipple) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(linetype.factor, linetype.bmap);
  }

  if (mOptions->pSurface && npy1 >= 0) {

    if (vcoord != 5) {
      xyclip(npd, &cdata1d[nxs][ipd1], &cdata1d[npy1][ipd1], xylim);
    } else if (vcoord == 5) {
      // sea bottom: coast drawn between the horizontal points
      int i = ipd1 - 1;
      while (i < ipd2) {
        i++;
        while (i <= ipd2 && cdata1d[npy1][i] == fieldUndef)
          i++;
        int ibgn = i;
        while (i <= ipd2 && cdata1d[npy1][i] != fieldUndef)
          i++;
        int iend = i - 1;
        if (ibgn <= ipd2) {
          if (ibgn > ipd1) {
            float x = (cdata1d[nxs][ibgn - 1] + cdata1d[nxs][ibgn]) * 0.5;
            float xline[3] = { x, x, cdata1d[nxs][ibgn] };
            float yline[3] = { yPlotmax, 0., cdata1d[npy1][ibgn] };
            xyclip(3, xline, yline, xylim);
          }
          xyclip(iend - ibgn + 1, &cdata1d[nxs][ibgn], &cdata1d[npy1][ibgn], xylim);
          if (iend < ipd2) {
            float x = (cdata1d[nxs][iend] + cdata1d[nxs][iend + 1]) * 0.5;
            float xline[3] = { cdata1d[nxs][iend], x, x };
            float yline[3] = { cdata1d[npy1][iend], 0., yPlotmax };
            xyclip(3, xline, yline, xylim);
          }
        }
      }
    }
  }

  if (npy2 >= 0) {
    xyclip(npd, &cdata1d[nxs][ipd1], &cdata1d[npy2][ipd1], xylim);
  }

  UpdateOutput();
  glDisable(GL_LINE_STIPPLE);
#endif
}

void VcrossPlot::plotVerticalGridLines()
{
  if (not mOptions->pVerticalGridLines)
    return;

  glColor3ubv(colourOrContrast(mOptions->vergridColour).RGB());
  glLineWidth(mOptions->vergridLinewidth);
  Linetype linetype(mOptions->vergridLinetype);
  if (linetype.stipple) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(linetype.factor, linetype.bmap);
  }
  glBegin(GL_LINES);

  const float ytop = mAxisY->getPaintMax(), ybot = mAxisY->getPaintMin();
  if (not isTimeGraph()) {
    for (size_t i=0; i<mCrossectionDistances.size(); ++i) {
      const float px = mAxisX->value2paint(mCrossectionDistances.at(i));
      if (mAxisX->legalPaint(px)) {
        glVertex2f(px, ybot);
        glVertex2f(px, ytop);
      }
    }
  } else {
    for (size_t i=0; i<mTimeDistances.size(); ++i) {
      const float px = mAxisX->value2paint(mTimeDistances.at(i));
      if (mAxisX->legalPaint(px)) {
        glVertex2f(px, ybot);
        glVertex2f(px, ytop);
      }
    }
  }
  glEnd();
  hardcopy->UpdateOutput();
  glDisable(GL_LINE_STIPPLE);
}

void VcrossPlot::plotMarkerLines()
{
  if (not mOptions->pMarkerlines)
    return;

#if 0
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

  float xylim[4] = { xPlotmin, xPlotmax, yPlotmin, yPlotmax };

  int numPar1d = cdata1d.size();
  vector<int> mlines;
  if (vcoordPlot == vcv_height) {
    for (int n = 0; n < numPar1d; n++) {
      if (idPar1d[n] == 907)
        mlines.push_back(n); // inflight height above sealevel
    }
  } else if (vcoordPlot == vcv_exner || vcoordPlot == vcv_pressure) {
    for (int n = 0; n < numPar1d; n++) {
      if (idPar1d[n] == 908)
        mlines.push_back(n); // inflight pressure
    }
  }
  if (mlines.size() > 0) {
    glColor3ubv(colourOrContrast(mOptions->markerlinesColour).RGB());
    glLineWidth(mOptions->markerlinesLinewidth);
    Linetype linetype(mOptions->markerlinesLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    float *y1d = new float[nPoint];
    for (unsigned int m = 0; m < mlines.size(); m++) {
      int n = mlines[m];
      int i = ipd1 - 1;
      while (i < ipd2) {
        i++;
        while (i <= ipd2 && cdata1d[n][i] == fieldUndef)
          i++;
        int ibgn = i;
        while (i <= ipd2 && cdata1d[n][i] != fieldUndef)
          i++;
        int iend = i;
        if (ibgn < iend - 1) {
          if (vcoordPlot == vcv_exner) {
            for (int j = ibgn; j < iend; j++) {
              float p = cdata1d[n][j];
              float pi = exnerFunction(p);
              y1d[j] = yconst + yscale * pi;
            }
          } else {
            for (int j = ibgn; j < iend; j++) {
              y1d[j] = yconst + yscale * cdata1d[n][j];
            }
          }
          xyclip(iend - ibgn, &cdata1d[nxs][ibgn], &y1d[ibgn], xylim);
        }
      }
    }
    delete[] y1d;
    hardcopy->UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }
#endif
}

void VcrossPlot::plotVerticalMarkerLines()
{
#if 0
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

  int ip1 = (cdata1d[nxs][ipd1] < xPlotmin) ? ipd1 + 1 : ipd1;
  int ip2 = (cdata1d[nxs][ipd2] > xPlotmax) ? ipd2 - 1 : ipd2;

  // vertical marker lines
  if (mOptions->pVerticalMarker) {
    if (ip2 - ip1 > 3) {
      glColor3ubv(colourOrContrast(mOptions->verticalMarkerColour).RGB());
      glLineWidth(mOptions->verticalMarkerLinewidth);
      Linetype linetype(mOptions->verticalMarkerLinetype);
      if (linetype.stipple) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(linetype.factor, linetype.bmap);
      }
      glBegin(GL_LINES);
      float delta1 = cdata1d[nxs][ip1 + 1] - cdata1d[nxs][ip1];
      for (int i = ip1 + 1; i < ip2; i++) {
        float delta2 = cdata1d[nxs][i + 1] - cdata1d[nxs][i];
        if (fabs(delta1 - delta2) > 1.0) {
          delta1 = delta2;
          glVertex2f(cdata1d[nxs][i], yPlotmin);
          glVertex2f(cdata1d[nxs][i], yPlotmax);
        }
      }
      glEnd();
      hardcopy->UpdateOutput();
      glDisable(GL_LINE_STIPPLE);
    }
  }
#endif
}

void VcrossPlot::plotAnnotations(const std::vector<std::string>& labels)
{
#if 0
  //Annotations
  float xoffset = (xPlotmax - xPlotmin) / 50;
  float yoffset = (yPlotmax - yPlotmin) / 50;
  bool left, top;
  int nlabels = labels.size();

  for (int i = 0; i < nlabels; i++) {
    left = top = true;
    vector<std::string> tokens = miutil::split_protected(labels[i], '"', '"');
    int ntokens = tokens.size();
    std::string text;
    float unit = 1.0, xfac = 1.0, yfac = 1.0;
    bool arrow = false;
    vector<float> arrow_x, arrow_y;
    Colour tcolour, fcolour, bcolour;
    float yoffsetfac = 1.0, xoffsetfac = 1.0;
    for (int j = 0; j < ntokens; j++) {
      vector<std::string> stokens = split(tokens[j], '<', '>');
      int nstokens = stokens.size();
      if (nstokens > 0) {
        for (int k = 0; k < nstokens; k++) {
          vector<std::string> sstokens = miutil::split_protected(stokens[k], '\"', '\"', ",", true);
          int nsstokens = sstokens.size();
          for (int l = 0; l < nsstokens; l++) {
            vector<std::string> ssstokens = miutil::split_protected(sstokens[l], '\"', '\"', "=", true);
            if (ssstokens.size() == 2) {
              if (miutil::to_upper(ssstokens[0]) == "TEXT") {
                text = ssstokens[1];
              } else if (miutil::to_upper(ssstokens[0]) == "ARROW") {
                unit = miutil::to_double(ssstokens[1]);;
                arrow = true;
              } else if (miutil::to_upper(ssstokens[0]) == "XFAC") {
                xfac = miutil::to_double(ssstokens[1]);
              } else if (miutil::to_upper(ssstokens[0]) == "YFAC") {
                yfac = miutil::to_double(ssstokens[1]);;
              }
            }
          }
        }
      } else {
        vector<std::string> stokens = miutil::split(tokens[j], "=");
        if (stokens.size() == 2) {
          if (miutil::to_upper(stokens[0]) == "TCOLOUR") {
            tcolour = Colour(stokens[1]);
          } else if (miutil::to_upper(stokens[0]) == "FCOLOUR") {
            fcolour = Colour(stokens[1]);
          } else if (miutil::to_upper(stokens[0]) == "BCOLOUR") {
            bcolour = Colour(stokens[1]);
          } else if (miutil::to_upper(stokens[0]) == "VALIGN" && miutil::to_upper(stokens[1])
              == "BOTTOM") {
            top = false;
          } else if (miutil::to_upper(stokens[0]) == "HALIGN" && miutil::to_upper(stokens[1])
              == "RIGHT") {
            left = false;
          } else if (miutil::to_upper(stokens[0]) == "XOFFSET") {
            xoffsetfac = miutil::to_double(stokens[1]);
          } else if (miutil::to_upper(stokens[0]) == "YOFFSET") {
            yoffsetfac = miutil::to_double(stokens[1]);
          }
        }
      }
    }

    xoffset *= xoffsetfac;
    yoffset *= yoffsetfac;
    float ddy = 0., ddx = 0.;
    if (arrow) {
      ddy = 3600 * unit * v2hRatio * yfac;
      ddx = 3600 * unit * xfac;
    }
    float dx,dy;
    fp->setFontSize(fontscale);
    fp->getStringSize(text.c_str(), dx, dy);
    dy = dy > ddy ? dy : ddy;
    dx += ddx;

    float xpos, ypos;
    if (left) {
      xpos = xPlotmin + xoffset;
    } else {
      xpos = xPlotmax - xoffset - dx;
    }

    if (top) {
      ypos = yPlotmax - yoffset;
    } else {
      ypos = yPlotmin + yoffset + dy * 2;
    }

    //textbox
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3ubv(fcolour.RGBA());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1);
    glBegin(GL_POLYGON);
    glVertex2f(xpos, ypos);
    glVertex2f(xpos + xoffset * 0.6 + dx, ypos);
    glVertex2f(xpos + xoffset * 0.6 + dx, ypos - dy * 2);
    glVertex2f(xpos, ypos - dy * 2);
    glEnd();
    glDisable(GL_BLEND);

    glColor3ubv(bcolour.RGB());
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
    glVertex2f(xpos, ypos);
    glVertex2f(xpos + xoffset * 0.6 + dx, ypos);
    glVertex2f(xpos + xoffset * 0.6 + dx, ypos - dy * 2);
    glVertex2f(xpos, ypos - dy * 2);
    glEnd();

    glColor3ubv(tcolour.RGB());

    if (arrow) {
      plotArrow(xpos + xoffset * 0.3, ypos - dy * 1.4, ddx, ddy, true);
    }
    if (not text.empty()) {
      miutil::remove(text, '"');
      text = validTime.format(text);
      miutil::trim(text);
      fp->drawStr(text.c_str(), xpos + xoffset * 0.3 + ddx * 2,
          ypos - dy * 1.4, 0.0);
    }
  }
#endif
}

void VcrossPlot::plotFrame()
{
  METLIBS_LOG_SCOPE();
  const int nzsteps = 10;
  const float zsteps[nzsteps] =
      { 5., 10., 25., 50., 100., 250., 500., 1000., 2500., 5000. };

  const int nflsteps = 8;
  const float flsteps[nflsteps] =
  { 1., 2., 5., 10., 50., 100., 200., 500. };

  const int npfixed1 = 17;
  const float pfixed1[npfixed1]
      = { 1000, 925, 850, 700, 600, 500, 400, 300, 250, 200, 150, 100, 70, 50, 30, 10, 5 };

  // P -> FlightLevels (used for remapping fields from P to FL)
  const int mfl = 16;
  const float plevels[mfl]
      = { 1000, 925, 850, 700, 600, 500, 400, 300, 250, 200, 150, 100, 70, 50, 30, 10 };
  const float flevels[mfl]
      = { 0, 25, 50, 100, 140, 180, 240, 300, 340, 390, 450, 530, 600, 700, 800, 999 };
  const float fl2m = 1. / 3.2808399; // flightlevel (100 feet unit) to meter

  int nticks = 0;
  const float *tickValues = 0, *tickLabels = 0;
  float scale = 1;

  if (mAxisY->quantity == VcrossPlotDetail::Axis::PRESSURE) {
    if (mAxisY->label == "hPa") {
      nticks = npfixed1;
      tickValues = tickLabels = pfixed1;
    } else if (mAxisY->label == "FL") {
      nticks = mfl;
      tickValues = plevels;
      tickLabels = flevels;
    }
  } else if (mAxisY->quantity == VcrossPlotDetail::Axis::HEIGHT) {
    if (mAxisY->label == "m") {
      nticks = nzsteps;
      tickValues = tickLabels = zsteps;
    } else if (mAxisY->label == "Ft") {
      scale = fl2m;
      tickValues = tickLabels = flsteps;
      nticks = nflsteps;
    }
  }
  METLIBS_LOG_DEBUG(LOGVAL(nticks));

  const float tickLeftEnd = mAxisX->getPaintMin(), tickLeftStart = tickLeftEnd - 0.5*mCharSize.width();
  const float tickRightStart = mAxisX->getPaintMax(), tickRightEnd = tickRightStart + 0.5*mCharSize.width();
  METLIBS_LOG_DEBUG(LOGVAL(tickLeftStart) << LOGVAL(tickLeftEnd) << LOGVAL(tickRightStart) << LOGVAL(tickRightEnd));

  Linetype linetype(mOptions->frameLinetype);
  if (linetype.stipple) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(linetype.factor, linetype.bmap);
  }
  if (mOptions->pFrame) {
    METLIBS_LOG_DEBUG("plotting frame");
    // colour etc. for frame etc.
    glColor3ubv(colourOrContrast(mOptions->frameColour).RGB());
    glLineWidth(mOptions->frameLinewidth);
    
    glBegin(GL_LINE_LOOP);
    glVertex2f(mAxisX->getPaintMin(), mAxisY->getPaintMin());
    glVertex2f(mAxisX->getPaintMax(), mAxisY->getPaintMin());
    glVertex2f(mAxisX->getPaintMax(), mAxisY->getPaintMax());
    glVertex2f(mAxisX->getPaintMin(), mAxisY->getPaintMax());
    glVertex2f(mAxisX->getPaintMin(), mAxisY->getPaintMin());
    glEnd();
    hardcopy->UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }

  // paint tick marks
  for (int i=0; i<nticks; ++i) {
    const float tickValue = tickValues[i];
    const float tickY = mAxisY->value2paint(tickValue);
    if (mAxisY->legalPaint(tickY)) {
      glBegin(GL_LINES);
      glVertex2f(tickLeftStart,  tickY);
      glVertex2f(tickLeftEnd,    tickY);
      glVertex2f(tickRightStart, tickY);
      glVertex2f(tickRightEnd,   tickY);
      glEnd();
    }
  }

  // paint tick labels
  if (mOptions->pLevelNumbers) {
    const float labelLeftEnd = tickLeftEnd - mCharSize.width();
    const float labelRightStart = tickRightStart + mCharSize.width();
    for (int i=0; i<nticks; ++i) {
      const float tickValue = tickValues[i];
      const float tickY = mAxisY->value2paint(tickValue);
      if (mAxisY->legalPaint(tickY)) {
        const float tickLabel = tickLabels[i] * scale;
        std::ostringstream ostr;
        ostr << int(tickLabel) << mAxisY->label;
        const std::string txt = ostr.str();
        const char* c_str = txt.c_str();
        float labelW=0, labelH=0;
        fp->getStringSize(c_str, labelW, labelH);
        const float labelY = tickY-labelH*0.5;
        fp->drawStr(c_str, labelLeftEnd - labelW, labelY, 0.0);
        fp->drawStr(c_str, labelRightStart,       labelY, 0.0);
      }
    }
  }
}

void VcrossPlot::plotData()
{
  METLIBS_LOG_SCOPE();

  BOOST_FOREACH(const Plot& plot, mPlots) {
    switch(plot.type) {
    case vcpt_contour:
      plotDataContour(plot);
      break;
    case vcpt_wind:
      plotDataWind(plot);
      break;
    case vcpt_vector:
      plotDataVector(plot);
      break;
    case vcpt_line:
      plotDataLine(plot);
      break;
    case vcpt_no_plot:
      // no plot, nothing to do (but compiler complains)
      break;
    }
    hardcopy->UpdateOutput();
  }
}

void VcrossPlot::plotDataContour(const Plot& plot)
{
  METLIBS_LOG_SCOPE();

  VcrossPlotDetail::VCContourField con_field(plot.p0, mAxisX, mAxisY, mCrossectionDistances, plot.zax);
  con_field.setLevels(plot.poptions.lineinterval);
  VcrossPlotDetail::VCContouring con(&con_field, fp.get());
  con.makeLines();
}

void VcrossPlot::plotDataWind(const Plot& plot)
{
  METLIBS_LOG_SCOPE();

  glLineWidth(plot.poptions.linewidth + 0.1); // +0.1 to avoid MesaGL coredump
  glColor3ubv(plot.poptions.linecolour.RGB());

  PaintWindArrow pw;
  pw.mWithArrowHead = (plot.poptions.arrowstyle == arrow_wind_arrow);

  METLIBS_LOG_DEBUG(LOGVAL(plot.poptions.density));
  const int xStep = std::max(plot.poptions.density, 1);
  const bool xStepAuto = (plot.poptions.density < 1);

  const int nx = plot.zax->mPoints, ny = plot.zax->mLevels;
  float nextX = mAxisX->getPaintMin() - 1;
  for (int ix=0; ix<nx; ix += xStep) {
    const float vx = mCrossectionDistances.at(ix);
    const float px = mAxisX->value2paint(vx);
    if (not mAxisX->legalPaint(px) or (xStepAuto and px < nextX))
      continue;
    bool painted = false;
    for (int iy=0; iy<ny; ++iy) {
      const float vy = plot.zax->value(iy, ix);
      const float py = mAxisY->value2paint(vy);
      if (mAxisY->legalPaint(py)) {
        const int idx = iy*nx + ix;
        const float WIND_SCALE = 3600.0 / 1852.0;

        const float wx = plot.p0[idx], wy = plot.p1[idx];
        if (not (isnan(wx) or isnan(wy))) {
          pw.paint(WIND_SCALE * wx, WIND_SCALE * wy, px, py);
          painted = true;
        }
      }
    }
    if (painted)
      nextX = px + 2*pw.mSize;
  }
}

void VcrossPlot::plotDataVector(const Plot& plot)
{
  METLIBS_LOG_SCOPE();

  glLineWidth(plot.poptions.linewidth + 0.1); // +0.1 to avoid MesaGL coredump
  glColor3ubv(plot.poptions.linecolour.RGB());

  PaintVector pv;
  pv.mScaleX = pv.mScaleY = 0.5;
  pv.mWithArrowHead = (plot.poptions.arrowstyle == arrow_wind_arrow);

  const int nx = plot.zax->mPoints, ny = plot.zax->mLevels;
  for (int ix=0; ix<nx; ++ix) {
    const float vx = mCrossectionDistances.at(ix);
    const float px = mAxisX->value2paint(vx);
    if (not mAxisX->legalPaint(px))
      continue;
    for (int iy=0; iy<ny; ++iy) {
      const float vy = plot.zax->value(iy, ix);
      const float py = mAxisY->value2paint(vy);
      if (mAxisY->legalPaint(py)) {
        const int idx = iy*nx + ix;

        const float u = plot.p0[idx], v = plot.p1[idx];
        if (not (isnan(u) or isnan(v)))
          pv.paint(u, v, px, py);
      }
    }
  }
}

void VcrossPlot::plotDataLine(const Plot& plot)
{
  METLIBS_LOG_SCOPE();

  glLineWidth(plot.poptions.linewidth + 0.1); // +0.1 to avoid MesaGL coredump
  glColor3ubv(plot.poptions.linecolour.RGB());

  glBegin(GL_LINES);

  bool last_ok = false;
  float last_px = 0, last_py = 0;

  const int nx = plot.zax->mPoints;
  for (int ix=0; ix<nx; ++ix) {
    const float vx = mCrossectionDistances.at(ix), p0 = plot.zax->value(0, ix);
    const float px = mAxisX->value2paint(vx), py = mAxisY->value2paint(p0);
    //METLIBS_LOG_DEBUG(LOGVAL(ix) << LOGVAL(vx) << LOGVAL(px) << LOGVAL(p0) << LOGVAL(py));
    if (mAxisX->legalPaint(px) and mAxisY->legalPaint(py)) {
      if (last_ok) {
        glVertex2f(last_px, last_py);
        glVertex2f(px, py);
      }
      last_px = px;
      last_py = py;
      last_ok = true;
    } else {
      last_ok = false;
    }
  }
}

void VcrossPlot::setFont()
{
  fp->setFont(std::string("BITMAPFONT"));
}
