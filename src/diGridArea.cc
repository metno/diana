#include <diGridArea.h>
#include <AbstractablePolygon.h>
#include <Point.h>
#include <diTesselation.h>
#include <polyStipMasks.h>
#include <Segment.h>
#include <list>
#include <math.h>

using namespace std;

int GridArea::maxBuffer = 8;

GLfloat GridArea::nodeMarkRadius = 6.0f;
GLfloat GridArea::nodeMarkMaxConstant = 0.2f;
double GridArea::maxNodeSelectDistance = 20.0;


Area GridArea::getStandardProjection(){
  Rectangle rect(-34.9654,350.661,-47.0971,524.112);
  float pgf[6] = {11.045,-5.455,0.036,0.036,-24,66.5};
// Hirlam20
//  Rectangle rect(1,468,1,378);
//  float pgf[6] = {-46.5,-36.5,0.2,0.2,0,65};
  Projection proj;
  proj.set_mi_gridspec(3,pgf);
  Area a(proj,rect);
  return a;
}


GridArea::GridArea():
  Plot(),polygon(""),displayPolygon(),editPolygon(""), displayEditPolygon(),drawstyle(DEFAULT),colours_defined(false){
	Area stdProj = getStandardProjection();
	init(stdProj,stdProj);
  saveChange();
}

GridArea::GridArea(string id):
  Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(),drawstyle(DEFAULT),colours_defined(false){
	Area stdProj = getStandardProjection();
	init(stdProj,stdProj);
  saveChange();
}

GridArea::GridArea(string id, Area org_proj):
  Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(),drawstyle(DEFAULT),colours_defined(false){
	init(org_proj,org_proj);
  saveChange();
}

GridArea::GridArea(string id, ProjectablePolygon area_):
  Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(),drawstyle(DEFAULT),colours_defined(false){
	init(area_.getOriginalProjection(),area_.getOriginalProjection());
	polygon.setOriginalProjectionPoints(area_);
  saveChange();
}

GridArea::GridArea(string id, Area org_proj, Polygon area_):
  Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(),drawstyle(DEFAULT),colours_defined(false){
	init(org_proj,org_proj);
	polygon.setOriginalProjectionPoints(area_);
  saveChange();
}

void GridArea::setColour(Colour & fc){
  fillcolour= fc;
  colours_defined = true;
}

void GridArea::setStyle(const DrawStyle & ds){
  drawstyle = ds;
}

void GridArea::init(Area orgProj, Area currentProj){
#ifndef NOLOG4CXX
	logger = log4cxx::Logger::getLogger("diana.GridArea");
#endif
	polygon.setIntersectionsAccepted(false);
	editPolygon.setIntersectionsAccepted(false);
	polygon.setOriginalProjection(orgProj);
	editPolygon.setOriginalProjection(orgProj);
	polygon.setCurrentProjection(currentProj);
	editPolygon.setCurrentProjection(currentProj);
	reset();
	dirty = true;
	selected = false;
	showNextPoint = false;
	nodeInFocus = false;
}

ProjectablePolygon & GridArea::getPolygon() {
	return polygon;
}

bool GridArea::addPoint(Point p){
  showNextPoint = false;
	if(mode == EDIT){
		displayEditPolygon.push_back(p);
		return true;
	}
	displayPolygon.push_back(p);
	return true;
}

bool GridArea::plot(){
	if (!Enabled())
    return false;
  if (dirty) {//Projection changed
    updateCurrentProjection();
  }
  if (mode == EDIT) {
    drawPolygon(displayPolygon, true);
    fillPolygon(displayPolygon, true);
    drawPolygon(displayEditPolygon, false, false);
  } else if (mode == MOVE) {
    displayEditPolygon = displayPolygon;
    displayEditPolygon.move(moveX, moveY);
    drawPolygon(displayEditPolygon, true);
  } else if (mode == NODE_MOVE) {
    displayEditPolygon = displayPolygon;
    displayEditPolygon.movePoint(focusedNode, moveX, moveY);
    drawPolygon(displayEditPolygon, true);
  } else if (mode == NODE_INSERT) {
    displayEditPolygon = displayPolygon;
    if (segmentInFocus) {
      displayEditPolygon.insertPoint(focusedSegment, focusedNode);
      drawStipledSegment(focusedSegment);
    }
    drawNodes(displayPolygon);
    drawPolygon(displayEditPolygon, true);
    fillPolygon(displayEditPolygon, true);
  } else if (drawstyle == GHOST) {
    displayEditPolygon = displayPolygon;
    displayEditPolygon.move(moveX, moveY);
    drawPolygon(displayEditPolygon, true);
    fillPolygon(displayEditPolygon, true);
  } else if (!isEmptyArea()) {
    drawPolygon(displayPolygon, true);
    fillPolygon(displayPolygon, true);
    fillActivePolygon(displayPolygon, true);
    if (mode == NODE_SELECT)
      drawNodes(displayPolygon);
  } else {
    drawPolygon(displayPolygon, true, false);
  }
  UpdateOutput();
  return true;
}

