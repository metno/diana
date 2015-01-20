/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2015 met.no

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

#include "VcrossOptions.h"
#include "VcrossQtAxis.h"
#include "VcrossQtContour.h"
#include "VcrossQtPaint.h"
#include "VcrossQtUtil.h"

#include "diLinetype.h"

#include <diField/diMetConstants.h>
#include <diField/VcrossUtil.h>

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miStringBuilder.h>
#include <puTools/miStringFunctions.h>

#include <QtGui/QPainter>

#include <boost/algorithm/string/join.hpp>
#include <boost/foreach.hpp>

#include <cmath>
#include <iterator>
#include <iomanip>

#define MILOGGER_CATEGORY "diana.VcrossPlot"
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

inline float square(float x)
{
  return x*x;
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

void QtPlot::plotText(QPainter& painter, const std::vector<std::string>& annotations)
{
  METLIBS_LOG_SCOPE();
  if (not mOptions->pText)
    return;

  float widthModel = 0, widthField = 0;
  BOOST_FOREACH(OptionPlot_cp plot, mPlots) {
    vcross::util::updateMaxStringWidth(painter, widthModel, plot->model());
    vcross::util::updateMaxStringWidth(painter, widthField, plot->name());
  }

  const float xModel = mCharSize.width(), xField = xModel + widthModel + mCharSize.width(),
      xAnnotations = xField + widthField + mCharSize.width();
  float yPlot = mTotalSize.height() - LINE_GAP*mCharSize.height();
  const float yCSName = yPlot, yStep = mCharSize.height() * LINES_1;

  size_t idx_annotations = 0;
  BOOST_FOREACH(OptionPlot_cp plot, mPlots) {
    if (plot->poptions.options_1)
      painter.setPen(util::QC(colourOrContrast(plot->poptions.linecolour)));
    else
      painter.setPen(util::QC(colourOrContrast(mOptions->textColour)));
    painter.drawText(QPointF(xModel, yPlot), QString::fromStdString(plot->model()));
    painter.drawText(QPointF(xField, yPlot), QString::fromStdString(plot->name()));
    if (idx_annotations < annotations.size())
      painter.drawText(QPointF(xAnnotations, yPlot), QString::fromStdString(annotations.at(idx_annotations)));

    yPlot -= yStep;
    idx_annotations += 1;
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
  painter.setPen(util::QC(colourOrContrast(mOptions->textColour)));
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
  if (mPlots.empty())
    return;

  prepareXAxis();
  prepareYAxisRange();

  float xMin = mAxisX->getDataMin(), xMax = mAxisX->getDataMax();
  if (mOptions->stdHorizontalArea) {
    const float range = (xMax - xMin) * 0.01;
    xMin += range * mOptions->minHorizontalArea;
    xMax -= range * (100-mOptions->maxHorizontalArea);
  }
  mAxisX->setValueRange(xMin, xMax);

  float yMin = mAxisY->getDataMin(), yMax = mAxisY->getDataMax();
  if (mOptions->stdVerticalArea) {
    const float range = (yMax - yMin) * 0.01;
    yMin += range * mOptions->minVerticalArea;
    yMax -= range * (100-mOptions->maxVerticalArea);
  }
  mAxisY->setValueRange(yMin, yMax);

  mViewChanged = true;
}

QtPlot::Rect QtPlot::viewGetCurrentZoom() const
{
  return Rect(mAxisX->getValueMin(), mAxisY->getValueMin(), mAxisX->getValueMax(), mAxisY->getValueMax());
}

void QtPlot::viewSetCurrentZoom(const Rect& r)
{
  mAxisX->setValueRange(r.x1, r.x2);
  mAxisY->setValueRange(r.y1, r.y2);
  mKeepX = true;
  mKeepY = true;
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
  mLines.clear();

  mViewChanged = true;
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
      mTimeDistances.push_back(miutil::miTime::minDiff(t1, t0));
    }
  }
}

