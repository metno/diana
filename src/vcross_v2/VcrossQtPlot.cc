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

#include "VcrossQtPlot.h"

#include "VcrossQtAxis.h"
#include "VcrossQtContour.h"
#include "VcrossOptions.h"
#include "VcrossQtPaint.h"
#include "VcrossQtUtil.h"
#include <diField/VcrossUtil.h>

#include "diLinetype.h"

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringFunctions.h>

#include <QtGui/QPainter>

#include <boost/algorithm/string/join.hpp>
#include <boost/foreach.hpp>

#include <cmath>
#include <iterator>
#include <iomanip>

#define MILOGGER_CATEGORY "diana.QtPlot"
#include <miLogger/miLogging.h>

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

namespace vcross {

// ########################################################################

QtPlot::QtPlot(VcrossOptions_p options)
  : mOptions(options)
  , mTotalSize(100, 100)
  , mFontSize(0)
  , mViewChanged(true)
  , mKeepX(false)
  , mKeepY(false)
  , mAxisX(new vcross::detail::Axis(true))
  , mAxisY(new vcross::detail::Axis(false))
{
  METLIBS_LOG_SCOPE();
}

QtPlot::~QtPlot()
{
  METLIBS_LOG_SCOPE();
}

void QtPlot::plotText(QPainter& painter)
{
  METLIBS_LOG_SCOPE();
  if (not mOptions->pText)
    return;

  float widthModel = 0, widthField = 0;
  BOOST_FOREACH(OptionPlot_cp plot, mPlots) {
    vcross::util::updateMaxStringWidth(painter, widthModel, plot->model());
    vcross::util::updateMaxStringWidth(painter, widthField, plot->name());
  }

  const float xModel = mCharSize.width(), xField = xModel + widthModel + mCharSize.width();
  float yPlot = mTotalSize.height() - LINE_GAP*mCharSize.height();
  const float yCSName = yPlot, yStep = mCharSize.height() * LINES_1;

  BOOST_FOREACH(OptionPlot_cp plot, mPlots) {
    painter.setPen(util::QC(colourOrContrast(plot->poptions.linecolour)));
    painter.drawText(QPointF(xModel, yPlot), QString::fromStdString(plot->model()));
    painter.drawText(QPointF(xField, yPlot), QString::fromStdString(plot->name()));
    yPlot -= yStep;
  }

  QString label;
  if (not isTimeGraph()) { // show cross section name
    label = QString::fromStdString(mCrossectionLabel);
  } else {
    std::ostringstream oposition;
    writeLonEW(oposition, mTimeCSPoint.lonDeg());
    oposition << ' ';
    writeLatNS(oposition, mTimeCSPoint.latDeg());
    label = QString::fromStdString(oposition.str());
  }
  const float label_w = painter.fontMetrics().width(label);
  painter.setPen(util::QC(colourOrContrast(mOptions->frameColour)));
  painter.drawText(QPointF(mTotalSize.width() - label_w - mCharSize.width(), yCSName), label);
}

void QtPlot::viewSetWindow(int w, int h)
{
  METLIBS_LOG_SCOPE();

  mTotalSize = QSize(w, h);
  mViewChanged = true;
}

void QtPlot::getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour)
{
  METLIBS_LOG_SCOPE();

  x1 = mAxisX->getPaintMin();
  y1 = mAxisX->getPaintMax();
  x2 = mAxisY->getPaintMin();
  y2 = mAxisY->getPaintMax();
  rubberbandColour = mContrastColour;
}

void QtPlot::viewZoomIn(int px1, int py1, int px2, int py2)
{
  METLIBS_LOG_SCOPE();

  py1 = mTotalSize.height() - py1;
  py2 = mTotalSize.height() - py2;

  // only zoom an axis if at least one point is inside this axis'
  // paint range
  const bool lx1 = mAxisX->legalPaint(px1), ly1 = mAxisY->legalPaint(py1),
      lx2 = mAxisX->legalPaint(px2), ly2 = mAxisY->legalPaint(py2);
  bool zx = false, zy = false;
  if (lx1 or lx2)
    zx = mAxisX->zoomIn(px1, px2);
  if (ly1 or ly2)
    zy = mAxisY->zoomIn(py1, py2);
  mViewChanged |= (zx or zy);
}