void GridArea::drawPolygon(Polygon & p, bool main_polygon, bool close){
	list<Point> points = p.get_points();
	if (points.empty()) return;
	list<Point>::iterator current = points.begin();
	Point p1, pb;
	p1 = pb = (Point) *current;

  glLineWidth(1);
	glBegin(GL_LINE_STRIP); // GL_LINE_LOOP
	if(!main_polygon)
		glColor3d(1,0.0,0.0);
	else if (drawstyle == GHOST) glColor3d(0.5,0.5,0.5);
	else glColor3d(0.0,0.0,0.0);
	do{
		p1 = (Point) *current;
	  	glVertex2f(p1.get_x(),p1.get_y());
	}
	while (++current != points.end());

	if (close){	//connect end to beginning
  		glVertex2f(pb.get_x(),pb.get_y());
	}
	glFlush();
	glEnd();

	if (showNextPoint && ( (mode == PAINTING && main_polygon ) ||
	        (mode == EDIT && !main_polygon) ) ) {
	  glColor3d(0.5,0.5,0.5);
	  glBegin(GL_LINE_STRIP); // GL_LINE_LOOP
    glVertex2f(p1.get_x(),p1.get_y());
    glVertex2f(mousePoint.get_x(),mousePoint.get_y());
    glFlush();
    glEnd();
	}
}

void GridArea::fillPolygon(Polygon & p, bool main_polygon){
	int nPoints = p.get_points().size();
	if(nPoints < 3) return;

	Polygon::iterator current = p.begin();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

	if ( colours_defined ){
	  glColor4ubv(fillcolour.RGBA());
	} else {
	  if (drawstyle == DEFAULT){
	    if(main_polygon){
	      if(selected)
	        glColor4d(0.2, 0.2, 0.8, 0.2);
	      else
	        glColor4d(0.6, 0.6, 0.6, 0.1);
	    }
	    else glColor4d(0.8, 0.2, 0.2, 0.3);
	  } else if (drawstyle == OVERVIEW){
	    glColor4d(0.5, 0.5, 0.5, 0.1);
	  } else if (drawstyle == GHOST){
	    glColor4d(0.5, 0.5, 0.5, 0.1);
	  }
	}

	if (drawstyle == GHOST){
	  glEnable(GL_POLYGON_STIPPLE);
	  glPolygonStipple(square);
	} else if (drawstyle == OVERVIEW){
/*
    glEnable(GL_POLYGON_STIPPLE);
    glPolygonStipple(vldiagright);
*/
	}

	// Using GL tesselation
	GLdouble *gldata= new GLdouble[nPoints*3];
	int j = 0;
	for (int i=0; i<nPoints; i++) {
	  Point point = (Point) *current;
	  gldata[j]  = point.get_x();
	  gldata[j+1]= point.get_y();
	  gldata[j+2]= 0.0;
	  j+=3;
	  current++;
	}
	beginTesselation();
	tesselation(gldata, 1, &nPoints);
	endTesselation();
	delete[] gldata;
	glDisable(GL_BLEND);
  glDisable(GL_POLYGON_STIPPLE);
}

void GridArea::fillActivePolygon(Polygon & p, bool main_polygon){

  list<Point> points = p.getActivePoints();
  int nPoints = points.size();

  if(nPoints < 3) return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

  if(main_polygon){
    if(selected)
      glColor4d(0.2, 0.2, 0.8, 0.3);
    else
      glColor4d(0.6, 0.6, 0.6, 0.2);
  }
  else glColor4d(0.8, 0.2, 0.2, 0.5);

  list<Point>::iterator q = points.begin();
  vector<GLfloat> x;
  vector<GLfloat> y;
  for (; q!=points.end(); q++) {
    x.push_back((*q).get_x());
    y.push_back((*q).get_y());
  }

  for(unsigned int i=0;i<x.size()-3;i+=4){
    glBegin(GL_POLYGON); // GL_LINE_LOOP
    glVertex2f(x[i],  y[i] );
    glVertex2f(x[i+1],y[i+1] );
    glVertex2f(x[i+2],y[i+2] );
    glVertex2f(x[i+3],y[i+3] );
    glEnd();
  }
  glFlush();
  glDisable(GL_BLEND);

}


