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
#include <diField/diCommonFieldTypes.h>
#include <diMapMode.h>
#include <diDrawingTypes.h>
#include <diSetupParser.h>
#include <diPrintOptions.h>
#include <diField/diArea.h>
#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <diLocationPlot.h>
#include <vector>
#include <set>
#include <diField/diColour.h>

#ifdef PROFET

#include <profet/ProfetController.h>
#include <profet/ProfetGUI.h>
#endif

using namespace std;

class PlotModule;
class FieldManager;
class FieldPlotManager;
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
  FieldPlotManager  *fieldplotm;
  ObsManager    *obsm;
  SatManager    *satm;
  ObjectManager *objm;
  EditManager   *editm;
#ifdef PROFET
  Profet::ProfetController * profetController;
#endif

  GridAreaManager *aream;

  SetupParser setupParser;

  bool editoverride; // do not route mouse/key-events to editmanager
  bool paintModeEnabled;
  bool scrollwheelZoom;

public:
  Controller();
  ~Controller();

  EditManager*   getEditManager()   { return editm; };
  ObjectManager* getObjectManager() { return objm; };
  FieldManager* getFieldManager() { return fieldm; };
  /// init static FontManager in class Plot
  void restartFontManager();
  /// parse setup
  bool parseSetup();
  /// set new plotcommands
  void plotCommands(const vector<miutil::miString>&);
  /// call PlotModule.plot()
  void plot(bool over =true, bool under =true);
  /// get plotwindow corners in GL-coordinates
  void getPlotSize(float& x1, float& y1, float& x2, float& y2);
  /// get plot area (incl. projection)
  Area getMapArea();
  /// zoom to rectangle r
  void zoomTo(const Rectangle & r);
  /// zoom out map
  void zoomOut();
  /// set plotwindow size in pixels (from GLwidget..)
  void setPlotWindow(const int, const int);
  /// receive rectangle..
  void PixelArea(const int x1, const int y1,
		 const int x2, const int y2);
  /// return latitude,longitude from physical x,y
  bool PhysToGeo(const float,const float,float&,float&);
  /// return physical x,y from latitude,longitude
  bool GeoToPhys(const float,const float,float&,float&);
  /// return map x,y from physical x,y
  void PhysToMap(const float,const float,float&,float&);
  /// return field grid x,y from map x,y if field defined and map proj = field proj
  bool MapToGrid(const float,const float,float&,float&);
  /// start hardcopy plot
  void startHardcopy(const printOptions&);
  /// end hardcopy plot
  void endHardcopy();
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
  /// returns the current product time
  bool getProductTime(miutil::miTime& t);
  /// returns the current product name
  miutil::miString getProductName();

  /// set plottime
  bool setPlotTime(miutil::miTime&);
  /// update plots
  void updatePlots();
  /// update FieldPlots
  void updateFieldPlot(const vector<miutil::miString>& pin);
  /// toggle area conservatism
  void keepCurrentArea(bool);
  /// get current area
  const Area& getCurrentArea();
  /// set colourindices from gui
  void setColourIndices(vector<Colour::ColourInfo>&);

  /// get colour wich is visible on the present background
  Colour getContrastColour();

  /// reload observations
  void updateObs();
  /// find obs in grid position x,y
  bool findObs(int,int);
  /// find name of obs in grid position x,y
  bool getObsName(int x,int y, miutil::miString& name);
  /// plot other observations
  void nextObs(bool);
  /// init hqcData from QSocket
  bool initHqcdata(int from, const miutil::miString&, const miutil::miString&,
		   const miutil::miString&, const vector<miutil::miString>&);
  /// update hqcData from QSocket
  void updateHqcdata(const miutil::miString&, const miutil::miString&,
		     const miutil::miString&, const vector<miutil::miString>&);
  /// select obs parameter to flag from QSocket
  void processHqcCommand(const miutil::miString&, const miutil::miString& ="");
  /// plot trajectory position
  void  trajPos(vector<miutil::miString>&);
  /// plot radar echo position
  void  radePos(vector<miutil::miString>&);
  /// get trajectory fields
  vector<miutil::miString> getTrajectoryFields();
  /// get radarecho fields
  vector<miutil::miString> getRadarEchoFields();
  /// start trajectory computation
  bool startTrajectoryComputation();