void QtPlot::viewZoomOut()
{
  METLIBS_LOG_SCOPE();

  const bool zx = mAxisX->zoomOut();
  const bool zy = mAxisY->zoomOut();
  mViewChanged |= (zx or zy);
}

void QtPlot::viewPan(int pxmove, int pymove)
{
  const bool px = mAxisX->pan(pxmove);
  const bool py = mAxisY->pan(pymove);
  mViewChanged |= (px or py);
}

void QtPlot::viewStandard()
{
  METLIBS_LOG_SCOPE();

  prepareXAxis();
  prepareYAxisRange();

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

void QtPlot::clear(bool keepX, bool keepY)
{
  mKeepX = keepX;
  mKeepY = keepY;

  mPlots.clear();

  mCrossectionLabel.clear();
  mCrossectionPoints.clear();
  mCrossectionDistances.clear();

  mTimePoints.clear();
  mTimeDistances.clear();

  mSurface = Values_cp();
}

void QtPlot::setHorizontalCross(std::string csLabel, const LonLat_v& csPoints)
{
  METLIBS_LOG_SCOPE(LOGVAL(csLabel) << LOGVAL(csPoints.size()));

  mCrossectionLabel = csLabel;
  mCrossectionPoints = csPoints;

  mCrossectionDistances.clear();
  mCrossectionDistances.reserve(mCrossectionPoints.size());
  mCrossectionDistances.push_back(0);
  for (size_t i=1; i<mCrossectionPoints.size(); ++i) {
    const LonLat &p0 = mCrossectionPoints.at(i-1), &p1 = mCrossectionPoints.at(i);
    mCrossectionDistances.push_back(mCrossectionDistances.back() + p0.distanceTo(p1));
  }
}

void QtPlot::setHorizontalTime(const LonLat& tgPosition, const std::vector<miutil::miTime>& times)
{
  METLIBS_LOG_SCOPE(LOGVAL(times.size()));

  mTimePoints = times;
  mTimeCSPoint = tgPosition;

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

void QtPlot::setVerticalAxis()
{
//  mAxisY->setType(mOptions->verticalScale);
  mAxisY->setQuantity(mOptions->verticalCoordinate);
  mAxisY->label = mOptions->verticalUnit;

}

QtPlot::OptionPlot::OptionPlot(EvaluatedPlot_cp e)
  : evaluated(e)
{
  std::string op_c = boost::algorithm::join(e->selected->resolved->configured->options, " ");
  std::string op_s = boost::algorithm::join(e->selected->options, " ");

  PlotOptions::parsePlotOption(op_c, poptions);
  PlotOptions::parsePlotOption(op_s, poptions);
}

void QtPlot::addPlot(EvaluatedPlot_cp ep)
{
  METLIBS_LOG_SCOPE();
  mPlots.push_back(miutil::make_shared<OptionPlot>(ep));
}

void QtPlot::prepare()
{
  METLIBS_LOG_SCOPE();

  if (not mKeepX)
    prepareXAxis();
  if (not mKeepY)
    prepareYAxis();
  if (not (mKeepX or mKeepY))
    viewStandard();
  mKeepX = mKeepY = false;

  mViewChanged = true;
}

void QtPlot::prepareXAxis()
{
  METLIBS_LOG_SCOPE();

  float xax_min = 0, xax_max;
  if (not isTimeGraph())
    xax_max = mCrossectionDistances.back();
  else
    xax_max = mTimeDistances.back();
  METLIBS_LOG_DEBUG(LOGVAL(xax_min) << LOGVAL(xax_max));
    
  mAxisX->setDataRange (xax_min, xax_max);
}

void QtPlot::prepareYAxis()
{
  METLIBS_LOG_SCOPE();

  prepareYAxisRange();
}

void QtPlot::prepareYAxisRange()
{
  METLIBS_LOG_SCOPE();
  bool have_z = false;
  float yax_min = 1e35, yax_max = -1e35;
  BOOST_FOREACH(OptionPlot_cp plot, mPlots) {
    Values_cp z_values = plot->evaluated->z_values;
    if (not z_values)
      continue;
    have_z = true;
    METLIBS_LOG_DEBUG("next plot" << LOGVAL(z_values->npoint()) << LOGVAL(z_values->nlevel()));
    for (size_t l=0; l<z_values->nlevel(); ++l) {
      for (size_t p=0; p<z_values->npoint(); ++p) {
        const float zax_value = z_values->value(p, l);
        vcross::util::minimaximize(yax_min, yax_max, zax_value);
      }
    }
  }
  if (not have_z)
    return;
  if (mAxisY->quantity == vcross::detail::Axis::PRESSURE)
    std::swap(yax_min, yax_max);
  METLIBS_LOG_DEBUG(LOGVAL(yax_min) << LOGVAL(yax_max));
  mAxisY->setDataRange(yax_min, yax_max);
  mAxisY->setValueRange(yax_min, yax_max);
}

void QtPlot::prepareView(QPainter& painter)
{
  METLIBS_LOG_SCOPE();

  calculateContrastColour();
  computeMaxPlotArea(painter);
  prepareAxesForAspectRatio();

  mViewChanged = false;
}

void QtPlot::prepareAxesForAspectRatio()
{
  METLIBS_LOG_SCOPE();
  float v2h = mOptions->verHorRatio;
  if (isTimeGraph() or (not mOptions->keepVerHorRatio) or v2h <= 0) {
    METLIBS_LOG_DEBUG("no aspect ratio");
    // horizontal axis has time unit, vertical axis pressure or height; aspect ratio is meaningless
    mAxisX->setPaintRange(mPlotAreaMax.left(),   mPlotAreaMax.right());
    mAxisY->setPaintRange(mPlotAreaMax.bottom(), mPlotAreaMax.top());
    return;
  }

  const float rangeX = std::abs(mAxisX->getValueMax() - mAxisX->getValueMin());
  float rangeY = v2h * std::abs(mAxisY->getValueMax() - mAxisY->getValueMin());
  if (mAxisY->quantity == vcross::detail::Axis::PRESSURE)
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

void QtPlot::computeMaxPlotArea(QPainter& painter)
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
      vcross::util::maximize(charsXleft, CHARS_DISTANCE);
      charsXrght = charsXleft;
    }
    if (mOptions->pGeoPos)
      linesYbot += LINES_2;
    if (mOptions->pGeoPos) {
      vcross::util::maximize(charsXleft, CHARS_POS_LEFT);
      vcross::util::maximize(charsXrght, CHARS_POS_RIGHT);
    }
  } else /* timeGraph */ {
    // time graph: hour/date/forecast (not if text is off)
    if (mOptions->pText) {
      linesYbot += LINES_3;
      vcross::util::maximize(charsXleft, CHARS_TIME);
      charsXrght = charsXleft;
    }
    // time graph: only one position (and only one line needed)
    if (mOptions->pDistance or mOptions->pGeoPos)
      linesYbot += LINES_1;
  }

  if (mOptions->pText)
    linesYbot += mPlots.size() + LINE_GAP;

  //if (mOptions->pPositionNames and not markName.empty())
  //  linesYtop += LINES_1;

  METLIBS_LOG_DEBUG(LOGVAL(charsXleft) << LOGVAL(charsXrght) << LOGVAL(linesYbot) << LOGVAL(linesYtop));

  // ----------------------------------------
  // calculate font size such that the text does not occupy more than MAX_FRAMETEXT of width or height
  mFontSize = 10.;
  updateCharSize(painter);

  float framePixelsX = (charsXleft + charsXrght) * mCharSize.width(), framePixelsY = (linesYbot + linesYtop) * mCharSize.height();

  const float maxFramePixelsX = mTotalSize.width() * MAX_FRAMETEXT, maxFramePixelsY = mTotalSize.height() * MAX_FRAMETEXT;
  if (framePixelsX > maxFramePixelsX or framePixelsY > maxFramePixelsY) {
    const float bigFramePixelsX = framePixelsX, bigFramePixelsY = framePixelsY;
    vcross::util::minimize(framePixelsX, maxFramePixelsX);
    vcross::util::minimize(framePixelsY, maxFramePixelsY);
    mFontSize *= std::min(framePixelsX / bigFramePixelsX, framePixelsY / bigFramePixelsY);
    updateCharSize(painter);
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

void QtPlot::updateCharSize(QPainter& painter)
{
  const float charPixelsX = painter.fontMetrics().width("M"), charPixelsY = painter.fontMetrics().height();
  mCharSize = QSizeF(charPixelsX, charPixelsY);
}

void QtPlot::calculateContrastColour()
{
  // contrast colour
  mBackColour = Colour(mOptions->backgroundColour);
  const int sum = mBackColour.R() + mBackColour.G() + mBackColour.B();
  if (sum > 255 * 3 / 2)
    mContrastColour.set(0, 0, 0);
  else
    mContrastColour.set(255, 255, 255);
}

int QtPlot::getNearestPos(int px)
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

void QtPlot::plot(QPainter& painter)
{
  METLIBS_LOG_SCOPE();

  if (mViewChanged)
    prepareView(painter);

  painter.fillRect(QRect(QPoint(0,0), mTotalSize), vcross::util::QC(mBackColour));

  updateCharSize(painter);

  plotFrame(painter);
  plotText(painter);
  plotXLabels(painter);
  plotTitle(painter);

  plotSurface(painter);
  plotVerticalGridLines(painter);

  plotData(painter);
}

void QtPlot::plotTitle(QPainter& painter)
{
}

void QtPlot::plotXLabels(QPainter& painter)
{
  METLIBS_LOG_SCOPE();
  const float lines_1 = LINES_1*mCharSize.height();

  if (not isTimeGraph()) {
    float labelY = mAxisY->getPaintMin() + lines_1;
    // Distance from reference position (default left end)
    if (mOptions->pDistance) {
      QPen pen(vcross::util::QC(colourOrContrast(mOptions->distanceColour)));
      painter.setPen(pen);
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
      float nextLabelX = mAxisX->getPaintMin();
      const int precision = (((mAxisX->getValueMax() - mAxisX->getValueMin()) / unit) > 100) ? 0 : 1;
      for (size_t i=0; i<mCrossectionDistances.size(); ++i) {
        const float distance = mCrossectionDistances.at(i); 
        const float tickX = mAxisX->value2paint(distance);
        
        if (mAxisX->legalPaint(tickX)) {
          int tickLineWidth = 1;
          if (tickX >= nextLabelX) {
            std::ostringstream xostr;
            xostr << std::setprecision(precision) << std::setiosflags(std::ios::fixed)
                  << std::abs(distance / unit) << uname;
            const QString q_str = QString::fromStdString(xostr.str());
            const float labelW = painter.fontMetrics().width(q_str), lX = tickX - labelW/2;
            if (lX >= nextLabelX) {
              painter.drawText(QPointF(lX, labelY), q_str);
              nextLabelX += labelW + mCharSize.width();
              tickLineWidth += 1;
            }
          }
          // FIXME this makes thick tick marks for if distance is shown, and no ticks if no distance
          pen.setWidth(tickLineWidth);
          painter.setPen(pen);
          painter.drawLine(QLineF(tickX, tickBotStart, tickX, tickBotEnd));
          painter.drawLine(QLineF(tickX, tickTopStart, tickX, tickTopEnd));
        }
      }
      labelY += lines_1;
    }
    // latitude,longitude
    if (mOptions->pGeoPos) {
      painter.setPen(vcross::util::QC(colourOrContrast(mOptions->geoposColour)));
      float nextLabelX = mAxisX->getPaintMin();
      for (size_t i=0; i<mCrossectionDistances.size(); ++i) {
        const float distance = mCrossectionDistances.at(i); 
        const float tickX = mAxisX->value2paint(distance);
        if (mAxisX->legalPaint(tickX)) {
          if (tickX >= nextLabelX) {
            std::ostringstream xostr, yostr;
            writeLonEW(xostr, mCrossectionPoints.at(i).lonDeg());
            writeLatNS(yostr, mCrossectionPoints.at(i).latDeg());
            const std::string xstr = xostr.str(), ystr = yostr.str();
            const QString x_str = QString::fromStdString(xstr), y_str = QString::fromStdString(ystr);
            const float labelWx=painter.fontMetrics().width(x_str), labelWy = painter.fontMetrics().width(y_str);
            painter.drawText(tickX - labelWx/2, labelY, x_str);
            painter.drawText(tickX - labelWy/2, labelY + lines_1, y_str);
            nextLabelX += std::min(labelWx, labelWy) + mCharSize.width();
          }
        }
      }
      labelY += LINES_2 * mCharSize.height();
    }
  } else { // time graph
    float labelY = mAxisY->getPaintMin() + lines_1;
    if (mOptions->pText) {
      painter.setPen(vcross::util::QC(colourOrContrast(mOptions->textColour)));
      float nextLabelX = mAxisX->getPaintMin();
      for (size_t i=0; i<mTimeDistances.size(); ++i) {
        const float minutes = mTimeDistances.at(i); 
        const float tickX = mAxisX->value2paint(minutes);
        if (mAxisX->legalPaint(tickX)) {
          if (tickX >= nextLabelX) {
            const miutil::miTime& t = mTimePoints.at(i);
            const QString t_str = QString("%1:%2").arg(t.hour(), 2, 10, QLatin1Char('0')).arg(t.min(),   2, 10, QLatin1Char('0'));
            const QString d_str = QString("%1.%2").arg(t.day(),  2, 10, QLatin1Char('0')).arg(t.month(), 2, 10, QLatin1Char('0'));
            const float labelWt=painter.fontMetrics().width(t_str), labelWd = painter.fontMetrics().width(d_str);
            painter.drawText(tickX - labelWt/2, labelY, t_str);
            painter.drawText(tickX - labelWd/2, labelY + lines_1, d_str);
            nextLabelX += std::min(labelWt, labelWd) + mCharSize.width();
          }
        }
      }
      labelY += LINES_2 * mCharSize.height();
    }
  }
}

void QtPlot::plotSurface(QPainter& painter)
{
  METLIBS_LOG_SCOPE();
  //if (not mOptions->pSurface)
  //  return;
  if (not mSurface) {
    METLIBS_LOG_DEBUG("surface plot enabled, but surface data missing");
    return;
  }

  float last_x = 0;
  bool last_ok = false;

  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;

  painter.save();
  painter.setBrush(Qt::black);

  QPolygonF polygon; // TODO set pen etc
  const int nx = mSurface->npoint();
  for (int ix=0; ix<nx; ++ix) {
    const float vx = distances.at(ix), p0 = mSurface->value(ix, 0);
    const float px = mAxisX->value2paint(vx), py = mAxisY->value2paint(p0);
    //METLIBS_LOG_DEBUG(LOGVAL(ix) << LOGVAL(vx) << LOGVAL(px) << LOGVAL(p0) << LOGVAL(py));
    if (mAxisX->legalPaint(px) and mAxisY->legalPaint(py)) {
      if (not last_ok)
        polygon << QPointF(px, mAxisY->getPaintMin());
      polygon << QPointF(px, py);
      last_ok = true;
      last_x = px;
    } else {
      last_ok = false;
      polygon << QPointF(last_x, mAxisY->getPaintMin());
      if (polygon.size() >= 3)
        painter.drawPolygon(polygon);
      polygon.clear();
    }
  }
  if (last_ok)
    polygon << QPointF(last_x, mAxisY->getPaintMin());
  if (polygon.size() >= 3)
    painter.drawPolygon(polygon);
  painter.restore();
}

void QtPlot::plotVerticalGridLines(QPainter& painter)
{
  if (not mOptions->pVerticalGridLines)
    return;

  QPen pen(vcross::util::QC(colourOrContrast(mOptions->vergridColour)), mOptions->vergridLinewidth);
  vcross::util::setDash(pen, mOptions->vergridLinetype);
  painter.setPen(pen);

  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;

  const float ytop = mAxisY->getPaintMax(), ybot = mAxisY->getPaintMin();
  for (size_t i=0; i<distances.size(); ++i) {
    const float px = mAxisX->value2paint(distances.at(i));
    if (mAxisX->legalPaint(px))
      painter.drawLine(QLineF(px, ybot, px, ytop));
  }
}

void QtPlot::plotFrame(QPainter& painter)
{
  METLIBS_LOG_SCOPE();

  if (not mOptions->pFrame)
    return;

  const int nzsteps = 9;
  const float zsteps[nzsteps] =
      { 10., 500., 1000., 2500., 5000., 10000, 15000, 20000, 25000 };

  const int nftsteps = 7;
  const float ftsteps[nftsteps] =
  { 1500., 3000., 8000., 15000., 30000., 50000, 60000 };

  // P -> FlightLevels (used for remapping fields from P to FL)
  const int mfl = 16;
  const float plevels[mfl]
      = { 1000, 925, 850, 700, 600, 500, 400, 300, 250, 200, 150, 100, 70, 50, 30, 10 };
  const float flevels[mfl]
      = { 0, 25, 50, 100, 140, 180, 240, 300, 340, 390, 450, 530, 600, 700, 800, 999 };

  const float fl2m = 3.2808399; // flightlevel (100 feet unit) to meter

  int nticks = 0;
  const float *tickValues = 0, *tickLabels = 0;
  float scale = 1;

  if (mAxisY->quantity == vcross::detail::Axis::PRESSURE) {
    tickValues = plevels;
    nticks = mfl;
    if (mAxisY->label == "hPa") {
      tickLabels = plevels;
    } else if (mAxisY->label == "FL") {
      tickLabels = flevels;
    }
  } else if (mAxisY->quantity == vcross::detail::Axis::HEIGHT) {
    if (mAxisY->label == "m") {
      nticks = nzsteps;
      tickValues = tickLabels = zsteps;
    } else if (mAxisY->label == "Ft") {
        scale = fl2m;
        nticks = nftsteps;
        tickValues = tickLabels = ftsteps;
    }
  }
  METLIBS_LOG_DEBUG(LOGVAL(nticks));

  const float tickLeftEnd = mAxisX->getPaintMin(), tickLeftStart = tickLeftEnd - 0.5*mCharSize.width();
  const float tickRightStart = mAxisX->getPaintMax(), tickRightEnd = tickRightStart + 0.5*mCharSize.width();
  METLIBS_LOG_DEBUG(LOGVAL(tickLeftStart) << LOGVAL(tickLeftEnd) << LOGVAL(tickRightStart) << LOGVAL(tickRightEnd));

  // colour etc. for frame etc.
  QPen pen(vcross::util::QC(colourOrContrast(mOptions->frameColour)), mOptions->frameLinewidth), penFrame(pen);
  vcross::util::setDash(penFrame, mOptions->frameLinetype);
  painter.setPen(penFrame);

  // paint frame
  const QPointF min(mAxisX->getPaintMin(), mAxisY->getPaintMin()), max(mAxisX->getPaintMax(), mAxisY->getPaintMax());
  painter.drawRect(QRectF(min, max));

  // no stipple for ticks
  painter.setPen(pen);

  // paint tick marks
  for (int i=0; i<nticks; ++i) {
    const float tickValue = tickValues[i]/scale;
    const float tickY = mAxisY->value2paint(tickValue);
    if (mAxisY->legalPaint(tickY)) {
      painter.drawLine(QLineF(tickLeftStart,  tickY, tickLeftEnd,    tickY));
      painter.drawLine(QLineF(tickRightStart, tickY, tickRightEnd,   tickY));
    }
  }

  // paint tick labels
  if (mOptions->pLevelNumbers) {
    const float labelLeftEnd = tickLeftEnd - mCharSize.width();
    const float labelRightStart = tickRightStart + mCharSize.width();
    for (int i=0; i<nticks; ++i) {
      const float tickValue = tickValues[i]/scale;
      const float tickY = mAxisY->value2paint(tickValue);
      if (mAxisY->legalPaint(tickY)) {
        const float tickLabel = tickLabels[i];
        std::ostringstream ostr;
        ostr << int(tickLabel) << mAxisY->label;
        const std::string txt = ostr.str();
        const QString c_str = QString::fromStdString(txt);
        const float labelW = painter.fontMetrics().width(c_str), labelH = painter.fontMetrics().height();
        const float labelY = tickY+labelH*0.5;
        painter.drawText(labelLeftEnd - labelW, labelY, c_str);
        painter.drawText(labelRightStart,       labelY, c_str);
      }
    }
  }
}

void QtPlot::plotData(QPainter& painter)
{
  METLIBS_LOG_SCOPE();

  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;
  const size_t npoint = distances.size();

  BOOST_FOREACH(OptionPlot_cp plot, mPlots) {
    EvaluatedPlot_cp ep = plot->evaluated;
    if ((ep->argument_values.empty()) or (not ep->z_values) or (ep->z_values->npoint() != npoint)
        or (ep->values(0)->npoint() != npoint))
    {
      METLIBS_LOG_ERROR("no argument_values, or no z_values, or unexpected point count, cannot plot");
      return;
    }
    
    switch(plot->type()) {
    case ConfiguredPlot::T_CONTOUR:
      plotDataContour(painter, plot);
      break;
    case ConfiguredPlot::T_WIND:
      plotDataWind(painter, plot);
      break;
    case ConfiguredPlot::T_VECTOR:
      plotDataVector(painter, plot);
      break;
    case ConfiguredPlot::T_NONE:
      // no plot, nothing to do (but compiler complains)
      break;
    }
  }
}

void QtPlot::plotDataContour(QPainter& painter, OptionPlot_cp plot)
{
  METLIBS_LOG_SCOPE(LOGVAL(plot->name()));
  
  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;

  DianaLevels_p levels = dianaLevelsForPlotOptions(plot->poptions, detail::UNDEF_VALUE);
  const detail::VCAxisPositions positions(mAxisX, mAxisY, distances, plot->evaluated->z_values);
  vcross::detail::VCContourField con_field(plot->evaluated->values(0), *levels, positions);
  vcross::detail::VCLines con_lines(*levels, plot->poptions);
  contouring::run(con_field, con_lines);
  con_lines.paint(painter);
}

void QtPlot::plotDataWind(QPainter& painter, OptionPlot_cp plot)
{
  METLIBS_LOG_SCOPE();
  EvaluatedPlot_cp ep = plot->evaluated;
  if (ep->argument_values.size() != 2) {
    METLIBS_LOG_ERROR("not exactly 2 argument values, cannot plot");
    return;
  }

  Values_cp av0 = util::unitConversion(ep->values(0), ep->argument(0)->unit(), "knot"),
      av1 = util::unitConversion(ep->values(1), ep->argument(1)->unit(), "knot");

  PaintWindArrow pw;
  pw.mWithArrowHead = (plot->poptions.arrowstyle == arrow_wind_arrow);
  plotDataArrow(painter, plot, pw, av0, av1);
}

void QtPlot::plotDataVector(QPainter& painter, OptionPlot_cp plot)
{
  METLIBS_LOG_SCOPE();
  EvaluatedPlot_cp ep = plot->evaluated;
  if (ep->argument_values.size() != 2) {
    METLIBS_LOG_ERROR("not exactly 2 argument values, cannot plot");
    return;
  }

  PaintVector pv;
  const float vu = plot->poptions.vectorunit;
  if (vu > 0)
    pv.setScale(vu);
  plotDataArrow(painter, plot, pv, ep->values(0), ep->values(1));
}

void QtPlot::plotDataArrow(QPainter& painter, OptionPlot_cp plot, const PaintArrow& pa, Values_cp av0, Values_cp av1)
{
  METLIBS_LOG_SCOPE(LOGVAL(plot->name()));

  const QColor color(vcross::util::QC(plot->poptions.linecolour));
  painter.setPen(QPen(color, plot->poptions.linewidth));
  painter.setBrush(color);

  const int density = plot->poptions.density;
  const int xStep = std::max(density, 1);
  const bool xStepAuto = (density < 1);

  const Values_cp z_values = plot->evaluated->z_values;
  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;

  const int nx = z_values->npoint(), ny = z_values->nlevel();
  float lastX = - 1;
  bool paintedX = false;
  for (int ix=0; ix<nx; ix += xStep) {
    const float vx = distances.at(ix);
    const float px = mAxisX->value2paint(vx);
    
    const bool paintThisX = mAxisX->legalPaint(px)
        and ((not xStepAuto) or (not paintedX) or (std::abs(px - lastX) >= 2*pa.size()));
    if (not paintThisX)
      continue;

    bool paintedY = false;
    float lastY = - 1;
    for (int iy=0; iy<ny; iy += xStep) {
      const float vy = z_values->value(ix, iy);
      const float py = mAxisY->value2paint(vy);
      const bool paintThisY = mAxisY->legalPaint(py)
          and ((not xStepAuto) or (not paintedY) or (std::abs(py - lastY) >= 2*pa.size()));
      if (paintThisY) {
        const float wx = av0->value(ix, iy), wy = av1->value(ix, iy);
        if (not (isnan(wx) or isnan(wy))) {
          pa.paint(painter, wx, wy, px, py);
          lastY = py;
          paintedY = true;
        }
      }
    }
    if (paintedY) {
      lastX = px;
      paintedX = true;
    }
  }
  painter.setBrush(Qt::NoBrush);
}

} // namespace vcross
