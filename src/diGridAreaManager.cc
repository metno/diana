#include <diGridAreaManager.h>
#include <iostream>
#include <AbstractablePolygon.h>
#include <diGridConverter.h>
#include <diGridArea.h>
#include <sstream>

GridAreaManager::GridAreaManager() : mapmode(normal_mode) {
	paintMode = SELECT_MODE;
#ifndef NOLOG4CXX
	logger = log4cxx::Logger::getLogger("diana.GridAreaManager");
#endif
	maxHistoryLength = 5;
	inDrawing = false;
	changeCursor = true;
	currentId = "";
	saveHistory();
}
GridAreaManager::~GridAreaManager(){	
}

bool GridAreaManager::setGridAreas(map<miString,Polygon> newAreas, Area currentProj ){
	LOG4CXX_DEBUG(logger,"setGridAreas ("<<newAreas.size()<<" areas)");
	if(currentProj.P().Gridtype() == 0)	return false;
	gridAreas.clear();
	map<miString,Polygon>::iterator iter = newAreas.begin();
	for(iter = newAreas.begin(); iter != newAreas.end(); iter++){
		LOG4CXX_DEBUG(logger,"setGridAreas adding "<<iter->first<<" : "<<iter->second.toString());
		GridArea newArea(iter->first,currentProj,iter->second);
		gridAreas.insert(make_pair(iter->first,newArea));
	}
	currentId = "";
	return true;
}

void GridAreaManager::clear(){
	gridAreas.clear();
	setCurrentArea("");
}

ProjectablePolygon GridAreaManager::getCurrentPolygon(){
	LOG4CXX_DEBUG(logger,"getCurrentPolygon (id ="<<currentId<<")");
	if(gridAreas.count(currentId))
		return gridAreas[currentId].getPolygon();
	return ProjectablePolygon();
}

ProjectablePolygon GridAreaManager::getArea(miString id){
	LOG4CXX_DEBUG(logger,"getArea("<<id<<")");
	if(gridAreas.count(id))
		return gridAreas[id].getPolygon();
	return ProjectablePolygon();
}

void GridAreaManager::sendMouseEvent(const mouseEvent& me, EventResult& res, float x, float y){
	newx = x;
	newy = y;
	if(changeCursor){
		LOG4CXX_DEBUG(logger,"Changing cursor to " << getCurrentCursor());
		res.newcursor = getCurrentCursor();
		changeCursor = false;
	}
	else{
		res.newcursor = keep_it;
	}
	if (me.type == mousepress){
		first_x = newx;
		first_y = newy;
		inDrawing = true;
		if (me.button == leftButton){
			if(paintMode==SELECT_MODE){
				miString prevId = currentId;
				selectArea(Point(newx,newy));
				LOG4CXX_DEBUG(logger,"Select area. Selected id = " << currentId);
				gridAreas[currentId].setMode(gridAreas[currentId].NORMAL);
				if(currentId != prevId){
					res.repaint = true;
					res.action=grid_area_changed;
				}
			}
			else if(paintMode==MOVE_MODE){
				if(gridAreas.count(currentId)){
					LOG4CXX_DEBUG(logger,"Starting move " << currentId);
					return gridAreas[currentId].startMove();
				}
			}
			else if(!gridAreas.count(currentId)){
				LOG4CXX_WARN(logger,getModeAsString() << 
					" not possible. No selected area " << currentId);
				return;
			}
			else if(paintMode==INCLUDE_MODE || paintMode==CUT_MODE){
				LOG4CXX_DEBUG(logger,"Starting edit " << currentId);
				gridAreas[currentId].startEdit(Point(newx,newy));
			}
			else if(paintMode==DRAW_MODE){
				LOG4CXX_DEBUG(logger,"Starting draw " << currentId);
				gridAreas[currentId].startDraw(Point(newx,newy));
			}
			res.repaint = true;
		}
	}
	else if(!gridAreas.count(currentId)){
		return;
	}
	else if (me.type == mousemove){
		if (me.button == leftButton){
			if(paintMode==MOVE_MODE){
				gridAreas[currentId].setMove((newx-first_x),(newy-first_y));
				res.repaint = true;
			}
			else if(paintMode != SELECT_MODE){
				gridAreas[currentId].addPoint(Point(newx,newy));
				res.repaint = true;
			}
		}
	} 
	else if (me.type == mouserelease){
  		if (me.button == leftButton){
			if(paintMode==SELECT_MODE){
				inDrawing = false;	
				return;
			}
			else if(paintMode==MOVE_MODE){
  				gridAreas[currentId].doMove();
				saveHistory();
  			}
  			else if(paintMode==INCLUDE_MODE){
				bool added = gridAreas[currentId].addEditPolygon();
				if(added)
					saveHistory();
  			}
  			else if(paintMode==CUT_MODE){
				bool deleted = gridAreas[currentId].deleteEditPolygon();
				if(deleted)
					saveHistory();
  			}
  			else if(paintMode==DRAW_MODE){
  				gridAreas[currentId].doDraw();
				saveHistory();
  			}
  			gridAreas[currentId].setMode(gridAreas[currentId].NORMAL);
			res.repaint= true;
			res.action=grid_area_changed;
  		}
		inDrawing = false;		
	} 
}

void GridAreaManager::setPaintMode(PaintMode mode){
	paintMode = mode;
	changeCursor = true;
	LOG4CXX_DEBUG(logger,"setPaintMode(" << getModeAsString() << ")");
}


