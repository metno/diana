/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2019 met.no

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

#include "diArea.h"
#include "diPlot.h"
#include "diPlotCommand.h"
#include "diPlotElement.h"
#include "diTimeTypes.h"

#include <vector>
#include <set>
#include <deque>
#include <memory>

class AnnotationPlot;
class AreaObjectsCluster;
class DrawingManager;
class EditManager;
class Field;
class FieldPlotCluster;
class FieldPlotManager;
class LocationPlotCluster;
class Manager;
class MapPlotCluster;
class MeasurementsPlot;
class ObjectManager;
class ObjectPlotCluster;
class ObsManager;
class ObsPlotCluster;
class SatManager;
class SatPlotCluster;
class StationManager;
class StationPlot;
class StationPlotCluster;
class TrajectoryPlot;
class TrajectoryPlotCluster;
struct LocationData;

class QMouseEvent;

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
  StationManager *stam;       // raster-data manager
  ObjectManager *objm;    // met.objects
  EditManager *editm;     // editing/drawing manager
  FieldPlotManager *fieldplotm;   // field plot manager

  std::unique_ptr<ObsPlotCluster> obsplots_;   // observation plots
  std::unique_ptr<SatPlotCluster> satplots_;   // satellite plots
  std::unique_ptr<FieldPlotCluster> fieldplots_; // field plots
  std::unique_ptr<MapPlotCluster> mapplots_;     // vector of map plots
  std::unique_ptr<StationPlotCluster> stationplots_; // vector of map plots
  std::unique_ptr<ObjectPlotCluster> objectplots_;   // obsject plots
  std::unique_ptr<TrajectoryPlotCluster> trajectoryplots_; // vector of trajectory plots
  std::vector<MeasurementsPlot*> vMeasurementsPlot; // vector of measurements plots
  std::unique_ptr<LocationPlotCluster> locationplots_; // location (vcross,...) to be plotted
  std::vector<AnnotationPlot*> vap; // vector of annotation plots
  std::vector<AnnotationPlot*> editVap;   //edit object labels
  std::unique_ptr<AreaObjectsCluster> areaobjects_;

  PlotCommand_cpv annotationCommands;  //orig. strings from setup

  bool mapDefinedByUser; // map area set by user
  bool mapDefinedByData; // for initial maps with fields or sat.
  bool mapDefinedByView; // for initial maps without any data

  Area previousrequestedarea;

  bool showanno;        // show standard annotations

  std::unique_ptr<StaticPlot> staticPlot_;

  DiCanvas* mCanvas;

  //! Rectangle with phys coordinates of rubberband.
  Rectangle rubberband;

  bool dorubberband;
  bool keepcurrentarea;

  /*! Initialize plotting.
   * Common for normal plot and annotation-only plot.
   * Sets gl matrix and clears background with bg colour.
   */
  void plotInit(DiGLPainter* gl);

  void plotUnder(DiGLPainter* gl);

  void plotOver(DiGLPainter* gl);

  static PlotModule *self;

  /// delete all data vectors
  void cleanup();

  /// handles fields plot info strings
  void prepareFields(const PlotCommand_cpv&);
  /// handles area info strings
  void prepareArea(const PlotCommand_cpv&);
  /// handles annotation plot info strings
  void prepareAnnotation(const PlotCommand_cpv&);

  double getWindowDistances(float x1, float y1, float x2, float y2, bool horizontal);
  double getEntireWindowDistances(const bool horizontal);
  double getWindowArea(int x1, int y1, int x2, int y2);

  static double getArea(float flat1, float flat2, float flat3, float flat4, float flon1, float flon2, float flon3, float flon4);
  static double calculateArea(double hLSide, double hUSide, double vLSide, double vRSide, double diag);

  void updateCanvasSize();
  void notifyChangeProjection();

  bool defineMapAreaFromData(Area& newMapArea, bool& allowKeepCurrentArea);

