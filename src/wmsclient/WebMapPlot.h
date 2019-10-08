/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2015-2019 met.no

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

#ifndef WebMapPlot_h
#define WebMapPlot_h 1

#include "diPlot.h"

#include "diArea.h"
#include "diTimeTypes.h"

#include <QObject>

class WebMapLayer;
class WebMapRequest;
class WebMapService;

namespace diutil {
class SimpleColourTransform;
} // namespace diutil

class WebMapPlot : public QObject, public Plot
{
  Q_OBJECT;

public:
  WebMapPlot(WebMapService* service, const std::string& layer);
  ~WebMapPlot();

  void plot(DiGLPainter* gl, PlotOrder porder) override;

  void changeProjection(const Area& mapArea, const Rectangle& plotSize) override;

  /*! select time; if invalid or not found, use default time; ignored
   *  if no time dimension */
  void changeTime(const miutil::miTime& time) override;

  void getAnnotation(std::string& str, Colour& col) const override;

  std::string getEnabledStateKey() const override;

  WebMapService* service() const { return mService; }

  std::string title() const;

  std::string attribution() const;

  /* set time tolerance in seconds */
  void setTimeTolerance(int tolerance)
    { mTimeTolerance = tolerance; }

  /* set time offset in seconds */
  void setTimeOffset(int offset)
    { mTimeOffset = offset; }

  void setCRS(const std::string& crs)
    { mCRS = crs; }

  void setStyleAlpha(float offset, float scale);

  void setStyleGrey(bool makeGrey);

  void setPlotOrder(PlotOrder po);

  size_t countDimensions() const;

  const std::string& dimensionTitle(size_t idx) const;

  const std::vector<std::string>& dimensionValues(size_t idx) const;

  /*! index of time dimension; < 0 if no time dimension */
  int timeDimension() const { return mTimeDimensionIdx; }

  void setDimensionValue(const std::string& dimId, const std::string& dimValue);

  /*! set to a fixed time; empty == no fixed time */
  void setFixedTime(const std::string& time);

  //! get a list of times for this plot
  plottimes_t getTimes();

Q_SIGNALS:
  void update();

private Q_SLOTS:
  void serviceRefreshStarting();
  void serviceRefreshFinished();
  void requestCompleted();

private:
  void dropRequest();
  void findLayerAndTimeDimension();

private:
  WebMapService* mService;
  std::string mLayerId;
  const WebMapLayer* mLayer;

  int mTimeDimensionIdx; //!< index of time dimension in layer's dimension list
  int mTimeSelected;
  int mTimeTolerance; // time tolerance in seconds
  int mTimeOffset; // time offset in seconds
  std::string mFixedTime;

  int mLegendOffsetX; // legend position x offset (<0 from left, >0 from right, ==0 off)
  int mLegendOffsetY; // legend position y offset (<0 from bottom, >0 from top, ==0 off)

  std::string mCRS;
  float mAlphaOffset, mAlphaScale;
  bool mMakeGrey;
  PlotOrder mPlotOrder;

  std::map<std::string, std::string> mDimensionValues;
  typedef std::map<miutil::miTime, size_t> TimeDimensionValues_t;
  TimeDimensionValues_t mTimeDimensionValues; //!< maps time value to index along time dimension

  WebMapRequest* mRequest;
  bool mRequestCompleted;

  Area mOldArea;
};

#endif // WebMapPlot_h
