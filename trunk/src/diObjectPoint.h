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
#ifndef _ObjectPoint_h
#define _ObjectPoint_h


/* Created at Thu Jul 18 14:15:29 2002 */

#include <diField/diRectangle.h>

using namespace std; 

/**

  \brief One point of an ObjectPlot

  can be marked joined etc

*/

class ObjectPoint {
private:
  float x;
  float y;
  bool marked;
  bool joined;
  Rectangle myRect;

  friend class ObjectPlot;   
  friend class WeatherFront;
  friend class WeatherSymbol;
  friend class WeatherArea;
  friend class AreaBorder;
   
public:
  ObjectPoint();
  ObjectPoint(float xin,float yin);
  /// true if point xm,ym is in rectangle with sides fdeltaw around point
  bool isInRectangle(float xm,float ym, float fdeltaw);
  /// distance from point to xm,ym
  float distSquared(float xm, float ym);
  /// true if points in same position
  bool operator==(const ObjectPoint &rhs ) const;
};

#endif