public:
  PlotModule();
  ~PlotModule();

  void setCanvas(DiCanvas* canvas);

  /// the main plot routine (plot for underlay, plot for overlay)
  void plot(DiGLPainter* gl, bool under = true, bool over = true);

  /// split plot info strings and reroute them to appropriate handlers
  void preparePlots(const PlotCommand_cpv&);

  /// get annotations
  const std::vector<AnnotationPlot*>& getAnnotations();

  /// plot annotations
  std::vector<Rectangle> plotAnnotations(DiGLPainter* gl);

  // show or hide all annotations (for fields, observations, satellite etc.)
  void showAnnotations(bool on) { showanno = on; }

  /// get annotations from all plots
  void setAnnotations();

  //! Check if any plot has data
  /*! Results are useful only after calling setMapArea and changeTime.
   *  \returns true if any plot says it has data
   */
  bool hasData();

  /// set maparea from map spec., sat or fields
  void defineMapArea();

  /// toggle conservative map area
  void setKeepCurrentArea(bool b) { keepcurrentarea = b; }

  //! query conservative map area
  bool isKeepCurrentArea() const { return keepcurrentarea; }

  /// get maparea for plot
  const Area& getMapArea();

  /// get plotwindow rectangle
  const Rectangle& getPlotSize();

  /// get the physical size of the plot window (pixels)
  const diutil::PointI& getPhysSize() const;

  /// resize physical plot window (pixels)
  void setPhysSize(int, int);

  /// return latitude,longitude from physical x,y
  bool PhysToGeo(float xphys, float yphys, float& lat, float& lon);

  /// return physical x,y from physical latitude,longitude
  bool GeoToPhys(float lat, float lon, float& xphys, float& yphys);

  /// return map x,y from physical x,y
  void PhysToMap(float xphys, float yphys, float& xmap, float& ymap);

  /// return field grid x,y from map x,y if field defined and map proj = field proj
  bool MapToGrid(float xmap, float ymap, float& gridx, float& gridy);

  double getWindowDistances(float x, float y, bool horizontal);
  double getMarkedArea(float x, float y);
  double getWindowArea();

  /// set managers
  void setManagers(FieldPlotManager*, ObsManager*, SatManager*, StationManager*, ObjectManager*, EditManager*);
  Manager *getManager(const std::string &name);

  /// return current plottime
  const miutil::miTime& getPlotTime() const;

  /// return data times (fields,images, observations, objects and editproducts)
  void getPlotTimes(std::map<std::string, plottimes_t>& times);

  //! returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(std::set<miutil::miTime>& okTimes, const PlotCommand_cpv& pinfos, bool time_union = true);

  /// set plottime and inform plots
  void changeTime(const miutil::miTime&);

  /// Update ObsPlots if data files have changed
  void updateObs();

  // locationPlot (vcross,...)
  void putLocation(const LocationData& locationdata);
  void deleteLocation(const std::string& name);
  void setSelectedLocation(const std::string& name,
      const std::string& elementname);
  std::string findLocation(int x, int y, const std::string& name);

  // Trajectories
  /// handles trajectory plot info strings
  void trajPos(const std::vector<std::string>&);

  bool startTrajectoryComputation();
  void stopTrajectoryComputation();
  // print trajectory positions to file
  bool printTrajectoryPositions(const std::string& filename);

  // Measurements (distance, velocity)
  void measurementsPos(const std::vector<std::string>&);

  /// mark editable annotationPlot if x,y inside plot
  bool markAnnotationPlot(int, int);
  /// get text of marked and editable annotationPlot
  std::string getMarkedAnnotation();
  /// change text of marked and editable annotationplot
  void changeMarkedAnnotation(std::string text, int cursor = 0, int sel1 = 0,
      int sel2 = 0);
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
  /// return vector std::strings with edited annotation for product prodname
  PlotCommand_cpv writeAnnotations(const std::string& prodname);
  /// put info from saved edit labels into new annotation
  void updateEditLabels(const PlotCommand_cpv& productLabelCommands,
      const std::string& productName, bool newProduct);

  void deleteAllEditAnnotations();

  void setPanning(bool pan);
  bool isPanning() const;

  void setRubberband(bool enable) { dorubberband = enable; }
  bool isRubberband() const { return dorubberband; }

  //! Set rectangle with phys coordinates of rubberband.
  void setRubberbandRectangle(const Rectangle& rr) { rubberband = rr; }

  void setMapArea(const Area& area);

  /// zoom to specified rectangle, using the same projection as before
  void setMapAreaFromMap(const Rectangle& rectangle);

  /// receive rectangle in pixels
  void setMapAreaFromPhys(const Rectangle& phys);

  // plotelements methods
  /// return PlotElement data (for the speedbuttons)
  std::vector<PlotElement> getPlotElements();
  /// enable one PlotElement
  void enablePlotElement(const PlotElement& pe);

  // Miscellaneous get methods
  AreaObjectsCluster* areaobjects();
  ObsPlotCluster* obsplots() const { return obsplots_.get(); }
  FieldPlotCluster* fieldplots() { return fieldplots_.get(); }
  SatPlotCluster* satplots() { return satplots_.get(); }
  StationPlotCluster* stationplots() { return stationplots_.get(); }

  typedef std::map<std::string, Manager*> managers_t;
  managers_t managers;

  static PlotModule *instance()
    { return self; }

  StaticPlot* getStaticPlot() const
    { return staticPlot_.get(); }
};

#endif
