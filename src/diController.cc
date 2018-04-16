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
#include "diStationManager.h"
#include "diStationPlot.h"
#include "miSetupParser.h"
#include "wmsclient/WebMapManager.h"

#include "diField/diArea.h"
#include "diField/diRectangle.h"
#include "diField/diFieldManager.h"

#include <puTools/miStringFunctions.h>

#include <QKeyEvent>
#include <QMouseEvent>

#include <boost/range/adaptor/map.hpp>

#define MILOGGER_CATEGORY "diana.Controller"
#include <miLogger/miLogging.h>

using namespace miutil;
using namespace std;

Controller::Controller()
: plotm(0), fieldm(0), fieldplotm(0), obsm(0), satm(0),
  objm(0), editm(0),editoverride(false)
{
  METLIBS_LOG_SCOPE();

  // data managers
  fieldm=     new FieldManager;
  fieldplotm= new FieldPlotManager(fieldm);
  obsm=       new ObsManager;
  satm=       new SatManager;
  stam=       new StationManager;
  // plot central
  plotm= new PlotModule();
  man_.reset(new MapAreaNavigator(plotm));
  // edit- and drawing-manager
  objm=  new ObjectManager(plotm);
  editm= new EditManager(plotm,objm,fieldplotm);
  plotm->setManagers(fieldm,fieldplotm,obsm,satm,stam,objm,editm);

  addManager("DRAWING", DrawingManager::instance());
  addManager("WEBMAP", WebMapManager::instance());
}

