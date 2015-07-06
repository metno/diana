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
#ifndef diPlot_h
#define diPlot_h

#include "diColour.h"
#include "diPoint.h"
#include "diPlotOptions.h"
#include "diPrintOptions.h"

#include <diField/diArea.h>
#include <diField/diGridConverter.h>
#include <puTools/miTime.h>

#include <vector>

class DiCanvas;
class DiGLPainter;

#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE /* override in c++ 11 */
#endif

/**
   StaticPlot keeps all previously static data shared by the various plotting classes.
   - postscript generation initiated here
*/
class StaticPlot {
private:
  Area area;          // Projection and size of current grid
  Area requestedarea; // Projection and size of requested grid
  Rectangle maprect;  // Size of map plot area
  Rectangle plotsize; // Size of full plot area
  miutil::miTime ctime;       // current time
  XY mPhys;           // physical size of plotarea
  XY mPhysToMapScale; // ratio of plot size to physical size
  bool dirty;         // plotarea has changed
  int verticalLevel;          // current vertical level
  Colour backgroundColour;   // background colour
  Colour backContrastColour; // suitable contrast colour
  float gcd;          // great circle distance
  bool panning;       // panning in progress
  printerManager printman;   // printer manager

public:
  static GridConverter gc;   // gridconverter class

public:
  StaticPlot();
  ~StaticPlot();

  /// return current area on map
  const Area& getMapArea() const
    { return area; }

  /// set area
  void setMapArea(const Area&);

  /// with a new projection: find the best matching physical area with the current one
  Area findBestMatch(const Area&);

  /// convert from physical xy to geographic coordinates in lat-lon degrees
  bool PhysToGeo(float x, float y, float& lat, float& lon) const;

  /// convert from physical xy to geographic coordinates in lon-lat degrees
  XY PhysToGeo(const XY& phys) const
    { return MapToGeo(PhysToMap(phys)); }

  /// convert from geographic coordinates in lat-lon degrees to physical xy
  bool GeoToPhys(float lat, float lon, float& x, float& y) const;

  /// convert from geographic coordinates in lon-lat degrees to physical xy
  XY GeoToPhys(const XY& lonlat) const
    { return MapToPhys(GeoToMap(lonlat)); }

  void PhysToMap(float x, float y, float& xmap, float& ymap) const
    { PhysToMap(XY(x, y)).unpack(xmap, ymap); }
  void MapToPhys(float xmap, float ymap, float& x, float& y) const
    { MapToPhys(XY(xmap, ymap)).unpack(x, y); }

  XY PhysToMap(const XY& phys) const;
  XY MapToPhys(const XY& map) const;

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
  bool ProjToMap(const Area& srcArea, int n,
      const float* x, const float* y, float* u, float* v) const;
  bool MapToProj(const Projection& targetProj, int n, float* x, float* y) const;

  /// this is the area we really want
  void setRequestedarea(const Area &a)
    { requestedarea = a; }

  /// this is the area we really wanted
  const Area& getRequestedarea()
    { return requestedarea; }

  /// this is the full size of the plot in the current projection
  const Rectangle& getPlotSize() const
    { return plotsize; }

  /// set the  full size of the plot in the current projection
  void setPlotSize(const Rectangle& r);

  const XY& getPhysToMapScale() const
    { return mPhysToMapScale; }

  float getPhysToMapScaleX() const
    { return mPhysToMapScale.x(); }

  float getPhysToMapScaleY() const
    { return mPhysToMapScale.y(); }

  /// this is size of the data grid
  const Rectangle& getMapSize() const
    { return maprect; }

  /// set the size of the data grid
  void setMapSize(const Rectangle& r);

  /// set the physical size of the map in pixels
  void setPhysSize(float w, float h);

  /// this is  the physical size of the map in pixels
  bool hasPhysSize() const
    { return mPhys.x() > 0 && mPhys.y() > 0; }

  float getPhysWidth() const
    { return mPhys.x(); }

  float getPhysHeight() const
    { return mPhys.y(); }

  /// set the current data time
  void setTime(const miutil::miTime& t)
    { ctime= t; }

  /// return the current data time
  const miutil::miTime& getTime()
    { return ctime; }

  /// set current vertical level
  void setVerticalLevel(int l)
    { verticalLevel= l; }

  /// this is the current vertical level
  int getVerticalLevel()
    { return verticalLevel; }

  /// set name of background colour
  void setBgColour(const std::string& cn);

  /// return the current background colour
  const Colour& getBackgroundColour() const
    { return backgroundColour; }

  /// return colour with good contrast to background
  const Colour& getBackContrastColour() const
    { return backContrastColour; }

  /*! return another colour than the current background colour
   * Warning: may return c, therefore c must not be a temporary object
   */
  const Colour& notBackgroundColour(const Colour& c) const;

  /// mark this as 'redraw needed'
  void setDirty(bool dirty=true);

  /// is redraw needed
  bool getDirty()
    { return dirty; }

  /// set great circle distance
  void updateGcd(DiGLPainter* gl);

  float getGcd()
    { return gcd; }

  /// toggle panning
  void panPlot(bool pan);

  bool isPanning()
    { return panning; }

private:
  void updatePhysToMapScale();
};

/**
   \brief Ancestor of all map plotting classes
*/

class Plot {
public:
  Plot();

  // "implemented" below
  virtual ~Plot() = 0;

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

  /// set the plot info string
  void setPlotInfo(const std::string& pin);

  /// return n elements of the current plot info string
  std::string getPlotInfo(int n=0) const;

  /// return the elements given
  std::string getPlotInfo(const std::string& str) const;

  /// return true if right plot string
  bool plotInfoOK(const std::string& pin) const
    { return (pinfo == pin); }

  /// return the current PlotOptions
  const PlotOptions& getPlotOptions() const
    { return poptions; }

  /// set colour mode (rgb or color-index)
  void setColourMode(bool isrgb=true);

  /// return current colourmode
  bool getColourMode() const
    { return rgbmode; }

  /// set name of this plot object
  void setPlotName(const std::string& name)
    { plotname= name; }

  /// return name of this plot object
  std::string getPlotName() const
    { return plotname; }

protected:
  std::string pinfo;            // plotinfo
  PlotOptions poptions;

private:
  bool enabled;               // plot enabled
  bool rgbmode;               // rgb or colour-index mode
  std::string plotname;       // name of plot
};

inline Plot::~Plot()
{
}

#endif
