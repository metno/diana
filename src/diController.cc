/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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

#include "diana_config.h"

#include "diController.h"

#include "diAreaObjectsCluster.h"
#include "diDrawingManager.h"
#include "diEditManager.h"
#include "diEventResult.h"
#include "diFieldPlotCluster.h"
#include "diFieldPlotManager.h"
#include "diImageGallery.h"
#include "diKVListPlotCommand.h"
#include "diLocalSetupParser.h"
#include "diManager.h"
#include "diMapAreaNavigator.h"
#include "diMapAreaSetup.h"
#include "diMapManager.h"
#include "diObjectManager.h"
#include "diObsManager.h"
#include "diObsPlotCluster.h"
#include "diPlotModule.h"
#include "diSatManager.h"
#include "diSatPlotCluster.h"
#include "diStationManager.h"
#include "diStationPlot.h"
#include "diStationPlotCluster.h"
#include "miSetupParser.h"
#include "wmsclient/WebMapManager.h"

#include "diField/diArea.h"
#include "diField/diRectangle.h"

#include <puTools/miStringFunctions.h>

#include <QKeyEvent>
#include <QMouseEvent>

#include <boost/range/adaptor/map.hpp>

#define MILOGGER_CATEGORY "diana.Controller"
#include <miLogger/miLogging.h>

using namespace miutil;

Controller::Controller()
    : plotm(0)
    , fieldplotm(new FieldPlotManager())
    , obsm(0)
    , satm(0)
    , objm(0)
    , editm(0)
    , editoverride(false)
{
  METLIBS_LOG_SCOPE();

  // data managers
  obsm=       new ObsManager;
  satm=       new SatManager;
  stam=       new StationManager;
  // plot central
  plotm= new PlotModule();
  man_.reset(new MapAreaNavigator(plotm));
  // edit- and drawing-manager
  objm=  new ObjectManager(plotm);
  editm = new EditManager(plotm, objm, fieldplotm.get());
  plotm->setManagers(fieldplotm.get(), obsm, satm, stam, objm, editm);

  addManager("DRAWING", DrawingManager::instance());
  addManager("WEBMAP", WebMapManager::instance());
}

Controller::~Controller()
{
  delete plotm;
  delete obsm;
  delete satm;
  delete stam;
  delete objm;
  delete editm;

  WebMapManager::destroy();
}

void Controller::setCanvas(DiCanvas* canvas)
{
  plotm->setCanvas(canvas);
}

DiCanvas* Controller::canvas()
{
  return DrawingManager::instance()->canvas(); // FIXME store elsewhere
}

bool Controller::updateFieldFileSetup(const std::vector<std::string>& lines,
    std::vector<std::string>& errors)
{
  return fieldplotm->updateFieldFileSetup(lines, errors);
}

bool Controller::parseSetup()
{
  METLIBS_LOG_SCOPE();

  //parse some setup sections
  if (!fieldplotm->parseSetup()) return false;
  if (!obsm->parseSetup()) return false;
  if (!satm->parseSetup()) return false;
  if (!objm->parseSetup()) return false;
  if (!editm->parseSetup()) return false;
  if (!stam->parseSetup()) return false;

  for (Manager* m : boost::adaptors::values(plotm->managers)) {
    if (!m->parseSetup())
      return false;
  }

  if (!MapAreaSetup::instance()->parseSetup())
    return false;

  MapManager mapm;
  if (!mapm.parseSetup()) return false;

  ImageGallery ig;
  ig.parseSetup();

  return true;
}

void Controller::plotCommands(const PlotCommand_cpv& inp)
{
  METLIBS_LOG_SCOPE();
  if (METLIBS_LOG_DEBUG_ENABLED()) {
    for (size_t q = 0; q < inp.size(); q++)
      METLIBS_LOG_DEBUG("inp['" << q << "]='" << inp[q]->toString() << "'");
  }

  plotm->processInput(inp);
}

