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
#ifndef diSatPlot_h
#define diSatPlot_h

#include "diPlotOptionsPlot.h"

#include "diRasterPlot.h"
#include "diSatTypes.h"

class Sat;
class SatPlotCommand;
typedef std::shared_ptr<const SatPlotCommand> SatPlotCommand_cp;

/**
  \brief Plot satellite and radar images
*/
class SatPlot : public PlotOptionsPlot, protected RasterPlot
{
public:
  SatPlot();
  ~SatPlot();

  Sat *satdata;

  void plot(DiGLPainter* gl, PlotOrder zorder) override;
  std::string getEnabledStateKey() const override;
  bool hasData() const override;

  void setData(Sat*);
  void setCommand(SatPlotCommand_cp cmd);
  SatPlotCommand_cp command() const { return command_; }

  GridArea& getSatArea();
  void getAnnotation(std::string &, Colour &) const override;
  void getCalibChannels(std::vector<std::string>& channels) const;
  ///get pixel value
  void values(float x, float y, std::vector<SatValues>& satval) const;
  ///get legend
  void getAnnotations(std::vector<std::string>& anno);
  void setSatAuto(bool, const std::string&, const std::string&);

protected:
  const PlotArea& rasterPlotArea() override;
  const GridArea& rasterArea() override;
  void rasterPixels(int n, const diutil::PointD& xy0, const diutil::PointD& dxy, QRgb* pixels) override;

private:
  SatPlot(const SatPlot &rhs);  // not implemented
  SatPlot& operator=(const SatPlot &rhs); // not implemented

  SatPlotCommand_cp command_;
};

#endif
