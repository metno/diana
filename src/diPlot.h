/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diColour.h"
#include "diPoint.h"
#include "diPlotOptions.h"

#include <diField/diArea.h>
#include <diField/diGridConverter.h>
#include <puTools/miTime.h>

#include <vector>

class DiCanvas;
class DiGLPainter;
class StaticPlot;

/**
   \brief Ancestor of all map plotting classes
*/
class Plot {
public:
  Plot();
  virtual ~Plot();

  bool operator==(const Plot &rhs) const;

  StaticPlot* getStaticPlot() const;

  enum PlotOrder {
    BACKGROUND,
    SHADE_BACKGROUND,
    SHADE,
    LINES_BACKGROUND,
    LINES,
    OVERLAY
  };

  virtual void setCanvas(DiCanvas* canvas);

  virtual void plot(DiGLPainter* gl, PlotOrder zorder) = 0;

  /// enable this plot object
  void setEnabled(bool enable=true);

  /// is this plot object enabled
  bool isEnabled() const
    { return enabled; }

  /// key identifiying plot for remembering enabled/disabled state
  virtual std::string getEnabledStateKey() const;

  /// set the plot info string
  void setPlotInfo(const miutil::KeyValue_v& kvs);

  /// return current plot info string
  const miutil::KeyValue_v& getPlotInfo() const
    { return ooptions; }

  /// return n elements of the current plot info string
  miutil::KeyValue_v getPlotInfo(int n) const;

  /// return the current PlotOptions
  const PlotOptions& getPlotOptions() const
    { return poptions; }

  /// set name of this plot object
  void setPlotName(const std::string& name)
    { plotname= name; }

  /// return name of this plot object
  const std::string& getPlotName() const
    { return plotname; }

  virtual void getAnnotation(std::string &str, Colour &col) const;

protected:
  PlotOptions poptions;
  miutil::KeyValue_v ooptions;

private:
  bool enabled;               // plot enabled
  std::string plotname;       // name of plot
};

#endif
