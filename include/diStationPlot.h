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
#ifndef StationPlot_h
#define StationPlot_h

#include <vector>
#include <deque>
#include <iostream>
#include <diPlot.h>
#include <diPlotOptions.h>
#include <miString.h>
#include <miCoordinates.h>
#include <diCommonTypes.h>
#include <diColour.h>
#include <diArea.h>

using namespace std;

  /**
     \brief text and alignment for one stationPlot point
  */
struct stationText{
  miString text;
  Alignment hAlign;
};


  /**
     \brief properties for a stationplot point

       coordinates, image and text to be shown etc.
  */
struct Station{
  miString name;
  float lat;
  float lon;
  miString image;
  miString image2;
  bool isVisible;
  bool isSelected;
  bool edit;
  int dd,ff,north;
  int alpha;
  Colour colour;
  vector <stationText> vsText;
};


/**
   \brief Plot pickable stations/positions on the map

   Contains data for a set of stations.
   Stations can be displayed by an image or a simple drawing
   with or without name/text. Misc options to set/change "state"
   of each position in the set.
*/
class StationPlot: public Plot{

public:
  //constructors
  StationPlot(){}
  /// constructor with station longitudes and latitudes
  StationPlot(const vector <float> & lons, const vector <float> & lats);
  /// constructor with station names,longitudes and latitudes
  StationPlot(const vector <miString> & names,const vector <float> & lons,
	      const vector <float> & lats);
  /// constructor with station names,longitudes,latitudes and images
  StationPlot(const vector <miString> & names,const vector <float> & lons,
	      const vector <float> & lats, const vector <miString> images);
  StationPlot(const miString& commondesc,
	      const miString& common,
	      const miString& description,
	      int from,
	      const  vector<miString>& data);
  //destructor
  ~StationPlot();


  /// plot stations
  bool plot();
  bool plot(const int){return false;}
  /// hide stations
  void hide();
  // show stations
  void show();
  /// unselects all stations
  void unselect();
  /// returns true if stationplot visible
  bool isVisible();
  /// change stationplot projection
  bool changeProjection();
  /// find stations in position x and y
  vector<miString> findStation(int x, int y, bool add=false);
  /// set station with name station to selected<br> if add is false, unselect all stations first
  void setSelectedStation(miString station, bool add=false);
  /// set station number i to selected<br> if add is false, unselect all stations first
  void setSelectedStation(int i, bool add=false);
  /// get annotation
  void getStationPlotAnnotation(miString &str,Colour &col);
  /// set annotation
  void setStationPlotAnnotation(miString &str);
  /// set name/plotname
  void setName(miString nm);
  /// return name
  miString getName(){return name;}
  /// set id to i
  void setId(int i){id = i;}
  /// return id
  int getId(){return id;}
  int getPriority(){return priority;}
  /// set normal and selected image to im1
  void setImage(miString im1);
  /// set normal image to im1, selected image to im2
  void setImage(miString im1,miString im2);
  /// clears all text
  void clearText();
  /// if normal=true write name on all plotted stations, if selected=true write name on all selected stations
  void setUseStationName(bool normal, bool selected);
  void setIcon(miString icon){iconName = icon;}
  miString getIcon(){return iconName;}
  void setEditStations(const vector<miString>& );
  bool getEditStation(int step, miString& name, int& id,
		      vector<miString>& stations, bool& updateArea);
  bool stationCommand(const miString& Command,
		      vector<miString>& data,
		      const miString& misc="");
  bool stationCommand(const miString& Command);
  miString stationRequest(const miString& Command);

  friend bool operator==(const StationPlot& lhs, const StationPlot& rhs)
  { return (lhs.stations.size()==rhs.stations.size()) ; }
  friend bool operator>(const StationPlot& lhs, const StationPlot& rhs)
  { return (lhs.stations.size()>rhs.stations.size()); }
  friend bool operator<(const StationPlot& lhs, const StationPlot& rhs)
  { return (lhs.stations.size()<rhs.stations.size()); }


private:

  enum thingToPlot{redRectangle,greyRectangle,redCross,
		   yellowCircle,whiteRectangle,redCircle};

  vector <Station*> stations; //stations, name, lon, lat etc...

  //  void addStation(const miString names);
  void addStation(const float lon, const float lat,
		  const miString name="",
		  const miString image="",
		  int alpha=255);
  void defineCoordinates();
  void init();
  void plotStation(int i);
  void glPlot(thingToPlot tp,float x, float y, float w, float h);
  void plotWind(int i, float x, float y,
		bool classic=false, float scale=1);

  vector <float> xplot; //x-positions to plot in current projection
  vector <float> yplot; //y-positions to plot in current projection
  Area oldarea;   //plotarea corresponding to xplot,yplot

  bool visible;
  miString annotation;
  miString name; //f.ex. "vprof"
  int id;
  int priority;
  bool useImage;
  bool useStationNameNormal,useStationNameSelected;
  bool showText;
  int textSize;
  Colour textColour;
  miString textStyle;
  miString imageNormal,imageSelected;
  miString iconName;
  int editIndex; //last selected editStation
  int index; //last selected

  float PI;
  GLuint circle;


  static miString ddString[16]; // NNØ,NØ,ØNØ,Ø,ØSØ etc.
 };

#endif