void QtPlot::setVerticalAxis()
{
  mAxisY->setType(mOptions->verticalScale);
  mAxisY->setQuantity(mOptions->verticalCoordinate);
  mAxisY->setLabel(mOptions->verticalUnit);
  mViewChanged = true;
}

QtPlot::OptionPlot::OptionPlot(EvaluatedPlot_cp e)
  : evaluated(e)
{
  std::string op_c = boost::algorithm::join(e->selected->resolved->configured->options, " ");
  std::string op_s = e->selected->optionString();

  PlotOptions::parsePlotOption(op_c, poptions);
  PlotOptions::parsePlotOption(op_s, poptions);
}

void QtPlot::addPlot(EvaluatedPlot_cp ep)
{
  METLIBS_LOG_SCOPE();
  mPlots.push_back(miutil::make_shared<OptionPlot>(ep));
}

void QtPlot::addLine(Values_cp linevalues, const std::string& linecolour, const std::string& linetype, float linewidth)
{
  METLIBS_LOG_SCOPE();
  mLines.push_back(OptionLine(linevalues, linecolour, linetype, linewidth));
}

void QtPlot::prepare()
{
  METLIBS_LOG_SCOPE();

  mViewChanged = true;
  if (mPlots.empty())
    return;

  if (not mKeepX)
    prepareXAxis();
  if (not mKeepY)
    prepareYAxis();
  if (not (mKeepX or mKeepY))
    viewStandard();
  mKeepX = mKeepY = false;
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
    
  mAxisX->setDataRange(xax_min, xax_max);
  mAxisX->setValueRange(xax_min, xax_max);
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
    const int nl = z_values->shape().length(Values::GEO_Z), np = z_values->shape().length(Values::GEO_X);
    METLIBS_LOG_DEBUG("next plot" << LOGVAL(np) << LOGVAL(nl));
    Values::ShapeIndex idx(z_values->shape());
    for (int l=0; l<nl; ++l) {
      idx.set(Values::GEO_Z, l);
      for (int p=0; p<np; ++p) {
        idx.set(Values::GEO_X, p);
        const float zax_value = z_values->value(idx);
        vcross::util::minimaximize(yax_min, yax_max, zax_value);
      }
    }
  }
  if (not have_z) {
    METLIBS_LOG_DEBUG("none of the plots has z values");
    return;
  }
  if (not mAxisY->increasing())
    std::swap(yax_min, yax_max);
  METLIBS_LOG_DEBUG(LOGVAL(yax_min) << LOGVAL(yax_max));
  mAxisY->setDataRange(yax_min, yax_max);
  mAxisY->setValueRange(yax_min, yax_max);
}

void QtPlot::prepareView(QPainter& painter)
{
  METLIBS_LOG_SCOPE();

  calculateContrastColour();
  if (not mPlots.empty()) {
    computeMaxPlotArea(painter);
    prepareAxesForAspectRatio();
  }

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
  if (mAxisY->quantity() == vcross::detail::Axis::PRESSURE)
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
#if 0
    // time graph: hour/date/forecast (not if text is off)
    if (mOptions->pText) {
      linesYbot += LINES_3;
      vcross::util::maximize(charsXleft, CHARS_TIME);
      charsXrght = charsXleft;
    }
#endif
    // time graph: only one position (and only one line needed)
    if (mOptions->pDistance or mOptions->pGeoPos)
      linesYbot += LINES_2;
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
  mBackColour = Colour(mOptions->backgroundColour);
  mContrastColour = mBackColour.contrastColour();
}

int QtPlot::getNearestPos(int px)
{
  METLIBS_LOG_SCOPE();

  const float valueX = mAxisX->paint2value(px);
  int n_min = 0;
  if (mAxisX->legalValue(valueX) and not mCrossectionDistances.empty()) {
    float dx2_min = square(mCrossectionDistances.at(0) - valueX);
    for (size_t i = 1; i < mCrossectionDistances.size(); i++) {
      const float dx2 = square(mCrossectionDistances.at(i) - valueX);
      if (dx2 < dx2_min) {
        dx2_min = dx2;
        n_min = i;
      }
    }
  }
  return n_min;
}

