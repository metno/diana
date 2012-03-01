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
#ifndef diPlotModule_h
#define diPlotModule_h

#include <diPlot.h>
#include <diDrawingTypes.h>
#include <diMapMode.h>
#include <diPrintOptions.h>
#include <puTools/miTime.h>
#include <diLocationPlot.h>
#include <vector>
#include <set>
#include <deque>
#include <diDisplayObjects.h>
#include <diEditObjects.h>
#include <diAreaObjects.h>
#include <miLogger/logger.h>
#include <miLogger/LogHandler.h>

class ObsPlot;
class SatPlot;
class MapPlot;
class AnnotationPlot;
class FieldManager;
class FieldPlotManager;
class FieldPlot;
class ObsManager;
class SatManager;
class ObjectManager;
class EditManager;
class GridAreaManager;
class Field;
class TrajectoryPlot;
class MeasurementsPlot;
class StationPlot;

using namespace std;

/**

  \brief Main plot engine

  The main plot engine, combining various plot layers into one map view
  - reroutes plot info strings to appropriate handlers
  - reroutes mouse and keyboard events
  - all plot layers stored within

*/

class PlotModule {
private:

  ObsManager *obsm;       // observation manager
  SatManager *satm;       // raster-data manager
  ObjectManager *objm;    // met.objects
  EditManager *editm;     // editing/drawing manager
  GridAreaManager *aream; // Polygon edit manager for continuous drwaing/editing

  friend class EditManager;   // editing and drawing

  friend class ObjectManager; // editing and drawing

  Plot splot;             // keep a Plot superclass for static members

  vector<ObsPlot*> vop;   // vector of observation plots
  vector<SatPlot*> vsp;   // vector of satellite plots
  vector<FieldPlot*> vfp; // vector of field plots
  vector<MapPlot*> vmp;   // vector of map plots
  vector<TrajectoryPlot*>vtp; // vector of trajectory plots
  vector<MeasurementsPlot*>vMeasurementsPlot; // vector of measurements plots
  vector<AnnotationPlot*> vap; // vector of annotation plots
  vector <AnnotationPlot*> obsVap; //display obs annotation
  vector <AnnotationPlot*> objectVap; //display object label
  vector <AnnotationPlot*> editVap;   //edit object labels
  DisplayObjects objects;             //objects to be displayed
  vector <AreaObjects> vareaobjects;  //QED areas
  AnnotationPlot* apEditmessage; // special edit message (region shown,...)

  FieldManager *fieldm;   // field manager
  FieldPlotManager *fieldplotm;   // field plot manager

  vector<miutil::miString> annotationStrings;//orig. strings from setup

  bool mapdefined;       // area/projection defined for plot
  bool mapDefinedByUser; // map area set by user
  bool mapDefinedByData; // for initial maps with fields or sat.
  bool mapDefinedByView; // for initial maps without any data

  Area requestedarea;
  Area previousrequestedarea;

  float plotw, ploth;     // width and height of plotwindow (pixels)
  bool resizezoom;      // should resizing zoom splot.area?
  bool showanno;        // show standard annotations

  // postscript production members
  printOptions printoptions;
  bool hardcopy;
  miutil::miString bgcolourname;

  // drawing and edit members
  bool inEdit;                 // edit in progress
  mapMode mapmode;             // current mapmode
  bool prodtimedefined;        // producttime is set
  miutil::miTime producttime;          // proper product time

  EditObjects editobjects;       // fronts,symbols,areas
  EditObjects combiningobjects;  // areaborders and textstrings

  vector <StationPlot*> stationPlots;//stations to be plotted

  vector <LocationPlot*> locationPlots; // location (vcross,...) to be plotted

  // event-handling
  float oldx, oldy,xmoved,ymoved;
  float newx, newy;
  Area myArea;
  deque<Area> areaQ;
  int areaIndex;
  bool areaSaved;
  bool dorubberband;
  bool dopanning;
  bool keepcurrentarea;

  struct obsOneTime{
    vector<ObsPlot*> vobsOneTime;// vector of obs plots, same time
  };
  vector<obsOneTime> vobsTimes;   // vector of structs, different times
  int obsnr; //which obs time
  int obsTimeStep;

  miutil::miString levelSpecified;  // for level up/down changes
  miutil::miString levelCurrent;

  miutil::miString idnumSpecified;  // for idnum up/down changes (class/type/...)
  miutil::miString idnumCurrent;

  vector<PlotElement> plotelements;

  // static members
  static GridConverter gc;   // gridconverter class

  void setEditMessage(const miutil::miString&); // special Edit message (shown region,...)
  //Plot underlay
  void plotUnder();
  //Plot overlay
  void plotOver();

