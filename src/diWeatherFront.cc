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

#include <diWeatherFront.h>
#include <math.h>
#include <miString.h>
#include <sstream>


vector<editToolInfo> WeatherFront::allFronts; //info about fronts
map<miString,int> WeatherFront::frontTypes;   //finds front type number from name

// Default constructor

WeatherFront::WeatherFront() : ObjectPlot(wFront),frontlinewidth(4)
{
#ifdef DEBUGPRINT
  cerr << "WeatherFront default constructror" << endl;
#endif
  setType(0);         // default fronttype
}

// Constructor taking front type as argument

WeatherFront::WeatherFront(int ty) : ObjectPlot(wFront),frontlinewidth(4)
{
#ifdef DEBUGPRINT
  cerr << "WeatherFront(int) constructor" << endl;
#endif
  setType(ty);         // fronttype
}

WeatherFront::WeatherFront(miString tystring) : ObjectPlot(wFront),frontlinewidth(4)
{
#ifdef DEBUGPRINT
  cerr << "WeatherFront(miString) constructor" << endl;
#endif
  // set correct fronttype
  if (!setType(tystring))
    cerr << "WeatherFront constructor error, type " << tystring << " not found !!!";  

}



//copy constructor

WeatherFront::WeatherFront(const WeatherFront &rhs) : ObjectPlot(rhs){
#ifdef DEBUGPRINT
  cerr << "WeatherFront copy constructror" << endl;
#endif
  linewidth=rhs.linewidth;    // default linewidth of front
  rubber = false; //draw rubber ?
  frontlinewidth=rhs.frontlinewidth;
  s_length = rhs.s_length;
  x_s = new float[s_length];
  y_s = new float[s_length];
  for (int i = 0;i<s_length;i++){
    x_s[i] = rhs.x_s[i];
    y_s[i] = rhs.y_s[i];
  }
}

// Destructor
WeatherFront::~WeatherFront(){
}


void WeatherFront::defineFronts(vector<editToolInfo> fronts)
{
  allFronts = fronts;
  for (int i = 0;i<fronts.size();i++)
    frontTypes[fronts[i].name] = i;
}




void WeatherFront::recalculate(){
  //cerr << "WeatherFront::recalculate" << endl;
  // Makes smooth lines
      int div=divSpline;  // div = subdivision points between to edge points

      if (x != NULL)  delete[] x; 
      if (y != NULL)  delete[] y; 
      if (x_s != NULL)  delete[] x_s; 
      if (y_s != NULL)  delete[] y_s;
      x= y= x_s= y_s= 0;
      s_length= 0;

      int length=nodePoints.size();
      if (length < 1) return;
      x= new float[length]; // x array of x coordinates 
      y= new float[length]; // y array of y coordinates 
      length = 0;

      for (int i=0; i<nodePoints.size(); i++){
	//skip points that are in the same position
	if (i!=0 && nodePoints[i]==nodePoints[i-1]) continue;
	  x[length]=nodePoints[i].x;
	  y[length]=nodePoints[i].y;
	  length++;
      }
      
      // smooth line made in x_s and y_s

      x_s=new float[(length-1)*div+length]; // x_s array of SMOOTH x coordinates 
      y_s=new float[(length-1)*div+length];  
      s_length=smoothline(length, &x[0], &y[0] , 0, length-1,div, &x_s[0], &y_s[0]);

      for (int i=0; i<s_length; i++){
	float x=x_s[i];
	float y=y_s[i];
	if (! boundBox.isinside(x,y)){   
	  if (x < boundBox.x1){ boundBox.x1=x; }
	  if (x > boundBox.x2){ boundBox.x2=x; }
	  if (y < boundBox.y1){ boundBox.y1=y; }
	  if (y > boundBox.y2){ boundBox.y2=y; }
	}	 
      }
      

}


// -- virtual change of object-state
// if set to passive: smooth and triangulate area
void WeatherFront::setState(const state s)
{
  currentState= s;
  setPlotVariables();
}