void Controller::plot(DiGLPainter* gl, bool under, bool over)
{
  METLIBS_LOG_SCOPE();
  plotm->plot(gl, under, over);
}

std::vector<AnnotationPlot*> Controller::getAnnotations()
{
  return plotm->getAnnotations();
}

std::vector<Rectangle> Controller::plotAnnotations(DiGLPainter* gl)
{
  return plotm->plotAnnotations(gl);
}

// get plot area (incl. projection)
const Area& Controller::getMapArea()
{
  return plotm->getMapArea();
}

void Controller::zoomAt(int steps, float frac_x, float frac_y)
{
  man_->zoomAt(steps, frac_x, frac_y);
}

void Controller::zoomOut()
{
  man_->zoomOut();
}

// plotwindow size from MainPaintable..
void Controller::setPlotWindow(const QSize& size)
{
  plotm->setPhysSize(size.width(), size.height());
}

// return latitude,longitude from physical x,y
bool Controller::PhysToGeo(const float x,const float y,
    float& lat,float& lon){
  return plotm->PhysToGeo(x,y,lat,lon);
}

// return physical x,y from latitude,longitude
bool Controller::GeoToPhys(const float lat,const float lon,
    float& x,float& y)
{
  return plotm->GeoToPhys(lat,lon,x,y);
}

// return map x,y from physical x,y
void Controller::PhysToMap(const float x,const float y,
    float& xmap,float& ymap)
{
  plotm->PhysToMap(x,y,xmap,ymap);
}

/// return field grid x,y from map x,y if field defined and map proj = field proj
bool Controller::MapToGrid(const float xmap, const float ymap,
    float& gridx, float& gridy)
{
  return plotm->MapToGrid(xmap,ymap,gridx,gridy);
}

double Controller::getWindowDistances(float x, float y, bool horizontal)
{
  return plotm->getWindowDistances(x, y, horizontal);
}

double Controller::getMarkedArea(const float& x, const float& y)
{
  return plotm->getMarkedArea(x, y);
}

double Controller::getWindowArea()
{
  return plotm->getWindowArea();
}

const miutil::miTime& Controller::getPlotTime()
{
  return plotm->getPlotTime();
}

void Controller::getPlotTimes(std::map<std::string, plottimes_t>& times)
{
  plotm->getPlotTimes(times);
}

// set plottime
void Controller::changeTime(const miTime& t)
{
  plotm->changeTime(t);
}

// toggle area conservatism
void Controller::keepCurrentArea(bool b)
{
  plotm->setKeepCurrentArea(b);
}

bool Controller::hasData()
{
  return plotm->hasData();
}

bool Controller::hasError()
{
  return plotm->hasError();
}

// reload obsevations
void Controller::updateObs(){
  plotm->updateObs();
}

// find obs in grid position x,y
bool Controller::findObs(int x, int y)
{
  return plotm->obsplots()->findObs(x,y);
}

std::string Controller::getObsPopupText(int x, int y)
{
  return plotm->obsplots()->getObsPopupText(x,y);
}

//plot trajectory position
void Controller::trajPos(const std::vector<std::string>& str)
{
  plotm->trajPos(str);
}

//plot measurements position
void Controller::measurementsPos(const std::vector<std::string>& str)
{
  plotm->measurementsPos(str);
}

// start trajectory computation
bool Controller::startTrajectoryComputation(){
  return plotm->startTrajectoryComputation();
}

// get trajectory fields
std::vector<std::string> Controller::getTrajectoryFields()
{
  return plotm->fieldplots()->getTrajectoryFields();
}

// write trajectory positions to file
bool Controller::printTrajectoryPositions(const std::string& filename ){
  return plotm->printTrajectoryPositions( filename );
}

// get name++ of current channels (with calibration)
std::vector<std::string> Controller::getCalibChannels()
{
  return plotm->satplots()->getCalibChannels();
}

