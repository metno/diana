/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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
#ifndef diPlot_h
#define diPlot_h

#include "diPlotOrder.h"
#include "diPlotStatus.h"
#include "diPoint.h"

#include <puTools/miTime.h>

#include <string>
#include <vector>

class Area;
class Colour;
class DiCanvas;
class DiGLPainter;
class Rectangle;
class StaticPlot;

/**
   \brief Ancestor of all map plotting classes
*/
class Plot {
public:
  Plot();
  virtual ~Plot();

  bool operator==(const Plot &rhs) const;

  virtual void setCanvas(DiCanvas* canvas);

  virtual void plot(DiGLPainter* gl, PlotOrder zorder) = 0;

  virtual void changeProjection(const Area& mapArea, const Rectangle& plotSize, const diutil::PointI& physSize);
  virtual void changeTime(const miutil::miTime& newTime);
  PlotStatusValue getStatus() const { return status_; }
  virtual void getAnnotation(std::string& str, Colour& col) const;
  virtual void getDataAnnotations(std::vector<std::string>& anno) const;

  /// key identifiying plot for remembering enabled/disabled state
  virtual std::string getEnabledStateKey() const = 0;

  const StaticPlot* getStaticPlot() const;

  /// enable this plot object
  virtual void setEnabled(bool enable=true);

  /// is this plot object enabled
  bool isEnabled() const
    { return enabled; }

  /// set name of this plot object
  void setPlotName(const std::string& name)
    { plotname= name; }

  /// return name of this plot object
  virtual const std::string& getPlotName() const;

  /// return name of this plot object
  virtual const std::string& getIconName() const;

protected:
  void setStatus(PlotStatusValue ps);

private:
  bool enabled;               // plot enabled
  std::string plotname;       // name of plot
  PlotStatusValue status_;
};

#endif
