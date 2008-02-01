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
#ifndef diController_h
#define diController_h

#include <diCommonTypes.h>
#include <diCommonFieldTypes.h>
#include <diMapMode.h>
#include <diDrawingTypes.h>
#include <diSetupParser.h>
#include <diPrintOptions.h>
#include <diArea.h>
#include <miString.h>
#include <miTime.h>
#include <diLocationPlot.h>
#include <vector>
#include <set>
#include <diColour.h>

#include <profet/ProfetController.h>
#include <profet/ProfetGUI.h>

using namespace std;

class PlotModule;
class FieldManager;
class ObsManager;
class SatManager;
class EditManager;
class GridAreaManager;
class ObjectManager;
class StationPlot;
class MapManager;


/**

  \brief Ui gate to main Diana engine

  The controller class acts as a gate to all plot and editing handlers
  - initialisation of all handlers
  - ui independent code beyond this point
  - used both in interactive and batch version of Diana

*/

class Controller {

private:
  PlotModule    *plotm;
  FieldManager  *fieldm;
  ObsManager    *obsm;
  SatManager    *satm;
  ObjectManager *objm;
  EditManager   *editm;
  Profet::ProfetController * profetController;
  GridAreaManager *aream;

  SetupParser setupParser;

  bool editoverride; // do not route mouse/key-events to editmanager
  bool paintModeEnabled; 

public:
  Controller();
  ~Controller();

  EditManager*   getEditManager()   { return editm; };
//  GridEditManager* getGridEditManager()   { return gridm; };
  ObjectManager* getObjectManager() { return objm; };

  /// parse setup
  bool parseSetup();
  /// set new plotcommands
  void plotCommands(const vector<miString>&);
  /// call PlotModule.plot()
  void plot(bool over =true, bool under =true);
  /// get plotwindow corners in GL-coordinates
  void getPlotSize(float& x1, float& y1, float& x2, float& y2);
  /// get plot area (incl. projection)
  Area getMapArea();
  /// zoom out map
  void zoomOut();
  /// set plotwindow size in pixels (from GLwidget..)
  void setPlotWindow(const int, const int);
  /// receive rectangle..
  void PixelArea(const int x1, const int y1,
		 const int x2, const int y2);
  /// return latitude,longitude from physical x,y
  void PhysToGeo(const float,const float,float&,float&);
  /// return physical x,y from latitude,longitude
  void GeoToPhys(const float,const float,float&,float&);
  /// return map x,y from physical x,y
  void PhysToMap(const float,const float,float&,float&);
  /// start hardcopy plot
  void startHardcopy(const printOptions&);
  /// end hardcopy plot
  void endHardcopy();
  /// return current plottime
  void getPlotTime(miString&);
  /// return current plottime
  void getPlotTime(miTime&);
  /// return data times (fields,images, observations, objects and editproducts)
  void getPlotTimes(vector<miTime>& fieldtimes,vector<miTime>& sattimes,
		    vector<miTime>& obstimes,vector<miTime>& objtimes,
		    vector<miTime>& ptimes);
  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(set<miTime>& okTimes,
			   set<miTime>& constTimes,
			   const vector<miString>& pinfos,
			   bool allTimes=true);
  /// returns the current product time
  bool getProductTime(miTime& t);
  /// returns the current product name
  miString getProductName();

  /// set plottime
  bool setPlotTime(miTime&);
  /// update plots
  void updatePlots();
  /// update level
  void updateLevel(const miString& levelSpec, const miString& levelSet);
  /// update idnum (class/type/...)
  void updateIdnum(const miString& idnumSpec, const miString& idnumSet);
  /// toggle area conservatism
  void keepCurrentArea(bool);
  /// update projection only
  void updateProjection();
  /// set colourindices from gui
  void setColourIndices(vector<Colour::ColourInfo>&);

  /// get colour wich is visible on the present background
  Colour getContrastColour();

