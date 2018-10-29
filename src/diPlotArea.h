/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2018 met.no

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

#ifndef diPlotArea_h
#define diPlotArea_h

#include "diField/diArea.h"
#include "diPoint.h"

class PlotArea
{
public:
  PlotArea();

  //! Return current area on map.
  /*! Note: the rectangle is not the visible map area, which is available via getPlotSize().
   * \see getMapSize() getPlotSize()
   */
  const Area& getMapArea() const { return area; }

  //! Get current projection of map.
  /*! \see getMapArea()
   */
  const Projection& getMapProjection() const { return getMapArea().P(); }

  /// set area
  /*! \returns true if something has actually changed
   */
  bool setMapArea(const Area&);

  //! Get the size of the map plot area / data grid, excluding \ref mapborder, in coordinates of getMapProjection().
  /*! This is the rectangle from getMapArea() extended to the aspect ratio of getPhysSize().
   */
  const Rectangle& getMapSize() const { return mapsize; }

  //! Get the size of the border of the map in units of getMapProjection() coordinates .
  /*! Note: this is projection dependent and thus not very useful.
   */
  float getMapBorder() const { return mapborder; }

  //! Get the full size of the plot, including \ref mapborder, in coordinates of getMapProjection().
  /*! This is the rectangle from getMapSize() extended by \ref getMapBorder().
   */
  const Rectangle& getPlotSize() const { return plotsize; }

  const XY& getPhysToMapScale() const { return mPhysToMapScale; }

  XY PhysToMap(const XY& phys) const;
  XY MapToPhys(const XY& map) const;

  /// set the physical size of the map in pixels
  /*! \returns true if something has actually changed
   */
  bool setPhysSize(int w, int h);

  /// this is  the physical size of the map in pixels
  const diutil::PointI& getPhysSize() const { return mPhys; }

  bool hasPhysSize() const { return getPhysSize().x() > 0 && getPhysSize().y() > 0; }

private:
  //! update mapsize, plotsize, mPhysToMapScale
  void PlotAreaSetup();

private:
  Area area;            // Projection and size of current grid
  diutil::PointI mPhys; // physical size of plotarea

  float mapborder; //!< size of border outside mapsize (\ref mapsize + \ref mapborder = \ref plotsize)

  // the following are updated in PlotAreaSetup
  Rectangle mapsize;  //!< size of map plot area, excluding \ref mapborder \see getMapSize()
  Rectangle plotsize; //!< size of the plot area, including \ref mapborder \see getPlotSize()
  XY mPhysToMapScale; // ratio of plot size to physical size
};

#endif // diPlotArea_h
