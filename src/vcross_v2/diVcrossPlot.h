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
#ifndef diVcrossPlot_h
#define diVcrossPlot_h

#include "diVcrossSetup.h"

#include "diColour.h"
#include "diPlotOptions.h"
#include "diPrintOptions.h"

#include <diField/diVcrossData.h>
#include <diField/diField.h>
#include <puTools/miTime.h>

#include <QtCore/QRectF>

#include <vector>
#include <map>

class FontManager;

//----------------------------------------------------

/// Posssible vertical types for Vertical Crossection plots
enum VcrossVertical {
  vcv_exner,
  vcv_pressure,
  vcv_height,
  vcv_none
};

class VcrossHardcopy;
class VcrossOptions;

namespace VcrossPlotDetail {
struct Axis;
typedef boost::shared_ptr<Axis> AxisPtr;
}

/**
   \brief Plots (prognostic) vertical crossection data or time graphs
*/
class VcrossPlot
{
public:
private:
  struct Plot {
    std::string modelName;
    std::string fieldName;
    VCPlotType type;
    VcrossData::values_t p0, p1;
    VcrossData::ZAxisPtr zax;
    PlotOptions poptions;
    Plot(const std::string& mn, const std::string& fn,
        VCPlotType t, const VcrossData::values_t& pp0, const VcrossData::values_t& pp1, VcrossData::ZAxisPtr z,
        const PlotOptions& po)
      : modelName(mn), fieldName(fn), type(t), p0(pp0), p1(pp1), zax(z), poptions(po) { }
  };
  typedef std::vector<Plot> Plots_t;

public:
  VcrossPlot(VcrossOptions* mOptions);
  ~VcrossPlot();

  void viewZoomIn(int px1, int py1, int px2, int py2);
  void viewZoomOut();
  void viewPan(int pxmove, int pymove);
  void viewStandard();
  void viewSetWindow(int w, int h);
  void getPlotSize(float& x1, float& y1, float& x2, float& y2, Colour& rubberbandColour);

  // postscript output
  bool startPSoutput(const printOptions& po);
  void nextPSpage();
  bool endPSoutput();

  int getNearestPos(int px);

  void clear();
  void setHorizontalCross(const std::string& csName, const VcrossData::Cut::lonlat_t& ll);
  void setHorizontalTime(const std::vector<miutil::miTime>& times, int csPoint);
  void setVerticalAxis(VcrossData::ZAxis::Quantity q);
  void addPlot(const std::string& model, const std::string& field,
      VCPlotType type, const VcrossData::values_t& p0, const VcrossData::values_t& p1, VcrossData::ZAxisPtr zax,
      const PlotOptions& poptions);
  void prepare();
  void plot();

private:
  bool plotBackground(const std::vector<std::string>& labels);
  void setFont();

  void plotFrame();
  void plotAnnotations(const std::vector<std::string>& labels);
  void plotVerticalMarkerLines();
  void plotMarkerLines();
  void plotVerticalGridLines();
  void plotSurfacePressure();
  void plotLevels();
  void plotXLabels();
  void plotTitle();
  void plotText();

  /*! Prepare axes' value ranges after all plots are added. */
  void prepareAxes();

  /*! Prepare view after changing plot or zoom. */
  void prepareView();

  /*! Compute maximal plot area (plotAreaMax), adjusting font size so
   *  that the plot area covers a resaonable part of the total
   *  area. */
  void computeMaxPlotArea();

  /*! Update axes to maintain aspect ratio */
  void prepareAxesForAspectRatio();

  void plotData();
  void plotDataContour(const Plot& plot);
  void plotDataWind(const Plot& plot);
  void plotDataVector(const Plot& plot);
  void plotDataLine(const Plot& plot);

  void calculateContrastColour();
  Colour colourOrContrast(const Colour& c) const
    { if (c == mBackColour) return mContrastColour; else return c; }
  Colour colourOrContrast(const std::string& ctxt) const
    { return colourOrContrast(Colour(ctxt)); }

  bool isTimeGraph() const
    { return not mTimePoints.empty(); }

  void updateCharSize();

private:
  std::auto_ptr<FontManager> fp;
  std::auto_ptr<VcrossHardcopy> hardcopy;
  VcrossOptions* mOptions; //! options, owned by VcrossManager

  QSizeF mTotalSize;  //! total plot area width (i.e. widget size)
  QRectF mPlotAreaMax;
  float  mFontSize;
  QSizeF mCharSize;
  Colour mBackColour;
  Colour mContrastColour;
  bool mViewChanged;

  VcrossPlotDetail::AxisPtr mAxisX, mAxisY;

  // for cross sections
  std::string mCrossectionName;
  VcrossData::Cut::lonlat_t mCrossectionPoints;
  std::vector<float> mCrossectionDistances; //! distance in m from first point

  // for time graph
  std::vector<miutil::miTime> mTimePoints;
  std::vector<float> mTimeDistances; //! "distance" in minutes from start
  int mTimeCSPoint;

  Plots_t mPlots;

  std::vector<std::string> markName;
  std::vector<float>    markNamePosMin;
  std::vector<float>    markNamePosMax;
};

#endif // diVcrossPlot_h