// draws the weather front 
bool WeatherFront::plot(){

  if (!enabled) return false;

  if (isVisible){
    int end = nodePoints.size();
    if (0<end){
      
      setWindowInfo();
      
      //recalculate();


      if (drawIndex>=SigweatherFront) 
	linewidth=siglinewidth*2;
      else { 
	//change the linewidth according to great circle distance
	float scalefactor = gcd/7000000;
	if (scalefactor <=1 )
	  linewidth = frontlinewidth;
	else if (scalefactor > 1.0 && scalefactor < 4)
	  linewidth = frontlinewidth/scalefactor;
	else if (scalefactor >=4)
	  linewidth = 1;
      }


    //enable blending and set colour
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4ub(col->R(),col->G(),col->B(),col->A());

      switch (drawIndex){
      case Cold:
	drawColds();    
	break;
      case Warm:
	drawWarms();    
	break;
      case Occluded:
	drawOccluded();    
	break;
      case ColdOccluded:
	drawColdOccluded();    
	break;
      case WarmOccluded:
	drawWarmOccluded();    
	break;
      case Stationary:
	drawStationary();    
	break;
      case TroughLine:
 	//drawTroughLine(); // nothing to draw, only the line (below)
	break;
      case SquallLine:
 	drawSquallLine();
	break;
      case SigweatherFront:
 	if (drawSig) drawSigweather();
	break;
      }

      // for PostScript generation
     UpdateOutput();

     if (!drawSig){
       glPushMatrix();
  
       //draw the line, first set colour and linewidth
       glColor4ub(col->R(),col->G(),col->B(),col->A());
       glLineWidth(linewidth);
       glBegin(GL_LINE_STRIP);        // Draws the smooth line
       if (spline){
	 for (int i=0; i<s_length; i++)
	   glVertex2f((x_s[i]),(y_s[i]));
       } else{
	 for (int i=0; i<end; i++)
	   glVertex2f(nodePoints[i].x,nodePoints[i].y);
       }
       glEnd();
       glPopMatrix();
       if (rubber) plotRubber();
    }
     
     UpdateOutput();
     
     glDisable(GL_BLEND);  
     
     drawNodePoints();
     UpdateOutput();
    }
  }

  return true;
}


int WeatherFront::hitPoint(float x,float y){
  //find out between which points we should split in two
  if (onLine(x,y)){
    return insert;
    }
  else return 0;
}


bool WeatherFront::addFront(ObjectPlot * qfront){
  //qfront is added to front
  float x,y,xnew,ynew;

  if (addTop){
    x=nodePoints.front().x;
    y=nodePoints.front().y;
  } else {
    x=nodePoints.back().x;
    y=nodePoints.back().y;
  }
  int end = qfront->getXYZsize();
  vector<float> xq=qfront->getX();
  vector<float> yq=qfront->getY();
  currentState = active;
  if (qfront->isBeginPoint(x,y,xnew,ynew)){
    movePoint(x,y,xnew,ynew);
    for (int i = 1;i<end;i++){
      addPoint(xq[i],yq[i]);
    }
  } else if (qfront->isEndPoint(x,y,xnew,ynew)){
    movePoint(x,y,xnew,ynew);
    for (int i = end-2;i>-1;i--){
      addPoint(xq[i],yq[i]);
    }
  } else{
    return false;
  }
  currentState = passive;
  return true;
}


ObjectPlot* WeatherFront::splitFront(float x,float y){
  //splits front in two at x,y
  //pointer to new front is returned
  WeatherFront* newFront = new WeatherFront();
  newFront->setType(type);
  currentState = active;
  addTop=false;
  newFront->currentState=active;
  newFront->setWindowInfo();
  int ihit = hitPoint(x,y);
  if (ihit==0){
    delete newFront;
    return NULL;
  }
  int start = nodePoints.size()-1;
  for(int i=start;i>=ihit;i--){
      newFront->addPoint(nodePoints[i].x,nodePoints[i].y);
      nodePoints.erase(nodePoints.begin()+i);
    }
  //last add point x,y to the beginning of new front
  newFront->addPoint(x,y);
  addPoint(x,y);

  unmarkAllPoints();
  newFront->unmarkAllPoints();

  return newFront;
}



/**
 * Algorithm for marking points on line
 * Idea is that if mousepointer is moved over a line 
 * all nodepoints shine up / are marked
 * If on nodepoint only this point is marked
 * Checks this only if current status is passive.
 */
bool WeatherFront::showLine(float x, float y){
  markedChanged=false;

  if (boundBox.isinside(x,y)) {
    if (inBoundBox==false) markedChanged=true;
    inBoundBox=true;
  }
  else{
    if (inBoundBox==true) markedChanged=true;
    inBoundBox=false;
  }

  if (isInside(x,y)){
    markPoint(x,y); // checks whether on nodepoint and marks
    return true;
  }
  else if (onLine(x,y)){ // else checkes whether on line
    markAllPoints();
    return true;
  }
  else {    
    unmarkAllPoints();
    return false;
  }
  return false;
}


