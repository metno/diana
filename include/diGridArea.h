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
#ifndef DIGRIDAREA_H_
#define DIGRIDAREA_H_


#include <vector>
#include <iostream>
#include <diPlot.h>
#include <diArea.h>
#include <AbstractablePolygon.h>
#include <miString.h>
#include <list>
#include <diProjectablePolygon.h>
#include <triangulation.h>

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#else
#include <miLogger/logger.h>
#endif

using namespace std;
/**
	\brief A plotable area (polygon) with projection independance

	A plotable polygon marking an area in diana.
	The area can be plotted under modification.
	The area maintains data in a original-projection while
	interacting in a current-projection.
*/
class GridArea : public Plot{

public:
	enum AreaMode{NORMAL,EDIT,MOVE,PAINTING,NODE_SELECT,NODE_MOVE};
  enum DrawStyle{DEFAULT,OVERVIEW,GHOST};

private:
  static int maxBuffer;
  static GLfloat nodeMarkRadius;
  static GLfloat nodeMarkMaxConstant;
  static double maxNodeSelectDistance;
#ifndef NOLOG4CXX
    log4cxx::LoggerPtr logger;
#endif
	//Selected area
	ProjectablePolygon polygon;
	//Displayed polygon
	Polygon displayPolygon;
	//Area to be added or deleted
	ProjectablePolygon editPolygon;
	//Displayed polygon to be added or deleted
	Polygon displayEditPolygon;
	// Last known location of the mouse pointer
	Point mousePoint;
	// Node currently in focus (used for highlighting)
	Point focusedNode;
	// Flag for node in focus
	bool nodeInFocus;
	// Display next point flag
	bool showNextPoint;

  list< ProjectablePolygon >  undobuffer;
  list< ProjectablePolygon >  redobuffer;

	bool selected;
	AreaMode mode;
	DrawStyle drawstyle;

	bool colours_defined;
  Colour fillcolour;

	void resetEditPolygon();
	void fillPolygon(Polygon & p,bool main);
	void fillActivePolygon(Polygon & p,bool main);
	void drawPolygon(Polygon & p,bool main);
	void drawNodes(const Polygon & p);
	//Used in move-mode to paint temp. moved polygon
	double moveX;
	double moveY;
	void init(Area org, Area current);
	// Add current polygon to undo buffer
  void saveChange();

public:
	GridArea();
	GridArea(string id);
	GridArea(string id, Area org_proj);
	GridArea(string id, ProjectablePolygon area);
	GridArea(string id, Area originalProjection, Polygon area);

  void setColour(Colour & fc);
  void setStyle(const DrawStyle & ds);
  DrawStyle getStyle() { return drawstyle; }

	ProjectablePolygon & getPolygon() ;
	///True if selected area is empty (no closed area selected)
	bool isEmptyArea();
	///True if selected edit-area is empty (no closed edit-area selected)
	bool isEmptyEditArea();
	///Adds point to selected area / edit-area depending on status
	bool addPoint(Point);
	///Paint current area
	bool plot();
	///Sets current area-mode (NORMAL,EDIT,MOVE)
	void setMode(AreaMode);
	///Gets current area-mode (NORMAL,EDIT,MOVE)
	AreaMode getMode();
	///Returns true if specified Point is inside this area
	bool inside(Point);
	///Start draw-session to select area
	void startDraw(Point startPoint);
	///End draw-session
	void doDraw();
	/// Set mouse-location
	void setMousePoint(const Point & p) { mousePoint = p; }
	/// Set point to be added (painting and editing mode)
	void setNextPoint(const Point & p) {
	  setMousePoint(p);
	  showNextPoint = true;
  }
	
	/// Set focused node. Returns true if changed
	bool setNodeFocus(const Point & mouse);
  ///Start node move session
  void startNodeMove();
  ///Move node to selected (setMove) position
  void doNodeMove();
	
	///Start move session
	void startMove();
	///Set difference between original position and new position
	void setMove(const double& x,const double& y);
	///Move area to selected (setMove) position
	void doMove();
	/// Remove focusedNode
	bool removeFocusedPoint();

	///Start edit (add/remove) session
	bool startEdit(Point startPoint);
	///Add selected editArea (returns true if success)
	bool addEditPolygon();
	///Delete (cut) selected editArea (returns true if success)
	bool deleteEditPolygon();
	///Reset area and status
	void reset();
	///Set area selected
	void setSelected(bool);
	///Check if area is selected
	bool isSelected();
	///Forced update to current diana projection
	void updateCurrentProjection();
  /// set the list of Points which are actually affected by the mask
  void setActivePoints(vector<Point>);
  /// true if undo is possible (buffer not empty)
  bool isUndoPossible();
  /// Perform undo. Returns true if success
  bool undo();
  /// true if redo is possible (buffer not empty)
  bool isRedoPossible();
  /// Perform redo. Returns true if success
  bool redo();
  ///Standard Projection
  static Area getStandardProjection();
};

#endif /*DIGRIDAREA_H_*/
