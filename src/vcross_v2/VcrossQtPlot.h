/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2013-2022 met.no

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
#ifndef VcrossQtPlot_h
#define VcrossQtPlot_h

#include "VcrossEvaluate.h"
#include "VcrossOptions.h"
#include "VcrossQtPaint.h"
#include "VcrossVerticalTicks.h"
#include "diColour.h"
#include "diPlotOptions.h"

#include "diField/VcrossData.h"

#include <puTools/miTime.h>

#include <QtCore/QRectF>

#include <vector>
#include <map>

namespace vcross {

namespace detail {
struct Axis;
typedef std::shared_ptr<Axis> AxisPtr;
typedef std::shared_ptr<const Axis> AxisCPtr;
} // namespace detail

//----------------------------------------------------

/**
   \brief Plots (prognostic) vertical crossection data or time graphs
*/
class QtPlot
{
private:
  struct OptionPlot {
    EvaluatedPlot_cp evaluated;
    PlotOptions poptions;

    OptionPlot(EvaluatedPlot_cp e);

    const std::string& model() const
      { return evaluated->model().model; }

    miutil::miTime reftime() const;

    const std::string& name() const
      { return evaluated->name(); }

    ConfiguredPlot::Type type() const
      { return evaluated->type(); }
  };
  typedef std::shared_ptr<OptionPlot> OptionPlot_p;
  typedef std::shared_ptr<const OptionPlot> OptionPlot_cp;
  typedef std::vector<OptionPlot_cp> OptionPlot_cpv;

  struct OptionLine {
    diutil::Values_cp linevalues;
    std::string linecolour;
    std::string linetype;
    float linewidth;

    OptionLine(diutil::Values_cp lv, const std::string& lc, const std::string& lt, float lw)
        : linevalues(lv)
        , linecolour(lc)
        , linetype(lt)
        , linewidth(lw)
    {
    }
  };
  typedef std::vector<OptionLine> OptionLine_v;

  struct OptionMarker {
    float position; //! fractional index, 2.75 meaning 75% of the way from 2 to 3
    float x, y; //! screen pixels
    std::string text;
    std::string colour;

    OptionMarker(float p, const std::string& t, const std::string& c)
      : position(p), x(-1), y(-1), text(t), colour(c) { }
    OptionMarker(float xx, float yy, const std::string& t, const std::string& c)
      : position(-1), x(xx), y(yy), text(t), colour(c) { }
  };
  typedef std::vector<OptionMarker> OptionMarker_v;

public:
  QtPlot(VcrossOptions_p options);
  ~QtPlot();

  void viewZoomIn(int px1, int py1, int px2, int py2);
  void viewZoomOut();
  void viewPan(int pxmove, int pymove);
  void viewStandard();
  void viewSetWindow(const QSize& size);
  const QSize& viewGetWindow() const { return mTotalSize; }
  void getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour);

  //! return axis position of point (x,y), formatted as text
  QString axisPosition(int x, int y);

  struct Rect {
    float x1, y1, x2, y2;
    Rect(float xx1, float yy1, float xx2, float yy2)
      : x1(xx1), y1(yy1), x2(xx2), y2(yy2) { }
    Rect()
      : x1(0), y1(0), x2(0), y2(0) { }
  };
  Rect viewGetCurrentZoom() const;
  void viewSetCurrentZoom(const Rect& r);

  int getNearestPos(int px);

  void clear(bool keepX=false, bool keepY=false);
  void setHorizontalCross(const std::string& csLabel, const miutil::miTime& csTime,
      const LonLat_v& csPoints, const LonLat_v& csPointsRequested);
  void setHorizontalTime(const LonLat& tgPosition, const std::vector<miutil::miTime>& times);
  bool setVerticalAxis();
  void setSurface(diutil::Values_cp s) { mSurface = s; }
  void addPlot(EvaluatedPlot_cp ep);
  void addLine(diutil::Values_cp linevalues, const std::string& linecolour, const std::string& linetype, float linewidth);
  void addMarker(float position, const std::string& text, const std::string& colour);
  void addMarker(float x, float y, const std::string& text, const std::string& colour);
  void setReferencePosition(float rp)
    { mReferencePosition = rp; }
  void prepare();
  void plot(QPainter& painter);

private:
  bool plotBackground(const std::vector<std::string>& labels);