/* Draw triangles on smooth line */
void WeatherFront::drawColds(){
  // cerr << "WeatherFront::drawColds" << endl;
  // colds are blue triangles on the front

  float r= linewidth*2*getDwidth();
  int end= s_length;
  int ncount=0;

  float xstart,ystart,xend,yend,xtop,ytop,dxs,dys,fraction;
  float s,slim,sprev, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
	xm= (x1+x2)*0.5;
	ym= (y1+y2)*0.5;
	sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
	if ((s1-slim)*(sm-slim)<=0.) {
	  x2= xm;
	  y2= ym;
	} else {
	  x1= xm;
	  y1= ym;
	  s1= sm;
	}
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }

    dxs= xend - xstart;
    dys= yend - ystart;

    xtop= (xstart+xend)*0.5 - dys*0.6;
    ytop= (ystart+yend)*0.5 + dxs*0.6;

    if (ncount%2==0){
      glBegin(GL_POLYGON);
      glVertex2f(xstart,ystart);
      glVertex2f(xend,yend);
      glVertex2f(xtop,ytop);
      glEnd();
    }
    ncount++;

    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r*1.5;
    s= 0.;

  }

  UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

/* 
   Draw arch on smooth line
*/
void WeatherFront::drawWarms(){
  //  cerr << "WeatherFront::drawWarms" << endl;
  // warms are red arches on the front

  const float PI=3.1415926535897932384626433832795;
  int ncount=0;

  float r= linewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,dxs,dys,fraction;
  float s,slim,sprev, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;

  const int nwarmflag= 19;
  float xwarmflag[nwarmflag], ywarmflag[nwarmflag];
  float flagstep= PI/(nwarmflag-1);

  for (j=0; j<nwarmflag; j++) {
    xwarmflag[j]= r*cos(j*flagstep);
    ywarmflag[j]= r*sin(j*flagstep);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
	xm= (x1+x2)*0.5;
	ym= (y1+y2)*0.5;
	sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
	if ((s1-slim)*(sm-slim)<=0.) {
	  x2= xm;
	  y2= ym;
	} else {
	  x1= xm;
	  y1= ym;
	  s1= sm;
	}
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }

    dxs= xend - xstart;
    dys= yend - ystart;
    xm= (xstart+xend)*0.5;
    ym= (ystart+yend)*0.5;

    if (ncount%2==0){
      glPushMatrix();
      glTranslatef(xm, ym, 0.0);
      glRotatef(atan2(dys,dxs)*180./PI,0.0,0.0,1.0);
      glBegin(GL_POLYGON);
      for (j=0; j<nwarmflag; j++)
	glVertex2f(xwarmflag[j],ywarmflag[j]);
      glEnd();
      glPopMatrix();
    } 
    ncount++;


    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r*1.5;
    s= 0.;

  }

  UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}


/*
  Draws triangles and arch on smooth line 
 */
void WeatherFront::drawOccluded(){
  //  cerr << "WeatherFront::drawOccluded" << endl;
  
  const float PI=3.1415926535897932384626433832795;

  float r= linewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,xtop,ytop,dxs,dys,fraction;
  float xstart1,ystart1,xend1,yend1;
  float s,slim,sprev, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;
  int ndrawflag= 0;
  int ncount=0;

  const int nwarmflag= 19;
  float xwarmflag[nwarmflag], ywarmflag[nwarmflag];
  float flagstep= PI/(nwarmflag-1);

  for (j=0; j<nwarmflag; j++) {
    xwarmflag[j]= r*cos(j*flagstep);
    ywarmflag[j]= r*sin(j*flagstep);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim= r*0.75;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;

    istart= i-1;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
	xm= (x1+x2)*0.5;
	ym= (y1+y2)*0.5;
	sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
	if ((s1-slim)*(sm-slim)<=0.) {
	  x2= xm;
	  y2= ym;
	} else {
	  x1= xm;
	  y1= ym;
	  s1= sm;
	}
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }




      ndrawflag++;
      
      if (ndrawflag==1) {
	
	xstart1= xstart;
	ystart1= ystart;
	xend1=   xend;
	yend1=   yend;
	
      } else {
	
	dxs= xend1 - xstart1;
	dys= yend1 - ystart1;
	xm= (xstart1+xend1)*0.5;
	ym= (ystart1+yend1)*0.5;
	
	if (ncount%2==0){
	  glPushMatrix();
	  glTranslatef(xm, ym, 0.0);
	  glRotatef(atan2(dys,dxs)*180./PI,0.0,0.0,1.0);
	  glBegin(GL_POLYGON);
	  for (j=0; j<nwarmflag; j++)
	    glVertex2f(xwarmflag[j],ywarmflag[j]);
	  glEnd();
	  glPopMatrix();
	  
	  dxs= xend - xstart;
	  dys= yend - ystart;
	  xtop= (xstart+xend)*0.5 - dys*0.6;
	  ytop= (ystart+yend)*0.5 + dxs*0.6;
	  
	  glBegin(GL_POLYGON);
	  glVertex2f(xstart,ystart);
	  glVertex2f(xend,yend);
	  glVertex2f(xtop,ytop);
	  glEnd();
	}
	ncount++;
	
	ndrawflag=0;
      }
      
    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys);
    if (ndrawflag==0) slim+= r*1.5;
    s= 0.;

  }

  UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}