bool GridArea::setNodeFocus(const Point & mouse) {
  // called on every mouse move action when in node-select-mode
  Point tmp = focusedNode;
  double dist = displayPolygon.getClosestPoint(mouse, focusedNode);
  if (nodeInFocus != (dist < maxNodeSelectDistance)) {
    nodeInFocus = !nodeInFocus;
    return true;
  } else if (nodeInFocus) {
    return (tmp != focusedNode);
  } else return false;
}

bool GridArea::setNodeInsertFocus(const Point & mouse) {
  double segmentDistance = displayPolygon.getClosestSegment(mouse, focusedSegment);
  if (segmentInFocus != (segmentDistance < maxNodeSelectDistance)) {
    segmentInFocus = !segmentInFocus;
  }
  if (segmentInFocus) {
    //if (focusedSegment.height() <= 1 &&  focusedSegment.length() <= 1)
    //  return false;
    focusedNode = mouse;
    return true;
  } else return false;
}

void GridArea::drawNodes(const Polygon & p) {
  list<Point> points = p.get_points();
  if (points.empty()) return;
  list<Point>::const_iterator i = points.begin();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(3);
  GLfloat constantSizeRatio = (fullrect.x2-fullrect.x1) / pwidth;
  if (constantSizeRatio > nodeMarkMaxConstant) constantSizeRatio = nodeMarkMaxConstant;
  constantSizeRatio = constantSizeRatio * nodeMarkRadius;

  do {
    if (nodeInFocus && *i == focusedNode) {
      glColor4f(0,1,1,1);
      //glColor4f(0.9,0.9,0.9,0.6);
    } else {
      glColor4f(0,0,0,0.3);
      //glColor4f(0.6,0.6,0.6,0.5);
    }
    GLfloat x = i->get_x();
    GLfloat y = i->get_y();
    glBegin(GL_LINE_LOOP);
    // draw rectangle
    glVertex2f(x-constantSizeRatio,y-constantSizeRatio);
    glVertex2f(x+constantSizeRatio,y-constantSizeRatio);
    glVertex2f(x+constantSizeRatio,y+constantSizeRatio);
    glVertex2f(x-constantSizeRatio,y+constantSizeRatio);
    // .. or N-gon
    //for (int a=1; a<7; a++)
    // glVertex2f((x + sin(a) * constantSizeRatio), (y + cos(a) * constantSizeRatio));
    glFlush();
    glEnd();
  } while (++i != points.end());
  glDisable(GL_BLEND);
}

void GridArea::drawStipledSegment(const Segment& segment) {
  glLineStipple(1, 0xAAAA);
  glEnable(GL_LINE_STIPPLE);
  glLineWidth(1);
  glColor3f(0.5,0.5,0.5);
  glBegin(GL_LINE_LOOP);
  Point p1 = segment.get_p1();
  Point p2 = segment.get_p2();
  glVertex2f(p1.get_x(), p1.get_y());
  glVertex2f(p2.get_x(), p2.get_y());
  glFlush();
  glEnd();
  glDisable(GL_LINE_STIPPLE);
}

void GridArea::setMode(GridArea::AreaMode am){
	mode = am;
}

GridArea::AreaMode GridArea::getMode(){
	return mode;
}


bool GridArea::removeFocusedPoint() {
  if (nodeInFocus && displayPolygon.removePoint(focusedNode)) {
    polygon.setCurrentProjectionPoints(displayPolygon);
    polygon.makeAbstract();
    displayPolygon = polygon.getInCurrentProjection();
    saveChange();
    mode = NODE_SELECT;
    return true;
  }
  mode = NODE_SELECT;
  return false;
}

bool GridArea::startEdit(Point start){
	setMode(EDIT);
	resetEditPolygon();
	return addPoint(start);
}

bool GridArea::addEditPolygon(){
	editPolygon.setCurrentProjectionPoints(displayEditPolygon);
	AbstractablePolygon a1(1,polygon);
	AbstractablePolygon a2(1,editPolygon);
	a2.removeSharedSegments(a1);
	bool added = a1.addPolygon(a2);
	if(added){
		polygon.setPoints(a1.getPoints());
		displayPolygon = polygon.getInCurrentProjection();
		saveChange();
	}
	resetEditPolygon();
	setMode(NORMAL);
	return added;
}