  /// reload observations
  void updateObs();
  /// find obs in grid position x,y
  bool findObs(int,int);
  /// find name of obs in grid position x,y
  bool getObsName(int x,int y, miString& name);
  /// plot other observations
  void nextObs(bool);
  /// init hqcData from QSocket
  bool initHqcdata(int from, const miString&, const miString&,
		   const miString&, const vector<miString>&);
  /// update hqcData from QSocket
  void updateHqcdata(const miString&, const miString&,
		     const miString&, const vector<miString>&);
  /// select obs parameter to flag from QSocket
  void processHqcCommand(const miString&, const miString& ="");
  /// plot trajectory position
  void  trajPos(vector<miString>&);
  /// get trajectory fields
  vector<miString> getTrajectoryFields();
  /// start trajectory computation
  bool startTrajectoryComputation();
// print trajectory positions to file
  bool printTrajectoryPositions(const miString& filename);
  /// get field models used (for Vprof etc.)
  vector<miString> getFieldModels();
  /// obs time step changed
  void obsStepChanged(int);
  /// get name++ of current channels (with calibration) 
  vector<miString> getCalibChannels();
  /// show pixel values in status bar
  vector<SatValues> showValues(int, int);
  /// get satellite name from all SatPlots
  vector <miString> getSatnames();
  //show or hide all annotations (for fields, observations, satellite etc.)
  void showAnnotations(bool);
  /// mark editable annotationPlot if x,y inside plot
  bool markAnnotationPlot(int, int);
  /// get text of marked and editable annotationPlot
  miString getMarkedAnnotation();
  /// change text of marked and editable annotationplot
  void changeMarkedAnnotation(miString text,int cursor=0,
			      int sel1=0,int sel2=0);
  /// delete marked and editable annotation
  void DeleteMarkedAnnotation();
  /// start editing annotations
  void startEditAnnotation();
  /// stop editing annotations
  void stopEditAnnotation(miString prodname);
  /// go to next element in annotation being edited
  void editNextAnnoElement();
  /// go to last element in annotation being edited
  void editLastAnnoElement();
  /// Archive mode
  void archiveMode( bool );

  /// mouse events
  void sendMouseEvent(const mouseEvent& me, EventResult& res);
  /// keyboard events
  void sendKeyboardEvent(const keyboardEvent& me, EventResult& res);

  // edit and drawing methods
  /// get mode of main workarea
  mapMode getMapMode();

  // Sat-dialog routines
  /// get list of satfiles of class satellite and subclass file. if update is true read new list from disk
  const vector<SatFileInfo>& getSatFiles(const miString & satellite, const miString & file,bool update);
  /// returns colour palette for subproduct of class satellite and subclass file
  const vector<Colour>& getSatColours(const miString & satellite, const miString & file);
  /// returns channels for subproduct of class satellite and subclass file
  const vector<miString>& getSatChannels(const miString & satellite, const miString &file ,
					 int index=-1);
  /// returns true if satellite picture is a mosaic
  bool isMosaic(const miString &, const miString &);
  /// refresh list of satellite files
  void SatRefresh(const miString &, const miString &);
  /// returns information about whether list of satellite files have changed
  void satFileListUpdated();
  /// called when the dialog and timeSlider updated with info from satellite
  bool satFileListChanged();
  /// satellite follows main plot time
  void setSatAuto(bool,const miString &, const miString &);
  /// get satellite classes for uffda dialog
  void getUffdaClasses(vector <miString> &,vector <miString> &);
  /// returns true if uffda option enables
  bool getUffdaEnabled();
  /// returns adress to send mail from uffda dialog
  miString getUffdaMailAddress();
  /// return button names for SatDialog
  SatDialogInfo initSatDialog();

  // Obs-dialog methods
  /// return button names for ObsDialog
  ObsDialogInfo initObsDialog();
  /// return button names for ObsDialog ... ascii files (when activated)
  ObsDialogInfo updateObsDialog(const miString& name);
  /// get observation times for plot types name
  vector<miTime> getObsTimes( vector<miString> name);

