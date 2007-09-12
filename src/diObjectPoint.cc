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
#include <fstream>
#include <iostream>
#include <diObjectPoint.h>

/* Created at Thu Jul 18 14:14:53 2002 */

// Default constructor
ObjectPoint::ObjectPoint() {
#ifdef DEBUGPRINT
  cerr << "Ny ObjectPoint() laget" << endl;
#endif
  x=0;
  y=0;
  marked=false;
  joined=false;
}


// x,y constructor
ObjectPoint::ObjectPoint(float xin,float yin) {
#ifdef DEBUGPRINT
  cerr << "Ny ObjectPoint(float,float) laget" << endl;
#endif
  x=xin;
  y=yin;
  marked=false;
  joined=false;
}


// Equality operator
bool ObjectPoint::operator==(const ObjectPoint &rhs) const{
  //true if points in same position
  if (x==rhs.x && y==rhs.y)
    return true;
  return false;
}

//distance from point xm,ym
float ObjectPoint::distSquared(float xm, float ym){
  return (x-xm)*(x-xm)+(y-ym)*(y-ym);
}

//check if point xm,ym is in rectangle with sides fdeltaw around point
bool ObjectPoint::isInRectangle(float xm,float ym, float fdeltaw){
  //cerr << "ObjectPoint::isInRectangle" << endl;
  myRect.x1=x - fdeltaw;
  myRect.x2=x + fdeltaw;
  myRect.y1=y - fdeltaw;
  myRect.y2=y + fdeltaw;
  if (myRect.isinside(xm,ym)){
    //cerr << "ObjectPoint::isInRectangle return true" << endl;
    return true;
  }
  //cerr << "ObjectPoint::isInRectangle return false" << endl;
  return false;
}




