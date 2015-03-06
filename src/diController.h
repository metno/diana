/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#include <diColour.h>
#include <diCommonTypes.h>
#include <diDrawingTypes.h>
#include <diMapMode.h>
#include <diPrintOptions.h>

#include <diField/diArea.h>
#include <diField/diCommonFieldTypes.h>
#include <puTools/miTime.h>

#include <map>
#include <vector>
#include <set>

class AnnotationPlot;
class PlotModule;
struct LocationData;
class FieldManager;
class FieldPlotManager;
class ObsManager;
class SatManager;
class DrawingManager;
class EditManager;
class Manager;
class ObjectManager;
class StationManager;
class StationPlot;
class MapManager;
class FieldPlot;
class ObsPlot;
class SatPlot;

class QKeyEvent;
class QMouseEvent;

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
  StationManager    *stam;
  ObjectManager *objm;
  EditManager   *editm;
  DrawingManager   *drawm;

  bool editoverride; // do not route mouse/key-events to editmanager
  bool scrollwheelZoom;

public:
  Controller();
  ~Controller();

  EditManager*   getEditManager()   { return editm; };
  ObjectManager* getObjectManager() { return objm; };
  FieldManager* getFieldManager() { return fieldm; };
  StationManager* getStationManager() { return stam; };
  SatManager* getSatelliteManager() { return satm; };
  ObsManager* getObservationManager() { return obsm; };

  void addManager(const std::string &name, Manager *man);
  Manager *getManager(const std::string &name);

  /// init static FontManager in class Plot
  void restartFontManager();
  /// parse setup
  bool parseSetup();
  /// set new plotcommands
  void plotCommands(const std::vector<std::string>&);
  /// call PlotModule.plot()
  void plot(bool over =true, bool under =true);
  /// get annotations
  std::vector<AnnotationPlot*> getAnnotations();
  /// plot annotations only
  std::vector<Rectangle> plotAnnotations();
  /// get plotwindow corners in GL-coordinates
  void getPlotSize(float& x1, float& y1, float& x2, float& y2);
  /// get plot area (incl. projection)
  const Area& getMapArea();
  /// zoom to rectangle r
  void zoomTo(const Rectangle & r);
  /// zoom out map
  void zoomOut();
  /// set plotwindow size in pixels (from GLwidget..)
  void setPlotWindow(const int, const int);
  /// return latitude,longitude from physical x,y
  bool PhysToGeo(const float,const float,float&,float&);
  /// return physical x,y from latitude,longitude
  bool GeoToPhys(const float,const float,float&,float&);
  /// return map x,y from physical x,y
  void PhysToMap(const float,const float,float&,float&);
  /// return field grid x,y from map x,y if field defined and map proj = field proj
  bool MapToGrid(const float,const float,float&,float&);
  /// return distance marked by the rubberband or entire window in m
  double getWindowDistances(const float& x, const float& y, const bool horizontal);
  /// return area marked by the rubberband in m2
  double getMarkedArea(const float& x, const float& y);
  /// return area of entire window in m2
  double getWindowArea();

  /// start hardcopy plot
  void startHardcopy(const printOptions&);
  /// end hardcopy plot
  void endHardcopy();
  /// return current plottime
  void getPlotTime(std::string&);
  /// return current plottime
  void getPlotTime(miutil::miTime&);
  /// return data times (fields,images, observations, objects and editproducts)
  void getPlotTimes(std::map<std::string, std::vector<miutil::miTime> >& times, bool updateSources=false);
  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(std::set<miutil::miTime>& okTimes,
      const std::vector<std::string>& pinfos,
      bool allTimes=true,
      bool updateSources=false);
  /// returns the current product time
  bool getProductTime(miutil::miTime& t);
  /// returns the current product name
  std::string getProductName();

  /// set plottime
  bool setPlotTime(miutil::miTime&);
  /// update plots
  bool updatePlots(bool failOnMissingData=false);
  /// update FieldPlots
  void updateFieldPlot(const std::vector<std::string>& pin);
  /// toggle area conservatism
  void keepCurrentArea(bool);
  /// set colourindices from gui
  void setColourIndices(std::vector<Colour::ColourInfo>&);

  /// reload observations
  void updateObs();
  /// find obs in grid position x,y
  bool findObs(int,int);
  /// find name of obs in grid position x,y
  bool getObsName(int x,int y, std::string& name);
  /// get obs popup text in grid position x,y 
  std::string getObsPopupText(int x,int y); 
  /// plot other observations
  void nextObs(bool);
  /// init hqcData from QSocket
  bool initHqcdata(int from, const std::string&, const std::string&,
      const std::string&, const std::vector<std::string>&);
  /// update hqcData from QSocket
  void updateHqcdata(const std::string&, const std::string&,
      const std::string&, const std::vector<std::string>&);
  /// select obs parameter to flag from QSocket
  void processHqcCommand(const std::string&, const std::string& ="");
  /// plot trajectory position
  void  trajPos(const std::vector<std::string>&);
  /// plot measurements position
  void  measurementsPos(const std::vector<std::string>&);
  /// get trajectory fields
  std::vector<std::string> getTrajectoryFields();
  /// start trajectory computation
  bool startTrajectoryComputation();
  // print trajectory positions to file
  bool printTrajectoryPositions(const std::string& filename);
  /// get field models used (for Vprof etc.)
  std::vector<std::string> getFieldModels();
  /// obs time step changed
  void obsStepChanged(int);
  /// get name++ of current channels (with calibration)
  std::vector<std::string> getCalibChannels();
  /// show pixel values in status bar
  std::vector<SatValues> showValues(float, float);
  /// get satellite name from all SatPlots
  std::vector<std::string> getSatnames();
  //show or hide all annotations (for fields, observations, satellite etc.)
  void showAnnotations(bool);
  /// toggle scrollwheelzoom
  void toggleScrollwheelZoom(bool);
  /// mark editable annotationPlot if x,y inside plot
  bool markAnnotationPlot(int, int);
  /// get text of marked and editable annotationPlot
  std::string getMarkedAnnotation();
  /// change text of marked and editable annotationplot
  void changeMarkedAnnotation(std::string text,int cursor=0,
      int sel1=0,int sel2=0);
  /// delete marked and editable annotation
  void DeleteMarkedAnnotation();
  /// start editing annotations
  void startEditAnnotation();
  /// stop editing annotations
  void stopEditAnnotation(std::string prodname);
  /// go to next element in annotation being edited
  void editNextAnnoElement();
  /// go to last element in annotation being edited
  void editLastAnnoElement();
  /// Archive mode
  void archiveMode( bool );

  /// mouse events
  void sendMouseEvent(QMouseEvent* me, EventResult& res);
  /// keyboard events
  void sendKeyboardEvent(QKeyEvent* me, EventResult& res);

  // edit and drawing methods
  /// get mode of main workarea
  mapMode getMapMode();

  // Sat-dialog routines
  /// get list of satfiles of class satellite and subclass file. if update is true read new list from disk
  const std::vector<SatFileInfo>& getSatFiles(const std::string & satellite, const std::string & file,bool update);
  /// returns colour palette for subproduct of class satellite and subclass file
  const std::vector<Colour>& getSatColours(const std::string & satellite, const std::string & file);
  /// returns channels for subproduct of class satellite and subclass file
  const std::vector<std::string>& getSatChannels(const std::string & satellite, const std::string &file ,
      int index=-1);
  /// returns true if satellite picture is a mosaic
  bool isMosaic(const std::string &, const std::string &);
  /// refresh list of satellite files
  void SatRefresh(const std::string &, const std::string &);
  /// returns information about whether list of satellite files have changed
  void satFileListUpdated();
  /// called when the dialog and timeSlider updated with info from satellite
  bool satFileListChanged();
  /// returns information about whether list of observation files have changed
  void obsTimeListUpdated();
  /// called when the dialog and timeSlider updated with info from obs
  bool obsTimeListChanged();
  /// satellite follows main plot time
  void setSatAuto(bool,const std::string &, const std::string &);
  /// get satellite classes for uffda dialog
  void getUffdaClasses(std::vector<std::string>&, std::vector<std::string>&);
  /// returns true if uffda option enables
  bool getUffdaEnabled();
  /// returns adress to send mail from uffda dialog
  std::string getUffdaMailAddress();
  /// return button names for SatDialog
  SatDialogInfo initSatDialog();

  stationDialogInfo initStationDialog();

  // Obs-dialog methods
  /// return button names for ObsDialog
  ObsDialogInfo initObsDialog();
  /// return button names for ObsDialog ... ascii files (when activated)
  ObsDialogInfo updateObsDialog(const std::string& name);
  /// get observation times for plot types name
  std::vector<miutil::miTime> getObsTimes(const std::vector<std::string>& name);

  // Field-dialog methods
  /// return model/file groups and contents to FieldDialog
  std::vector<FieldDialogInfo> initFieldDialog();
  ///return all reference times for the given model
  std::set<std::string> getFieldReferenceTimes(const std::string model);
  ///return the reference time given by refOffset and refhour or the last reference time for the given model
  std::string getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour);
  /// return plot options for all defined plot fields in setup
  void getAllFieldNames(std::vector<std::string>& fieldNames);
  ///return levels
  std::vector<std::string> getFieldLevels(const std::string& pinfo);
  /// return FieldGroupInfo for one model to FieldDialog
  void getFieldGroups(const std::string& modelName, std::string refTime, bool plotGroups, std::vector<FieldGroupInfo>& vfgi);
  /// Returns available times for the requested fields.
  std::vector<miutil::miTime> getFieldTime(std::vector<FieldRequest>& request);
  ///update list of fieldsources (field files)
  void updateFieldSource(const std::string & modelName);

  // Map-dialog methods
  MapDialogInfo initMapDialog();
  bool MapInfoParser(std::string& str, MapInfo& mi, bool tostr);

  // Edit-dialog methods --------
  /// returns current EditDialogInfo for gui
  EditDialogInfo initEditDialog();
  /// get text list from complex weather symbol
  std::set<std::string> getComplexList();

  // object-dialog methods
  /// get ObjectNames from setup file to be used in dialog etc.
  std::vector<std::string> getObjectNames(bool);
  ///objects follow main plot time
  void setObjAuto(bool autoFile);
  /// returns list of objectfiles for use in dialog
  std::vector<ObjFileInfo> getObjectFiles(std::string objectname, bool refresh);
  /// decode string with types of objects to plot
  std::map<std::string,bool> decodeTypeString(std::string);

  // various GUI-methods
  std::vector< std::vector<Colour::ColourInfo> > getMultiColourInfo(int multiNum);
  bool getQuickMenus(std::vector<QuickMenuDefs>& qm);

  //stations
  void putStations(StationPlot*);
  void makeStationPlot(const std::string& commondesc, const std::string& common,
      const std::string& description, int from,
      const  std::vector<std::string>& data);
  void deleteStations(std::string name);
  void deleteStations(int id=-2);
  std::string findStation(int, int,std::string name,int id=-1);
  std::vector<std::string> findStations(int, int,std::string name,int id=-1);
  void findStations(int, int, bool add, std::vector<std::string>& name, std::vector<int>& id,
      std::vector<std::string>& station);
  void getEditStation(int step, std::string& name, int& id,
      std::vector<std::string>& stations);
  void getStationData(std::vector<std::string>& data);
  void stationCommand(const std::string& Command,
      const std::vector<std::string>& data,
      const std::string& name="", int id=-1,
      const std::string& misc="");
  void stationCommand(const std::string& Command,
      const std::string& name="", int id=-1);
  float getStationsScale();
  void setStationsScale(float new_scale);

  //areas
  ///put area into list of area objects
  void makeAreas(const std::string& name, std::string areaString, int id=-1);
  ///send command to right area object
  void areaCommand(const std::string& command, const std::string& dataSet,
      const std::string& data, int id );
  ///find areas in position x,y
  std::vector <selectArea> findAreas(int x, int y, bool newArea=false);

  // location (vcross,...)
  void putLocation(const LocationData& locationdata);
  void updateLocation(const LocationData& locationdata);
  void deleteLocation(const std::string& name);
  void setSelectedLocation(const std::string& name,
      const std::string& elementname);
  std::string findLocation(int x, int y, const std::string& name);

  std::map<std::string,InfoFile> getInfoFiles();

  std::vector<PlotElement> getPlotElements();
  void enablePlotElement(const PlotElement& pe);

  /********************* reading and writing log file *******************/
  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);
  ///Enable and disable paint mode
  void setPaintModeEnabled(bool);

  bool useScrollwheelZoom();

  // Miscellaneous get methods
  const std::vector<SatPlot*>& getSatellitePlots() const;   // Returns a vector of defined satellite plots.
  std::vector<FieldPlot*> getFieldPlots() const;      // Returns a vector of defined field plots.
  std::vector<ObsPlot*> getObsPlots() const;         // Returns a vector of defined observation plots.
};

#endif