// show values in grid position x,y
std::vector<SatValues> Controller::showValues(float x, float y)
{
  return plotm->satplots()->showValues(x, y);
}


void Controller::showAnnotations(bool on)
{
  plotm->showAnnotations(on);
}

bool Controller::markAnnotationPlot(int x, int y)
{
  return plotm->markAnnotationPlot(x,y);
}

std::string Controller::getMarkedAnnotation()
{
  return plotm->getMarkedAnnotation();
}

void Controller::changeMarkedAnnotation(std::string text,int cursor,
    int sel1, int sel2)
{
  plotm->changeMarkedAnnotation(text,cursor,sel1,sel2);
}

void Controller::DeleteMarkedAnnotation()
{
  plotm->DeleteMarkedAnnotation();
}

void Controller::startEditAnnotation()
{
  plotm->startEditAnnotation();
}

void Controller::stopEditAnnotation(std::string prodname)
{
  const PlotCommand_cpv labels  = plotm->writeAnnotations(prodname);
  editm->saveProductLabels(labels);
  plotm->stopEditAnnotation();
}

void Controller::editNextAnnoElement()
{
  plotm->editNextAnnoElement();
}


void Controller::editLastAnnoElement()
{
  plotm->editLastAnnoElement();
}


//Archive mode
void Controller::archiveMode(bool on)
{
  obsm->archiveMode(on);
  satm->archiveMode(on);
}

// keyboard/mouse events

bool Controller::sendMouseEventToManagers(QMouseEvent* me, EventResult& res)
{
  if (editm->isInEdit() && !editm->getEditPause()) {
    if (editm->sendMouseEvent(me, res))
      return true;
  }

  // Send the event to the other managers to see if one of them will handle it.
  for (Manager* m : boost::adaptors::values(plotm->managers)) {
    if (m->isEnabled()) {
      m->sendMouseEvent(me, res);
      if (me->isAccepted())
        return true;
    }
  }
  return false;
}

//------------------------------------------------------------
// the mouseevents are sent from GUI to controller
//
// the events are routed to other managers, depending
// on the current mapmode
//
// a struct EventResult is sent back to the GUI, informing
// if repaint is required and if any action should be taken
// (emitting mousemove or mouseclick signals etc.)
//
void Controller::sendMouseEvent(QMouseEvent* me, EventResult& res)
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_SCOPE();
#endif
  res.do_nothing();

  me->setAccepted(false);

  const bool shiftmodifier = (me->modifiers() & Qt::ShiftModifier);
  const bool inEdit = editm->isInEdit();

  // first check events independent of mode
  //-------------------------------------
  if (me->type() == QEvent::MouseButtonPress && shiftmodifier) {
    // turn off editing functions temporarily until mouse-release
    // event. Pan and Zoom will now work when editing
    editoverride= true;
  }

  bool handled = false;
  if (!editoverride) {
    handled = sendMouseEventToManagers(me, res);
  }
  if (!handled) {
    res.newcursor = normal_cursor;
    handled = man_->sendMouseEvent(me, res);
  }
  if (me->type() == QEvent::MouseButtonRelease){
    // mouse released: turn on editing functions again
    editoverride= false;
    handled = true;
  }
  me->setAccepted(handled);

  if (editoverride || shiftmodifier) { // set normal cursor
    res.newcursor= normal_cursor;
  }

  if (inEdit) // always use underlay when in edit-mode
    res.enable_background_buffer = true;
}

bool Controller::sendKeyboardEventToManagers(QKeyEvent* ke, EventResult& res)
{
  if (editm->isInEdit() && !editm->getEditPause()) {
    if (editm->sendKeyboardEvent(ke, res))
      return true;
  }

  for (Manager* m : boost::adaptors::values(plotm->managers)) {
    if (m->isEnabled() && m->isEditing()) {
      m->sendKeyboardEvent(ke, res);
      if (ke->isAccepted() || m->hasFocus())
        return true;
    }
  }

  return false;
}

