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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diController.h>
#include <diManager.h>
#include <diPlotModule.h>
#include <diField/diRectangle.h>
#include <diField/diArea.h>
#include <diField/diFieldManager.h>
#include <diFieldPlotManager.h>
#include <diObsManager.h>
#include <diSatManager.h>
#include <diObjectManager.h>
#include <diDrawingManager.h>
#include <diEditManager.h>
#include <diStationManager.h>
#include <diStationPlot.h>
#include <diImageGallery.h>
#include <diMapManager.h>
#include <diLocalSetupParser.h>

#include <puTools/miSetupParser.h>

#include <QKeyEvent>
#include <QMouseEvent>

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
  // edit- and drawing-manager
  objm=  new ObjectManager(plotm);
  editm= new EditManager(plotm,objm,fieldplotm);
  scrollwheelZoom = false;
  plotm->setManagers(fieldm,fieldplotm,obsm,satm,stam,objm,editm);

  addManager("DRAWING", DrawingManager::instance());
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
}

// hack: indices for colorIndex mode set from gui
void Controller::setColourIndices(std::vector<Colour::ColourInfo>& vc){
  int n= vc.size();
  for (int i=0; i<n; i++){
    Colour::setindex(vc[i].name,vc[i].rgb[0]);
  }
}

void  Controller::restartFontManager()
{
  plotm->getStaticPlot()->restartFontManager();
}

bool Controller::parseSetup()
{
  METLIBS_LOG_SCOPE();

  plotm->getStaticPlot()->initFontManager();

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

  for (PlotModule::managers_t::iterator it = plotm->managers.begin(); it != plotm->managers.end(); ++it) {
    if (!it->second->parseSetup())
      return false;
  }

  MapManager mapm;
  if (!mapm.parseSetup()) return false;

  ImageGallery ig;
  ig.parseSetup();

  return true;
}

void Controller::plotCommands(const vector<string>& inp){
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
  for (int q = 0; q < inp.size(); q++)
    METLIBS_LOG_DEBUG("inp['" << q << "]='" << inp[q] << "'");
#endif
  plotm->preparePlots(inp);
}

void Controller::plot(bool under, bool over)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  plotm->plot(under, over);
}

vector<AnnotationPlot*> Controller::getAnnotations()
{
  return plotm->getAnnotations();
}

vector<Rectangle> Controller::plotAnnotations()
{
  return plotm->plotAnnotations();
}

// receive rectangle..
void Controller::PixelArea(const int x1, const int y1,
    const int x2, const int y2){
  Rectangle r(x1,y1,x2,y2);
  plotm->PixelArea(r);
}


// get plotwindow corners in GL-coordinates
void Controller::getPlotSize(float& x1, float& y1, float& x2, float& y2){
  Rectangle r= plotm->getPlotSize();
  x1= r.x1;
  x2= r.x2;
  y1= r.y1;
  y2= r.y2;
}

// get plot area (incl. projection)
const Area& Controller::getMapArea(){
  return plotm->getMapArea();
}

void Controller::zoomTo(const Rectangle & r) {
  plotm->zoomTo(r);
}

void Controller::zoomOut(){
  plotm->zoomOut();
}

// plotwindow size from GLwidget..
void Controller::setPlotWindow(const int w, const int h){
  plotm->setPlotWindow(w,h);
}

// return latitude,longitude from physical x,y
bool Controller::PhysToGeo(const float x,const float y,
    float& lat,float& lon){
  return plotm->PhysToGeo(x,y,lat,lon);
}

// return physical x,y from latitude,longitude
bool Controller::GeoToPhys(const float lat,const float lon,
    float& x,float& y){
  return plotm->GeoToPhys(lat,lon,x,y);
}

// return map x,y from physical x,y
void Controller::PhysToMap(const float x,const float y,
    float& xmap,float& ymap){
  plotm->PhysToMap(x,y,xmap,ymap);
}

