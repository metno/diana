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
#include <diPlotModule.h>
#include <diField/diRectangle.h>
#include <diField/diArea.h>
#include <diField/diFieldManager.h>
#include <diFieldPlotManager.h>
#include <diObsManager.h>
#include <diSatManager.h>
#include <diObjectManager.h>
#include <diEditManager.h>
#include <diGridAreaManager.h>
#include <diStationPlot.h>
#include <diImageGallery.h>
#include <diMapManager.h>

using namespace::miutil;

// Default constructor
Controller::Controller()
  : plotm(0), fieldm(0), fieldplotm(0), obsm(0), satm(0),
    objm(0), editm(0), aream(0),editoverride(false)
{
#ifdef DEBUGPRINT
  cerr << "Controller Constructor" << endl;
#endif
  // data managers
#ifdef PROFET
  profetController=0;
#endif
  fieldm=     new FieldManager;
  fieldplotm= new FieldPlotManager(fieldm);
  obsm=       new ObsManager;
  satm=       new SatManager;
  // plot central
  plotm= new PlotModule();
  // edit- and drawing-manager
  objm=  new ObjectManager(plotm);
  editm= new EditManager(plotm,objm);
  aream = new GridAreaManager();
  paintModeEnabled = false;
  scrollwheelZoom = false;
  plotm->setManagers(fieldm,fieldplotm,obsm,satm,objm,editm,aream);
//  profetController = new Profet::ProfetController(fieldm);
}

// Destructor
Controller::~Controller(){
#ifdef PROFET
  delete profetController;
#endif
  delete plotm;
  delete fieldm;
  delete fieldplotm;
  delete obsm;
  delete satm;
  delete objm;
  delete editm;
  delete aream;
}

// hack: indices for colorIndex mode set from gui
void Controller::setColourIndices(vector<Colour::ColourInfo>& vc){
  int n= vc.size();
  for (int i=0; i<n; i++){
    Colour::setindex(vc[i].name,vc[i].rgb[0]);
  }
}

void  Controller::restartFontManager()
{
  Plot::restartFontManager();
}

// parse setup
bool Controller::parseSetup()
{

  Plot::initFontManager();

  //Parse field sections
  vector<miString> fieldSubSect = fieldm->subsections();
  int nsect = fieldSubSect.size();
  vector<miString> errors;
  for( int i=0; i<nsect; i++){
    vector<miString> lines;
    if (!setupParser.getSection(fieldSubSect[i],lines)) {
      //      cerr<<"Missing section "<<fieldSubSect[i]<<" in setupfile."<<endl;
    }
    fieldm->parseSetup(lines,fieldSubSect[i],errors,false);
  }
  //Write error messages
  int nerror = errors.size();
  for( int i=0; i<nerror; i++){
    vector<miString> token = errors[i].split("|");
    setupParser.errorMsg(token[0],atoi(token[1].cStr()),token[2]);
  }

  //parse some setup sections
  if (!fieldplotm->parseSetup(setupParser)) return false;
  fieldm->setFieldNames(fieldplotm->getFields());
  if (!obsm->parseSetup(setupParser)) return false;
  if (!satm->parseSetup(setupParser)) return false;
  if (!objm->parseSetup(setupParser)) return false;
  if (!editm->parseSetup(setupParser)) return false;

  MapManager mapm;
  if (!mapm.parseSetup(setupParser)) return false;

  ImageGallery ig;
  ig.parseSetup(setupParser);

  return true;
}

void Controller::plotCommands(const vector<miString>& inp){
#ifdef DEBUGPRINT
  for (int q = 0; q < inp.size(); q++)
  cerr << "++ Controller::plotCommands:" << inp[q] << endl;
#endif
  plotm->preparePlots(inp);
#ifdef DEBUGPRINT
  cerr << "++ Returning from Controller::plotCommands ++" << endl;
#endif
}

