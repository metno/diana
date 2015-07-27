/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#ifndef RASTERPLOT_H
#define RASTERPLOT_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diField/diArea.h>
#include <QImage>
#include <boost/shared_array.hpp>

class DiPainter;
class StaticPlot;

class RasterPlot {
protected:
  RasterPlot();
  ~RasterPlot();

  //! must provide access to the map area
  virtual StaticPlot* rasterStaticPlot() = 0;

  //! must provide access to the raster data grid
  virtual const GridArea& rasterArea() = 0;

  //! must be implmented to create an image of the raster data matching scaledArea()
  virtual QImage rasterScaledImage(const GridArea& scar, int scale) = 0;

  void rasterClear();

  // update image if necessary, then paint
  void rasterPaint(DiPainter* gl);

private:
  int calculateScaleFactor();
  void updateImage();
  void getGridPoints();
  void checkVisible(int& ix1, int& ix2, int& iy1, int& iy2);

private:
  Projection mMapProjection;
  bool mSimilarProjection;
  GridArea mAreaScaled;
  QImage mImageScaled;
  int mScaleFactor;

  size_t mNPositions;
  boost::shared_array<float> mPositionsXY;
};

#endif // RASTERPLOT_H