//------------------------------------------------------------
// the keyboardevents are sent from GUI to controller
//
// the events are routed to other managers, depending
// on the current mapmode
//
// a struct EventResult is sent back to the GUI, informing
// if repaint is required and if any action should be taken
//
void Controller::sendKeyboardEvent(QKeyEvent* ke, EventResult& res)
{
  METLIBS_LOG_SCOPE();
  res.do_nothing();

  if (ke->key() == Qt::Key_unknown)
    return;

  const bool shiftmodifier = (ke->modifiers() & Qt::ShiftModifier);
  // Old editing checks to override normal keypress behaviour.
  const bool inEdit = editm->isInEdit();
  res.enable_background_buffer = inEdit;

  // Access to normal keypress behaviour is obtained by holding down the Shift key
  // when in the old editing mode, or when editing is paused.
  const bool keyoverride = ((ke->type() == QEvent::KeyPress && shiftmodifier) || editm->getEditPause());
  METLIBS_LOG_DEBUG(LOGVAL(shiftmodifier) << LOGVAL(inEdit) << LOGVAL(keyoverride));

  bool handled = sendKeyboardEventToManagers(ke, res);
  METLIBS_LOG_DEBUG("managers:" << LOGVAL(handled));
  if (!handled && ke->type() == QEvent::KeyPress) {
    const bool pageup = (ke->key() == Qt::Key_PageUp), pagedown = (ke->key() == Qt::Key_PageDown);
    if (pageup || pagedown) {
      const bool forward = pagedown;
      plotm->obsplots()->nextObs(forward); // browse through observations
      handled = true;
      res.repaint = true;
      res.update_background_buffer = true;
    }
    METLIBS_LOG_DEBUG("obsplots:" << LOGVAL(handled));
  }
  if (!handled) {
    handled = man_->sendKeyboardEvent(ke, res);
    METLIBS_LOG_DEBUG("m-a-n:" << LOGVAL(handled));
  }
  if (!inEdit || keyoverride) {
    if (ke->type() == QEvent::KeyPress) {
      res.action = keypressed;
      handled = true;
    }
  }
  ke->setAccepted(handled);

  if (inEdit) // always use underlay when in edit-mode
    res.enable_background_buffer = true;
}

// ----- edit and drawing methods ----------

bool Controller::editManagerIsInEdit()
{
  return editm->isInEdit();
}

std::set<std::string> Controller::getComplexList()
{
  return objm->getComplexList();
}


// ------------ Dialog Info Routines ----------------


//returns union or intersection of plot times from all pinfos
void Controller::getCapabilitiesTime(plottimes_t& okTimes, const PlotCommand_cpv& pinfos, bool allTimes)
{
  plotm->getCapabilitiesTime(okTimes,pinfos,allTimes);
}

// return button names for ObsDialog
ObsDialogInfo Controller::initObsDialog()
{
  return obsm->initDialog();
}

// return button names for ObsDialog ... ascii files (when activated)
void Controller::updateObsDialog(ObsDialogInfo::PlotType& pt, const std::string& readername)
{
  obsm->updateDialog(pt, readername);
}

EditDialogInfo Controller::initEditDialog()
{
  return editm->getEditDialogInfo();
}

std::vector<std::string> Controller::getFieldLevels(const PlotCommand_cp& pinfo)
{
  if (FieldPlotCommand_cp cmd = std::dynamic_pointer_cast<const FieldPlotCommand>(pinfo))
    return fieldplotm->getFieldLevels(cmd);
  else
    return std::vector<std::string>();
}

miutil::miTime Controller::getFieldReferenceTime()
{
  return plotm->fieldplots()->getFieldReferenceTime();
}

plottimes_t Controller::getObsTimes(const std::vector<std::string>& name, bool update)
{
  return obsm->getTimes(name, update);
}

//********** plotting and selecting stations on the map***************

QString Controller::getStationsText(int x, int y)
{
  return plotm->stationplots()->getStationsText(x, y);
}

