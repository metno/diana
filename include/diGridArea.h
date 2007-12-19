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
	enum AreaMode{NORMAL,EDIT,MOVE};
	
private:
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
	bool selected;
	AreaMode mode;
	Area getStandardProjection();
	void resetEditPolygon();
	void fillPolygon(Polygon & p,bool main);
	void drawPolygon(Polygon & p,bool main);
	//Used in move-mode to paint temp. moved polygon
	double moveX;
	double moveY;
	void init(Area org, Area current);

	
public:
	GridArea();
	GridArea(string id);
        GridArea(string id, Area org_proj);
	GridArea(string id, ProjectablePolygon area);
	GridArea(string id, Area originalProjection, Polygon area);
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
	///Start move session
	void startMove();
	///Set difference between original position and new position
	void setMove(double x,double y);
	///Move area to selected (setMove) position
	void doMove();
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
};

#endif /*DIGRIDAREA_H_*/
