/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no

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

#include "diAreaTypes.h"
#include "diColour.h"
#include "diCommonTypes.h"
#include "diDrawingTypes.h"
#include "diEditTypes.h"
#include "diMapInfo.h"
#include "diMapMode.h"
#include "diObjTypes.h"
#include "diObsDialogInfo.h"
#include "diPlotCommand.h"
#include "diQuickMenuTypes.h"
#include "diRectangle.h"
#include "diSatTypes.h"
#include "diStationTypes.h"
#include "diTimeTypes.h"

#include <puTools/miTime.h>

#include <map>
#include <vector>
#include <set>

class AnnotationPlot;
class Area;
class DiCanvas;
class DiGLPainter;
class DrawingManager;
class EditManager;
class EventResult;
class FieldPlot;
class FieldPlotManager;
class Manager;
class MapAreaNavigator;
class MapManager;
class ObjectManager;
class ObsManager;
class ObsPlot;
class PlotElement;
class PlotModule;
class SatManager;
class SatPlot;
class StationManager;
class StationPlot;

struct LocationData;

// from diCommonFieldTypes.h
struct FieldModelGroupInfo;
typedef std::vector<FieldModelGroupInfo> FieldModelGroupInfo_v;
struct FieldPlotGroupInfo;
typedef std::vector<FieldPlotGroupInfo> FieldPlotGroupInfo_v;
struct FieldRequest;

class QKeyEvent;
class QMouseEvent;
class QSize;

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
  std::unique_ptr<MapAreaNavigator> man_;

  std::unique_ptr<FieldPlotManager> fieldplotm;
  ObsManager    *obsm;
  SatManager    *satm;
  StationManager    *stam;
  ObjectManager *objm;
  EditManager   *editm;
  DrawingManager   *drawm;

  bool editoverride; // do not route mouse/key-events to editmanager

public:
  Controller();
  ~Controller();

  void setCanvas(DiCanvas* canvas);
  DiCanvas* canvas();

  FieldPlotManager* getFieldPlotManager() { return fieldplotm.get(); };
  EditManager*   getEditManager()   { return editm; };
  ObjectManager* getObjectManager() { return objm; };
  StationManager* getStationManager() { return stam; };
  SatManager* getSatelliteManager() { return satm; };

  void addManager(const std::string &name, Manager *man);
  Manager *getManager(const std::string &name);

  bool updateFieldFileSetup(const std::vector<std::string>& lines,
      std::vector<std::string>& errors);

  /// parse setup
  bool parseSetup();
  /// set new plotcommands
  void plotCommands(const PlotCommand_cpv&);
  /// call PlotModule.plot()
  void plot(DiGLPainter* gl, bool over =true, bool under =true);
  /// get annotations
  std::vector<AnnotationPlot*> getAnnotations();
  /// plot annotations only
  std::vector<Rectangle> plotAnnotations(DiGLPainter* gl);
  /// get plotwindow corners in GL-coordinates
  const Rectangle& getPlotSize();
  /// get plot area (incl. projection)
  const Area& getMapArea();

  //! Zoom at position, \see MapAreaNavigator::zoomAt
  void zoomAt(int steps, float frac_x, float frac_y);

  //! Zoom out map, \see MapAreaNavigator::zoomOut
  void zoomOut();

  /// set plotwindow size in pixels (from MainPaintable..)
  void setPlotWindow(const QSize& size);
  /// return latitude,longitude from physical x,y
  bool PhysToGeo(const float,const float,float&,float&);
  /// return physical x,y from latitude,longitude
  bool GeoToPhys(const float,const float,float&,float&);
  /// return map x,y from physical x,y
  void PhysToMap(const float,const float,float&,float&);
  /// return field grid x,y from map x,y if field defined and map proj = field proj
  bool MapToGrid(const float,const float,float&,float&);
  /// return distance marked by the rubberband or entire window in m
  double getWindowDistances(float x, float y, bool horizontal);
  /// return area marked by the rubberband in m2
  double getMarkedArea(const float& x, const float& y);
  /// return area of entire window in m2
  double getWindowArea();

  /// return current plottime
  const miutil::miTime& getPlotTime();

  /// return data times (fields,images, observations, objects and editproducts)
  void getPlotTimes(std::map<std::string, plottimes_t>& times);
  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(plottimes_t& okTimes, const PlotCommand_cpv& pinfos, bool allTimes = true);

  /// set plottime
  void setPlotTime(const miutil::miTime&);
  /// update plots
  bool updatePlots();
  /// toggle area conservatism
  void keepCurrentArea(bool);
  /// set colourindices from gui
  void setColourIndices(std::vector<Colour::ColourInfo>&);

  /// reload observations
  void updateObs();
  /// find obs in grid position x,y
  bool findObs(int,int);
  /// get obs popup text in grid position x,y
  std::string getObsPopupText(int x,int y);
  /// plot other observations
  void nextObs(bool);
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
  /// get name++ of current channels (with calibration)
  std::vector<std::string> getCalibChannels();
  /// show pixel values in status bar
  std::vector<SatValues> showValues(float, float);
  //show or hide all annotations (for fields, observations, satellite etc.)
  void showAnnotations(bool);
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
  bool sendMouseEventToManagers(QMouseEvent* me, EventResult& res);
  /// keyboard events
  void sendKeyboardEvent(QKeyEvent* me, EventResult& res);
  bool sendKeyboardEventToManagers(QKeyEvent* ke, EventResult& res);

  // edit and drawing methods
  /// get mode of main workarea
  mapMode getMapMode();

  stationDialogInfo initStationDialog();

  // Obs-dialog methods
  /// return button names for ObsDialog
  ObsDialogInfo initObsDialog();
  /// return button names for ObsDialog ... ascii files (when activated)
  void updateObsDialog(ObsDialogInfo::PlotType& pt, const std::string& name);
  /// get observation times for plot types name
  plottimes_t getObsTimes(const std::vector<std::string>& name, bool update);

  // bdiana methods
  /// return referenceTime of first FieldPlot
  miutil::miTime getFieldReferenceTime();
  ///return levels
  std::vector<std::string> getFieldLevels(const PlotCommand_cp& pinfo);

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
  std::string findStation(int, int, const std::string& name, int id=-1);
  std::vector<std::string> findStations(int, int, const std::string& name, int id=-1);
  void findStations(int, int, bool add, std::vector<std::string>& name, std::vector<int>& id,
      std::vector<std::string>& station);
  void getStationData(std::vector< std::vector<std::string> >& data);
  void stationCommand(const std::string& Command,
      const std::vector<std::string>& data,
      const std::string& name="", int id=-1,
      const std::string& misc="");
  void stationCommand(const std::string& Command,
      const std::string& name="", int id=-1);

  //areas
  ///put area into list of area objects
  void makeAreaObjects(const std::string& name, std::string areaString, int id=-1);
  ///send command to right area object
  void areaObjectsCommand(const std::string& command, const std::string& dataSet,
      const std::vector<std::string>& data, int id);

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

  // Miscellaneous get methods
  std::vector<SatPlot*> getSatellitePlots() const;   // Returns a vector of defined satellite plots.
  std::vector<FieldPlot*> getFieldPlots() const;     // Returns a vector of defined field plots.
  std::vector<ObsPlot*> getObsPlots() const;         // Returns a vector of defined observation plots.
};

#endif
