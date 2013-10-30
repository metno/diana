#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diGridAreaManager.h>
#include <iostream>
#include <propoly/AbstractablePolygon.h>
#include <propoly/Point.h>
#include <sstream>

#include <QKeyEvent>
#include <QMouseEvent>

using namespace::miutil;

GridAreaManager::GridAreaManager() :
  mapmode(normal_mode), hasinterpolated(false) {
  paintMode = SELECT_MODE;
#ifndef NOLOG4CXX
  logger = log4cxx::Logger::getLogger("diana.GridAreaManager");
#endif
  overrideMouseEvent = false;
  modeChanged = true;
  currentId = "";
}
GridAreaManager::~GridAreaManager() {
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

ProjectablePolygon GridAreaManager::getArea(std::string id) {
  LOG4CXX_DEBUG(logger,"getArea("<<id<<")");
  if (gridAreas.count(id))
    return gridAreas[id].getPolygon();
  return ProjectablePolygon();
}

void GridAreaManager::sendMouseEvent(QMouseEvent* me, EventResult& res,
    float x, float y) {
  handleModeChanged(me, res);

  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
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


void GridAreaManager::handleModeChanged(QMouseEvent* me, EventResult& res) {
  if (modeChanged) {
    LOG4CXX_DEBUG(logger,"Changing cursor to " << getCurrentCursor());
    res.newcursor = getCurrentCursor();
    modeChanged = false;
    if (gridAreas.count(currentId) && me->button() == Qt::NoButton) {
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

void GridAreaManager::handleSelectEvent(QMouseEvent* me, EventResult& res,
    const float& x, const float& y)
{
  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::RightButton) {
    res.action = rightclick;
  } else if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
    std::string prevId = currentId;
    selectArea(Point(x, y));
    LOG4CXX_DEBUG(logger,"Selected area: " << currentId);
    gridAreas[currentId].setMode(GridArea::NORMAL);
    res.repaint = true;
    res.action=grid_area_changed;
  } else if (me->type() == QEvent::MouseButtonRelease) {
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleDrawEvent(QMouseEvent* me, EventResult& res,
    const float& x, const float& y)
{
  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
    if(gridAreas[currentId].getMode() == GridArea::NORMAL) {
      LOG4CXX_DEBUG(logger,"Starting draw " << currentId);
      gridAreas[currentId].startDraw(Point(x, y));
    } else if (gridAreas[currentId].getMode() == GridArea::PAINTING) {
      gridAreas[currentId].addPoint(Point(x, y));
    }
    res.repaint = true;
  } else if (me->type() == QEvent::MouseMove && me->buttons() & Qt::LeftButton) {
    gridAreas[currentId].addPoint(Point(x, y));
    res.repaint = true;
  } else if (me->type() == QEvent::MouseMove && !(me->buttons() & Qt::LeftButton)) {
    gridAreas[currentId].setNextPoint(Point(x, y));
    res.repaint = true;
  } else if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::RightButton) {
    gridAreas[currentId].addPoint(Point(x, y));
    gridAreas[currentId].doDraw();
    res.repaint = true;
    res.action = grid_area_changed;
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleEditEvent(QMouseEvent* me, EventResult& res,
    const float& x, const float& y)
{
  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
    if(gridAreas[currentId].getMode() == GridArea::NORMAL) {
      LOG4CXX_DEBUG(logger,"Starting edit " << currentId);
      gridAreas[currentId].startEdit(Point(x, y));
    } else if (gridAreas[currentId].getMode() == GridArea::EDIT) {
      gridAreas[currentId].addPoint(Point(x, y));
    }
    res.repaint = true;
  } else if (me->type() == QEvent::MouseMove && me->buttons() & Qt::LeftButton) {
    gridAreas[currentId].addPoint(Point(x, y));
    res.repaint = true;
  } else if (me->type() == QEvent::MouseMove && !(me->buttons() & Qt::LeftButton)) {
    gridAreas[currentId].setNextPoint(Point(x, y));
    res.repaint = true;
  } else if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::RightButton) {
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

void GridAreaManager::handleMoveEvent(QMouseEvent* me, EventResult& res,
    const float& x, const float& y, const float& first_x, const float& first_y)
{
  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
    LOG4CXX_DEBUG(logger,"Starting move " << currentId);
    gridAreas[currentId].startMove();
    res.repaint = true;
  } else if (me->type() == QEvent::MouseMove && me->buttons() & Qt::LeftButton) {
    gridAreas[currentId].setMove((x-first_x), (y-first_y));
    res.repaint = true;
  } else if (me->type() == QEvent::MouseButtonRelease && me->button() == Qt::LeftButton) {
    gridAreas[currentId].doMove();
    res.repaint = true;
    res.action = grid_area_changed;
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleMovePointEvent(QMouseEvent* me, EventResult& res,
    const float& x, const float& y, const float& first_x, const float& first_y)
{
  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
    LOG4CXX_DEBUG(logger,"Starting move point " << currentId);
    gridAreas[currentId].startNodeMove();
    res.repaint = true;
  } else if (me->type() == QEvent::MouseMove && me->buttons() & Qt::LeftButton) {
    gridAreas[currentId].setMove((x-first_x), (y-first_y));
    res.repaint = true;
  } else if (me->type() == QEvent::MouseMove && !(me->buttons() & Qt::LeftButton)) {
    if(gridAreas[currentId].setNodeFocus(Point(x, y))) // focus changed
      res.repaint = true;
  } else if (me->type() == QEvent::MouseButtonRelease && me->button() == Qt::LeftButton) {
    gridAreas[currentId].doNodeMove();
    res.repaint = true;
    res.action = grid_area_changed;
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleSpatialInterpolationEvent(QMouseEvent* me, EventResult& res,
    const float& x, const float& y, const float& first_x, const float& first_y)
{
  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
    LOG4CXX_DEBUG(logger,"Starting Spatial Interpolation");
    gridAreas[currentId].startMove();
    res.repaint = true;
  } else if (me->type() == QEvent::MouseMove && me->buttons() & Qt::LeftButton) {
    doSpatialInterpolation(currentId, (x-first_x), (y-first_y));
    res.repaint = true;
  } else if (me->type() == QEvent::MouseButtonRelease && me->button() == Qt::LeftButton) {
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

void GridAreaManager::handleAddPointEvent(QMouseEvent* me, EventResult& res,
    const float& x, const float& y)
{
  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
    gridAreas[currentId].doNodeInsert();
    res.repaint = true;
    res.action = grid_area_changed;
  } else if (me->type() == QEvent::MouseMove) {
    if(gridAreas[currentId].setNodeInsertFocus(Point(x, y))) { // focus changed
      res.newcursor = paint_add_crusor;
    } else res.newcursor = paint_forbidden_crusor;
    res.repaint = true;
  } else if (me->type() == QEvent::MouseButtonRelease && me->button() == Qt::LeftButton) {
    overrideMouseEvent = false;
  }
}

void GridAreaManager::handleRemovePointEvent(QMouseEvent* me, EventResult& res,
    const float& x, const float& y)
{
  if (me->type() == QEvent::MouseButtonPress && me->button() == Qt::LeftButton) {
    LOG4CXX_DEBUG(logger,"Remove point from " << currentId);
    if(gridAreas[currentId].removeFocusedPoint()) {
      res.repaint = true;
      res.action = grid_area_changed;
    }
  } else if (me->type() == QEvent::MouseMove) {
    if(gridAreas[currentId].setNodeFocus(Point(x, y))) // focus changed
      res.repaint = true;
  } else if (me->type() == QEvent::MouseButtonRelease && me->button() == Qt::LeftButton) {
    overrideMouseEvent = false;
  }
}

void GridAreaManager::doSpatialInterpolation(const std::string & movedId, float moveX, float moveY) {
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

void GridAreaManager::addOverviewArea(std::string id, ProjectablePolygon area, Colour & colour){
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

void GridAreaManager::addSpatialInterpolateArea(std::string id, std::string parent, miTime valid, ProjectablePolygon area){
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


bool GridAreaManager::addArea(std::string id) {
  LOG4CXX_DEBUG(logger,"Adding empty area with id " << id);
  if (gridAreas.count(id)) {
    setCurrentArea(id);
    LOG4CXX_WARN(logger,"Add area failed. Existing id " << id);
    return false;
  }
//  GridArea newArea(id, base_proj);
  GridArea newArea(id);
  newArea.updateCurrentProjection();
  gridAreas[id] = newArea;
  bool foundNewArea = setCurrentArea(id);
  updateSelectedArea();
  if (!foundNewArea)LOG4CXX_ERROR(logger,"Unable to select added area " << id);
  return foundNewArea;
}

bool GridAreaManager::addArea(std::string id, ProjectablePolygon area,
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

bool GridAreaManager::updateArea(std::string id, ProjectablePolygon area) {
  if (gridAreas.count(id)) {
    GridArea newArea(id, area);
    newArea.updateCurrentProjection();
    gridAreas[id] = newArea;
    return true;
  }
  return false;
}

bool GridAreaManager::removeArea(std::string id) {
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

bool GridAreaManager::changeAreaId(std::string oldId, std::string newId) {
  if (gridAreas.count(oldId)) {
    gridAreas[newId] = gridAreas[oldId];
    gridAreas.erase(oldId);
    updateSelectedArea();
    return true;
  }
  return false;
}

bool GridAreaManager::setCurrentArea(std::string id) {
  if (!gridAreas.count(id)) { // no area with this id
    currentId = "";
    updateSelectedArea();
    return false;
  }
  currentId = id;
  updateSelectedArea();
  return true;
}

void GridAreaManager::sendKeyboardEvent(QKeyEvent* ke, EventResult& res)
{
  if (ke->key() == Qt::Key_Shift) {
    if (ke->type() == QEvent::KeyPress)
      res.newcursor = normal_cursor;
    if (ke->type() == QEvent::KeyRelease)
      res.newcursor = getCurrentCursor();
    res.repaint = true;
  }
}

/**
 * Has to be called from PlotModule::plot()
 */
bool GridAreaManager::plot() {
  map<std::string,GridArea>::iterator iter;
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

std::string GridAreaManager::getCurrentId() {
  return currentId;
}

bool GridAreaManager::selectArea(Point p) {
  map<std::string,GridArea>::iterator iter;
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
  return ok;

}

bool GridAreaManager::redo() {
  bool ok = false;
  if (hasCurrentArea()){
    ok = gridAreas[currentId].redo();
  }

  return ok;
}

std::string GridAreaManager::getModeAsString() {
  if (paintMode==SELECT_MODE)
    return std::string("Select");
  else if (paintMode==MOVE_MODE)
    return std::string("Move");
  else if (paintMode==SPATIAL_INTERPOLATION)
    return std::string("Spatial Interpolation");
  else if (paintMode==INCLUDE_MODE)
    return std::string("Include");
  else if (paintMode==CUT_MODE)
    return std::string("Cut");
  else if (paintMode==DRAW_MODE)
    return std::string("Draw");
  else if (paintMode==MOVE_POINT)
    return std::string("Move Point");
  else if (paintMode==ADD_POINT)
    return std::string("Add Point");
  else if (paintMode==REMOVE_POINT)
    return std::string("Remove Point");
  else return std::string("Unknown");
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
  map<std::string,GridArea>::iterator iter;
  for (iter = gridAreas.begin(); iter != gridAreas.end(); iter++) {
    if (iter->first == currentId) {
      iter->second.setSelected(true);
    } else
      iter->second.setSelected(false);
  }
}

bool GridAreaManager::setEnabled(std::string id, bool enabled) {
  if (gridAreas.count(id)) {
    gridAreas[id].enable(enabled);
    return true;
  }
  return false;
}

void GridAreaManager::setActivePoints(list<Point> points) {

  if (gridAreas.count(currentId))
    gridAreas[currentId].setActivePoints(points);

}

vector<std::string> GridAreaManager::getId(Point p) {
  vector<std::string> vId;
  map<std::string,GridArea>::iterator iter;
  for (iter = gridAreas.begin(); iter != gridAreas.end(); iter++) {
    if (iter->second.inside(p)) {
      vId.push_back(iter->first);
    }
  }
  return vId;
}

