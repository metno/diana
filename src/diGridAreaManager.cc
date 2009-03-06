#include <diGridAreaManager.h>
#include <iostream>
#include <AbstractablePolygon.h>
#include <propoly/Point.h>
#include <diGridConverter.h>
#include <sstream>

GridAreaManager::GridAreaManager() :
  mapmode(normal_mode), hasinterpolated(false) {
  paintMode = SELECT_MODE;
#ifndef NOLOG4CXX
  logger = log4cxx::Logger::getLogger("diana.GridAreaManager");
#endif
  overrideMouseEvent = false;
  modeChanged = true;
  currentId = "";
  base_proj = GridArea::getStandardProjection();
}
GridAreaManager::~GridAreaManager() {
}

bool GridAreaManager::setGridAreas(map<miString,Polygon> newAreas,
    Area currentProj) {
  LOG4CXX_DEBUG(logger,"setGridAreas ("<<newAreas.size()<<" areas)");
  if (currentProj.P().Gridtype() == 0)
    return false;
  gridAreas.clear();
  map<miString,Polygon>::iterator iter = newAreas.begin();
  for (iter = newAreas.begin(); iter != newAreas.end(); iter++) {
    LOG4CXX_DEBUG(logger,"setGridAreas adding "<<iter->first<<" : "<<iter->second.toString());
    GridArea newArea(iter->first, currentProj, iter->second);
    gridAreas.insert(make_pair(iter->first, newArea));
  }
  currentId = "";
  return true;
}

void GridAreaManager::clear() {
  gridAreas.clear();
  tmp_gridAreas.clear();
  setCurrentArea("");
}

ProjectablePolygon GridAreaManager::getCurrentPolygon() {
  LOG4CXX_DEBUG(logger,"getCurrentPolygon (id ="<<currentId<<")");
  if (gridAreas.count(currentId))
    return gridAreas[currentId].getPolygon();
  return ProjectablePolygon();
}

ProjectablePolygon GridAreaManager::getArea(miString id) {
  LOG4CXX_DEBUG(logger,"getArea("<<id<<")");
  if (gridAreas.count(id))
    return gridAreas[id].getPolygon();
  return ProjectablePolygon();
}

void GridAreaManager::sendMouseEvent(const mouseEvent& me, EventResult& res,
    float x, float y) {
  handleModeChanged(me, res);

  if (me.type == mousepress && me.button == leftButton) {
    first_x = x;
    first_y = y;
    overrideMouseEvent = true;
  }
  
  if (paintMode == SELECT_MODE) {
    handleSelectEvent(me, res, x, y);
  } else if (!gridAreas.count(currentId)) {
    LOG4CXX_WARN(logger,getModeAsString() << " not possible (no area)");
    return;
  }
  if (paintMode == DRAW_MODE) {
    handleDrawEvent(me, res, x, y);
  } else if (paintMode == INCLUDE_MODE || paintMode == CUT_MODE) {
    handleEditEvent(me, res, x, y);
  } else if (paintMode == MOVE_MODE) {
    handleMoveEvent(me, res, x, y, first_x, first_y);
  } else if (paintMode == ADD_POINT) {
    handleAddPointEvent(me, res, x, y);
  } else if (paintMode == REMOVE_POINT) {
    handleRemovePointEvent(me, res, x, y);
  } else if (paintMode == MOVE_POINT) {
    handleMovePointEvent(me, res, x, y, first_x, first_y);
  } else if (paintMode == SPATIAL_INTERPOLATION) {
    handleSpatialInterpolationEvent(me, res, x, y, first_x, first_y);
  }
}


void GridAreaManager::handleModeChanged(const mouseEvent& me, EventResult& res) {
  if (modeChanged) {
    LOG4CXX_DEBUG(logger,"Changing cursor to " << getCurrentCursor());
    res.newcursor = getCurrentCursor();
    modeChanged = false;
    if (gridAreas.count(currentId) && me.button == noButton) {
      if (paintMode == REMOVE_POINT || paintMode == MOVE_POINT) {
        gridAreas[currentId].setMode(GridArea::NODE_SELECT);
      } else if (paintMode == ADD_POINT) {
        gridAreas[currentId].setMode(GridArea::NODE_INSERT);
      } else { 
        gridAreas[currentId].setMode(GridArea::NORMAL);
      }
    }
    LOG4CXX_DEBUG(logger,"Paint mode = " << getModeAsString());
    LOG4CXX_DEBUG(logger,"Area mode = " << gridAreas[currentId].getMode());
  } else {
    res.newcursor = keep_it;
  }
}