GridAreaManager::PaintMode GridAreaManager::getPaintMode() const{
	return paintMode;
}

bool GridAreaManager::addArea(miString id){
	LOG4CXX_DEBUG(logger,"Adding empty area with id " << id);
	if(gridAreas.count(id)){
		setCurrentArea(id);
		LOG4CXX_WARN(logger,"Add area failed. Existing id " << id);
		return false;
	}
	GridArea newArea(id,base_proj);
	newArea.updateCurrentProjection();
	gridAreas[id] = newArea;
	saveHistory();
	bool foundNewArea = setCurrentArea(id);
	updateSelectedArea();
	if(!foundNewArea)
		LOG4CXX_ERROR(logger,"Unable to select added area " << id);
	return foundNewArea;
}

bool GridAreaManager::addArea(miString id, ProjectablePolygon area, bool overwrite){
	LOG4CXX_DEBUG(logger,"Adding " << id << ": " << area.toString());
	if(gridAreas.count(id) && !overwrite){
		setCurrentArea(id);
		LOG4CXX_WARN(logger,"Add area failed. Existing id " << id);
		return false;
	}
	GridArea newArea(id,area);
	newArea.updateCurrentProjection();
	gridAreas[id] = newArea;
	saveHistory();
	bool foundNewArea = setCurrentArea(id);
	if(!foundNewArea)
		LOG4CXX_ERROR(logger,"Unable to select added area " << id);
	return foundNewArea;
}

bool GridAreaManager::removeArea(miString id){
	if(gridAreas.count(id)){
		if(id == currentId)
			setCurrentArea("");
		gridAreas.erase(id);
		return true;
	}
	return false;
}

bool GridAreaManager::removeCurrentArea(){
	return removeArea(currentId);
}

bool GridAreaManager::changeAreaId(miString oldId, miString newId){
	if(gridAreas.count(oldId)) {
		gridAreas[newId] = gridAreas[oldId];
		gridAreas.erase(oldId);
		updateSelectedArea();
		return true;
	}
	return false;
}

bool GridAreaManager::setCurrentArea(miString id){
	if(!gridAreas.count(id)){ // no area with this id
		currentId = "";
		updateSelectedArea();
		return false;
	}
	currentId = id;
	updateSelectedArea();
	return true;
}

void GridAreaManager::sendKeyboardEvent(const keyboardEvent& me, EventResult& res){
	if(me.key == key_Shift){
		if(me.type == keypress)
			res.newcursor = normal_cursor;
		if(me.type == keyrelease)
			res.newcursor = getCurrentCursor();
		res.repaint = true;
	}
}

/**
 * Has to be called from PlotModule::plot()
 */
bool GridAreaManager::plot(){
	map<miString,GridArea>::iterator iter;
	for(iter = gridAreas.begin(); iter != gridAreas.end(); iter++){
		iter->second.plot();
	}
	return true;
}

miString GridAreaManager::getCurrentId(){
	return currentId;
}

bool GridAreaManager::selectArea(Point p){
	map<miString,GridArea>::iterator iter;
	for(iter = gridAreas.begin(); iter != gridAreas.end(); iter++){
		if(iter->second.inside(p)){
			setCurrentArea(iter->first);
			return true;
		}
	}
	return false;
}


void GridAreaManager::saveHistory(){
	history.push_front(gridAreas);
	if(history.size()>maxHistoryLength)
		history.pop_back();
}

bool GridAreaManager::isUndoPossible(){
	if(history.size()>1)
		return true;
	return false;
}

bool GridAreaManager::hasCurrentArea(){
	if(currentId == "")
		return false;
	if(!gridAreas.count(currentId)) // No current area
		return false;
	return true;
}

bool GridAreaManager::isAreaSelected(){
	if(!hasCurrentArea())
		return false;
	if(gridAreas[currentId].isEmptyArea())
		return false;
	return true;
}

bool GridAreaManager::isEmptyAreaSelected(){
	if(!hasCurrentArea())
		return false;
	if(gridAreas[currentId].isEmptyArea())
		return true;
	return false;
}

bool GridAreaManager::undo(){
	if(!isUndoPossible())
		return false;
	miString id = getCurrentId();
	history.pop_front(); // remove current
	gridAreas = history.front(); // set current to previous gridAreas
	return setCurrentArea(id); // will set currentId="" if not exist anymore
}

miString GridAreaManager::getModeAsString(){
	if(paintMode==SELECT_MODE)
		return miString("Select");
	else if(paintMode==MOVE_MODE)
		return miString("Move");
	else if(paintMode==INCLUDE_MODE)
		return miString("Include");
	else if(paintMode==CUT_MODE)
		return miString("Cut");
	else if(paintMode==DRAW_MODE)
		return miString("Draw");
}

cursortype GridAreaManager::getCurrentCursor(){
	if(paintMode==SELECT_MODE)
		return paint_select_cursor;
	else if(paintMode==MOVE_MODE)
		return paint_move_cursor;
	else 
		return paint_draw_cursor;
}


void GridAreaManager::updateSelectedArea(){
	map<miString,GridArea>::iterator iter;
	for(iter = gridAreas.begin(); iter != gridAreas.end(); iter++){
		if(iter->first == currentId){
			iter->second.setSelected(true);
		}
		else
			iter->second.setSelected(false);
	}
}

bool GridAreaManager::setEnabled(miString id, bool enabled){
	if(gridAreas.count(id)){
		gridAreas[id].enable(enabled);
		return true;
	}
	return false;
}


