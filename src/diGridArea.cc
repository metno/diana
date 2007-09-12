#include <diGridArea.h>
#include <AbstractablePolygon.h>
#include <Point.h>
#include <diTesselation.h>
#include <Segment.h>
#include <list>

using namespace std;

GridArea::GridArea():Plot(),polygon(""),displayPolygon(),editPolygon(""), displayEditPolygon(){
	Area stdProj = getStandardProjection();
	init(stdProj,stdProj);
}

GridArea::GridArea(string id):Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(){
	Area stdProj = getStandardProjection();
	init(stdProj,stdProj);
}
 
GridArea::GridArea(string id, ProjectablePolygon area):Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(){
	init(area.getOriginalProjection(),area.getOriginalProjection());
	polygon.setOriginalProjectionPoints(area);
}

GridArea::GridArea(string id, Area org_proj, Polygon area):Plot(),polygon(id),displayPolygon(),editPolygon(id), displayEditPolygon(){
	init(org_proj,org_proj);
	polygon.setOriginalProjectionPoints(area);
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
	else if(!isEmptyArea()){
		drawPolygon(displayPolygon,true);
		fillPolygon(displayPolygon,true);
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
	glLineWidth(1);
	glBegin(GL_LINE_STRIP); // GL_LINE_LOOP
	if(!main_polygon)
		glColor3d(1,0.0,0.0);
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
} 

void GridArea::fillPolygon(Polygon & p, bool main_polygon){
	int nPoints = p.get_points().size();
	if(nPoints < 3) return;
	

	Polygon::iterator current = p.begin();
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