  void plotFrame(QPainter& painter,
      const ticks_t& tickValues, tick_to_axis_f& tta);
  void plotVerticalGridLines(QPainter& painter);
  void plotHorizontalGridLines(QPainter& painter,
      const ticks_t& tickValues, tick_to_axis_f& tta);
  void plotSurface(QPainter& painter);
  void plotXLabels(QPainter& painter);
  void plotLegend(QPainter& painter);
  void plotTitle(QPainter& painter);
  void plotText(QPainter& painter, const std::vector<std::string>& annotations);

  /*! Prepare x axis value range after all plots are added. */
  void prepareXAxis();

  /*! Prepare y axis value range after all plots are added. */
  void prepareYAxis();
  void prepareYAxisRange();

  /*! Prepare view after changing plot or zoom. */
  void prepareView(QPainter& painter);

  /*! Compute maximal plot area (plotAreaMax), adjusting font size so
   *  that the plot area covers a resaonable part of the total
   *  area. */
  void computeMaxPlotArea(QPainter& painter);

  /*! Update axes to maintain aspect ratio */
  void prepareAxesForAspectRatio();

  std::vector<std::string> plotData(QPainter& painter);
  std::string plotDataExtremes(QPainter& painter, OptionPlot_cp plot);
  void plotDataContour(QPainter& painter, OptionPlot_cp plot);
  void plotDataArrow(QPainter& painter, OptionPlot_cp plot, const PaintArrow& pa, diutil::Values_cp av0, diutil::Values_cp av1);
  void plotDataWind(QPainter& painter, OptionPlot_cp plot);
  void plotDataVector(QPainter& painter, OptionPlot_cp plot);
  void plotDataVectorExample(QPainter& painter, OptionPlot_cp plot);
  void plotDataLine(QPainter& painter, const OptionLine& ol);

  bool isTimeGraph() const
    { return not mTimePoints.empty(); }

  void updateCharSize(QPainter& painter);
  void calculateContrastColour();
  Colour colourOrContrast(const Colour& c) const
    { if (c == mBackColour) return mContrastColour; else return c; }
  Colour colourOrContrast(const std::string& ctxt) const
    { return colourOrContrast(Colour(ctxt)); }

  static float absValue(OptionPlot_cp plot, int ix, int iy, bool timegraph);
  std::string formatExtremeAnnotationValue(float value, float y);

  float fractionalRequestedDistance(float p);
  float relative2screenx(const float& x);
  float relative2screeny(const float& y);

private:
  VcrossOptions_p mOptions;
  Colour mBackColour, mContrastColour;
  QSize mTotalSize;  //! total plot area width (i.e. widget size)
  QRectF mPlotAreaMax;
  float  mFontSize;
  QSizeF mCharSize;
  bool mViewChanged;
  bool mKeepX, mKeepY;

  vcross::detail::AxisPtr mAxisX, mAxisY;

  // for cross sections
  std::string mCrossectionLabel;
  miutil::miTime mCrossectionTime;
  LonLat_v mCrossectionPoints;
  LonLat_v mCrossectionPointsRequested;
  std::vector<float> mCrossectionDistances; //! distance in m from first point
  std::vector<float> mCrossectionBearings;  //! direction to next point
  std::vector<float> mRequestedDistances; //! distance in m from first point, only requested points

  // for time graph
  std::vector<miutil::miTime> mTimePoints;
  std::vector<float> mTimeDistances; //! "distance" in minutes from start
  LonLat mTimeCSPoint;

  OptionPlot_cpv mPlots;
  diutil::Values_cp mSurface;

  OptionLine_v mLines;
  OptionMarker_v mMarkers;
  float mReferencePosition; //! fractional index in requested points
};

typedef std::shared_ptr<QtPlot> QtPlot_p;

} // namespace vcross

#endif // VcrossQtPlot_h
