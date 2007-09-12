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
#ifndef _diShapeObject_h
#define _diShapeObject_h
#include <diObjectPlot.h>
#include <miString.h>
#include <shapefil.h>

using namespace std; 

/**

  \brief Plots generic shape data 
  
*/
class ShapeObject: public ObjectPlot
{
private:
  
  vector <Colour> colours;
  miString fname;            //field name to colour by
  map <miString,Colour> stringcolourmap; //descr, colour
  map <int,Colour> intcolourmap; //descr, colour
  map <double,Colour> doublecolourmap; //descr, colour
  bool colourmapMade;        //true if colour map made  
  bool stringcolourmapMade;
  bool intcolourmapMade;
  bool doublecolourmapMade;
  vector<miString> dbfStringDescr;
  vector<int> dbfIntDescr;
  vector<double> dbfDoubleDescr;

  void makeColourmap();
  void writeCoordinates();

  vector <SHPObject*> shapes;   

  vector<miString> dbfIntName;
  vector<miString> dbfDoubleName;
  vector<miString> dbfStringName;
  vector< vector<int> >      dbfIntDesc;
  vector< vector<double> >   dbfDoubleDesc;
  vector< vector<miString> > dbfStringDesc;



  int readDBFfile(const miString& filename,
		  vector<miString>& dbfIntName, 
		  vector< vector<int> >& dbfIntDesc,
		  vector<miString>& dbfDoubleName,
		  vector< vector<double> >&   dbfDoubleDesc,
		  vector<miString>& dbfStringName, 
		  vector< vector<miString> >& dbfStringDesc);
  
public:
  ShapeObject();
  ShapeObject(const ShapeObject & rhs);
  ~ShapeObject();
  bool read(miString filename);
  virtual bool plot();


  virtual int getXYZsize();
  virtual vector<float> getX();
  virtual vector<float> getY();
  virtual void setXY(vector<float> x,vector <float> y);
  virtual bool getAnnoTable(miString & str);

};

#endif