bool GridArea::deleteEditPolygon(){
	editPolygon.setCurrentProjectionPoints(displayEditPolygon);
	AbstractablePolygon a1(1,polygon);
	AbstractablePolygon a2(1,editPolygon);
  a2.removeSharedSegments(a1);
	bool deleted = a1.deletePolygon(a2);
	if(deleted){
		polygon.setPoints(a1.getPoints());
		displayPolygon = polygon.getInCurrentProjection();
    saveChange();
	}
	resetEditPolygon();
	setMode(NORMAL);
	return deleted;
}

bool GridArea::inside(Point p){
	return displayPolygon.contains(p);
}


void GridArea::startNodeMove(){
  mode = NODE_MOVE;
  moveX = moveY = 0;
}

void GridArea::doNodeMove(){
  if(moveX != 0 && moveY != 0){
    displayPolygon.movePoint(focusedNode,moveX,moveY);
    polygon.setCurrentProjectionPoints(displayPolygon);
    polygon.makeAbstract();
    displayPolygon = polygon.getInCurrentProjection();
    moveX = moveY = 0;
    saveChange();
  }
  mode = NODE_SELECT;
}

void GridArea::doNodeInsert() {
  if (segmentInFocus) {
    displayPolygon.insertPoint(focusedSegment, focusedNode);
    polygon.setCurrentProjectionPoints(displayPolygon);
    polygon.makeAbstract();
    displayPolygon = polygon.getInCurrentProjection();
    segmentInFocus = false;
    saveChange();
  }
  mode = NODE_INSERT;
}

void GridArea::startMove(){
	mode = MOVE;
  moveX = moveY = 0;
}

void GridArea::setMove(const double& x,const double& y){
	moveX = x;
	moveY = y;
}

void GridArea::doMove(){
	if(moveX != 0 && moveY != 0){
		displayPolygon.move(moveX,moveY);
		polygon.setCurrentProjectionPoints(displayPolygon);
		polygon.makeAbstract();
		displayPolygon = polygon.getInCurrentProjection();
		moveX = moveY = 0;
    saveChange();
	}
	mode = NORMAL;
}

void GridArea::startDraw(Point p){
	LOG4CXX_DEBUG(logger,"startDraw ("<<p.toString()<<")");
	mode = PAINTING;
	displayPolygon.clearPoints();
	polygon.clearPoints();
	addPoint(p);
}

void GridArea::doDraw(){
	if(displayPolygon.empty()){
		polygon.clearPoints();
	}
	else{
		polygon.setCurrentProjectionPoints(displayPolygon);
		polygon.makeAbstract(true);
		displayPolygon = polygon.getInCurrentProjection();
	}
  saveChange();
  mode = NORMAL;
}


void GridArea::setSelected(bool s){
	selected = s;
}

bool GridArea::isSelected(){
	return selected;
}

bool GridArea::isEmptyArea(){
	return (polygon.getPointCount() < 3);
}

bool GridArea::isEmptyEditArea(){
	return (editPolygon.getPointCount() < 3);
}

void GridArea::reset(){
	mode = NORMAL;
	moveX = moveY = 0;
	polygon.clearPoints();
	displayPolygon.clearPoints();
	resetEditPolygon();
}

void GridArea::resetEditPolygon(){
	editPolygon.clearPoints();
	displayEditPolygon.clearPoints();
}

void GridArea::updateCurrentProjection(){
	polygon.setCurrentProjection(area);
	editPolygon.setCurrentProjection(area);
	displayPolygon = polygon.getInCurrentProjection();
	displayEditPolygon = editPolygon.getInCurrentProjection();
}

void  GridArea::setActivePoints(vector<Point> points){
  polygon.initActivePoints(points);
  displayPolygon = polygon.getInCurrentProjection();
}

void GridArea::saveChange() {
  if(redobuffer.size())
      redobuffer.clear();
  undobuffer.push_front(polygon);
  if(int(undobuffer.size())>maxBuffer)
    undobuffer.pop_back();
}

bool GridArea::isRedoPossible() {
  return (redobuffer.size());
}

bool GridArea::redo() {
  if (!isRedoPossible())
    return false;

  polygon = redobuffer.front();
  redobuffer.pop_front();
  undobuffer.push_front(polygon);
  displayPolygon = polygon.getInCurrentProjection();
  dirty = true;
  return true;
}

bool GridArea::isUndoPossible() {
  if (undobuffer.size()>1)
    return true;
  return false;
}

bool GridArea::undo() {
  if (!isUndoPossible())
    return false;

  redobuffer.push_front(polygon);
  undobuffer.pop_front();
  polygon = undobuffer.front();
  displayPolygon = polygon.getInCurrentProjection();

  dirty = true;
  return true;
}

