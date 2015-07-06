/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006 met.no

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

#include "diPlot.h"

#include "diRasterPlot.h"
#include "diSat.h"

/**
  \brief Plot satellite and radar images
*/
class SatPlot : public Plot, protected RasterPlot {
public:
  SatPlot();
  ~SatPlot();

  Sat *satdata;

  void plot(DiGLPainter* gl, PlotOrder zorder);
  void setData(Sat *);
  void clearData();
  Area& getSatArea()
    { return satdata->area; }
  double getGridResolutionX() const
    { return satdata->area.resolutionX; }
  double getGridResolutionY() const
    { return satdata->area.resolutionY; }
  void getSatAnnotation(std::string &, Colour &);
  void getSatName(std::string &);
  void getCalibChannels(std::vector<std::string>& channels );
  ///get pixel value
  void values(float x,float y, std::vector<SatValues>& satval);
  ///get legend
  bool getAnnotations(std::vector<std::string>& anno);
  void setSatAuto(bool, const std::string&, const std::string&);

protected:
  StaticPlot* rasterStaticPlot() Q_DECL_OVERRIDE
    { return getStaticPlot(); }
  const GridArea& rasterArea() Q_DECL_OVERRIDE
    { return satdata->area; }
  QImage rasterScaledImage(const GridArea&, int scale) Q_DECL_OVERRIDE;

private:
  SatPlot(const SatPlot &rhs);  // not implemented
  SatPlot& operator=(const SatPlot &rhs); // not implemented
};

#endif
