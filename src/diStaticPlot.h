/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2019 met.no

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

#ifndef diStaticPlot_h
#define diStaticPlot_h

#include "diPlotArea.h"

#include "diColour.h"
#include "diField/diGridConverter.h"

#include <puTools/miTime.h>

class DiGLPainter;

/**
   StaticPlot keeps all previously static data shared by the various plotting classes.
*/
class StaticPlot
{
private:
  PlotArea pa_;
  Area requestedarea;        // Projection and size of requested grid
  miutil::miTime ctime;      // current time
  bool dirty;                // plotarea has changed
  int verticalLevel;         // current vertical level
  Colour backgroundColour;   // background colour
  Colour backContrastColour; // suitable contrast colour
  float gcd;                 // great circle distance

  bool panning;              // panning in progress

public:
  static GridConverter gc; // gridconverter class

public:
  StaticPlot();
  ~StaticPlot();

  const PlotArea& plotArea() const { return pa_; }

  //! Return current area on map.
  /*! Note: the rectangle is not the visible map area, which is available via getPlotSize().
   * \see getMapSize() getPlotSize()
   */
  const Area& getMapArea() const { return pa_.getMapArea(); }

  //! Get current projection of map.
  /*! \see getMapArea()
   */
  const Projection& getMapProjection() const { return getMapArea().P(); }

  /// set area
  bool setMapArea(const Area& area);

  /// with a new projection: find the best matching physical area with the current one
  Area findBestMatch(const Area&) const;

  /// convert from physical xy to geographic coordinates in lat-lon degrees
  bool PhysToGeo(float x, float y, float& lat, float& lon) const;

  /// convert from physical xy to geographic coordinates in lon-lat degrees
  XY PhysToGeo(const XY& phys) const { return MapToGeo(PhysToMap(phys)); }

  /// convert from geographic coordinates in lat-lon degrees to physical xy
  bool GeoToPhys(float lat, float lon, float& x, float& y) const;

  /// convert from geographic coordinates in lon-lat degrees to physical xy
  XY GeoToPhys(const XY& lonlat) const { return MapToPhys(GeoToMap(lonlat)); }

  void PhysToMap(float x, float y, float& xmap, float& ymap) const { PhysToMap(XY(x, y)).unpack(xmap, ymap); }
  void MapToPhys(float xmap, float ymap, float& x, float& y) const { MapToPhys(XY(xmap, ymap)).unpack(x, y); }

  XY PhysToMap(const XY& phys) const { return pa_.PhysToMap(phys); }
  XY MapToPhys(const XY& map) const { return pa_.MapToPhys(map); }

  /// convert from geographic coordinates in lon-lat degrees to map xy
  bool GeoToMap(int n, float* x, float* y) const;

  /** convert from geographic coordinates in lon-lat degrees to map xy
   * x and y must already be converted, e.g. with GeoToMap(int, float*, float*)
   * u and v will be converted
   */
  bool GeoToMap(int n, const float* x, const float* y, float* u, float* v) const;

  /// convert from geographic coordinates in lon-lat degrees to map xy
  XY GeoToMap(const XY& lonlatdeg) const;

  /// convert from map xy to geographic coordinates in lon-lat degrees
  bool MapToGeo(int n, float* x, float* y) const;

  /// convert from map xy to geographic coordinates in lon-lat degrees
  XY MapToGeo(const XY& map) const;

  bool ProjToMap(const Projection& srcProj, int n, float* x, float* y) const;

  bool MapToProj(const Projection& targetProj, int n, float* x, float* y) const;
  bool MapToProj(const Projection& targetProj, int n, diutil::PointD* xy) const;

  /// this is the area we really want
  void setRequestedarea(const Area& a) { requestedarea = a; }

  /// this is the area we really wanted
  const Area& getRequestedarea() const { return requestedarea; }

  //! Get the size of the map plot area / data grid, excluding \ref mapborder, in coordinates of getMapProjection().
  /*! This is the rectangle from getMapArea() extended to the aspect ratio of getPhysSize().
   */
  const Rectangle& getMapSize() const { return pa_.getMapSize(); }

  //! Get the size of the border of the map in units of getMapProjection() coordinates .
  /*! Note: this is projection dependent and thus not very useful.
   */
  float getMapBorder() const { return pa_.getMapBorder(); }

  //! Get the full size of the plot, including \ref mapborder, in coordinates of getMapProjection().
  /*! This is the rectangle from getMapSize() extended by \ref getMapBorder().
   */
  const Rectangle& getPlotSize() const { return pa_.getPlotSize(); }

  const XY& getPhysToMapScale() const { return pa_.getPhysToMapScale(); }

  float getPhysToMapScaleX() const { return pa_.getPhysToMapScale().x(); }

  float getPhysToMapScaleY() const { return pa_.getPhysToMapScale().y(); }

  /// set the physical size of the map in pixels
  bool setPhysSize(int w, int h);

  /// this is  the physical size of the map in pixels
  bool hasPhysSize() const { return pa_.hasPhysSize(); }

  int getPhysWidth() const { return pa_.getPhysSize().x(); }

  int getPhysHeight() const { return pa_.getPhysSize().y(); }

  const diutil::PointI& getPhysSize() const { return pa_.getPhysSize(); }

  float getPhysDiagonal() const;

  /// set the current data time
  void setTime(const miutil::miTime& t) { ctime = t; }

  /// return the current data time
  const miutil::miTime& getTime() const { return ctime; }

  /// set current vertical level
  void setVerticalLevel(int l) { verticalLevel = l; }

  /// this is the current vertical level
  int getVerticalLevel() const { return verticalLevel; }

  /// set name of background colour
  void setBgColour(const std::string& cn);

  /// return the current background colour
  const Colour& getBackgroundColour() const { return backgroundColour; }

  /// return colour with good contrast to background
  const Colour& getBackContrastColour() const { return backContrastColour; }

  /*! return another colour than the current background colour
   * Warning: may return c, therefore c must not be a temporary object
   */
  const Colour& notBackgroundColour(const Colour& c) const;

  float getGcd() const { return gcd; }

  //! Set flag indicating if panning is in progress.
  void setPanning(bool pan);

  //! Get flag indicating if panning is in progress.
  /*! \return true if currently panning
   */
  bool isPanning() const { return panning; }

private:
  /// set great circle distance
  void updateGcd();
};

#endif // diStaticPlot_h