void QtPlot::plot(QPainter& painter)
{
  METLIBS_LOG_SCOPE();

  if (mViewChanged)
    prepareView(painter);

  painter.fillRect(QRect(QPoint(0,0), mTotalSize), vcross::util::QC(mBackColour));
  if (mPlots.empty())
    return;

  updateCharSize(painter);

  plotVerticalGridLines(painter);
  plotSurface(painter);
  const std::vector<std::string> annotations = plotData(painter);

  plotFrame(painter);
  plotText(painter, annotations);
  plotXLabels(painter);
  plotTitle(painter);
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
    if (mOptions->pDistance or mOptions->pGeoPos) {
      painter.setPen(vcross::util::QC(colourOrContrast(mOptions->distanceColour)));
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
  if (not mOptions->pSurface)
    return;

  if (not mSurface) {
    METLIBS_LOG_DEBUG("surface plot enabled, but surface data missing");
    return;
  }

  float last_x = 0;
  bool last_ok = false;

  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;

  const float vYMin = mAxisY->getValueMin(), vYMax = mAxisY->getValueMax();
  const bool up = vYMin < vYMax;

  painter.save();
  painter.setPen(Qt::NoPen);
  painter.setBrush(vcross::util::QC(mOptions->surfaceColour));

  QPolygonF polygon; // TODO set pen etc
  const int nx = mSurface->shape().length(Values::GEO_X);
  Values::ShapeIndex idx_surface(mSurface->shape());
  for (int ix=0; ix<nx; ++ix) {
    idx_surface.set(Values::GEO_X, ix);
    const float vx = distances.at(ix), p0 = mSurface->value(idx_surface);
    const float px = mAxisX->value2paint(vx);
    float py = mAxisY->value2paint(p0);
    bool this_ok = mAxisX->legalPaint(px);
    if (this_ok and not mAxisY->legalPaint(py)) {
      if ((up and p0 > vYMax) or (not up and p0 < vYMin))
        py = mAxisY->getPaintMax();
      else
        this_ok = false;
    }
    if (this_ok) {
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

  const int nzsteps = 9;
  const float zsteps[nzsteps] =
      { 10., 500., 1000., 2500., 5000., 10000, 15000, 20000, 25000 };

  const int nftsteps = 7;
  const float ftsteps[nftsteps] =
  { 1500., 3000., 8000., 15000., 30000., 50000, 60000 };

  int nticks = 0;
  const float *tickValues = 0, *tickLabels = 0;
  float scale = 1;

  if (mAxisY->quantity() == vcross::detail::Axis::PRESSURE) {
    tickValues = MetNo::Constants::pLevelTable;
    nticks = MetNo::Constants::nLevelTable;
    if (mAxisY->label() == "hPa") {
      tickLabels = MetNo::Constants::pLevelTable;
    } else if (mAxisY->label() == "FL") {
      tickLabels = MetNo::Constants::fLevelTable;
    } else {
      METLIBS_LOG_WARN("unknown y axis label '" << mAxisY->label() << "'");
      return;
    }
  } else if (mAxisY->quantity() == vcross::detail::Axis::ALTITUDE) {
    if (mAxisY->label() == "m") {
      nticks = nzsteps;
      tickValues = tickLabels = zsteps;
    } else if (mAxisY->label() == "Ft") {
      scale = MetNo::Constants::ft_per_m;
      nticks = nftsteps;
      tickValues = tickLabels = ftsteps;
    } else {
      METLIBS_LOG_WARN("unknown y axis label '" << mAxisY->label() << "'");
      return;
    }
  }
  if (nticks == 0 or not tickValues or not tickLabels) {
    METLIBS_LOG_WARN("missing y axis ticks or labels");
    return;
  }
  METLIBS_LOG_DEBUG(LOGVAL(nticks));

  const float tickLeftEnd = mAxisX->getPaintMin(), tickLeftStart = tickLeftEnd - 0.5*mCharSize.width();
  const float tickRightStart = mAxisX->getPaintMax(), tickRightEnd = tickRightStart + 0.5*mCharSize.width();
  METLIBS_LOG_DEBUG(LOGVAL(tickLeftStart) << LOGVAL(tickLeftEnd) << LOGVAL(tickRightStart) << LOGVAL(tickRightEnd));

  // colour etc. for frame etc.
  QPen pen(vcross::util::QC(colourOrContrast(mOptions->frameColour)), mOptions->frameLinewidth), penFrame(pen);
  vcross::util::setDash(penFrame, mOptions->frameLinetype);
  painter.setPen(penFrame);

  if (mOptions->pFrame) {
    // paint frame
    const QPointF min(mAxisX->getPaintMin(), mAxisY->getPaintMin()), max(mAxisX->getPaintMax(), mAxisY->getPaintMax());
    painter.drawRect(QRectF(min, max));

    // no stipple for ticks
    painter.setPen(pen);
  }

  if (mOptions->pLevelNumbers) {
    // paint tick marks
    for (int i=0; i<nticks; ++i) {
      const float tickValue = tickValues[i]/scale;
      const float tickY = mAxisY->value2paint(tickValue);
      if (mAxisY->legalPaint(tickY)) {
        painter.drawLine(QLineF(tickLeftStart,  tickY, tickLeftEnd,    tickY));
        painter.drawLine(QLineF(tickRightStart, tickY, tickRightEnd,   tickY));
      }
    }

    const bool unitFirst = (mAxisY->quantity() == vcross::detail::Axis::PRESSURE)
        && (mAxisY->label() == "FL");

    // paint tick labels
    const float labelLeftEnd = tickLeftEnd - mCharSize.width();
    const float labelRightStart = tickRightStart + mCharSize.width();
    for (int i=0; i<nticks; ++i) {
      const float tickValue = tickValues[i]/scale;
      const float tickY = mAxisY->value2paint(tickValue);
      if (mAxisY->legalPaint(tickY)) {
        const float tickLabel = tickLabels[i];
        std::ostringstream ostr;
        if (unitFirst)
          ostr << mAxisY->label();
        ostr << int(tickLabel);
        if (!unitFirst)
          ostr << mAxisY->label();
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

static bool isPlotOk(EvaluatedPlot_cp ep, int npoint, std::string& error)
{
  if (ep->argument_values.empty()) {
    error = "no argument_values, cannot plot";
    return false;
  } else if (not ep->z_values) {
    error = "no z_values, cannot plot";
    return false;
  }
  const int np_z = ep->z_values->shape().length(Values::GEO_X),
      np_v0 = ep->values(0)->shape().length(Values::GEO_X);
  if (np_z != npoint && np_z != 1) {
    error = (miutil::StringBuilder() << "unexpected z point count " << np_z
        << " != " << npoint << ", cannot plot").str();
  } else if (np_v0 != npoint) {
    error = (miutil::StringBuilder() << "unexpected v0 point count " << np_v0
        << " != " << npoint << ", cannot plot").str();
  } else {
    error = std::string();
    return true;
  }
  return false;
}

std::vector<std::string> QtPlot::plotData(QPainter& painter)
{
  METLIBS_LOG_SCOPE();

  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;
  const size_t npoint = distances.size();

  BOOST_FOREACH(OptionPlot_cp plot, mPlots) {
    EvaluatedPlot_cp ep = plot->evaluated;
    std::string error;
    if (isPlotOk(ep, npoint, error)) {
      // seems ok
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
    } else {
      METLIBS_LOG_ERROR(error);
    }
  }

  std::vector<std::string> annotations;
  BOOST_FOREACH(OptionPlot_cp plot, mPlots) {
    EvaluatedPlot_cp ep = plot->evaluated;
    std::string annotation, error;
    if (isPlotOk(ep, npoint, error)) {
      if (miutil::to_lower(plot->poptions.extremeType) == "value")
        annotation += plotDataExtremes(painter, plot);
    } else {
      // do not repeat error message here
    }
    annotations.push_back(annotation);
  }

  for (OptionLine_v::const_iterator itL = mLines.begin(); itL != mLines.end(); ++itL)
    plotDataLine(painter, *itL);

  return annotations;
}

void QtPlot::plotDataContour(QPainter& painter, OptionPlot_cp plot)
{
  METLIBS_LOG_SCOPE(LOGVAL(plot->name()));
  
  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;

  const QRect area(QPoint(mAxisX->getPaintMin(), mAxisY->getPaintMax()),
      QPoint(mAxisX->getPaintMax(), mAxisY->getPaintMin()));

  DianaLevels_p levels = dianaLevelsForPlotOptions(plot->poptions, detail::UNDEF_VALUE);
  const detail::VCAxisPositions positions(mAxisX, mAxisY, distances, plot->evaluated->z_values);
  vcross::detail::VCContourField con_field(plot->evaluated->values(0), *levels, positions);
  vcross::detail::VCLines con_lines(plot->poptions, *levels, painter, area);
  try {
    contouring::run(con_field, con_lines);
    con_lines.paint();
  } catch (contouring::too_many_levels& ex) {
    QPen pen(vcross::util::QC(plot->poptions.linecolour));
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawText(area.center(), "too many lines");
  }
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
  pv.setThickArrowScale(mOptions->thickArrowScale);
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

  const int ny = z_values->shape().length(Values::GEO_Z), nx = av0->shape().length(Values::GEO_X);
  Values::ShapeIndex idx_z(z_values->shape()), idx_av0(av0->shape()), idx_av1(av1->shape());
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
    idx_z.set(Values::GEO_X, ix);
    idx_av0.set(Values::GEO_X, ix);
    idx_av1.set(Values::GEO_X, ix);
    for (int iy=0; iy<ny; iy += 1) {
      idx_z.set(Values::GEO_Z, iy);
      idx_av0.set(Values::GEO_Z, iy);
      idx_av1.set(Values::GEO_Z, iy);
      const float vy = z_values->value(idx_z);
      const float py = mAxisY->value2paint(vy);
      const bool paintThisY = mAxisY->legalPaint(py)
          and ((not xStepAuto) or (not paintedY) or (std::abs(py - lastY) >= 2*pa.size()));
      if (paintThisY) {
        const float wx = av0->value(idx_av0), wy = av1->value(idx_av1);
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

// static
float QtPlot::absValue(OptionPlot_cp plot, int ix, int iy)
{
  const size_t n = plot->evaluated->argument_values.size();
  if (n == 0)
    return 0;
  Values::ShapeIndex idx_v0(plot->evaluated->values(0)->shape());
  idx_v0.set(Values::GEO_X, ix);
  idx_v0.set(Values::GEO_Z, iy);
  float v = plot->evaluated->values(0)->value(idx_v0);
  if (n == 1)
    return v;
  v *= v;
  for (size_t i=1; i<n; ++i) {
    Values::ShapeIndex idx_vi(plot->evaluated->values(i)->shape());
    idx_vi.set(Values::GEO_X, ix);
    idx_vi.set(Values::GEO_Z, iy);
    const float vi = plot->evaluated->values(i)->value(idx_vi);
    v += vi*vi;
  }
  return sqrt(v);
}

std::string QtPlot::formatExtremeAnnotationValue(float value, float y)
{
  std::string text = miutil::from_number(value) + " (";

  if (mAxisY->quantity() == vcross::detail::Axis::PRESSURE && mAxisY->label() == "FL") {
    const double a_m = MetNo::Constants::ICAO_geo_altitude_from_pressure(y);
    const double FL = MetNo::Constants::FL_from_geo_altitude(a_m);
    text += "FL" + miutil::from_number(FL);
  } else {
    if (mAxisY->quantity() == vcross::detail::Axis::ALTITUDE && mAxisY->label() == "Ft")
      y *= MetNo::Constants::ft_per_m;
    text += miutil::from_number(y) + mAxisY->label();
  }
  return text + ")";
}

std::string QtPlot::plotDataExtremes(QPainter& painter, OptionPlot_cp plot)
{
  METLIBS_LOG_SCOPE(LOGVAL(plot->name()));

  const Values_cp z_values = plot->evaluated->z_values;
  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;

  // step 1: loop through all values, find minimum / maximum visible value

  const int ny = z_values->shape().length(Values::GEO_Z), nx = z_values->shape().length(Values::GEO_X);
  Values::ShapeIndex idx_z(z_values->shape());
  bool have_max = false, have_min = false;
  float max_px = -1, max_py = -1, max_vy = 0, min_px = -1, min_py = -1, min_vy = 0;
  float max_v = 0, min_v = 0;
  for (int ix=0; ix<nx; ix += 1) {
    const float vx = distances.at(ix);
    const float px = mAxisX->value2paint(vx);
    
    if (not mAxisX->legalPaint(px))
      continue;

    idx_z.set(Values::GEO_X, ix);
    for (int iy=0; iy<ny; iy += 1) {
      idx_z.set(Values::GEO_Z, iy);
      const float vy = z_values->value(idx_z);
      const float py = mAxisY->value2paint(vy);
      if (not mAxisY->legalPaint(py))
        continue;

      const float v = absValue(plot, ix, iy);
      if (isnan(v))
        continue;

      if ((not have_max) or v > max_v) {
        have_max = true;
        max_px = px;
        max_py = py;
        max_vy = vy;
        max_v = v;
      }
      if ((not have_min) or v < min_v) {
        have_min = true;
        min_px = px;
        min_py = py;
        min_vy = vy;
        min_v = v;
      }
    }
  }

  // step 2 draw cross for min, point for max
  std::string annotation;
  if (have_max or have_min) {
    const QColor color(vcross::util::QC(plot->poptions.linecolour));
    painter.setPen(QPen(color, plot->poptions.linewidth));
    painter.setBrush(Qt::NoBrush);
    
    const float R = 9*plot->poptions.extremeSize, D = R*sqrt(2)/2;
    if (have_min) {
      painter.drawEllipse(QPointF(min_px, min_py), R, R);
      painter.drawLine(QPointF(min_px+D, min_py+D), QPointF(min_px-D, min_py-D));
      painter.drawLine(QPointF(min_px+D, min_py-D), QPointF(min_px-D, min_py+D));
      annotation += "min=" + formatExtremeAnnotationValue(min_v, min_vy);
    }
    if (have_max) {
      painter.drawEllipse(QPointF(max_px, max_py), R, R);
      painter.setBrush(color);
      painter.drawEllipse(QPointF(max_px, max_py), R/3, R/3);
      painter.setBrush(Qt::NoBrush);
      if (have_min)
        annotation += " ";
      annotation += "max=" + formatExtremeAnnotationValue(max_v, max_vy);
    }
  }
  return annotation;
}

void QtPlot::plotDataLine(QPainter& painter, const OptionLine& ol)
{
  METLIBS_LOG_SCOPE();
  if (not ol.linevalues) {
    METLIBS_LOG_DEBUG("no line values");
    return;
  }

  painter.save();
  QPen pen(vcross::util::QC(colourOrContrast(ol.linecolour)), ol.linewidth);
  vcross::util::setDash(pen, ol.linetype);
  painter.setPen(pen);

  const std::vector<float>& distances = isTimeGraph() ? mTimeDistances
      : mCrossectionDistances;

  QPolygonF polyline;
  const int nx = ol.linevalues->shape().length(Values::GEO_X);
  Values::ShapeIndex ol_idx(ol.linevalues->shape());
  for (int ix=0; ix<nx; ++ix) {
    ol_idx.set(Values::GEO_X, ix);
    const float vx = distances.at(ix), lv = ol.linevalues->value(ol_idx);
    const float px = mAxisX->value2paint(vx);
    float py = mAxisY->value2paint(lv);
    if (mAxisX->legalPaint(px) and mAxisY->legalPaint(py))
      polyline << QPointF(px, py);
    else if (polyline.size() >= 2) {
      painter.drawPolyline(polyline);
      polyline.clear();
    }
  }
  if (polyline.size() >= 2)
    painter.drawPolyline(polyline);
  painter.restore();
}

} // namespace vcross
