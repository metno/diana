/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "diSatPlotBase.h"

#include "diArea.h"
#include "diSatTypes.h"

class Sat;
class SatManager;

/**
  \brief Plot satellite and radar images
*/
class SatPlot : public SatPlotBase
{
public:
  SatPlot(SatPlotCommand_cp cmd, SatManager* satm);
  ~SatPlot();

  void changeTime(const miutil::miTime& mapTime) override;
  void changeProjection(const Area& mapArea, const Rectangle& plotSize, const diutil::PointI& physSize) override;
  void plot(DiGLPainter* gl, PlotOrder zorder) override;
  void getAnnotation(std::string&, Colour&) const override;
  void getDataAnnotations(std::vector<std::string>& anno) const override;

  Sat* getData() { return satdata.get(); }

  const GridArea& getSatArea() override;
  void getCalibChannels(std::vector<std::string>& channels) const override;
  void values(float x, float y, std::vector<SatValues>& satval) const override;

private:
  SatPlot(const SatPlot& rhs) = delete;
  SatPlot& operator=(const SatPlot& rhs) = delete;

  bool hasData() const;
  void updateMapSatTransformation();

private:
  SatManager* satm_;
  std::unique_ptr<Sat> satdata;
  Projection mapP_;
  Transformation_cp mapT_;
};

#endif