  //Free fields in FieldPlot
  void freeFields(FieldPlot *);


protected:
  void PlotAreaSetup();

public:
  // Constructor
  PlotModule();
  // Destructor
  ~PlotModule();

  /// the main plot routine (plot for underlay, plot for overlay)
  void plot(bool under =true, bool over =true);
  /// split plot info strings and reroute them to appropriate handlers
  void preparePlots(const vector<miutil::miString>&);
  /// handles fields plot info strings
  void prepareFields(const vector<miutil::miString>&);
  /// handles observations plot info strings
  void prepareObs(const vector<miutil::miString>&);
  /// handles map plot info strings
  void prepareMap(const vector<miutil::miString>&);
  /// handles images plot info strings
  void prepareSat(const vector<miutil::miString>&);
  /// handles met. objects plot info strings
  void prepareObjects(const vector<miutil::miString>&);
  /// handles trajectory plot info strings
  void prepareTrajectory(const vector<miutil::miString>&);
  /// handles annotation plot info strings
  void prepareAnnotation(const vector<miutil::miString>&);
  /// plot annotations
  vector<Rectangle> plotAnnotations();

  /// get annotations from all plots
  void setAnnotations();
  /// get current Area
  const Area& getCurrentArea(){return splot.getMapArea();}

  /// update FieldPlots
  bool updateFieldPlot(const vector<miutil::miString>& pin);
  /// update all plot objects, returning true if successful
  bool updatePlots();
  /// toggle conservative map area
  void keepCurrentArea(bool b){keepcurrentarea= b;}

  /// delete all data vectors
  void cleanup();
  /// get static maparea in plot superclass
  Area& getMapArea(){return splot.getMapArea();}
  /// get plotwindow rectangle
  Rectangle& getPlotSize(){return splot.getPlotSize();}
  /// new size of plotwindow
  void setPlotWindow(const int&, const int&);
  /// receive rectangle in pixels
  void PixelArea(const Rectangle r);
  /// return latitude,longitude from physical x,y
  bool PhysToGeo(const float,const float,float&,float&);
  /// return physical x,y from physical latitude,longitude
  bool GeoToPhys(const float,const float,float&,float&);
  /// return map x,y from physical x,y
  void PhysToMap(const float,const float,float&,float&);
  /// return field grid x,y from map x,y if field defined and map proj = field proj
  bool MapToGrid(const float,const float,float&,float&);
  /// calculate distance between two points
  float GreatCircleDistance(float lat1,float lat2,float lon1 ,float lon2);
  /// start hardcopy plot
  void startHardcopy(const printOptions& po);
  /// end hardcopy plot
  void endHardcopy();
  /// set managers
  void setManagers(FieldManager*,
		   FieldPlotManager*,
		   ObsManager*,
		   SatManager*,
		   ObjectManager*,
		   EditManager*,
		   GridAreaManager*);

  /// return current plottime
  void getPlotTime(miutil::miString&);
  /// return current plottime
  void getPlotTime(miutil::miTime&);
  /// return data times (fields,images, observations, objects and editproducts)
  void getPlotTimes(vector<miutil::miTime>& fieldtimes,vector<miutil::miTime>& sattimes,
		    vector<miutil::miTime>& obstimes,vector<miutil::miTime>& objtimes,
		    vector<miutil::miTime>& ptimes);
  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(set<miutil::miTime>& okTimes,
			   set<miutil::miTime>& constTimes,
			   const vector<miutil::miString>& pinfos,
			   bool allTimes=true);

  /// set plottime
  bool setPlotTime(miutil::miTime&);
  ///return black/white depending on background colour
  Colour getContrastColour();

  // Observation
  /// Update ObsPlots if data files have changed
  void updateObs();
  ///find obs in pos x,y
  bool findObs(int x,int y);
  ///get id of obsevation i pos x,y
  bool getObsName(int x,int y, miutil::miString& name);
  ///plot next/prev set of observations(PageUp/PageDown)
  void nextObs(bool next);
  ///in edit mode: change obs time, leave the rest unchanged
  void obsTime(const keyboardEvent& me, EventResult& res);
  ///sets the step used in obsTime()
  void obsStepChanged(int step);

