#include <diGridArea.h>
#include <AbstractablePolygon.h>
#include <Point.h>
#include <diTesselation.h>
#include <polyStipMasks.h>
#include <Segment.h>
#include <list>

using namespace std;

int GridArea::maxBuffer = 8;

GridArea::GridArea():Plot(),polygon(""),displayPolygon(),editPolygon(""), displayEditPolygon(),colours_defined(false),drawstyle(DEFAULT){
	Area stdProj = getStandardProjection();
	init(stdProj,stdProj);
  saveChange();
}

GridArea::GridArea(string id):Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(),colours_defined(false),drawstyle(DEFAULT){
	Area stdProj = getStandardProjection();
	init(stdProj,stdProj);
  saveChange();
}

GridArea::GridArea(string id, Area org_proj):Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(),colours_defined(false),drawstyle(DEFAULT){
	init(org_proj,org_proj);
  saveChange();
}

GridArea::GridArea(string id, ProjectablePolygon area_):Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(),colours_defined(false),drawstyle(DEFAULT){
	init(area_.getOriginalProjection(),area_.getOriginalProjection());
	polygon.setOriginalProjectionPoints(area_);
  saveChange();
}

GridArea::GridArea(string id, Area org_proj, Polygon area_):Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(),colours_defined(false),drawstyle(DEFAULT){
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
	polygon.setOriginalProjection(orgProj);
	editPolygon.setOriginalProjection(orgProj);
	polygon.setCurrentProjection(currentProj);
	editPolygon.setCurrentProjection(currentProj);
	reset();
	dirty = true;
	selected = false;
}

Area GridArea::getStandardProjection(){
	Rectangle rect(-34.9654,350.661,-47.0971,524.112);
	float pgf[6] = {11.045,-5.455,0.036,0.036,-24,66.5};

// Hirlam20
// 	Rectangle rect(1,468,1,378);
// 	float pgf[6] = {-46.5,-36.5,0.2,0.2,0,65};

	Projection proj(3,pgf);
	Area a(proj,rect);
	return a;
}


ProjectablePolygon & GridArea::getPolygon() {
	return polygon;
}

bool GridArea::addPoint(Point p){
	if(mode == EDIT){
		displayEditPolygon.push_back(p);
		return true;
	}
	displayPolygon.push_back(p);
	return true;
}

bool GridArea::plot(){
	if(!Enabled())
		return false;
	if(dirty){//Projection changed
		updateCurrentProjection();
	}
	if(mode == EDIT){
		drawPolygon(displayEditPolygon,false);
	}
	if(mode == MOVE) {
		displayEditPolygon = displayPolygon;
		displayEditPolygon.move(moveX,moveY);
		drawPolygon(displayEditPolygon,true);
	}
  else if(drawstyle == GHOST){
    displayEditPolygon = displayPolygon;
    displayEditPolygon.move(moveX,moveY);
    drawPolygon(displayEditPolygon,true);
    fillPolygon(displayEditPolygon,true);
  }
	else if(!isEmptyArea()){
		drawPolygon(displayPolygon,true);
		fillPolygon(displayPolygon,true);
		fillActivePolygon(displayPolygon,true);
	}
	else{
		drawPolygon(displayPolygon,true);
	}
    UpdateOutput();
    return true;
}

void GridArea::drawPolygon(Polygon & p, bool main_polygon){
	list<Point> points = p.get_points();
	list<Point>::iterator current = points.begin();
	Point p1, pb;
	p1 = pb = (Point) *current;
	glLineWidth(2);
  glColor3d(1,1,0.0);
	if(p.getPointCount() > 4) {
	  Point center = p.getCenterPoint();
	  double d = 1.0;
	  glRectd(center.get_x()-d, center.get_y()-d, center.get_x()+d, center.get_y()+d);
	}
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

	if( (main_polygon && !isEmptyArea()) || (!main_polygon && !isEmptyEditArea()) ){	//connect end to beginning
  		glVertex2f(pb.get_x(),pb.get_y());
	}
	glFlush();
	glEnd();
	//  Point center = p.getCenterPoint();
	  //TEST
	//  glRectd(center.get_x(), center.get_y(), center.get_x(), center.get_y());
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

  for(int i=0;i<x.size()-3;i+=4){
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

void GridArea::setMode(GridArea::AreaMode am){
	mode = am;
}

GridArea::AreaMode GridArea::getMode(){
	return mode;
}


bool GridArea::startEdit(Point start){
	setMode(EDIT);
	resetEditPolygon();
	addPoint(start);
}

bool GridArea::addEditPolygon(){
	editPolygon.setCurrentProjectionPoints(displayEditPolygon);
	AbstractablePolygon a1(1,polygon);
	AbstractablePolygon a2(1,editPolygon);
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

void GridArea::startMove(){
	mode = MOVE;
}

void GridArea::setMove(double move_x, double move_y){
	moveX = move_x;
	moveY = move_y;
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
	mode = NORMAL;
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
		polygon.makeAbstract();
		displayPolygon = polygon.getInCurrentProjection();
	}
  saveChange();
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
  if(undobuffer.size()>maxBuffer)
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