void GridAreaManager::handleSelectEvent(const mouseEvent& me, EventResult& res,
    const float& x, const float& y)
{
  if (me.type == mousepress && me.button == rightButton) {
    res.action = rightclick;
  } else if (me.type == mousepress && me.button == leftButton) {
    miString prevId = currentId;
    selectArea(Point(x, y));
    LOG4CXX_DEBUG(logger,"Selected area: " << currentId);
    gridAreas[currentId].setMode(GridArea::NORMAL);
    if (currentId != prevId) {
      res.repaint = true;
      res.action=grid_area_changed;
    }
  } else if (me.type == mouserelease) {
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleDrawEvent(const mouseEvent& me, EventResult& res,
    const float& x, const float& y)
{
  if (me.type == mousepress && me.button == leftButton) {
    if(gridAreas[currentId].getMode() == GridArea::NORMAL) {
      LOG4CXX_DEBUG(logger,"Starting draw " << currentId);
      gridAreas[currentId].startDraw(Point(x, y));
    } else if (gridAreas[currentId].getMode() == GridArea::PAINTING) {
      gridAreas[currentId].addPoint(Point(x, y));
    }
    res.repaint = true;
  } else if (me.type == mousemove && me.button == leftButton) {
    gridAreas[currentId].addPoint(Point(x, y));
    res.repaint = true;
  } else if (me.type == mousemove && me.button != leftButton) {
    gridAreas[currentId].setNextPoint(Point(x, y));
    res.repaint = true;
  } else if (me.type == mousepress && me.button == rightButton) {
    gridAreas[currentId].addPoint(Point(x, y));
    gridAreas[currentId].doDraw();
    res.repaint = true;
    res.action = grid_area_changed;
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleEditEvent(const mouseEvent& me, EventResult& res,
    const float& x, const float& y)
{
  if (me.type == mousepress && me.button == leftButton) {
    if(gridAreas[currentId].getMode() == GridArea::NORMAL) {
      LOG4CXX_DEBUG(logger,"Starting edit " << currentId);
      gridAreas[currentId].startEdit(Point(x, y));
    } else if (gridAreas[currentId].getMode() == GridArea::EDIT) {
      gridAreas[currentId].addPoint(Point(x, y));
    }
    res.repaint = true;
  } else if (me.type == mousemove && me.button == leftButton) {
    gridAreas[currentId].addPoint(Point(x, y));
    res.repaint = true;
  } else if (me.type == mousemove && me.button != leftButton) {
    gridAreas[currentId].setNextPoint(Point(x, y));
    res.repaint = true;
  } else if (me.type == mousepress && me.button == rightButton) {
    if (paintMode == INCLUDE_MODE) {
      gridAreas[currentId].addPoint(Point(x, y));
      gridAreas[currentId].addEditPolygon();
    } else if (paintMode == CUT_MODE) {
      gridAreas[currentId].addPoint(Point(x, y));
      gridAreas[currentId].deleteEditPolygon();
    }
    res.repaint = true;
    res.action = grid_area_changed;
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleMoveEvent(const mouseEvent& me, EventResult& res,
    const float& x, const float& y, const float& first_x, const float& first_y)
{
  if (me.type == mousepress && me.button == leftButton) {
    LOG4CXX_DEBUG(logger,"Starting move " << currentId);
    gridAreas[currentId].startMove();
    res.repaint = true;
  } else if (me.type == mousemove && me.button == leftButton) {
    gridAreas[currentId].setMove((x-first_x), (y-first_y));
    res.repaint = true;
  } else if (me.type == mouserelease && me.button == leftButton) {
    gridAreas[currentId].doMove();
    res.repaint = true;
    res.action = grid_area_changed;
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleMovePointEvent(const mouseEvent& me, EventResult& res,
    const float& x, const float& y, const float& first_x, const float& first_y)
{
  if (me.type == mousepress && me.button == leftButton) {
    LOG4CXX_DEBUG(logger,"Starting move point " << currentId);
    gridAreas[currentId].startNodeMove();
    res.repaint = true;
  } else if (me.type == mousemove && me.button == leftButton) {
    gridAreas[currentId].setMove((x-first_x), (y-first_y));
    res.repaint = true;
  } else if (me.type == mousemove && me.button != leftButton) {
    if(gridAreas[currentId].setNodeFocus(Point(x, y))) // focus changed
      res.repaint = true;
  } else if (me.type == mouserelease && me.button == leftButton) {
    gridAreas[currentId].doNodeMove();
    res.repaint = true;
    res.action = grid_area_changed;
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleSpatialInterpolationEvent(const mouseEvent& me, EventResult& res,
    const float& x, const float& y, const float& first_x, const float& first_y)
{
  if (me.type == mousepress && me.button == leftButton) {
    LOG4CXX_DEBUG(logger,"Starting Spatial Interpolation");
    gridAreas[currentId].startMove();
    res.repaint = true;
  } else if (me.type == mousemove && me.button == leftButton) {
    doSpatialInterpolation(currentId, (x-first_x), (y-first_y));
    res.repaint = true;
  } else if (me.type == mouserelease && me.button == leftButton) {
    vector<SpatialInterpolateArea>::iterator gitr;
    for (gitr=spatialAreas.begin(); gitr!=spatialAreas.end(); gitr++){
      gitr->area.doMove();
    }
    gridAreas[currentId].doMove();
    hasinterpolated = true;
    res.repaint = true;
    res.action = grid_area_changed;
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleAddPointEvent(const mouseEvent& me, EventResult& res,
    const float& x, const float& y)
{
  if (me.type == mousepress && me.button == leftButton) {
    gridAreas[currentId].doNodeInsert();
    res.repaint = true;
    res.action = grid_area_changed;
  } else if (me.type == mousemove) {
    if(gridAreas[currentId].setNodeInsertFocus(Point(x, y))) { // focus changed
      res.newcursor = paint_add_crusor;
    } else res.newcursor = paint_forbidden_crusor;
    res.repaint = true;
  } else if (me.type == mouserelease && me.button == leftButton) {
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleRemovePointEvent(const mouseEvent& me, EventResult& res,
    const float& x, const float& y)
{
  if (me.type == mousepress && me.button == leftButton) {
    LOG4CXX_DEBUG(logger,"Remove point from " << currentId);
    if(gridAreas[currentId].removeFocusedPoint()) {
      res.repaint = true;
      res.action = grid_area_changed;
    }
  } else if (me.type == mousemove) {
    if(gridAreas[currentId].setNodeFocus(Point(x, y))) // focus changed
      res.repaint = true;
  } else if (me.type == mouserelease && me.button == leftButton) {
    overrideMouseEvent = false;
  }
}

void GridAreaManager::doSpatialInterpolation(const miString & movedId, float moveX, float moveY) {
  if (spatialAreas.empty()) return;
  GridArea & movedArea = gridAreas[movedId];
  movedArea.setMove(moveX, moveY);
  Point movedCenter = movedArea.getPolygon().getCenterPoint();
  movedCenter.move(moveX, moveY); // not moved with GridArea::setMove

  int n = 0; // index to current area
  vector<SpatialInterpolateArea>::iterator gitr;
  for (gitr=spatialAreas.begin(); gitr!=spatialAreas.end(); gitr++){
    if (gitr->id == movedId) break;
    ++n;
  }
  int m = 0; // index to parent area
  for (gitr=spatialAreas.begin(); gitr!=spatialAreas.end(); gitr++){
    if (gitr->parent == "") break;
    ++m;
  }
  if (n == m) {
     LOG4CXX_ERROR(logger, "doSpatialInterpolation failed - trying to move parent");
     return;
  }
  Point parentCenter = spatialAreas[m].area.getPolygon().getCenterPoint();
  int d = abs(n-m); // number of time units between parent and current area
  Point unit = ( ( movedCenter - parentCenter) / d);

  gitr = spatialAreas.begin();
  int i = -m; // number of time units from start to parent
  for (gitr=spatialAreas.begin(); gitr!=spatialAreas.end();++gitr,++i){
    Point p = (i * unit); // displacement from parent
    Point p1 = gitr->area.getPolygon().getCenterPoint() - parentCenter; // correct for previous moves
    Point mp = p - p1; // to move..
    gitr->area.setMove(mp.get_x(), mp.get_y());
  }
}

void GridAreaManager::setPaintMode(PaintMode mode) {
  paintMode = mode;
  modeChanged = true;
  LOG4CXX_DEBUG(logger,"setPaintMode(" << getModeAsString() << ")");
}

GridAreaManager::PaintMode GridAreaManager::getPaintMode() const {
  return paintMode;
}

void GridAreaManager::clearTemporaryAreas(){
  tmp_gridAreas.clear();
}

void GridAreaManager::addOverviewArea(miString id, ProjectablePolygon area, Colour & colour){
  GridArea newArea(id, area);
  newArea.updateCurrentProjection();
  newArea.setStyle(GridArea::OVERVIEW);
  newArea.setColour(colour);
  tmp_gridAreas[id] = newArea;
}

void GridAreaManager::clearSpatialInterpolation(){
  spatialAreas.clear();
  hasinterpolated = false;
}

void GridAreaManager::addSpatialInterpolateArea(miString id, miString parent, miTime valid, ProjectablePolygon area){
  GridArea newArea(id, area);
  newArea.updateCurrentProjection();
  newArea.setStyle(GridArea::GHOST);
  SpatialInterpolateArea newSIA;
  newSIA.id=id;
  newSIA.parent=parent;
  newSIA.validTime=valid;
  newSIA.area=newArea;
  spatialAreas.push_back(newSIA);
  std::sort(spatialAreas.begin(),spatialAreas.end());
}


bool GridAreaManager::addArea(miString id) {
  LOG4CXX_DEBUG(logger,"Adding empty area with id " << id);
  if (gridAreas.count(id)) {
    setCurrentArea(id);
    LOG4CXX_WARN(logger,"Add area failed. Existing id " << id);
    return false;
  }
  GridArea newArea(id, base_proj);
  newArea.updateCurrentProjection();
  gridAreas[id] = newArea;
  bool foundNewArea = setCurrentArea(id);
  updateSelectedArea();
  if (!foundNewArea)LOG4CXX_ERROR(logger,"Unable to select added area " << id);
  return foundNewArea;
}

bool GridAreaManager::addArea(miString id, ProjectablePolygon area,
    bool overwrite) {
  LOG4CXX_DEBUG(logger,"Adding " << id << ": " << area.toString());
  if (gridAreas.count(id) && !overwrite) {
    setCurrentArea(id);
    LOG4CXX_WARN(logger,"Add area failed. Existing id " << id);
    return false;
  }
  GridArea newArea(id, area);
  newArea.updateCurrentProjection();
  gridAreas[id] = newArea;
  bool foundNewArea = setCurrentArea(id);
  if (!foundNewArea)LOG4CXX_ERROR(logger,"Unable to select added area " << id);
  return foundNewArea;
}

bool GridAreaManager::updateArea(miString id, ProjectablePolygon area) {
  if (gridAreas.count(id)) {
    GridArea newArea(id, area);
    newArea.updateCurrentProjection();
    gridAreas[id] = newArea;
    return true;
  }
  return false;
}

bool GridAreaManager::removeArea(miString id) {
  if (gridAreas.count(id)) {
    if (id == currentId)
      setCurrentArea("");
    gridAreas.erase(id);
    return true;
  }
  return false;
}

bool GridAreaManager::removeCurrentArea() {
  return removeArea(currentId);
}

bool GridAreaManager::changeAreaId(miString oldId, miString newId) {
  if (gridAreas.count(oldId)) {
    gridAreas[newId] = gridAreas[oldId];
    gridAreas.erase(oldId);
    updateSelectedArea();
    return true;
  }
  return false;
}

bool GridAreaManager::setCurrentArea(miString id) {
  if (!gridAreas.count(id)) { // no area with this id
    currentId = "";
    updateSelectedArea();
    return false;
  }
  currentId = id;
  updateSelectedArea();
  return true;
}

void GridAreaManager::sendKeyboardEvent(const keyboardEvent& me,
    EventResult& res) {
  if (me.key == key_Shift) {
    if (me.type == keypress)
      res.newcursor = normal_cursor;
    if (me.type == keyrelease)
      res.newcursor = getCurrentCursor();
    res.repaint = true;
  }
}

/**
 * Has to be called from PlotModule::plot()
 */
bool GridAreaManager::plot() {
  map<miString,GridArea>::iterator iter;
  // draw temporary areas
  for (iter = tmp_gridAreas.begin(); iter != tmp_gridAreas.end(); iter++) {
    iter->second.plot();
  }
  if (paintMode == SPATIAL_INTERPOLATION) {
    vector<SpatialInterpolateArea>::iterator gitr;
    for (gitr = spatialAreas.begin(); gitr != spatialAreas.end(); gitr++) {
      gitr->area.plot();
    }
  }
  // draw active areas
  for (iter = gridAreas.begin(); iter != gridAreas.end(); iter++) {
    iter->second.plot();
  }
  return true;
}

miString GridAreaManager::getCurrentId() {
  return currentId;
}

bool GridAreaManager::selectArea(Point p) {
  map<miString,GridArea>::iterator iter;
  for (iter = gridAreas.begin(); iter != gridAreas.end(); iter++) {
    if (iter->second.inside(p)) {
      setCurrentArea(iter->first);
      return true;
    }
  }
  return false;
}

bool GridAreaManager::isUndoPossible() {
  if (hasCurrentArea()){
    return  gridAreas[currentId].isUndoPossible();
  }
  return false;
}

bool GridAreaManager::isRedoPossible() {
  if (hasCurrentArea()){
    return  gridAreas[currentId].isRedoPossible();
  }
  return false;
}

bool GridAreaManager::hasCurrentArea() {
  if (currentId == "")
    return false;
  if (!gridAreas.count(currentId)) // No current area
    return false;
  return true;
}

bool GridAreaManager::isAreaSelected() {
  if (!hasCurrentArea())
    return false;
  if (gridAreas[currentId].isEmptyArea())
    return false;
  return true;
}

bool GridAreaManager::isEmptyAreaSelected() {
  if (!hasCurrentArea())
    return false;
  if (gridAreas[currentId].isEmptyArea())
    return true;
  return false;
}

bool GridAreaManager::undo() {
  bool ok = false;
  if (hasCurrentArea()){
    ok = gridAreas[currentId].undo();
  }
  if(ok)
  return ok;
}

bool GridAreaManager::redo() {
  bool ok = false;
  if (hasCurrentArea()){
    ok = gridAreas[currentId].redo();
  }
  if(ok)
  return ok;
}

miString GridAreaManager::getModeAsString() {
  if (paintMode==SELECT_MODE)
    return miString("Select");
  else if (paintMode==MOVE_MODE)
    return miString("Move");
  else if (paintMode==SPATIAL_INTERPOLATION)
    return miString("Spatial Interpolation");
  else if (paintMode==INCLUDE_MODE)
    return miString("Include");
  else if (paintMode==CUT_MODE)
    return miString("Cut");
  else if (paintMode==DRAW_MODE)
    return miString("Draw");
  else if (paintMode==MOVE_POINT)
    return miString("Move Point");
  else if (paintMode==ADD_POINT)
    return miString("Add Point");
  else if (paintMode==REMOVE_POINT)
    return miString("Remove Point");
  else return miString("Unknown");
}

cursortype GridAreaManager::getCurrentCursor() {
  if (paintMode==SELECT_MODE)
    return paint_select_cursor;
  else if (paintMode==MOVE_MODE)
    return paint_move_cursor;
  else if (paintMode==MOVE_POINT)
    return paint_move_cursor;
  else if (paintMode==SPATIAL_INTERPOLATION)
    return paint_move_cursor;
  else if (paintMode==ADD_POINT)
    return paint_add_crusor;
  else if (paintMode==REMOVE_POINT)
    return paint_remove_crusor;
  else
    return paint_draw_cursor;
}

void GridAreaManager::updateSelectedArea() {
  map<miString,GridArea>::iterator iter;
  for (iter = gridAreas.begin(); iter != gridAreas.end(); iter++) {
    if (iter->first == currentId) {
      iter->second.setSelected(true);
    } else
      iter->second.setSelected(false);
  }
}

bool GridAreaManager::setEnabled(miString id, bool enabled) {
  if (gridAreas.count(id)) {
    gridAreas[id].enable(enabled);
    return true;
  }
  return false;
}

void GridAreaManager::setActivePoints(vector<Point> points) {

  if (gridAreas.count(currentId))
    gridAreas[currentId].setActivePoints(points);

}

vector<miString> GridAreaManager::getId(Point p) {
  vector<miString> vId;
  map<miString,GridArea>::iterator iter;
  for (iter = gridAreas.begin(); iter != gridAreas.end(); iter++) {
    if (iter->second.inside(p)) {
      vId.push_back(iter->first);
    }
  }
  return vId;
}