// print trajectory positions to file
  bool printTrajectoryPositions(const miutil::miString& filename);
  /// get field models used (for Vprof etc.)
  vector<miutil::miString> getFieldModels();
  /// obs time step changed
  void obsStepChanged(int);
  /// get name++ of current channels (with calibration)
  vector<miutil::miString> getCalibChannels();
  /// show pixel values in status bar
  vector<SatValues> showValues(int, int);
  /// get satellite name from all SatPlots
  vector <miutil::miString> getSatnames();
  //show or hide all annotations (for fields, observations, satellite etc.)
  void showAnnotations(bool);
  /// toggle scrollwheelzoom
  void toggleScrollwheelZoom(bool);
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
  void stopEditAnnotation(miutil::miString prodname);
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
  const vector<SatFileInfo>& getSatFiles(const miutil::miString & satellite, const miutil::miString & file,bool update);
  /// returns colour palette for subproduct of class satellite and subclass file
  const vector<Colour>& getSatColours(const miutil::miString & satellite, const miutil::miString & file);
  /// returns channels for subproduct of class satellite and subclass file
  const vector<miutil::miString>& getSatChannels(const miutil::miString & satellite, const miutil::miString &file ,
					 int index=-1);
  /// returns true if satellite picture is a mosaic
  bool isMosaic(const miutil::miString &, const miutil::miString &);
  /// refresh list of satellite files
  void SatRefresh(const miutil::miString &, const miutil::miString &);
  /// returns information about whether list of satellite files have changed
  void satFileListUpdated();
  /// called when the dialog and timeSlider updated with info from satellite
  bool satFileListChanged();
  /// returns information about whether list of observation files have changed
  void obsTimeListUpdated();
  /// called when the dialog and timeSlider updated with info from obs
  bool obsTimeListChanged();
  /// satellite follows main plot time
  void setSatAuto(bool,const miutil::miString &, const miutil::miString &);
  /// get satellite classes for uffda dialog
  void getUffdaClasses(vector <miutil::miString> &,vector <miutil::miString> &);
  /// returns true if uffda option enables
  bool getUffdaEnabled();
  /// returns adress to send mail from uffda dialog
  miutil::miString getUffdaMailAddress();
  /// return button names for SatDialog
  SatDialogInfo initSatDialog();

  // Obs-dialog methods
  /// return button names for ObsDialog
  ObsDialogInfo initObsDialog();
  /// return button names for ObsDialog ... ascii files (when activated)
  ObsDialogInfo updateObsDialog(const miutil::miString& name);
  /// get observation times for plot types name
  vector<miutil::miTime> getObsTimes( vector<miutil::miString> name);

  // Field-dialog methods
  /// return model/file groups and contents to FieldDialog
  vector<FieldDialogInfo> initFieldDialog();
  /// return plot options for all defined plot fields in setup
  void getAllFieldNames(vector<miutil::miString>& fieldNames,
			set<std::string>& fieldprefixes,
			set<std::string>& fieldsuffixes);
  ///return levels
  vector<miutil::miString> getFieldLevels(const miutil::miString& pinfo);
  /// return FieldGroupInfo for one model to FieldDialog
  void getFieldGroups(const miutil::miString& modelNameRequest,
		      miutil::miString& modelName, miutil::miTime refTime, vector<FieldGroupInfo>& vfgi);
  /// return available times for the requested fields
  vector<miutil::miTime> getFieldTime(vector<FieldTimeRequest>& request,
			      bool allTimeSteps);

  // Map-dialog methods
  MapDialogInfo initMapDialog();
  bool MapInfoParser(miutil::miString& str, MapInfo& mi, bool tostr);

  // Edit-dialog methods --------
  /// returns current EditDialogInfo for gui
  EditDialogInfo initEditDialog();
  /// get text list from complex weather symbol
  set <miutil::miString> getComplexList();
  /// return class specifications from fieldplot setup to EditDialog
  miutil::miString getFieldClassSpecifications(const miutil::miString& fieldName);

  // object-dialog methods
  /// get ObjectNames from setup file to be used in dialog etc.
  vector<miutil::miString> getObjectNames(bool);
  ///objects follow main plot time
  void setObjAuto(bool autoFile);
  /// returns list of objectfiles for use in dialog
  vector<ObjFileInfo> getObjectFiles(miutil::miString objectname, bool refresh);
  /// decode string with types of objects to plot
  map <miutil::miString,bool> decodeTypeString(miutil::miString);

  // various GUI-methods
  vector< vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);
  bool getQuickMenus(vector<QuickMenuDefs>& qm);

  //stations
  void putStations(StationPlot*);
  void makeStationPlot(const miutil::miString& commondesc, const miutil::miString& common,
		       const miutil::miString& description, int from,
		       const  vector<miutil::miString>& data);
  void deleteStations(miutil::miString name);
  void deleteStations(int id=-2);
  miutil::miString findStation(int, int,miutil::miString name,int id=-1);
  void findStations(int, int, bool add, vector<miutil::miString>& name,vector<int>& id,
		    vector<miutil::miString>& station);
  void getEditStation(int step, miutil::miString& name, int& id,
		      vector<miutil::miString>& stations);
  void stationCommand(const miutil::miString& Command,
		      vector<miutil::miString>& data,
		      const miutil::miString& name="", int id=-1,
		      const miutil::miString& misc="");
  void stationCommand(const miutil::miString& Command,
		      const miutil::miString& name="", int id=-1);

  //areas
  ///put area into list of area objects
  void makeAreas(const miutil::miString& name, miutil::miString areaString, int id=-1);
  ///send command to right area object
  void areaCommand(const miutil::miString& command, const miutil::miString& dataSet,
		   const miutil::miString& data, int id );
  ///find areas in position x,y
  vector <selectArea> findAreas(int x, int y, bool newArea=false);

  // location (vcross,...)
  void putLocation(const LocationData& locationdata);
  void updateLocation(const LocationData& locationdata);
  void deleteLocation(const miutil::miString& name);
  void setSelectedLocation(const miutil::miString& name,
			 const miutil::miString& elementname);
  miutil::miString findLocation(int x, int y, const miutil::miString& name);

  map<miutil::miString,InfoFile> getInfoFiles();

  vector<PlotElement>& getPlotElements();
  void enablePlotElement(const PlotElement& pe);

/********************* reading and writing log file *******************/
 vector<miutil::miString> writeLog();
 void readLog(const vector<miutil::miString>& vstr,
	      const miutil::miString& thisVersion, const miutil::miString& logVersion);
#ifdef PROFET
  bool initProfet();
//  bool registerProfetUser(const Profet::PodsUser & u);
  Profet::ProfetController * getProfetController();
//  bool setProfetGUI(Profet::ProfetGUI * gui);
  GridAreaManager * getAreaManager() { return aream; }
#endif
  ///Enable and disable paint mode
  void setPaintModeEnabled(bool);

  bool useScrollwheelZoom();

};

#endif