  // Stations
  ///put StationPlot in list of StationPlots
  void putStations(StationPlot*);
  ///make StationPlot and put it in list of StationPlots
  void makeStationPlot(const miutil::miString& commondesc, const miutil::miString& common,
		       const miutil::miString& description, int from,
		       const  vector<miutil::miString>& data);
  ///find station in position x,y in StationPlot with name and id
  miutil::miString findStation(int x, int y,miutil::miString name,int id=-1);
  ///look for station in position x,y in all StationPlots
  void findStations(int x, int y, bool add,
		    vector<miutil::miString>& name,vector<int>& id,
		    vector<miutil::miString>& station);
  ///get editable stations, returns name/id of StationPlot and stations
  void getEditStation(int step, miutil::miString& name, int& id,
		      vector<miutil::miString>& stations);
  ///send command to StationPlot with name and id
  void stationCommand(const miutil::miString& Command,
		      vector<miutil::miString>& data,
		      const miutil::miString& name="", int id=-1,
		      const miutil::miString& misc="");
  ///send command to StationPlot with name and id
  void stationCommand(const miutil::miString& Command,
		      const miutil::miString& name="", int id=-1);
  /**
   * This method is only sound as long as all Stations in all StationPlots have the same scale.
   * @return Current scale for the first Station in the first StationPlot
   */
  float getStationsScale();

  /**
   * Set new scale for all Stations.
   * @param new_scale New scale (1.0 original size)
   */
  void setStationsScale(float new_scale);

  //Area
  ///put area into list of area objects
  void makeAreas(miutil::miString name,miutil::miString areastring, int id);
  ///send command to right area object
  void areaCommand(const miutil::miString& command, const miutil::miString& dataSet,
		   const miutil::miString& data, int id );
  ///find areas in position x,y
  vector <selectArea> findAreas(int x, int y, bool newArea=false);

  // locationPlot (vcross,...)
  void putLocation(const LocationData& locationdata);
  void updateLocation(const LocationData& locationdata);
  void deleteLocation(const miutil::miString& name);
  void setSelectedLocation(const miutil::miString& name,
			 const miutil::miString& elementname);
  miutil::miString findLocation(int x, int y, const miutil::miString& name);

  vector<miutil::miString> getFieldModels();

  // Trajectories
  /// handles trajectory plot info strings
  void trajPos(vector<miutil::miString>&);
  vector<miutil::miString> getTrajectoryFields();
  bool startTrajectoryComputation();
  void stopTrajectoryComputation();
  // print trajectory positions to file
  bool printTrajectoryPositions(const miutil::miString& filename);

  // Measurements (distance, velocity)
  void measurementsPos(vector<miutil::miString>&);

  //Satellite and radar
  /// get name++ of current channels (with calibration)
  vector<miutil::miString> getCalibChannels();
  ///show pixel values in status bar
  vector<SatValues> showValues(int, int);
  ///get satellite name from all SatPlots
  vector <miutil::miString> getSatnames();
  ///satellite follows main plot time
  void setSatAuto(bool, const miutil::miString&, const miutil::miString&);

  //show or hide all annotations (for fields, observations, satellite etc.)
  void showAnnotations(bool on){showanno=on;}
  /// mark editable annotationPlot if x,y inside plot
  bool markAnnotationPlot(int, int);
  /// get text of marked and editable annotationPlot
  miutil::miString getMarkedAnnotation();
  /// change text of marked and editable annotationplot
  void changeMarkedAnnotation(miutil::miString text,int cursor=0,
			      int sel1=0,int sel2=0);
  /// delete marked and editable annotation
  void DeleteMarkedAnnotation();
  /// start editing annotations
  void startEditAnnotation();
  /// stop editing annotations
  void stopEditAnnotation();
  /// go to next element in annotation being edited
  void editNextAnnoElement();
  /// go to last element in annotation being edited
  void editLastAnnoElement();
  /// return vector miutil::miStrings with edited annotation for product prodname
  vector <miutil::miString> writeAnnotations(miutil::miString prodname);
  /// put info from saved edit labels into new annotation
  void updateEditLabels(vector <miutil::miString> productLabelstrings,
		    miutil::miString productName, bool newProduct);

  //Objects
  ///objects follow main plot time
  void setObjAuto(bool autoF);

  /// push a new area onto the area stack
  void areaInsert(Area, bool);
  /// respond to shortcuts to move to predefined areas
  void changeArea(const keyboardEvent& me);
  /// zoom to specified rectangle
  void zoomTo(const Rectangle& rectangle);
  /// zoom out (about 1.3 times)
  void zoomOut();

  // plotelements methods
  /// return PlotElement data (for the speedbuttons)
  vector<PlotElement>& getPlotElements();
  /// enable one PlotElement
  void enablePlotElement(const PlotElement& pe);

  // keyboard/mouse events
  /// send one mouse event
  void sendMouseEvent(const mouseEvent& me, EventResult& res);
  /// send one keyboard event
  void sendKeyboardEvent(const keyboardEvent& me, EventResult& res);

  // return settings formatted for log file
  vector<miutil::miString> writeLog();
  // read settings from log file data
  void readLog(const vector<miutil::miString>& vstr,
	       const miutil::miString& thisVersion, const miutil::miString& logVersion);

};

#endif

