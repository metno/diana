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
#include <diColour.h>

#include <QKeyEvent>
#include <QMouseEvent>

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
      std::string id;
      std::string parent;
      miutil::miTime validTime;
      GridArea area;

      std::string sortBy() const {return validTime.isoTime()+id;}

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
	map<std::string,GridArea> gridAreas;
	std::string currentId;
  map<std::string,GridArea> tmp_gridAreas;
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
	void doSpatialInterpolation(const std::string & movedId, float moveX, float moveY);
	void handleModeChanged(QMouseEvent* me, EventResult& res);
	void handleSelectEvent(QMouseEvent* me, EventResult& res,
	    const float& x, const float& y);
	void handleDrawEvent(QMouseEvent* me, EventResult& res,
	    const float& x, const float& y);
	void handleEditEvent(QMouseEvent* me, EventResult& res,
      const float& x, const float& y);
	void handleMoveEvent(QMouseEvent* me, EventResult& res,
      const float& x, const float& y, const float& first_x, const float& first_y);
  void handleMovePointEvent(QMouseEvent* me, EventResult& res,
      const float& x, const float& y, const float& first_x, const float& first_y);
  void handleAddPointEvent(QMouseEvent* me, EventResult& res,
      const float& x, const float& y);
  void handleRemovePointEvent(QMouseEvent* me, EventResult& res,
      const float& x, const float& y);
  void handleSpatialInterpolationEvent(QMouseEvent* me, EventResult& res,
      const float& x, const float& y, const float& first_x, const float& first_y);
public:
	GridAreaManager();
	~GridAreaManager();
	/// Replace all Grid-areas with specified areas
//	bool setGridAreas(map<std::string,Polygon> newAreas, Projection currentProj );
	/// Returns current polygon
	ProjectablePolygon getCurrentPolygon();
	bool overrideMouseEvent;
  /// handling temporary areas
	void clearTemporaryAreas();
  void addOverviewArea(std::string id, ProjectablePolygon area, Colour & colour);
  /// handling ghost areas for spatial interpolation
  void clearSpatialInterpolation();
  void addSpatialInterpolateArea(std::string id, std::string parent, miutil::miTime valid, ProjectablePolygon area);
  vector<SpatialInterpolateArea> getSpatialInterpolateAreas() const {return spatialAreas;}
  bool hasInterpolated() const {return hasinterpolated;}
  /// Setting current paint mode
	void setPaintMode(PaintMode mode);
	/// Returns current paint mode
	PaintMode getPaintMode() const;
	/// Adding new empty area with specified id (returns true if added)
	bool addArea(std::string id);
	/// Adding specified area with specified id. (returns true if added)
	/// Current area changed if added!
	bool addArea(std::string id, ProjectablePolygon area, bool overwrite);
	/// Replaces area if id exist. Current area not affected.
	bool updateArea(std::string id, ProjectablePolygon area);
	/// Change area id (returns true if success, false if old ID not found)
	bool changeAreaId(std::string oldId, std::string newId);
	/// Get area with specified id (returns empty polygon if ID not found)
	ProjectablePolygon getArea(std::string id);
	/// Removes area with specified id (returns true if removed)
	bool removeArea(std::string id);
	/// Removes current area (returns true if removed)
	bool removeCurrentArea();
	/// Handle mouse event
	void sendMouseEvent(QMouseEvent* me, EventResult& res, float x, float y);
	/// Handle keyboard event
	void sendKeyboardEvent(QKeyEvent* ke, EventResult& res);
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
	bool setCurrentArea(std::string id);
	/// True if current area exist
	bool hasCurrentArea();
	/// True if selected area is not empty (defined area selected)
	bool isAreaSelected();
	/// True if an empty area is selected. (returns false if no area selected)
	bool isEmptyAreaSelected();
	/// Returns selected id;
	std::string getCurrentId();
	/// Returns current mode as string
	std::string getModeAsString();
	/// Set plot enabled / disabled. Returns false if area id not found.
	bool setEnabled(std::string id, bool enabled);
	/// Clear / Remove all areas. No current area.
	void clear();
	/// Set projection of active field
//	void setBaseProjection(Projection proj){base_proj = proj; }
	/// Get number of Areas
	int getAreaCount(){ return gridAreas.size(); }
  /// set the list of Points which are actually affected by the mask
  void setActivePoints(list<Point>);
  /// Returns id of all areas at p
  vector<std::string> getId(Point p);
};


#endif /*DIGRIDAREAMANAGER_H_*/
