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
#ifndef VcrossQtPlot_h
#define VcrossQtPlot_h

#include "VcrossQtAxis.h"
#include "VcrossQtPaint.h"
#include <diField/VcrossData.h>
#include "VcrossEvaluate.h"
#include "VcrossOptions.h"

#include "diColour.h"
#include "diPlotOptions.h"

#include <puTools/miTime.h>

#include <QtCore/QRectF>

#include <vector>
#include <map>

namespace vcross {

//----------------------------------------------------

/// Posssible vertical types for Vertical Crossection plots
enum VcrossVertical {
  vcv_exner,
  vcv_pressure,
  vcv_height,
  vcv_none
};

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

    std::string model() const
      { return evaluated->model(); }
    std::string name() const
      { return evaluated->name(); }
    ConfiguredPlot::Type type() const
      { return evaluated->type(); }
  };
  typedef boost::shared_ptr<OptionPlot> OptionPlot_p;
  typedef boost::shared_ptr<const OptionPlot> OptionPlot_cp;
  typedef std::vector<OptionPlot_cp> OptionPlot_cpv;

  struct OptionLine {
    Values_cp linevalues;
    std::string linecolour;
    std::string linetype;
    float linewidth;

    OptionLine(Values_cp lv, const std::string& lc, const std::string& lt, float lw)
      : linevalues(lv), linecolour(lc), linetype(lt), linewidth(lw) { }
  };
  typedef std::vector<OptionLine> OptionLine_v;

public:
  QtPlot(VcrossOptions_p options);
  ~QtPlot();

  void viewZoomIn(int px1, int py1, int px2, int py2);
  void viewZoomOut();
  void viewPan(int pxmove, int pymove);
  void viewStandard();
  void viewSetWindow(int w, int h);
  void getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour);

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
  void setHorizontalCross(std::string csLabel, const LonLat_v& csPoints);
  void setHorizontalTime(const LonLat& tgPosition, const std::vector<miutil::miTime>& times);
  void setVerticalAxis();
  void setSurface(Values_cp s)
    { mSurface = s; }
  void addPlot(EvaluatedPlot_cp ep);
  void addLine(Values_cp linevalues, const std::string& linecolour, const std::string& linetype, float linewidth);
  void prepare();
  void plot(QPainter& painter);

private:
  bool plotBackground(const std::vector<std::string>& labels);

  void plotFrame(QPainter& painter);
  void plotVerticalGridLines(QPainter& painter);
  void plotSurface(QPainter& painter);
  void plotXLabels(QPainter& painter);
  void plotTitle(QPainter& painter);
  void plotText(QPainter& painter);

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

  void plotData(QPainter& painter);
  void plotDataExtremes(QPainter& painter, OptionPlot_cp plot);
  void plotDataContour(QPainter& painter, OptionPlot_cp plot);
  void plotDataArrow(QPainter& painter, OptionPlot_cp plot, const PaintArrow& pa, Values_cp av0, Values_cp av1);
  void plotDataWind(QPainter& painter, OptionPlot_cp plot);
  void plotDataVector(QPainter& painter, OptionPlot_cp plot);
  void plotDataLine(QPainter& painter, const OptionLine& ol);

  bool isTimeGraph() const
    { return not mTimePoints.empty(); }

  void updateCharSize(QPainter& painter);
  void calculateContrastColour();
  Colour colourOrContrast(const Colour& c) const
    { if (c == mBackColour) return mContrastColour; else return c; }
  Colour colourOrContrast(const std::string& ctxt) const
    { return colourOrContrast(Colour(ctxt)); }

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
  LonLat_v mCrossectionPoints;
  std::vector<float> mCrossectionDistances; //! distance in m from first point

  // for time graph
  std::vector<miutil::miTime> mTimePoints;
  std::vector<float> mTimeDistances; //! "distance" in minutes from start
  LonLat mTimeCSPoint;

  OptionPlot_cpv mPlots;
  Values_cp mSurface;

  OptionLine_v mLines;
};

typedef boost::shared_ptr<QtPlot> QtPlot_p;

} // namespace vcross

#endif // VcrossQtPlot_h