Controller::~Controller()
{
  delete plotm;
  delete fieldm;
  delete fieldplotm;
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

// hack: indices for colorIndex mode set from gui
void Controller::setColourIndices(std::vector<Colour::ColourInfo>& vc){
  int n= vc.size();
  for (int i=0; i<n; i++){
    Colour::setindex(vc[i].name,vc[i].rgb[0]);
  }
}

bool Controller::updateFieldFileSetup(const std::vector<std::string>& lines,
    std::vector<std::string>& errors)
{
  return getFieldManager()->updateFileSetup(lines, errors);
}

bool Controller::parseSetup()
{
  METLIBS_LOG_SCOPE();

  // plotm->getStaticPlot()->initFontManager();

  //Parse field sections
  vector<std::string> fieldSubSect = fieldm->subsections();
  int nsect = fieldSubSect.size();
  vector<std::string> errors;
  for( int i=0; i<nsect; i++){
    vector<std::string> lines;
    if (!SetupParser::getSection(fieldSubSect[i],lines)) {
      //      METLIBS_LOG_WARN("Missing section "<<fieldSubSect[i]<<" in setupfile.");
    }
    vector<std::string> string_lines;
    for (size_t j=0; j<lines.size(); j++) {
      string_lines.push_back(lines[j]);
    }
    fieldm->parseSetup(string_lines,fieldSubSect[i],errors);
  }
  //Write error messages
  int nerror = errors.size();
  for( int i=0; i<nerror; i++){
    vector<std::string> token = miutil::split(errors[i],"|");
    SetupParser::errorMsg(token[0],atoi(token[1].c_str()),token[2]);
  }

  //parse some setup sections
  if (!fieldplotm->parseSetup()) return false;
  fieldm->setFieldNames(fieldplotm->getFields());
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

  plotm->preparePlots(inp);
}

void Controller::plot(DiGLPainter* gl, bool under, bool over)
{
  METLIBS_LOG_SCOPE();
  plotm->plot(gl, under, over);
}

vector<AnnotationPlot*> Controller::getAnnotations()
{
  return plotm->getAnnotations();
}

vector<Rectangle> Controller::plotAnnotations(DiGLPainter* gl)
{
  return plotm->plotAnnotations(gl);
}

// get plotwindow corners in GL-coordinates
const Rectangle& Controller::getPlotSize()
{
  return plotm->getPlotSize();
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
  plotm->setPlotWindow(size.width(), size.height());
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

double Controller::getWindowDistances(const float& x, const float& y, const bool horizontal)
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

bool Controller::getProductTime(miTime& t)
{
  return editm->getProductTime(t);
}

std::string Controller::getProductName()
{
  return editm->getProductName();
}


// vector<std::string> Controller::getProductLabels(){
//   return editm->getProductLabels();
// }

// set plottime
void Controller::setPlotTime(const miTime& t)
{
  plotm->setPlotTime(t);
}

// toggle area conservatism
void Controller::keepCurrentArea(bool b)
{
  plotm->setKeepCurrentArea(b);
}

// update plot-classes with new data
bool Controller::updatePlots()
{
  return plotm->updatePlots();
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

bool Controller::getObsName(int x, int y, std::string& name)
{
  return plotm->obsplots()->getObsName(x,y,name);
}

std::string Controller::getObsPopupText(int x, int y)
{
  return plotm->obsplots()->getObsPopupText(x,y);
}

// plot other observations
void Controller::nextObs(bool next)
{
  plotm->obsplots()->nextObs(next);
}

//init hqcData from QSocket
bool Controller::initHqcdata(int from,
    const string& commondesc,
    const string& common,
    const string& desc,
    const vector<string>& data){
  return false; // FIXME obsm->initHqcdata(from,commondesc,common,desc,data);
}

//update hqcData from QSocket
void Controller::updateHqcdata(const string& commondesc,
    const string& common,
    const string& desc,
    const vector<string>& data){
  // FIXME obsm->updateHqcdata(commondesc,common,desc,data);
}

//select obs parameter to flag from QSocket
void Controller::processHqcCommand(const std::string& command,
    const std::string& str){
  // FIXME obsm->processHqcCommand(command, str);
}

//plot trajectory position
void Controller::trajPos(const vector<string>& str)
{
  plotm->trajPos(str);
}

//plot measurements position
void Controller::measurementsPos(const vector<string>& str)
{
  plotm->measurementsPos(str);
}

// start trajectory computation
bool Controller::startTrajectoryComputation(){
  return plotm->startTrajectoryComputation();
}

// get trajectory fields
vector<string> Controller::getTrajectoryFields()
{
  return plotm->fieldplots()->getTrajectoryFields();
}

// write trajectory positions to file
bool Controller::printTrajectoryPositions(const std::string& filename ){
  return plotm->printTrajectoryPositions( filename );
}

// get name++ of current channels (with calibration)
vector<string> Controller::getCalibChannels()
{
  return satm->getCalibChannels();
}

// show values in grid position x,y
vector<SatValues> Controller::showValues(float x, float y){
  return satm->showValues(x,y);
}

// show or hide satelitte classificiation table
// void Controller::showSatTable(int x,int y){
//   plotm->showSatTable(x,y);
// }

// bool Controller::inSatTable(int x,int y){
//   return plotm->inSatTable(x,y);
// }

vector<string> Controller::getSatnames()
{
  return satm->getSatnames();
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

mapMode Controller::getMapMode()
{
  return editm->getMapMode();
}


set<string> Controller::getComplexList()
{
  return objm->getComplexList();
}


// ------------ Dialog Info Routines ----------------


// return satfileinfo
const vector<SatFileInfo>& Controller::getSatFiles(const std::string& satellite,
    const std::string& file,
    bool update)
{
  return satm->getFiles(satellite,file,update);
}

//returns union or intersection of plot times from all pinfos
void Controller::getCapabilitiesTime(plottimes_t& okTimes, const PlotCommand_cpv& pinfos, bool allTimes)
{
  plotm->getCapabilitiesTime(okTimes,pinfos,allTimes);
}

const vector<Colour>& Controller::getSatColours(const std::string& satellite,
    const std::string& file)
{
  return satm->getColours(satellite,file);
}


const vector<std::string>& Controller::getSatChannels(const std::string& satellite,
    const std::string& file, int index)
{
  return satm->getChannels(satellite,file,index);
}

bool Controller::isMosaic(const std::string & satellite, const std::string & file)
{
  return satm->isMosaic(satellite,file);
}

void Controller::SatRefresh(const std::string& satellite, const std::string& file){
  // HK set flag to refresh all files
  satm->updateFiles();
  satm->getFiles(satellite,file,true);
}




bool Controller::satFileListChanged()
{
  // returns information about whether list of satellite files have changed
  //hence dialog and timeSlider times should change as well
  return satm->isFileListChanged();
}

void Controller::satFileListUpdated()
{
  //called when the dialog and timeSlider updated with info from satellite
  //file list
  satm->setFileListChanged(false);
}

bool Controller::obsTimeListChanged()
{
  // returns information about whether list of observation files have changed
  //hence dialog and timeSlider times should change as well
  return false; // FIXME obsm->timeListChanged;
}

void Controller::obsTimeListUpdated()
{
  //called when the dialog and timeSlider updated with info from observation
  //file list
  // FIXME obsm->timeListChanged = false;
}


void Controller::setSatAuto(bool autoFile,const std::string& satellite,
    const std::string& file)
{
  satm->setSatAuto(autoFile,satellite,file);
}


void Controller::getUffdaClasses(vector <std::string> & vUffdaClass,
    vector <std::string> &vUffdaClassTip)
{
  vUffdaClass=satm->vUffdaClass;
  vUffdaClassTip=satm->vUffdaClassTip;
}

bool Controller::getUffdaEnabled()
{
  return satm->uffdaEnabled;
}

std::string Controller::getUffdaMailAddress()
{
  return satm->uffdaMailAddress;
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

// return button names for SatDialog
SatDialogInfo Controller::initSatDialog()
{
  return satm->initDialog();
}

stationDialogInfo Controller::initStationDialog()
{
  return stam->initDialog();
}

EditDialogInfo Controller::initEditDialog()
{
  return editm->getEditDialogInfo();
}

vector<FieldDialogInfo> Controller::initFieldDialog()
{
  return fieldm->getFieldDialogInfo();
}

void Controller::getAllFieldNames(vector<std::string> & fieldNames)
{
  fieldplotm->getAllFieldNames(fieldNames);
}

vector<std::string> Controller::getFieldLevels(const PlotCommand_cp& pinfo)
{
  if (KVListPlotCommand_cp cmd = std::dynamic_pointer_cast<const KVListPlotCommand>(pinfo))
    return fieldplotm->getFieldLevels(cmd->all());
  else
    return std::vector<std::string>();
}

set<std::string> Controller::getFieldReferenceTimes(const std::string model)
{
  return fieldm->getReferenceTimes(model);
}

std::string Controller::getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour)
{
  return fieldm->getBestReferenceTime(model, refOffset, refHour);
}

miutil::miTime Controller::getFieldReferenceTime()
{
  return plotm->fieldplots()->getFieldReferenceTime();
}

void Controller::getFieldGroups(const std::string& modelName,
    std::string refTime,
    bool plotGroups,
    vector<FieldGroupInfo>& vfgi)
{
  fieldplotm->getFieldGroups(modelName, refTime, plotGroups, vfgi);
}

std::map<std::string,std::string> Controller::getFieldGlobalAttributes(const std::string& modelName,
    const std::string& refTime)
{
  return fieldm->getGlobalAttributes(modelName, refTime);
}

plottimes_t Controller::getFieldTime(vector<FieldRequest>& request)
{
  return fieldplotm->getFieldTime(request);
}

void Controller::updateFieldSource(const std::string & modelName)
{
  fieldm->updateSource(modelName);
}

//object dialog

vector<std::string> Controller::getObjectNames(bool useArchive)
{
  return objm->getObjectNames(useArchive);
}

void Controller::setObjAuto(bool autoFile)
{
  plotm->setObjAuto(autoFile);
}

vector<ObjFileInfo> Controller::getObjectFiles(std::string objectname,
    bool refresh) {
  return objm->getObjectFiles(objectname,refresh);
}

map<std::string,bool> Controller::decodeTypeString( std::string token)
{
  return objm->decodeTypeString(token);
}

vector< vector<Colour::ColourInfo> > Controller::getMultiColourInfo(int multiNum)
{
  return LocalSetupParser::getMultiColourInfo(multiNum);
}

bool Controller::getQuickMenus(vector<QuickMenuDefs>& qm)
{
  return LocalSetupParser::getQuickMenus(qm);
}

plottimes_t Controller::getObsTimes(const vector<string>& name, bool update)
{
  return obsm->getTimes(name, update);
}

//********** plotting and selecting stations on the map***************

void Controller::putStations(StationPlot* stationPlot)
{
  stam->putStations(stationPlot);
  plotm->setAnnotations();
}

void Controller::makeStationPlot(const string& commondesc,
    const string& common,
    const string& description,
    int from,
    const  vector<string>& data)
{
  stam->makeStationPlot(commondesc,common,description,from,data);
}

std::string Controller::findStation(int x, int y, const std::string& name, int id)
{
  return stam->findStation(x,y,name,id);
}

std::vector<std::string> Controller::findStations(int x, int y, const std::string& name, int id)
{
  return stam->findStations(x,y,name,id);
}

void Controller::findStations(int x, int y, bool add,
    vector<std::string>& name,
    vector<int>& id,
    vector<std::string>& station)
{
  stam->findStations(x,y,add,name,id,station);
}

void Controller::getStationData(vector< vector<std::string> >& data)
{
  stam->getStationData(data);
}

void Controller::stationCommand(const string& command,
    const vector<string>& data,
    const string& name, int id,
    const string& misc)
{
  stam->stationCommand(command,data,name,id,misc);

  if (command == "annotation")
    plotm->setAnnotations();
}

void Controller::stationCommand(const string& command,
    const string& name, int id)
{
  stam->stationCommand(command,name,id);

  plotm->setAnnotations();
}

float Controller::getStationsScale()
{
  return stam->getStationsScale();
}

void Controller::setStationsScale(float new_scale)
{
  stam->setStationsScale(new_scale);
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

void Controller::updateLocation(const LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();
  plotm->updateLocation(locationdata);
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

string Controller::findLocation(int x, int y, const string& name)
{
  METLIBS_LOG_SCOPE(LOGVAL(x) << LOGVAL(y) << LOGVAL(name));
  return plotm->findLocation(x,y,name);
}

//******************************************************************

map<string,InfoFile> Controller::getInfoFiles()
{
  return LocalSetupParser::getInfoFiles();
}


vector<PlotElement> Controller::getPlotElements()
{
  return plotm->getPlotElements();
}

void Controller::enablePlotElement(const PlotElement& pe)
{
  plotm->enablePlotElement(pe);
}

/********************* reading and writing log file *******************/

vector<string> Controller::writeLog()
{
  return man_->writeLog();
}

void Controller::readLog(const vector<string>& vstr,
    const string& thisVersion,
    const string& logVersion)
{
  man_->readLog(vstr, thisVersion, logVersion);
}

// Miscellaneous get methods
vector<SatPlot*> Controller::getSatellitePlots() const
{
  return satm->getSatellitePlots();
}

std::vector<FieldPlot*> Controller::getFieldPlots() const
{
  return plotm->fieldplots()->getFieldPlots();
}

std::vector<ObsPlot*> Controller::getObsPlots() const
{
  return plotm->obsplots()->getObsPlots();
}

void Controller::addManager(const std::string &name, Manager *man)
{
  plotm->managers[name] = man;
}

Manager *Controller::getManager(const std::string &name)
{
  PlotModule::managers_t::iterator it = plotm->managers.find(name);
  if (it != plotm->managers.end())
    return it->second;
  else
    return 0;
}