  // Field-dialog methods
  /// return model/file groups and contents to FieldDialog
  vector<FieldDialogInfo> initFieldDialog();
  /// return plot options for all defined plot fields in setup
  void getAllFieldNames(vector<miString>& fieldNames,
			set<miString>& fieldprefixes,
			set<miString>& fieldsuffixes);
  ///return levels
  vector<miString> getFieldLevels(const miString& pinfo);
  /// return FieldGroupInfo for one model to FieldDialog
  void getFieldGroups(const miString& modelNameRequest,
		      miString& modelName, vector<FieldGroupInfo>& vfgi);
  /// return available times for the requested fields
  vector<miTime> getFieldTime(const vector<FieldTimeRequest>& request,
			      bool allTimeSteps);

  // Map-dialog methods
  MapDialogInfo initMapDialog();
  bool MapInfoParser(miString& str, MapInfo& mi, bool tostr);

  // Edit-dialog methods --------
  /// returns current EditDialogInfo for gui
  EditDialogInfo initEditDialog();
  /// get text list from complex weather symbol
  set <miString> getComplexList();
  /// return class specifications from fieldplot setup to EditDialog
  miString getFieldClassSpecifications(const miString& fieldName);

  // object-dialog methods
  /// get ObjectNames from setup file to be used in dialog etc.
  vector<miString> getObjectNames(bool);
  ///objects follow main plot time
  void setObjAuto(bool autoFile);
  /// returns list of objectfiles for use in dialog
  vector<ObjFileInfo> getObjectFiles(miString objectname, bool refresh);
  /// decode string with types of objects to plot
  map <miString,bool> decodeTypeString(miString);

  // various GUI-methods
  vector< vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);
  bool getQuickMenus(vector<QuickMenuDefs>& qm);

  //stations
  void putStations(StationPlot*);
  void makeStationPlot(const miString& commondesc, const miString& common,
		       const miString& description, int from,
		       const  vector<miString>& data);
  void deleteStations(miString name);
  void deleteStations(int id=-2);
  miString findStation(int, int,miString name,int id=-1);
  void findStations(int, int, bool add, vector<miString>& name,vector<int>& id,
		    vector<miString>& station);
  void getEditStation(int step, miString& name, int& id,
		      vector<miString>& stations);
  void stationCommand(const miString& Command,
		      vector<miString>& data,
		      const miString& name="", int id=-1,
		      const miString& misc="");
  void stationCommand(const miString& Command,
		      const miString& name="", int id=-1);

  //areas
  ///put area into list of area objects
  void makeAreas(const miString& name, miString areaString, int id=-1);
  ///send command to right area object
  void areaCommand(const miString& command, const miString& dataSet,
		   const miString& data, int id );
  ///find areas in position x,y
  vector <selectArea> findAreas(int x, int y, bool newArea=false);

  // location (vcross,...)
  void putLocation(const LocationData& locationdata);
  void updateLocation(const LocationData& locationdata);
  void deleteLocation(const miString& name);
  void setSelectedLocation(const miString& name,
			 const miString& elementname);
  miString findLocation(int x, int y, const miString& name);

  vector<InfoFile> getInfoFiles();

  vector<PlotElement>& getPlotElements();
  void enablePlotElement(const PlotElement& pe);

/********************* reading and writing log file *******************/
 vector<miString> writeLog();
 void readLog(const vector<miString>& vstr,
	      const miString& thisVersion, const miString& logVersion);
  bool initProfet();  
  Profet::ProfetController * getProfetController(); 
  bool setProfetGUI(Profet::ProfetGUI * gui); 
  GridAreaManager * getAreaManager() { return aream; }
  ///Enable and disable paint mode
  void setPaintModeEnabled(bool);  

};

#endif
