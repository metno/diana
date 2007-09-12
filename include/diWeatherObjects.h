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
#ifndef _diWeatherObjects_h
#define _diWeatherObjects_h
#include <vector>
#include <diArea.h>
#include <diObjectPlot.h>
#include <diAreaBorder.h>
#include <diUndoFront.h>
#include <diMapMode.h>
#include <diGridConverter.h>

using namespace std; 


/**

  \brief WeatherObjects to be edited or displayed


*/

class WeatherObjects{
public:

  WeatherObjects();
  ~WeatherObjects(){}

  /// the weather objects to plot
  vector<ObjectPlot*> objects;   
  /// clears all variables
  void clear();
  virtual void init(){}
  /// returns true of no objects or labels
  bool empty();
  /// plot all weather objects
  void plot();
 /// check if objectplots are enabled
  bool isEnabled();
 /// enable/disable objectplots
  void enable(const bool b);
  /// set prefix for object files
  void setPrefix(miString p){prefix=p;}
  /// sets the object time 
 void setTime(miTime t){itsTime=t;}
  /// gets the object time
  miTime getTime(){if (itsTime.undef()) return ztime; else return itsTime;}
  /// returns number of object plots
  int getSize(){return objects.size();}
  /// return objects area
  Area getArea(){return itsArea;}
  /// set objects area
  void setArea(const Area & area){itsArea=area;};
  /// updates the bound box
  void updateObjects();
  /// change projection to newAreas projection
  bool changeProjection(const Area& newArea);
  /// read file with comments
  bool readEditCommentFile(const miString fn);
  /// returns objects' old comments
  miString readComments();

  ///return oldLabels from object file
  vector <miString> getObjectLabels();
  /// return new edited labels 
  vector <miString> getEditLabels(); 

  /// read file with weather objects and change projection to newArea
  bool readEditDrawFile(const miString ,const Area& newArea );
  /// read  string with weather objects and change projection to newArea
  bool readEditDrawString(const miString ,const Area& newArea, bool replace=false);
  /// read file with area borders (for combining analyses)
  bool readAreaBorders(const miString ,const Area& );
  /// write file with area borders (for combining analyses)
  bool writeAreaBorders(const miString);
  /// writes string with edited objects
  miString writeEditDrawString(const miTime& );
  /// sets which object types should be plotted
  void setSelectedObjectTypes(miString t){useobject = decodeTypeString(t);}
  /// returns number of object of this type
  int objectCount(int type );
  /// add an object 
  void addObject(ObjectPlot * object, bool replace=false);
  /// remove an object
  vector<ObjectPlot*>::iterator removeObject(vector<ObjectPlot*>::iterator );
 /// the file the objects are read from
  miString filename;     
  /// decode string with types of objects to plot
  static map <miString,bool> decodeTypeString(miString);
  /// x,y for copied objects
  float xcopy,ycopy; 

private:

  map<miString,bool> useobject;
  static miTime ztime;
  GridConverter gc;              // gridconverter class
  bool enabled;


protected:

  miString prefix;               //VA,VV,VNN...   
  Area itsArea;                  // current object area
  Area geoArea;



  miTime itsTime;                //plot time 
  miString itsOldComments;       // the comment string to edit
  vector <miString> itsLabels;            //edited labels
  vector <miString> itsOldLabels;         //labels read in from object file

  //static members
  static miTime timeFromString(miString timeString);
  static miString stringFromTime(const miTime& t,bool addMinutes);

};


#endif

