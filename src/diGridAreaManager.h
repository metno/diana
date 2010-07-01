/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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

#ifndef DIGRIDAREAMANAGER_H_
#define DIGRIDAREAMANAGER_H_
#include <map>
#include <list>
#include <diMapMode.h>
#include <diField/diGridConverter.h>
#include <diField/diProjectablePolygon.h>
#include <diGridArea.h>
#include <puTools/miString.h>
#include <diField/diColour.h>

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#else
#include <miLogger/logger.h>
#endif

using namespace std;

class Point;
enum cursortype;

/**
	\brief Manager for GridAreas

	Manages a collection of GridAreas.
	Maintains a current area.
	Modifies the current area depending on the state (paint-mode).
*/
class GridAreaManager {

public:
    enum PaintMode{SELECT_MODE, DRAW_MODE, INCLUDE_MODE, CUT_MODE, MOVE_MODE,
      ADD_POINT, REMOVE_POINT, MOVE_POINT, SPATIAL_INTERPOLATION};

    class SpatialInterpolateArea {
    public:
      miutil::miString id;
      miutil::miString parent;
      miutil::miTime validTime;
      GridArea area;

      miutil::miString sortBy() const {return validTime.isoTime()+id;}

      friend bool operator>( const SpatialInterpolateArea& lhs,  const SpatialInterpolateArea& rhs)
      { return (lhs.sortBy() >  rhs.sortBy()); }
      friend bool operator<( const SpatialInterpolateArea& lhs,  const SpatialInterpolateArea& rhs)
      { return (lhs.sortBy() <  rhs.sortBy()); }
      friend bool operator==( const SpatialInterpolateArea& lhs, const SpatialInterpolateArea& rhs)
      { return (lhs.sortBy() == rhs.sortBy()); }

    };

private:
#ifndef NOLOG4CXX
    log4cxx::LoggerPtr logger;
#endif
	map<miutil::miString,GridArea> gridAreas;
	miutil::miString currentId;
  map<miutil::miString,GridArea> tmp_gridAreas;
  vector<SpatialInterpolateArea> spatialAreas;
  mapMode mapmode;
	GridConverter gc;   // gridconverter class
	float first_x;
	float first_y;
	float newx,newy;
	PaintMode paintMode;
	bool modeChanged;
//  Projection base_proj;
  bool hasinterpolated;
  cursortype getCurrentCursor();
	bool selectArea(Point p);
	void updateSelectedArea();
	void doSpatialInterpolation(const miutil::miString & movedId, float moveX, float moveY);
	void handleModeChanged(const mouseEvent& me, EventResult& res);
	void handleSelectEvent(const mouseEvent& me, EventResult& res,
	    const float& x, const float& y);
	void handleDrawEvent(const mouseEvent& me, EventResult& res,
	    const float& x, const float& y);
	void handleEditEvent(const mouseEvent& me, EventResult& res,
      const float& x, const float& y);
	void handleMoveEvent(const mouseEvent& me, EventResult& res,
      const float& x, const float& y, const float& first_x, const float& first_y);
  void handleMovePointEvent(const mouseEvent& me, EventResult& res,
      const float& x, const float& y, const float& first_x, const float& first_y);
  void handleAddPointEvent(const mouseEvent& me, EventResult& res,
      const float& x, const float& y);
  void handleRemovePointEvent(const mouseEvent& me, EventResult& res,
      const float& x, const float& y);
  void handleSpatialInterpolationEvent(const mouseEvent& me, EventResult& res,
      const float& x, const float& y, const float& first_x, const float& first_y);
public:
	GridAreaManager();
	~GridAreaManager();
	/// Replace all Grid-areas with specified areas
//	bool setGridAreas(map<miutil::miString,Polygon> newAreas, Projection currentProj );
	/// Returns current polygon
	ProjectablePolygon getCurrentPolygon();
	bool overrideMouseEvent;
  /// handling temporary areas
	void clearTemporaryAreas();
  void addOverviewArea(miutil::miString id, ProjectablePolygon area, Colour & colour);
  /// handling ghost areas for spatial interpolation
  void clearSpatialInterpolation();
  void addSpatialInterpolateArea(miutil::miString id, miutil::miString parent, miutil::miTime valid, ProjectablePolygon area);
  vector<SpatialInterpolateArea> getSpatialInterpolateAreas() const {return spatialAreas;}
  bool hasInterpolated() const {return hasinterpolated;}
  /// Setting current paint mode
	void setPaintMode(PaintMode mode);
	/// Returns current paint mode
	PaintMode getPaintMode() const;
	/// Adding new empty area with specified id (returns true if added)
	bool addArea(miutil::miString id);
	/// Adding specified area with specified id. (returns true if added)
	/// Current area changed if added!
	bool addArea(miutil::miString id, ProjectablePolygon area, bool overwrite);
	/// Replaces area if id exist. Current area not affected.
	bool updateArea(miutil::miString id, ProjectablePolygon area);
	/// Change area id (returns true if success, false if old ID not found)
	bool changeAreaId(miutil::miString oldId, miutil::miString newId);
	/// Get area with specified id (returns empty polygon if ID not found)
	ProjectablePolygon getArea(miutil::miString id);
	/// Removes area with specified id (returns true if removed)
	bool removeArea(miutil::miString id);
	/// Removes current area (returns true if removed)
	bool removeCurrentArea();
	/// Handle mouse event
	void sendMouseEvent(const mouseEvent& me, EventResult& res, float x, float y);
	/// Handle keyboard event
	void sendKeyboardEvent(const keyboardEvent& me, EventResult& res);
	/// Plotting current areas
	bool plot();
	/// true if undo is possible (history available)
	bool isUndoPossible();
  /// Perform undo. Returns true if success
  bool undo();
  /// true if redo is possible (history available)
  bool isRedoPossible();
  /// Perform redo. Returns true if success
  bool redo();
	/// Sets selected area to specified id. Returns false if id not found.
	bool setCurrentArea(miutil::miString id);
	/// True if current area exist
	bool hasCurrentArea();
	/// True if selected area is not empty (defined area selected)
	bool isAreaSelected();
	/// True if an empty area is selected. (returns false if no area selected)
	bool isEmptyAreaSelected();
	/// Returns selected id;
	miutil::miString getCurrentId();
	/// Returns current mode as string
	miutil::miString getModeAsString();
	/// Set plot enabled / disabled. Returns false if area id not found.
	bool setEnabled(miutil::miString id, bool enabled);
	/// Clear / Remove all areas. No current area.
	void clear();
	/// Set projection of active field
//	void setBaseProjection(Projection proj){base_proj = proj; }
	/// Get number of Areas
	int getAreaCount(){ return gridAreas.size(); }
  /// set the list of Points which are actually affected by the mask
  void setActivePoints(list<Point>);
  /// Returns id of all areas at p
  vector<miutil::miString> getId(Point p);
};


#endif /*DIGRIDAREAMANAGER_H_*/