/// return field grid x,y from map x,y if field defined and map proj = field proj
bool Controller::MapToGrid(const float xmap, const float ymap,
    float& gridx, float& gridy){
  return plotm->MapToGrid(xmap,ymap,gridx,gridy);
}

double Controller::getWindowDistances(const float& x, const float& y, const bool horizontal){
  return plotm->getWindowDistances(x, y, horizontal);
}

double Controller::getMarkedArea(const float& x, const float& y){
  return plotm->getMarkedArea(x, y);
}

double Controller::getWindowArea(){
  return plotm->getWindowArea();
}

// start hardcopy plot
void Controller::startHardcopy(const printOptions& po){
  plotm->startHardcopy(po);
}

// end hardcopy plot
void Controller::endHardcopy(){
  plotm->endHardcopy();
}


// return current plottime
void Controller::getPlotTime(std::string& s){
  plotm->getPlotTime(s);
}

void Controller::getPlotTime(miTime& t){
  plotm->getPlotTime(t);
}

void Controller::getPlotTimes(map<string,vector<miutil::miTime> >& times, bool updateSources)
{
  plotm->getPlotTimes(times, updateSources);
}

bool Controller::getProductTime(miTime& t){
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
bool Controller::setPlotTime(miTime& t){

  return plotm->setPlotTime(t);
}

// toggle area conservatism
void Controller::keepCurrentArea(bool b){
  plotm->keepCurrentArea(b);
}

// update plot-classes with new data
bool Controller::updatePlots(bool failOnMissingData){
  return plotm->updatePlots( failOnMissingData );
}

void Controller::updateFieldPlot(const vector<string>& pin)
{
  plotm->updateFieldPlot(pin);
}

const Area& Controller::getCurrentArea(){
  return plotm->getCurrentArea();
}

// get colour which is visible on the present background
Colour Controller::getContrastColour(){
  return plotm->getContrastColour();
}

// reload obsevations
void Controller::updateObs(){
  plotm->updateObs();
}

// find obs in grid position x,y
bool Controller::findObs(int x, int y){
  return plotm->findObs(x,y);
}

bool Controller::getObsName(int x, int y, std::string& name){
  return plotm->getObsName(x,y,name);
}

std::string Controller::getObsPopupText(int x, int y){
  return plotm->getObsPopupText(x,y); 
}
// plot other observations
void Controller::nextObs(bool next){
  plotm->nextObs(next);
}

//init hqcData from QSocket
bool Controller::initHqcdata(int from,
    const string& commondesc,
    const string& common,
    const string& desc,
    const vector<string>& data){
  return obsm->initHqcdata(from,commondesc,common,desc,data);
}

//update hqcData from QSocket
void Controller::updateHqcdata(const string& commondesc,
    const string& common,
    const string& desc,
    const vector<string>& data){
  obsm->updateHqcdata(commondesc,common,desc,data);
}

//select obs parameter to flag from QSocket
void Controller::processHqcCommand(const std::string& command,
    const std::string& str){
  obsm->processHqcCommand(command, str);
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
  return plotm->getTrajectoryFields();
}

// write trajectory positions to file
bool Controller::printTrajectoryPositions(const std::string& filename ){
  return plotm->printTrajectoryPositions( filename );
}

// get field models used (for Vprof etc.)
vector<string> Controller::getFieldModels()
{
  return plotm->getFieldModels();
}

//obs time step changed in edit dialog
void Controller::obsStepChanged(int step){
  plotm->obsStepChanged(step);
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

void Controller::showAnnotations(bool on){
  plotm->showAnnotations(on);
}

void Controller::toggleScrollwheelZoom(bool on){
  scrollwheelZoom = !scrollwheelZoom;
}

bool Controller::markAnnotationPlot(int x, int y){
  return plotm->markAnnotationPlot(x,y);
}

std::string Controller::getMarkedAnnotation(){
  return plotm->getMarkedAnnotation();
}

void Controller::changeMarkedAnnotation(std::string text,int cursor,
    int sel1, int sel2){
  plotm->changeMarkedAnnotation(text,cursor,sel1,sel2);
}


void Controller::DeleteMarkedAnnotation()
{
  plotm->DeleteMarkedAnnotation();
}

void Controller::startEditAnnotation(){
  plotm->startEditAnnotation();
}

void Controller::stopEditAnnotation(std::string prodname){
  vector <string> labels  = plotm->writeAnnotations(prodname);
  editm->saveProductLabels(labels);
  plotm->stopEditAnnotation();
}

void Controller::editNextAnnoElement(){
  plotm->editNextAnnoElement();
}


void Controller::editLastAnnoElement(){
  plotm->editLastAnnoElement();
}


//Archive mode
void Controller::archiveMode(bool on){
  obsm->archiveMode(on);
  satm->archiveMode(on);
}

// keyboard/mouse events

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
  METLIBS_LOG_DEBUG("Controller::sendMouseEvent................................");
#endif
  res.repaint= false;        // whether event is followed by a repaint
  res.background= false;     // ...and should the background be drawn too
  res.savebackground= false; // if background should be saved after paint
  res.newcursor= keep_it;    // leave the cursor be for now
  res.action= no_action;     // trigger GUI-action


  mapMode mm= editm->getMapMode();
  bool inEdit = (mm != normal_mode);
  bool editpause= editm->getEditPause();

  // first check events independent of mode
  //-------------------------------------
  if (me->type() == QEvent::MouseButtonPress && me->modifiers() & Qt::ShiftModifier){
    // turn off editing functions temporarily until mouse-release
    // event. Pan and Zoom will now work when editing
    editoverride= true;
  }
  // catch events to editmanager
  //-------------------------------------
  if (inEdit && !editoverride && !editpause){

    if (inEdit){
      editm->sendMouseEvent(me,res);
#ifdef DEBUGREDRAW
      METLIBS_LOG_DEBUG("Controller::sendMouseEvent editm res.repaint,bg,savebg,action: "
          <<res.repaint<<" "<<res.background<<" "<<res.savebackground<<" "
          <<res.action);
#endif
    }
  } else {
    // Send the event to the other managers to see if one of them will handle it.
    bool handled = false;
    if (!(me->modifiers() & Qt::ShiftModifier)) {
      for (PlotModule::managers_t::iterator it = plotm->managers.begin(); it != plotm->managers.end(); ++it) {
        if (it->second->isEditing()) {
          it->second->sendMouseEvent(me, res);
          if (me->isAccepted())
            handled = true;
          break;
        }
      }
    }
    if (!handled) {
      res.newcursor= normal_cursor;
      plotm->sendMouseEvent(me,res);
    }
  }

  // final mode-independent checks
  if (me->type() == QEvent::MouseButtonRelease){
    // mouse released: turn on editing functions again
    editoverride= false;
  }

  if (editoverride || me->modifiers() & Qt::ShiftModifier){ // set normal cursor
    res.newcursor= normal_cursor;
  }

  if (inEdit) // always use underlay when in edit-mode
    res.savebackground= true;

  return;
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
  bool keyoverride = false;
  res.repaint= false;        // whether event is followed by a repaint
  res.background= true;      // ...and should the background be drawn too
  res.savebackground= false; // if background should be saved after paint
  res.newcursor= keep_it;    // leave the cursor be for now
  res.action= no_action;     // trigger GUI-action

  if (ke->key() == Qt::Key_unknown) return;

  // Old editing checks to override normal keypress behaviour.
  mapMode mm = editm->getMapMode();
  bool inEdit = (mm != normal_mode);

  // A more general way to override normal keypress behaviour is to query
  // the managers to find any that are in editing mode.
  for (PlotModule::managers_t::iterator it = plotm->managers.begin(); it != plotm->managers.end(); ++it) {
    if (it->second->isEditing()) {
      res.savebackground = true;
      it->second->sendKeyboardEvent(ke, res);
      if (it->second->hasFocus())
        return;
    }
  }

  // Access to normal keypress behaviour is obtained by holding down the Shift key
  // when in the old editing mode, or when editing is paused.
  if ((ke->type() == QEvent::KeyPress && ke->modifiers() & Qt::ShiftModifier) ||editm->getEditPause()) {
    keyoverride= true;
  }

  //TESTING GRIDEDITMANAGER
  //  gridm->sendKeyboardEvent(ke,res);

  // first check keys independent of mode
  //-------------------------------------
  if (ke->type() == QEvent::KeyPress){
    if (ke->key() == Qt::Key_PageUp or ke->key() == Qt::Key_PageDown) {
      const bool forward = ke->key() == Qt::Key_PageDown;
      plotm->nextObs(forward);  // browse through observations
      res.repaint= true;
      res.background= true;
      if (inEdit)
        res.savebackground= true;
      return;
    } else if (!(ke->modifiers() & Qt::AltModifier) &&
        (ke->key() == Qt::Key_F2 || ke->key() == Qt::Key_F3 ||
            ke->key() == Qt::Key_F4 || ke->key() == Qt::Key_F5 ||
            ke->key() == Qt::Key_F6 || ke->key() == Qt::Key_F7 ||
            ke->key() == Qt::Key_F8)) {
      plotm->changeArea(ke);
      res.repaint= true;
      res.background= true;
      if (inEdit) res.savebackground= true;
      return;
    } else if (ke->key() == Qt::Key_F9){
      //    METLIBS_LOG_WARN("F9 - not defined");
      return;
    } else if (ke->key() == Qt::Key_F10){
      //    METLIBS_LOG_WARN("Show previus plot (apply)");
      return;
    } else if (ke->key() == Qt::Key_F11){
      //    METLIBS_LOG_WARN("Show next plot (apply)");
      return;
      //####################################################################
    } else if ((ke->key() == Qt::Key_Left && ke->modifiers() & Qt::ShiftModifier) ||
        (ke->key() == Qt::Key_Right && ke->modifiers() & Qt::ShiftModifier) ){
      plotm->obsTime(ke,res);  // change observation time only
      res.repaint= true;
      res.background= true;
      if (inEdit) res.savebackground= true;
      return;
      //####################################################################
    } else if (!(ke->modifiers() & Qt::ControlModifier) &&
        !(ke->modifiers() & Qt::GroupSwitchModifier) && // "Alt Gr" modifier
        (ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right ||
            ke->key() == Qt::Key_Down || ke->key() == Qt::Key_Up    ||
            ke->key() == Qt::Key_Z    || ke->key() == Qt::Key_X     ||
            // 		ke->key() == Qt::Key_A    || ke->key() == Qt::Key_D     ||
            // 		ke->key() == Qt::Key_S    || ke->key() == Qt::Key_W     ||
            ke->key() == Qt::Key_Home)) {
      plotm->sendKeyboardEvent(ke,res);
      res.repaint= true;
      res.background= true;
      if (inEdit) res.savebackground= true;
      return;
    } else if (ke->key() == Qt::Key_R) {
      plotm->sendKeyboardEvent(ke,res);
      return;
    }
  }

  // catch events to editmanager
  //-------------------------------------
  if (inEdit ){
    editm->sendKeyboardEvent(ke,res);
  }

  // catch events to PlotModule
  //-------------------------------------
  if( !inEdit || keyoverride ) {
    //    plotm->sendKeyboardEvent(ke,res);
    if (ke->type() == QEvent::KeyPress)
      res.action = keypressed;
  }

  if (inEdit) // always use underlay when in edit-mode
    res.savebackground= true;

  return;
}

// ----- edit and drawing methods ----------

mapMode Controller::getMapMode(){
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
    bool update){
  return satm->getFiles(satellite,file,update);
}

//returns union or intersection of plot times from all pinfos
void Controller::getCapabilitiesTime(set<miTime>& okTimes,
    const vector<string>& pinfos,
    bool allTimes,
    bool updateSources)
{
  plotm->getCapabilitiesTime(okTimes,pinfos,allTimes,updateSources);
}

const vector<Colour>& Controller::getSatColours(const std::string& satellite,
    const std::string& file){
  return satm->getColours(satellite,file);
}


const vector<std::string>& Controller::getSatChannels(const std::string& satellite,
    const std::string& file, int index)
{
  return satm->getChannels(satellite,file,index);
}

bool Controller::isMosaic(const std::string & satellite, const std::string & file){
  return satm->isMosaic(satellite,file);
}

void Controller::SatRefresh(const std::string& satellite, const std::string& file){
  // HK set flag to refresh all files
  satm->updateFiles();
  satm->getFiles(satellite,file,true);
}




bool Controller::satFileListChanged(){
  // returns information about whether list of satellite files have changed
  //hence dialog and timeSlider times should change as well
  return satm->isFileListChanged();
}

void Controller::satFileListUpdated(){
  //called when the dialog and timeSlider updated with info from satellite
  //file list
  satm->setFileListChanged(false);
}

bool Controller::obsTimeListChanged(){
  // returns information about whether list of observation files have changed
  //hence dialog and timeSlider times should change as well
  return obsm->timeListChanged;
}

void Controller::obsTimeListUpdated(){
  //called when the dialog and timeSlider updated with info from observation
  //file list
  obsm->timeListChanged = false;
}


void Controller::setSatAuto(bool autoFile,const std::string& satellite,
    const std::string& file)
{
  satm->setSatAuto(autoFile,satellite,file);
}


void Controller::getUffdaClasses(vector <std::string> & vUffdaClass,
    vector <std::string> &vUffdaClassTip){
  vUffdaClass=satm->vUffdaClass;
  vUffdaClassTip=satm->vUffdaClassTip;
}

bool Controller::getUffdaEnabled(){
  return satm->uffdaEnabled;
}

std::string Controller::getUffdaMailAddress(){
  return satm->uffdaMailAddress;
}

// return button names for ObsDialog
ObsDialogInfo Controller::initObsDialog(){
  return obsm->initDialog();
}

// return button names for ObsDialog ... ascii files (when activated)
ObsDialogInfo Controller::updateObsDialog(const std::string& name){
  return obsm->updateDialog(name);
}

// return button names for SatDialog
SatDialogInfo Controller::initSatDialog(){
  return satm->initDialog();
}

stationDialogInfo Controller::initStationDialog(){
  return stam->initDialog();
}

EditDialogInfo Controller::initEditDialog(){
  return editm->getEditDialogInfo();
}

vector<FieldDialogInfo> Controller::initFieldDialog(){
  return fieldm->getFieldDialogInfo();
}

void Controller::getAllFieldNames(vector<std::string> & fieldNames)
{
  fieldplotm->getAllFieldNames(fieldNames);
}

vector<std::string> Controller::getFieldLevels(const std::string& pinfo)
{
  return fieldplotm->getFieldLevels(pinfo);
}

set<std::string> Controller::getFieldReferenceTimes(const std::string model)
{
  return fieldm->getReferenceTimes(model);
}

std::string Controller::getBestFieldReferenceTime(const std::string& model, int refOffset, int refHour)
{
  return fieldm->getBestReferenceTime(model, refOffset, refHour);
}

void Controller::getFieldGroups(const std::string& modelName,
    std::string refTime,
    bool plotGroups,
    vector<FieldGroupInfo>& vfgi)
{

  fieldplotm->getFieldGroups(modelName, refTime, plotGroups, vfgi);

}

vector<miTime> Controller::getFieldTime(vector<FieldRequest>& request)
{
  return fieldplotm->getFieldTime(request);
}

void Controller::updateFieldSource(const std::string & modelName)
{
  fieldm->updateSource(modelName);
}

MapDialogInfo Controller::initMapDialog(){
  MapManager mapm;
  return mapm.getMapDialogInfo();
}

bool Controller::MapInfoParser(std::string& str, MapInfo& mi, bool tostr)
{
  MapManager mapm;
  if (tostr){
    str= mapm.MapInfo2str(mi);
    return true;
  } else {
    PlotOptions a,b,c,d,e;
    return mapm.fillMapInfo(str,mi,a,b,c,d,e);
  }
}

//object dialog

vector<std::string> Controller::getObjectNames(bool useArchive){
  return objm->getObjectNames(useArchive);
}

void Controller::setObjAuto(bool autoFile){
  plotm->setObjAuto(autoFile);
}

vector<ObjFileInfo> Controller::getObjectFiles(std::string objectname,
    bool refresh) {
  return objm->getObjectFiles(objectname,refresh);
}



map<std::string,bool> Controller::decodeTypeString( std::string token){
  return objm->decodeTypeString(token);
}

vector< vector<Colour::ColourInfo> > Controller::getMultiColourInfo(int multiNum){
  return LocalSetupParser::getMultiColourInfo(multiNum);
}

bool Controller::getQuickMenus(vector<QuickMenuDefs>& qm)
{
  return LocalSetupParser::getQuickMenus(qm);
}

vector<miTime> Controller::getObsTimes(const vector<string>& name)
{
  return obsm->getTimes(name);
}


//********** plotting and selecting stations on the map***************

void Controller::putStations(StationPlot* stationPlot){
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

std::string Controller::findStation(int x, int y, std::string name, int id){
  return stam->findStation(x,y,name,id);
}

std::vector<std::string> Controller::findStations(int x, int y, std::string name, int id){
  return stam->findStations(x,y,name,id);
}

void Controller::findStations(int x, int y, bool add,
    vector<std::string>& name,
    vector<int>& id,
    vector<std::string>& station){
  stam->findStations(x,y,add,name,id,station);
}

void Controller::getEditStation(int step,
    std::string& name, int& id,
    vector<std::string>& stations){
  if (stam->getEditStation(step,name,id,stations))
    plotm->PlotAreaSetup();
}

void Controller::getStationData(vector<std::string>& data)
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

//areas
void Controller::makeAreas(const std::string& name, std::string areastring, int id){
  //METLIBS_LOG_DEBUG("Controller::makeAreas ");
  plotm->makeAreas(name,areastring,id);
}

void Controller::areaCommand(const std::string& command,const std::string& dataSet,
    const std::string& data, int id ){
  //METLIBS_LOG_DEBUG("Controller::areaCommand");
  plotm->areaCommand(command,dataSet,data,id);
}


vector <selectArea> Controller::findAreas(int x, int y, bool newArea){
  return plotm->findAreas(x,y,newArea);
}


//********** plotting and selecting locationPlots on the map **************
void Controller::putLocation(const LocationData& locationdata){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Controller::putLocation");
#endif
  plotm->putLocation(locationdata);
}

void Controller::updateLocation(const LocationData& locationdata){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Controller::updateLocation");
#endif
  plotm->updateLocation(locationdata);
}

void Controller::deleteLocation(const std::string& name){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Controller::deleteLocation: " << name);
#endif
  plotm->deleteLocation(name);
}

void Controller::setSelectedLocation(const std::string& name,
    const std::string& elementname){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Controller::setSelectedLocation: " << name << "," << elementname);
#endif
  plotm->setSelectedLocation(name,elementname);
}

string Controller::findLocation(int x, int y, const string& name)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Controller::findLocation: " << x << "," << y << "," << name);
#endif
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
  return plotm->writeLog();
}

void Controller::readLog(const vector<string>& vstr,
    const string& thisVersion,
    const string& logVersion)
{
  plotm->readLog(vstr,thisVersion,logVersion);
}

bool Controller::useScrollwheelZoom() {
  return scrollwheelZoom;
}

// Miscellaneous get methods
const vector<SatPlot*>& Controller::getSatellitePlots() const
{
  return satm->getSatellitePlots();
}

vector<FieldPlot*> Controller::getFieldPlots() const
{
  return plotm->getFieldPlots();
}

vector<ObsPlot*> Controller::getObsPlots() const
{
  return plotm->getObsPlots();
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