void Controller::plot(bool under, bool over){
#ifdef DEBUGPRINT
  cerr << "++ Controller::plot() ++" << endl;
#endif
  plotm->plot(under, over);
#ifdef DEBUGPRINT
  cerr << "++ Returning from Controller::plot() ++" << endl;
#endif
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
Area Controller::getMapArea(){
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

// start hardcopy plot
void Controller::startHardcopy(const printOptions& po){
  plotm->startHardcopy(po);
}

// end hardcopy plot
void Controller::endHardcopy(){
  plotm->endHardcopy();
}


// return current plottime
void Controller::getPlotTime(miString& s){
  plotm->getPlotTime(s);
}

void Controller::getPlotTime(miTime& t){
  plotm->getPlotTime(t);
}

void Controller::getPlotTimes(vector<miTime>& fieldtimes,
			      vector<miTime>& sattimes,
			      vector<miTime>& obstimes,
			      vector<miTime>& objtimes,
			      vector<miTime>& ptimes)
{
  plotm->getPlotTimes(fieldtimes,sattimes,obstimes,objtimes,ptimes);
}

bool Controller::getProductTime(miTime& t){
  return editm->getProductTime(t);
}

miString Controller::getProductName()
{
  return editm->getProductName();
}


// vector<miString> Controller::getProductLabels(){
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
void Controller::updatePlots(){
  plotm->updatePlots();
}

void Controller::updateFieldPlot(const vector<miString>& pin){
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

bool Controller::getObsName(int x, int y, miString& name){
  return plotm->getObsName(x,y,name);
}

// plot other observations
void Controller::nextObs(bool next){
  plotm->nextObs(next);
}

//init hqcData from QSocket
bool Controller::initHqcdata(int from,
			     const miString& commondesc,
			     const miString& common,
			     const miString& desc,
			     const vector<miString>& data){
   return obsm->initHqcdata(from,commondesc,common,desc,data);
}

//update hqcData from QSocket
void Controller::updateHqcdata(const miString& commondesc,
			       const miString& common,
			       const miString& desc,
			       const vector<miString>& data){
  obsm->updateHqcdata(commondesc,common,desc,data);
}

//select obs parameter to flag from QSocket
void Controller::processHqcCommand(const miString& command,
				   const miString& str){
  obsm->processHqcCommand(command, str);
}

//plot trajectory position
void Controller::trajPos(vector<miString>& str){
  plotm->trajPos(str);
}

//plot radar echo position
void Controller::radePos(vector<miString>& str){
  plotm->radePos(str);
}

// start trajectory computation
bool Controller::startTrajectoryComputation(){
  return plotm->startTrajectoryComputation();
}

// get trajectory fields
vector<miString> Controller::getTrajectoryFields(){
  return plotm->getTrajectoryFields();
}

// get radarecho fields
vector<miString> Controller::getRadarEchoFields(){
  return plotm->getRadarEchoFields();
}


// write trajectory positions to file
bool Controller::printTrajectoryPositions(const miString& filename ){
  return plotm->printTrajectoryPositions( filename );
}

// get field models used (for Vprof etc.)
vector<miString> Controller::getFieldModels(){
  return plotm->getFieldModels();
}

//obs time step changed in edit dialog
void Controller::obsStepChanged(int step){
  plotm->obsStepChanged(step);
}

// get name++ of current channels (with calibration)
vector<miString> Controller::getCalibChannels(){
  return plotm->getCalibChannels();
}

// show values in grid position x,y
vector<SatValues> Controller::showValues(int x, int y){
  return plotm->showValues(x,y);
}

// show or hide satelitte classificiation table
// void Controller::showSatTable(int x,int y){
//   plotm->showSatTable(x,y);
// }

// bool Controller::inSatTable(int x,int y){
//   return plotm->inSatTable(x,y);
// }

vector <miString> Controller::getSatnames(){
  return plotm->getSatnames();
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

miString Controller::getMarkedAnnotation(){
  return plotm->getMarkedAnnotation();
}

void Controller::changeMarkedAnnotation(miString text,int cursor,
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

void Controller::stopEditAnnotation(miString prodname){
  vector <miString> labels  = plotm->writeAnnotations(prodname);
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
// the mouseevents are sent from GUI to controller,
// using a mouseEvent structure (diMapMode.h)
//
// the events are routed to other managers, depending
// on the current mapmode
//
// a struct EventResult is sent back to the GUI, informing
// if repaint is required and if any action should be taken
// (emitting mousemove or mouseclick signals etc.)
//
void Controller::sendMouseEvent(const mouseEvent& me,
				EventResult& res){
#ifdef DEBUGREDRAW
  cerr<<"Controller::sendMouseEvent................................"<<endl;
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
  if (me.type== mousepress && me.modifier==key_Shift){
    // turn off editing functions temporarily until mouse-release
    // event. Pan and Zoom will now work when editing
    editoverride= true;
  }
  // catch events to editmanager
  //-------------------------------------
  if ( (paintModeEnabled || inEdit) && !editoverride && !editpause){

    if(paintModeEnabled){

      if (me.type == mousemove && me.modifier==key_Shift){
        // shift + mouse move
        res.action=browsing;
      } else if (me.button== leftButton && me.type == mousepress && me.modifier==key_Shift){
        // shift + mouse click
        res.action=pointclick;
      } else {
        // Area
        float map_x,map_y;
        plotm->PhysToMap(me.x,me.y,map_x,map_y);
        aream->sendMouseEvent(me,res,map_x,map_y);
      }
    } else if (inEdit){

      editm->sendMouseEvent(me,res);
#ifdef DEBUGREDRAW
      cerr<<"Controller::sendMouseEvent editm res.repaint,bg,savebg,action: "
	  <<res.repaint<<" "<<res.background<<" "<<res.savebackground<<" "
	  <<res.action<<endl;
#endif
    }
  }
  // catch events to PlotModule
  //-------------------------------------
  else {
    res.newcursor= normal_cursor;
    plotm->sendMouseEvent(me,res);
  }

  // final mode-independent checks
  if (me.type==mouserelease){
    // mouse released: turn on editing functions again
    editoverride= false;
  }

  if (editoverride || me.modifier==key_Shift){ // set normal cursor
    res.newcursor= normal_cursor;
  }

  if (inEdit || paintModeEnabled) // always use underlay when in edit-mode
    res.savebackground= true;

  return;
}

//------------------------------------------------------------
// the keyboardevents are sent from GUI to controller,
// using a keyboardEvent structure (diMapMode.h)
//
// the events are routed to other managers, depending
// on the current mapmode
//
// a struct EventResult is sent back to the GUI, informing
// if repaint is required and if any action should be taken
//
void Controller::sendKeyboardEvent(const keyboardEvent& me,
				   EventResult& res){


  bool keyoverride = false;
  res.repaint= false;        // whether event is followed by a repaint
  res.background= true;      // ...and should the background be drawn too
  res.savebackground= false; // if background should be saved after paint
  res.newcursor= keep_it;    // leave the cursor be for now
  res.action= no_action;     // trigger GUI-action

  if (me.key==key_unknown) return;

  mapMode mm= editm->getMapMode();
  bool inEdit = (mm != normal_mode);

  if ((me.type== keypress && me.modifier==key_Shift) ||editm->getEditPause()) {
    keyoverride= true;
  }

    //TESTING GRIDEDITMANAGER
//  gridm->sendKeyboardEvent(me,res);

  // first check keys independent of mode
  //-------------------------------------
  if (me.type==keypress){
    if (me.key==key_PageUp){
      plotm->nextObs(false);  // browse through observations, backwards
      res.repaint= true;
      res.background= true;
      if (inEdit || paintModeEnabled) res.savebackground= true;
      return;
    } else if (me.key==key_PageDown){
      plotm->nextObs(true);  // browse through observations, forwards
      res.repaint= true;
      res.background= true;
      if (inEdit || paintModeEnabled) res.savebackground= true;
      return;
    } else if (me.modifier!=key_Alt &&
	       (me.key==key_F2 || me.key==key_F3 ||
	       me.key==key_F4 || me.key==key_F5 ||
	       me.key==key_F6 || me.key==key_F7 ||
	       me.key==key_F8)) {
      plotm->changeArea(me);
      res.repaint= true;
      res.background= true;
      if (inEdit || paintModeEnabled) res.savebackground= true;
      return;
    } else if (me.key==key_F9){
//    cerr << "F9 - ikke definert" << endl;
      return;
    } else if (me.key==key_F10){
//    cerr << "Vis forrige plott(utf�r)" << endl;
      return;
    } else if (me.key==key_F11){
//    cerr << "Vis neste plott (utf�r)" << endl;
      return;
      //####################################################################
    } else if ((me.key==key_Left && me.modifier==key_Shift) ||
	       (me.key==key_Right && me.modifier==key_Shift) ){
      plotm->obsTime(me,res);  // change observation time only
      res.repaint= true;
      res.background= true;
      if (inEdit || paintModeEnabled) res.savebackground= true;
      return;
      //####################################################################
     } else if (me.modifier!=key_Control &&
		(me.key==key_Left || me.key==key_Right ||
		me.key==key_Down || me.key==key_Up    ||
		me.key==key_Z    || me.key==key_X     ||
// 		me.key==key_A    || me.key==key_D     ||
// 		me.key==key_S    || me.key==key_W     ||
		me.key==key_Home)) {
      plotm->sendKeyboardEvent(me,res);
      res.repaint= true;
      res.background= true;
      if (inEdit || paintModeEnabled) res.savebackground= true;
      return;
    } else if (me.key==key_R) {
      plotm->sendKeyboardEvent(me,res);
      return;
    }
  }

  // catch events to editmanager
  //-------------------------------------
  if (inEdit ){
    editm->sendKeyboardEvent(me,res);
  }
  // catch events to PlotModule
  //-------------------------------------
  if( !inEdit || keyoverride ) {
    //    plotm->sendKeyboardEvent(me,res);
    if (me.type==keypress)
      res.action = keypressed;
  }

  if (inEdit || paintModeEnabled) // always use underlay when in edit-mode
    res.savebackground= true;

  return;
}

// ----- edit and drawing methods ----------

mapMode Controller::getMapMode(){
  return editm->getMapMode();
}


set <miString> Controller::getComplexList(){
  return objm->getComplexList();
}


// ------------ Dialog Info Routines ----------------


// return satfileinfo
const vector<SatFileInfo>& Controller::getSatFiles(const miString& satellite,
                                                   const miString& file,
						   bool update){
  return satm->getFiles(satellite,file,update);
}

//returns union or intersection of plot times from all pinfos
void Controller::getCapabilitiesTime(set<miTime>& okTimes,
				     set<miTime>& constTimes,
				     const vector<miString>& pinfos,
				     bool allTimes)
{
  return plotm->getCapabilitiesTime(okTimes,constTimes,pinfos,allTimes);

}

const vector<Colour>& Controller::getSatColours(const miString& satellite,
                                                   const miString& file){
  return satm->getColours(satellite,file);
}


const vector<miString>& Controller::getSatChannels(const miString& satellite,
                                                   const miString& file,
						   int index){
  return satm->getChannels(satellite,file,index);
}

bool Controller::isMosaic(const miString & satellite, const miString & file){
  return satm->isMosaic(satellite,file);
}

void Controller::SatRefresh(const miString& satellite, const miString& file){
  // HK set flag to refresh all files
  satm->updateFiles();
  satm->getFiles(satellite,file,true);
}




bool Controller::satFileListChanged(){
  // returns information about whether list of satellite files have changed
  //hence dialog and timeSlider times should change as well
  return satm->fileListChanged;
}

void Controller::satFileListUpdated(){
  //called when the dialog and timeSlider updated with info from satellite
  //file list
  satm->fileListChanged = false;
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


void Controller::setSatAuto(bool autoFile,const miString& satellite,
			    const miString& file){
  plotm->setSatAuto(autoFile,satellite,file);
}


void Controller::getUffdaClasses(vector <miString> & vUffdaClass,
				 vector <miString> &vUffdaClassTip){
  vUffdaClass=satm->vUffdaClass;
  vUffdaClassTip=satm->vUffdaClassTip;
}

bool Controller::getUffdaEnabled(){
  return satm->uffdaEnabled;
}

miString Controller::getUffdaMailAddress(){
  return satm->uffdaMailAddress;
}

// return button names for ObsDialog
ObsDialogInfo Controller::initObsDialog(){
  return obsm->initDialog();
}

// return button names for ObsDialog ... ascii files (when activated)
ObsDialogInfo Controller::updateObsDialog(const miString& name){
  return obsm->updateDialog(name);
}

// return button names for SatDialog
SatDialogInfo Controller::initSatDialog(){
  return satm->initDialog();
}

EditDialogInfo Controller::initEditDialog(){
  return editm->getEditDialogInfo();
}

vector<FieldDialogInfo> Controller::initFieldDialog(){
  return fieldm->getFieldDialogInfo();
}

void Controller::getAllFieldNames(vector<miString> & fieldNames,
				    set<std::string>& fieldprefixes,
				    set<std::string>& fieldsuffixes)
{
  fieldplotm->getAllFieldNames(fieldNames,fieldprefixes,fieldsuffixes);
}

miString Controller::getFieldClassSpecifications(const miString& fieldname)
{
  return fieldm->getFieldClassSpecifications(fieldname);
}

vector<miString> Controller::getFieldLevels(const miString& pinfo)
{
  return fieldplotm->getFieldLevels(pinfo);
}

set<std::string> Controller::getFieldReferenceTimes(const std::string model)
{
  return fieldm->getReferenceTimes(model);
}

void Controller::getFieldGroups(const miString& modelNameRequest,
				miString& modelName,
				std::string refTime,
				vector<FieldGroupInfo>& vfgi)
{
//   cerr <<"modelNameRequest: "<<modelNameRequest<<endl;

  fieldplotm->getFieldGroups(modelNameRequest, modelName, refTime, vfgi);
//   for(int i=0;i<vfgi.size();i++){
//     cerr <<"------------ "<<vfgi[i].groupName<<" ---------------------"<<endl;
//     for(int j=0;j<vfgi[i].fieldNames.size();j++)
//       cerr <<vfgi[i].fieldNames[j]<<endl;
//     cerr <<"+++++++++++++++++++++++++++++++"<<endl;
//     for(int j=0;j<vfgi[i].idnumNames.size();j++)
//       cerr <<vfgi[i].idnumNames[j]<<endl;
//   }
//   cerr <<"modelName: "<<modelName<<endl;

}

vector<miTime> Controller::getFieldTime(vector<FieldRequest>& request)
{
  bool constT;
  return fieldplotm->getFieldTime(request,constT);
}

MapDialogInfo Controller::initMapDialog(){
  MapManager mapm;
  return mapm.getMapDialogInfo();
}

bool Controller::MapInfoParser(miString& str, MapInfo& mi, bool tostr)
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

vector<miString> Controller::getObjectNames(bool useArchive){
  return objm->getObjectNames(useArchive);
}

void Controller::setObjAuto(bool autoFile){
  plotm->setObjAuto(autoFile);
}

vector<ObjFileInfo> Controller::getObjectFiles(miString objectname,
					       bool refresh) {
  return objm->getObjectFiles(objectname,refresh);
}



map<miString,bool> Controller::decodeTypeString( miString token){
  return objm->decodeTypeString(token);
}

vector< vector<Colour::ColourInfo> > Controller::getMultiColourInfo(int multiNum){
  return setupParser.getMultiColourInfo(multiNum);
}

bool Controller::getQuickMenus(vector<QuickMenuDefs>& qm)
{
  return setupParser.getQuickMenus(qm);
}


vector<miTime> Controller::getObsTimes( vector<miString> name){

  return obsm->getTimes(name);

}


//********** plotting and selecting stations on the map***************

void Controller::putStations(StationPlot* stationPlot){
  plotm->putStations(stationPlot);
}

void Controller::makeStationPlot(const miString& commondesc,
			 const miString& common,
			 const miString& description,
			 int from,
			 const  vector<miString>& data)
{
  plotm->makeStationPlot(commondesc,common,description,from,data);
}

miString Controller::findStation(int x, int y, miString name, int id){
  return plotm->findStation(x,y,name,id);
}

void Controller::findStations(int x, int y, bool add,
			      vector<miString>& name,
			      vector<int>& id,
			      vector<miString>& station){
  plotm->findStations(x,y,add,name,id,station);
}

void Controller::getEditStation(int step,
				miString& name, int& id,
				vector<miString>& stations){
  plotm->getEditStation(step,name,id,stations);
}

void Controller::stationCommand(const miString& command,
				vector<miString>& data,
				const miString& name, int id,
				const miString& misc)
{
  plotm->stationCommand(command,data,name,id,misc);
}
void Controller::stationCommand(const miString& command,
				const miString& name, int id)
{
  plotm->stationCommand(command,name,id);
}
float Controller::getStationsScale()
{
  return plotm->getStationsScale();
}
void Controller::setStationsScale(float new_scale)
{
  plotm->setStationsScale(new_scale);
}

//areas
void Controller::makeAreas(const miString& name, miString areastring, int id){
  //cerr << "Controller::makeAreas " << endl;
  plotm->makeAreas(name,areastring,id);
}

void Controller::areaCommand(const miString& command,const miString& dataSet,
			     const miString& data, int id ){
  //cerr << "Controller::areaCommand" << endl;
  plotm->areaCommand(command,dataSet,data,id);
}


vector <selectArea> Controller::findAreas(int x, int y, bool newArea){
  return plotm->findAreas(x,y,newArea);
}


//********** plotting and selecting locationPlots on the map **************
void Controller::putLocation(const LocationData& locationdata){
#ifdef DEBUGPRINT
	cerr << "Controller::putLocation" << endl;
#endif
  plotm->putLocation(locationdata);
}

void Controller::updateLocation(const LocationData& locationdata){
#ifdef DEBUGPRINT
	cerr << "Controller::updateLocation" << endl;
#endif
  plotm->updateLocation(locationdata);
}

void Controller::deleteLocation(const miString& name){
#ifdef DEBUGPRINT
	cerr << "Controller::deleteLocation: " << name << endl;
#endif
  plotm->deleteLocation(name);
}

void Controller::setSelectedLocation(const miString& name,
			           const miString& elementname){
#ifdef DEBUGPRINT
	cerr << "Controller::setSelectedLocation: " << name << "," << elementname << endl;
#endif
  plotm->setSelectedLocation(name,elementname);
}

miString Controller::findLocation(int x, int y, const miString& name){
#ifdef DEBUGPRINT
	cerr << "Controller::findLocation: " << x << "," << y << "," << name << endl;
#endif
  return plotm->findLocation(x,y,name);
}

//******************************************************************

map<miString,InfoFile> Controller::getInfoFiles()
{
  return setupParser.getInfoFiles();
}


vector<PlotElement>& Controller::getPlotElements()
{
  return plotm->getPlotElements();
}

void Controller::enablePlotElement(const PlotElement& pe)
{
  plotm->enablePlotElement(pe);
}

/********************* reading and writing log file *******************/

vector<miString> Controller::writeLog()
{
   return plotm->writeLog();
}

void Controller::readLog(const vector<miString>& vstr,
			 const miString& thisVersion,
			 const miString& logVersion)
{
  plotm->readLog(vstr,thisVersion,logVersion);
}

void Controller::setPaintModeEnabled(bool pm_enabled){
	paintModeEnabled = pm_enabled;
}

bool Controller::useScrollwheelZoom() {
  return scrollwheelZoom;
}

#ifdef PROFET
bool Controller::initProfet(){
  if(!fieldm) return false;
  if(!profetController){
    profetController = new Profet::ProfetController(fieldm);
  }
  return true;
}
/*
bool Controller::setProfetGUI(Profet::ProfetGUI * gui){
	if(profetController){
		profetController->setGUI(gui);
		return true;
	}
	return false;
}

bool Controller::profetConnect(const Profet::PodsUser & user,
    Profet::DataManagerType & type){
  if(profetController){
    type = profetController->registerUser(user,type); // throws Profet::ServerException
    return true;
  }
  return false;
}

void Controller::profetDisconnect(){
  if(profetController){
}
*/
Profet::ProfetController * Controller::getProfetController(){
	return profetController;
}

#endif