/*
  For blue occlution
 */
void WeatherFront::drawColdOccluded(){
  //    cerr << "WeatherFront::drawColdOccluded" << endl;
  drawOccluded();
}

/*
  For red occlution
 */
void WeatherFront::drawWarmOccluded(){
  //    cerr << "WeatherFront::drawWarmOccluded" << endl;
  drawOccluded();
}

/*
  Draws stationary front on smooth line
 */
void WeatherFront::drawStationary(){
  //  cerr << "WeatherFront::drawStationary" << endl;
  // colds are blue triangles on the front

  const float PI=3.1415926535897932384626433832795;

  float r= linewidth*2*getDwidth();
  int end= s_length;

  float xstart,ystart,xend,yend,xtop,ytop,dxs,dys,fraction;
  float s,slim,sprev, x1,y1,s1,x2,y2,xm,ym,sm;
  int i,istart,j;
  int ndrawflag= 0;
  int ncount=0;

  const int nwarmflag= 19;
  float xwarmflag[nwarmflag], ywarmflag[nwarmflag];
  float flagstep= PI/(nwarmflag-1);

  for (j=0; j<nwarmflag; j++) {
    xwarmflag[j]= r*cos(j*flagstep);
    ywarmflag[j]= r*sin(j*flagstep);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(1);

  slim= r*0.5;
  s= 0.;
  i=1;

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    istart= i-1;
    fraction= (slim-sprev)/(s-sprev);
    xstart= x_s[i-1] + dxs * fraction;
    ystart= y_s[i-1] + dys * fraction;

    istart= i-1;
    s= 0.;
    slim= r*2.;

    while (s<slim && i<end) {
      dxs= x_s[i] - xstart;
      dys= y_s[i] - ystart;
      sprev= s;
      s= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    if (istart==i-1) {
      fraction= slim/s;
      xend= xstart + dxs * fraction;
      yend= ystart + dys * fraction;
    } else {
      x1= x_s[i-1];
      y1= y_s[i-1];
      s1= sqrtf((x1-xstart)*(x1-xstart)+(y1-ystart)*(y1-ystart));
      x2= x_s[i];
      y2= y_s[i];
      for (j=0; j<10; j++) {
	xm= (x1+x2)*0.5;
	ym= (y1+y2)*0.5;
	sm= sqrtf((xm-xstart)*(xm-xstart)+(ym-ystart)*(ym-ystart));
	if ((s1-slim)*(sm-slim)<=0.) {
	  x2= xm;
	  y2= ym;
	} else {
	  x1= xm;
	  y1= ym;
	  s1= sm;
	}
      }
      xend= (x1+x2)*0.5;
      yend= (y1+y2)*0.5;
    }


    if (ncount%2==0){
      ndrawflag++;
      
      if (ndrawflag==1) {
	
	dxs= xend - xstart;
	dys= yend - ystart;
	xm= (xstart+xend)*0.5;
	ym= (ystart+yend)*0.5;
	
	glColor3f(1.,0.,0.);
	glPushMatrix();
	glTranslatef(xm, ym, 0.0);
	glRotatef(atan2(dys,dxs)*180./PI,0.0,0.0,1.0);
	glBegin(GL_POLYGON);
	for (j=0; j<nwarmflag; j++)
	  glVertex2f(xwarmflag[j],ywarmflag[j]);
	glEnd();
	glPopMatrix();
	
      } else {
	
	dxs= xend - xstart;
	dys= yend - ystart;
	xtop= (xstart+xend)*0.5 + dys*0.6;
	ytop= (ystart+yend)*0.5 - dxs*0.6;
	
	glColor3f(0.,0.,1.);
	glBegin(GL_POLYGON);
	glVertex2f(xstart,ystart);
	glVertex2f(xend,yend);
	glVertex2f(xtop,ytop);
	glEnd();
	
	ndrawflag=0;
      }
    }
    ncount++;
    
    dxs= xend - x_s[i-1];
    dys= yend - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r;
    s= 0.;

  }

  UpdateOutput();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}


/*
  Draws Squall line crosses on smooth line
 */
void WeatherFront::drawSquallLine(){
  //  cerr << "WeatherFront::drawSquallLine" << endl;
  // colds are blue triangles on the front

  const float PI=3.1415926535897932384626433832795;

  //float r= linewidth*2*getDwidth();
  float r= linewidth*getDwidth();
  int end= s_length;
  int ncount=0;

  float dxs,dys,fraction;
  float s,slim,sprev, xm,ym;
  int i;

  slim= r*2.;
  s= 0.;
  r*=1.5;
  i=1;

  glLineWidth(linewidth/2);

  while (i<end) {

    while (s<slim && i<end) {
      sprev= s;
      dxs= x_s[i] - x_s[i-1];
      dys= y_s[i] - y_s[i-1];
      s+= sqrtf(dxs*dxs+dys*dys);
      i++;
    }
    if (s<slim) break;

    i--;
    fraction= (slim-sprev)/(s-sprev);
    xm= x_s[i-1] + dxs * fraction;
    ym= y_s[i-1] + dys * fraction;

    if (ncount%2==0){

      glPushMatrix();
      glTranslatef(xm, ym, 0.0);
      glRotatef(atan2(dys,dxs)*180./PI,0.0,0.0,1.0);
      
      glBegin(GL_LINES);
      glVertex2f(-r,-r);
      glVertex2f( r, r);
      glVertex2f( r,-r);
      glVertex2f(-r, r);
      glEnd();
      
      glPopMatrix();
    }
    ncount++;

    dxs= xm - x_s[i-1];
    dys= ym - y_s[i-1];
    slim= sqrtf(dxs*dxs+dys*dys) + r*4.;
    s= 0.;

  }
  UpdateOutput();
}








void WeatherFront::flip(){
  int end = nodePoints.size();
  for (int j=0; j < end/2; j++){
    float x=nodePoints[j].x;   
    float y=nodePoints[j].y;
    bool joined = nodePoints[j].joined;
    nodePoints[j].x=nodePoints[end-1-j].x;
    nodePoints[j].y=nodePoints[end-1-j].y;
    nodePoints[j].joined=nodePoints[end-1-j].joined;
    nodePoints[end-1-j].x=x;
    nodePoints[end-1-j].y=y;
    nodePoints[end-1-j].joined=joined;
  }
  recalculate();
}


void WeatherFront::setType(int ty){
  if (-1<ty && ty<allFronts.size())
    type=ty;
  else if (ty==allFronts.size())
    type=0;
  else if (ty==-1)
    type =allFronts.size()-1;
  else 
    return;
  setIndex(allFronts[type].index);
  setBasisColor(allFronts[type].colour);
  setPlotVariables();
}

bool WeatherFront::setType(miString tystring){
#ifdef DEBUGPRINT
  cerr << "WeatherFront::setType(miString)=" << tystring <<  endl;
#endif
  if (frontTypes.find(tystring)!=frontTypes.end()){
    type = frontTypes[tystring];
    setType(type);
    return true;
  }
  return false;
}


void WeatherFront::setPlotVariables(){
  drawSig=false;
  switch (drawIndex){
    //significant weather - if active just a line, otherwise curls
  case SigweatherFront:
    if (currentState==active){
      setSpline(false);     
    }
    else{
      setSpline(true);
      drawSig=true;
    }
    break;
  case BlackSharpLine:
    setSpline(false);
    break;
  case BlackSmoothLine:
    setSpline(true);
    break;
  case RedSharpLine:
    setSpline(false);
    break;
  case RedSmoothLine:
    setSpline(true);
    break;
  default:
    // all fronts have spline interpolation
    setSpline(true);
  }
}


bool WeatherFront::setSpline(bool s){
  if (spline!=s){
    spline=s;
    recalculate();
    return true;
  }
  return false;
}





miString WeatherFront::writeTypeString()
{
  miString ret ="Object=Front;\n";
  ret+="Type=";
  ret+=allFronts[type].name;
  ret+=";\n";
  return ret;
}



void WeatherFront::drawSigweather(){
  const float PI=3.1415926535897932384626433832795;
  //cerr << "WeatherFront::drawSigweather"<< endl;
  recalculate();
  first=true;
  npoints=0;
  if (!smooth()) return;
  if (x_s != 0)  delete[] x_s;   
  if (y_s != 0)  delete[] y_s;
  x_s= y_s= 0;
  s_length= npoints;
  x_s=new float[s_length];
  y_s=new float[s_length];
  for (int i = 0; i < npoints; i++){ 
    x_s[i]=xplot[i];
    y_s[i]=yplot[i];         
  }
  if (xplot!=0) delete[] xplot;
  if (yplot!=0) delete[] yplot;
  xplot=yplot=0;
  //smooth once more for better fit...
  first=false;
  if (!smooth()) return;
  if (x_s != 0)  delete[] x_s; 
  if (y_s != 0)  delete[] y_s;
  x_s= y_s= 0;
  glLineWidth(siglinewidth);
  for (int i = 0; i < npoints-1; i++){          
    float deltay,deltax;
    deltay = yplot[i+1]-yplot[i];
    deltax = xplot[i+1]-xplot[i];
    float hyp = sqrtf(deltay*deltay+deltax*deltax);
    const int nflag= 19;
    float xflag[nflag], yflag[nflag];
    float flagstep= PI/(nflag-1);	  
    for (int j=0; j<nflag; j++) {
      xflag[j]= hyp/2*cos(j*flagstep);
      yflag[j]= hyp/2*sin(j*flagstep);
    }
    float xx1=xplot[i]+deltax/2;
    float yy1=yplot[i]+deltay/2;
    glPushMatrix();
    glTranslatef(xx1, yy1, 0.0);
    glRotatef(atan2(deltay,deltax)*180./PI,0.0,0.0,1.0);
    glBegin(GL_LINE_STRIP);
    for (int j=0; j<nflag; j++)
      glVertex2f(xflag[j],yflag[j]);
    glEnd();
    glPopMatrix();
  }  
  if (xplot!=0) delete[] xplot;
  if (yplot!=0) delete[] yplot;
  xplot=yplot=0;
  recalculate();
}


bool WeatherFront::smooth(){
  //cerr << "WeatherFront::smoth" << endl;
  //produces a curve with evenly spaced points
  float totalLength=0;
  for (int i = 0; i < s_length-1; i++){          
    float  deltay = y_s[i+1]-y_s[i];
    float  deltax = x_s[i+1]-x_s[i];
    float hyp = sqrtf(deltay*deltay+deltax*deltax);
    totalLength+=hyp;
  }
  int nplot;
  float radius;
  if (first){
    radius= siglinewidth*8*getDwidth();
    nplot= (int)(totalLength/radius)+1;
    if (nplot<2) return false;
  }
  else nplot=npoints;
  radius = totalLength/(nplot-1);
  xplot = new float[nplot];
  yplot = new float[nplot];
  xplot[0]=x_s[0];yplot[0]=y_s[0];
  int j,i_s=0;//index of x_s,y_s
  float totalDist=0; // distance
  bool end= false;
  for (j=1;j<nplot &&totalDist<totalLength && !end;j++){
    //starting point for this step
    float xstart=xplot[j-1];
    float ystart=yplot[j-1];
    //how many steps ???
    float dist=0; // distance
    int i = i_s;
    while(dist<radius){
      i++;
      float deltay = y_s[i]-ystart;
      float deltax = x_s[i]-xstart;
      float hyp = sqrtf(deltay*deltay+deltax*deltax);    
      dist+=hyp;      
      //next step
      xstart=x_s[i];
      ystart=y_s[i];
    }
    float deltax,deltay,hyp;
    deltay = ystart-yplot[j-1];
    deltax = xstart-xplot[j-1];
    hyp = sqrtf(deltay*deltay+deltax*deltax);    
    float cosalfa=deltax/hyp;
    float sinalfa=deltay/hyp;
    xplot[j]=xplot[j-1]+radius*cosalfa;
    yplot[j]=yplot[j-1]+radius*sinalfa; 
    deltay = yplot[j]-yplot[j-1];
    deltax = xplot[j]-xplot[j-1];
    hyp = sqrtf(deltay*deltay+deltax*deltax);    
    totalDist+=hyp;      
    i_s=i-1;
  }
  npoints=j;
  xplot[npoints-1]=x_s[s_length-1];
  yplot[npoints-1]=y_s[s_length-1];
  return true;
}