void Controller::putStations(StationPlot* stationPlot)
{
  plotm->stationplots()->putStations(stationPlot);
  plotm->setAnnotations();
}

void Controller::makeStationPlot(const std::string& commondesc,
    const std::string& common,
    const std::string& description,
    int from,
    const  std::vector<std::string>& data)
{
  plotm->stationplots()->makeStationPlot(commondesc, common, description, from, data);
}

std::string Controller::findStation(int x, int y, const std::string& name, int id)
{
  return plotm->stationplots()->findStation(x, y, name, id);
}

std::vector<std::string> Controller::findStations(int x, int y, const std::string& name, int id)
{
  return plotm->stationplots()->findStations(x, y, name, id);
}

void Controller::findStations(int x, int y, bool add,
    std::vector<std::string>& name,
    std::vector<int>& id,
    std::vector<std::string>& station)
{
  plotm->stationplots()->findStations(x, y, add, name, id, station);
}

void Controller::stationCommand(const std::string& command,
    const std::vector<std::string>& data,
    const std::string& name, int id,
    const std::string& misc)
{
  plotm->stationplots()->stationCommand(command, data, name, id, misc);

  if (command == "annotation")
    plotm->setAnnotations();
}

void Controller::stationCommand(const std::string& command,
    const std::string& name, int id)
{
  plotm->stationplots()->stationCommand(command, name, id);
  plotm->setAnnotations();
}

// area objects
void Controller::makeAreaObjects(const std::string& name, std::string areastring, int id)
{
  //METLIBS_LOG_DEBUG("Controller::makeAreas ");
  plotm->areaobjects()->makeAreaObjects(name,areastring,id);
}

void Controller::areaObjectsCommand(const std::string& command,const std::string& dataSet,
    const std::vector<std::string>& data, int id)
{
  //METLIBS_LOG_DEBUG("Controller::areaCommand");
  plotm->areaobjects()->areaObjectsCommand(command,dataSet,data,id);
}

//********** plotting and selecting locationPlots on the map **************
void Controller::putLocation(const LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();
  plotm->putLocation(locationdata);
}

void Controller::deleteLocation(const std::string& name)
{
  METLIBS_LOG_SCOPE(LOGVAL(name));
  plotm->deleteLocation(name);
}

void Controller::setSelectedLocation(const std::string& name,
    const std::string& elementname)
{
  METLIBS_LOG_SCOPE(LOGVAL(name) << LOGVAL(elementname));
  plotm->setSelectedLocation(name, elementname);
}

std::string Controller::findLocation(int x, int y, const std::string& name)
{
  METLIBS_LOG_SCOPE(LOGVAL(x) << LOGVAL(y) << LOGVAL(name));
  return plotm->findLocation(x,y,name);
}

//******************************************************************

std::map<std::string,InfoFile> Controller::getInfoFiles()
{
  return LocalSetupParser::getInfoFiles();
}


std::vector<PlotElement> Controller::getPlotElements()
{
  return plotm->getPlotElements();
}

void Controller::enablePlotElement(const PlotElement& pe)
{
  plotm->enablePlotElement(pe);
}

/********************* reading and writing log file *******************/

std::vector<std::string> Controller::writeLog()
{
  return man_->writeLog();
}

void Controller::readLog(const std::vector<std::string>& vstr,
    const std::string& thisVersion,
    const std::string& logVersion)
{
  man_->readLog(vstr, thisVersion, logVersion);
}

// Miscellaneous get methods
StationPlotCluster* Controller::getStationPlotCluster() const
{
  return plotm->stationplots();
}

std::vector<ObsPlot*> Controller::getObsPlots() const
{
  return plotm->obsplots()->getObsPlots();
}

void Controller::addManager(const std::string &name, Manager *man)
{
  plotm->managers[name] = man;
  connect(man, &Manager::repaintNeeded, this, &Controller::repaintNeeded);
}
