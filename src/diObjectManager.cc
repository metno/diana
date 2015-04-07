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

#include <diObjectManager.h>
#include <diPlotModule.h>
#include <diDrawingTypes.h>
#include <diObjectPlot.h>
#include <diWeatherObjects.h>
#include <diEditObjects.h>
#include <diDisplayObjects.h>
#include <diWeatherSymbol.h>
#include "diUtilities.h"
#include <puTools/miSetupParser.h>

#include <cstdio>
#include <fstream>
#include <iomanip>

#define MILOGGER_CATEGORY "diana.ObjectManager"
#include <miLogger/miLogging.h>

using namespace std;
using namespace::miutil;

ObjectManager::ObjectManager(PlotModule* pl)
  : plotm(pl), mapmode(normal_mode)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  //initialize variables
  doCombine = false;
  objectsSaved= true;
  objectsChanged = false;
  undoTemp = 0;

  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970,1,1,0,0,0);
}

ObjectManager::~ObjectManager()
{
}

void ObjectManager::changeProjection(const Area& newArea)
{
  editobjects.changeProjection(newArea);
  combiningobjects.changeProjection(newArea);
}

bool ObjectManager::parseSetup() {

  std::string section="OBJECTS";
  vector<std::string> vstr;

  if (!SetupParser::getSection(section,vstr)){
    METLIBS_LOG_ERROR("No " << section << " section in setupfile, ok.");
    return true;
  }

  //clear old setup info
  objectNames.clear();
  objectFiles.clear();

  std::string key,value,error;
  int i,n,nv,nvstr=vstr.size();

  for (nv=0; nv<nvstr; nv++) {
//#####################################################################
//  METLIBS_LOG_DEBUG("ObjectManager::parseSetup: " << vstr[nv]);
//#####################################################################
    vector<std::string> tokens= miutil::split_protected(vstr[nv], '\"','\"'," ",true);
    n= tokens.size();
    std::string name;
    ObjectList olist;
    olist.updated= false;
    bool ok= true;

    for (i=0; i<n; i++) {
      SetupParser::splitKeyValue(tokens[i],key,value);
      if (key=="name"){
	name= value;
	olist.archive=false;
      }else if (key=="plotoptions"){
	PlotOptions::parsePlotOption(value,olist.poptions);
      } else if (key=="archive_name"){
	name= value;
	olist.archive=true;
      }
      else if (key=="file"){
	TimeFilter tf;
	// init time filter and replace yyyy etc with *
	tf.initFilter(value);
	olist.filter=tf;
	olist.filename= value;
      } else {
	ok= false;
      }
    }
    if (not name.empty() && not olist.filename.empty()) {
      if (objectFiles.find(name)==objectFiles.end()) {     //add new name
	objectNames.push_back(name);
      }
      objectFiles[name]= olist;       //add or replace object
    } else {
      ok= false;
    }
    if (!ok) {
      error= "Bad object";
      SetupParser::errorMsg(section,nv,error);
    }
  }

  return true;
}


vector<std::string> ObjectManager::getObjectNames(bool archive) {
  //return objectnames. with/without archive
  vector<std::string> objNames;
  int n = objectNames.size();
  for(int i = 0; i<n;i++){
    if (archive){
      objNames.push_back(objectNames[i]);
    }
    else{
      if (objectFiles[objectNames[i]].archive==false)
	objNames.push_back(objectNames[i]);
    }
  }
  return objNames;
}

vector<miTime> ObjectManager::getObjectTimes()
//  * PURPOSE:   return times for list of PlotInfo's
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  const std::vector<std::string> pinfos(1, objects.getPlotInfo());
  set<miTime> timeset;

  int nn= pinfos.size();
  for (int i=0; i<nn; i++){
    vector<miTime> tv = getObjectTimes(pinfos[i]);
    for (unsigned int j=0; j<tv.size(); j++){
      timeset.insert(tv[j]);
    }
  }

  vector<miTime> timevec;
  if (timeset.size()>0) {
    set<miTime>::iterator p= timeset.begin();
    for (; p!=timeset.end(); p++) timevec.push_back(*p);
  }

  return timevec;
}

vector<miTime> ObjectManager::getObjectTimes(const string& pinfo)
//  * PURPOSE:   return times for list of PlotInfo's
{
  vector<miTime> timevec;

  vector<string> tokens= miutil::split_protected(pinfo, '"', '"');
  int m= tokens.size();
  for (int j=0; j<m; j++){
    std::string key,value;
    SetupParser::splitKeyValue(tokens[j],key,value);
    if (key=="name"){
      vector<ObjFileInfo> ofi= getObjectFiles(value,true);
      for (unsigned int k=0; k<ofi.size(); k++){
	timevec.push_back(ofi[k].time);
      }
      break;
    }
  }

  return timevec;
}

void ObjectManager::getCapabilitiesTime(vector<miTime>& normalTimes,
					int& timediff,
					const std::string& pinfo)
{
  //Finding times from pinfo
  //If pinfo contains "file=", return constTime

  std::string fileName;
  std::string objectname;
  timediff=0;

  vector<std::string> tokens= miutil::split_protected(pinfo, '"','"');
  int m= tokens.size();
  for (int j=0; j<m; j++){
    std::string key,value;
    SetupParser::splitKeyValue(tokens[j],key,value);
    if (key=="name"){
      objectname = value;
    } else if( key=="timediff"){
      timediff = miutil::to_int(value);
    } else if( key=="file"){ //Product with no time
      fileName=value;
    }
  }

  //Product with prog times
  if (fileName.empty()) {
    vector<ObjFileInfo> ofi= getObjectFiles(objectname,true);
    int nfinfo=ofi.size();
    for (int k=0; k<nfinfo; k++){
      normalTimes.push_back(ofi[k].time);
    }
  }

}


PlotOptions ObjectManager::getPlotOptions(std::string objectName){
  return objectFiles[objectName].poptions;
}


bool ObjectManager::insertObjectName(const std::string & name,
				     const std::string & file)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE(name << " " << file);
#endif
  bool ok=false;
  ObjectList olist;
  olist.updated= false;
  olist.filename= file;
  if (objectFiles.find(name)==objectFiles.end()) {
    objectNames.push_back(name);
    objectFiles[name]= olist;
    ok=true;
  }
  return ok;
}



/*----------------------------------------------------------------------
-----------  methods for finding and showing objectfiles ----------------
 -----------------------------------------------------------------------*/

void ObjectManager::prepareObjects(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  const std::string old_plotinfo = objects.getPlotInfo();
  const bool old_enabled = objects.isEnabled();

  objects.init();

  for (size_t i = 0; i < inp.size(); i++)
    objects.define(inp[i]);

  const std::string new_plotinfo = objects.getPlotInfo();
  objects.enable((new_plotinfo != old_plotinfo) or old_enabled);
}


bool ObjectManager::prepareObjects(const miTime& t, const Area& area)
{
  //are objects defined (in objects.define() if not return false)
  if (!objects.isDefined())
    return false;

  //if autoFile or wrong time, set correct time
  if (objects.isAutoFile() || objects.getTime() ==ztime)
    objects.setTime(t);

  if (objects.isAutoFile() || objects.filename.empty()){
    //not approved, we have to read new objects
    objects.setApproved(false);
    //clear objects !
    objects.clear();
    //get new filename
    if (!getFileName(objects))
      return false;
  }

  if (!objects.isApproved()){
    if (!objects.readEditDrawFile(objects.filename,area))
      return false;
  }

  return objects.prepareObjects();
}


vector<ObjFileInfo> ObjectManager::getObjectFiles(const std::string objectname,
    bool refresh)
{
  // called from ObjectDialog
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::getObjectFiles");
#endif

  if (refresh) {
    map<std::string,ObjectList>::iterator p,pend= objectFiles.end();
    for (p=objectFiles.begin(); p!=pend; p++)
      p->second.updated= false;
  }

  vector<ObjFileInfo> files;

  map<std::string,ObjectList>::iterator po= objectFiles.find(objectname);
  if (po==objectFiles.end()) return files;

  if (!po->second.updated || !po->second.files.size()) {
    po->second.files= listFiles(po->second);
    po->second.updated= true;
  }

  return po->second.files;
}


vector<ObjFileInfo> ObjectManager::listFiles(ObjectList & ol)
{
  METLIBS_LOG_SCOPE();
  std::string fileString= ol.filename + "*";
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("search string " << fileString);
#endif

  vector<ObjFileInfo> files;

  const diutil::string_v matches = diutil::glob(fileString);
  for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
    const std::string& name = *it;
    miTime time = timeFilterFileName(name,ol.filter);
    if(!time.undef() && time!=ztime){
      ObjFileInfo info;
      info.name = name;
      info.time = time;
      //sort files with the newest files first !
      if (files.empty()) {
	files.push_back(info);
      } else {
	vector <ObjFileInfo>::iterator p =  files.begin();
	while (p!=files.end() && p->time>info.time) p++;
	files.insert(p,info);
      }
    }
  }

  return files;
}


std::string ObjectManager::prefixFileName(std::string fileName)
{
  //get prefix from a file with name  /.../../prefix_*.yyyymmddhh
    vector <std::string> parts= miutil::split(fileName, 0, "/");
    std::string prefix = parts.back();
    vector <std::string> sparts= miutil::split(prefix, 0, "_");
    prefix=sparts[0];
    return prefix;
}

miTime ObjectManager::timeFilterFileName(std::string fileName,TimeFilter filter)
{
  if (filter.ok()){
    miTime t;
    filter.getTime(fileName,t);
    return t;
  }
  else
    return timeFileName(fileName);
}


miTime ObjectManager::timeFileName(std::string fileName)
{
  //get time from a file with name *.yyyymmddhh
  vector <std::string> parts= miutil::split(fileName, 0, ".");
  int nparts= parts.size();
  //if (parts.size() != 2) {
  //if (parts.size() < 2) {
  //return ztime ;
  //}
  if (parts[nparts-1].length() < 10) {

    size_t pos1 = fileName.find_last_of("_");
    size_t pos2 = fileName.find_last_of(".");
    if ( pos1 != string::npos && pos2 != string::npos && pos2 > pos1 + 10 ) {
      std::string tStr = fileName.substr(pos1+1,pos2-pos1-1);
      miutil::replace(tStr,"t","T");
      miTime time(tStr);
      return time;
    }
    return miTime();
  }
  return timeFromString(parts[nparts-1]);
}

miTime ObjectManager::timeFromString(std::string timeString)
{
  //get time from a string with yyyymmddhhmm
  if ( timeString.size() < 10 )
    return miTime();
  int year= atoi(timeString.substr(0,4).c_str());
  int mon=  atoi(timeString.substr(4,2).c_str());
  int day=  atoi(timeString.substr(6,2).c_str());
  int hour= atoi(timeString.substr(8,2).c_str());
  int min= 0;
  if (timeString.length() >= 12)
    min= atoi(timeString.substr(10,2).c_str());
  if (year<0 || mon <0 || day<0 || hour<0 || min < 0) return ztime;
  return miTime(year,mon,day,hour,min,0);
}


bool ObjectManager::getFileName(DisplayObjects& wObjects)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  if (wObjects.getObjectName().empty() || wObjects.getTime()==ztime)
    return false;


  map<std::string,ObjectList>::iterator po;
  po= objectFiles.find(wObjects.getObjectName());
  if (po==objectFiles.end()) return false;

  if (!po->second.updated || !po->second.files.size()) {
    po->second.files= listFiles(po->second);
    po->second.updated= true;
  }

  int n= po->second.files.size();

  int fileno=-1;
  int d, diff=wObjects.getTimeDiff() + 1;

  miTime t = wObjects.getTime();
  for (int i=0; i<n; i++) {
    d= abs(miTime::minDiff(t,po->second.files[i].time));
    if (d<diff) {
      diff= d;
      fileno= i;
    }
  }
  if (fileno<0) return false;

  wObjects.filename= po->second.files[fileno].name;
  wObjects.setTime(po->second.files[fileno].time);

  return true;
}

void ObjectManager::addPlotElements(std::vector<PlotElement>& pel)
{ std::string str = objects.getName();
  if (not str.empty()) {
    str += "# 0";
    bool enabled = objects.isEnabled();
    pel.push_back(PlotElement("OBJECTS", str, "OBJECTS", enabled));
  }
}

void ObjectManager::enablePlotElement(const PlotElement& pe)
{
  if (pe.type != "OBJECTS")
    return;
  std::string str = objects.getName() += "# 0" ;
  if (str == pe.str)
    objects.enable(pe.enabled);
}

void ObjectManager::getObjAnnotation(std::string &str, Colour &col)
{
  if (objects.isEnabled())
    objects.getObjAnnotation(str, col);
  else
    str.clear();
}

void ObjectManager::plotObjects(Plot::PlotOrder zorder)
{
  if (zorder == Plot::LINES) {
    objects.changeProjection(plotm->getStaticPlot()->getMapArea());
    objects.plot(zorder);
  }
}

/*----------------------------------------------------------------------
-----------  end of methods for finding and showing objectfiles ---------
 -----------------------------------------------------------------------*/
bool ObjectManager::editCommandReadCommentFile(const std::string filename)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::editCommandReadCommentfile");
#endif

  //read file with comments
  readEditCommentFile(filename,editobjects);

  return true;
}


bool ObjectManager::readEditCommentFile(const std::string filename,
				     WeatherObjects& wObjects){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::readEditCommentFile");
#endif

  return wObjects.readEditCommentFile(filename);
}

void ObjectManager::putCommentStartLines(const std::string name, const std::string prefix, const std::string lines){
  //return the startline of the comments file
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::putCommentStartLines");
#endif

  editobjects.putCommentStartLines(name,prefix,lines);
}

std::string ObjectManager::getComments(){
  //return the comments
  return editobjects.getComments();
}

void ObjectManager::putComments(const std::string & comments){
  editobjects.putComments(comments);
}

std::string ObjectManager::readComments(bool inEditSession){
  if (inEditSession)
    return editobjects.readComments();
  else
    return objects.readComments();
}

/*----------------------------------------------------------------------
----------- end of methods for reading and writing comments -------------
 -----------------------------------------------------------------------*/

bool ObjectManager::editCommandReadDrawFile(const std::string filename)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::editCommandReadDrawfile");
  METLIBS_LOG_DEBUG("ObjectManager::filename =   "<< filename);
#endif

  //size of objects to start with
  int edSize = editobjects.getSize();

  readEditDrawFile(filename,plotm->getMapArea(),editobjects);

  editNewObjectsAdded(edSize);

  // set symbol default size to size of last read object !
  editobjects.changeDefaultSize();

  if (autoJoinOn()) editobjects.editJoinFronts(true, true,false);

  setAllPassive();

  return true;
}


bool ObjectManager::readEditDrawFile(const std::string filename,
				     const Area& area,
				     WeatherObjects& wObjects){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::readEditDrawFile(2)");
#endif


  std::string fileName = filename;


  //check if filename exists, if not look for other files
  //with same time.
  //HK ??? to solve temporary problem with file names with/without mins
  if (!checkFileName(fileName)){
    METLIBS_LOG_ERROR("FILE " << fileName << " does not exist !");
    return false;
  }


  return wObjects.readEditDrawFile(fileName,area);
}

std::string ObjectManager::writeEditDrawString(const miTime& t,
					WeatherObjects& wObjects){
  return wObjects.writeEditDrawString(t);
}



bool ObjectManager::writeEditDrawFile(const std::string filename,
				      const std::string outputString){

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::writeEditDrawFile");
#endif

  if (outputString.empty()) return false;

  // open filestream
  ofstream file(filename.c_str());
  if (!file){
    METLIBS_LOG_ERROR("ERROR OPEN (WRITE) " << filename);
    return false;
  }

  file << outputString;

  file.close();

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("File " << filename   << " closed ");
#endif

  return true;
}


/************************************************
 *  Methods for reading and writing text  *
 *************************************************/

void ObjectManager::setCurrentText(const std::string & newText){
  WeatherSymbol::setCurrentText(newText);
}

void ObjectManager::setCurrentColour(const Colour::ColourInfo & newColour){
  WeatherSymbol::setCurrentColour(newColour);
}


std::string ObjectManager::getCurrentText(){
  return WeatherSymbol::getCurrentText();
}

Colour::ColourInfo ObjectManager::getCurrentColour(){
  return WeatherSymbol::getCurrentColour();
}


std::string ObjectManager::getMarkedText(){
  return editobjects.getMarkedText();
}

Colour::ColourInfo ObjectManager::getMarkedTextColour(){
  return editobjects.getMarkedTextColour();
}

Colour::ColourInfo ObjectManager::getMarkedColour(){
  return editobjects.getMarkedColour();
}


void ObjectManager::changeMarkedText(const std::string & newText){
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    editobjects.changeMarkedText(newText);
    editPostOperation();
  }
}

void ObjectManager::changeMarkedTextColour(const Colour::ColourInfo & newColour){
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    editobjects.changeMarkedTextColour(newColour);
    editPostOperation();
  }
}

void ObjectManager::changeMarkedColour(const Colour::ColourInfo & newColour){
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    editobjects.changeMarkedColour(newColour);
    editPostOperation();
  }
}



set<string> ObjectManager::getTextList()
{
  return WeatherSymbol::getTextList();
}

void ObjectManager::getMarkedMultilineText(vector<string>& symbolText)
{
  editobjects.getMarkedMultilineText(symbolText);
}


void ObjectManager::getMarkedComplexText(vector<string>& symbolText, vector<string>& xText)
{
  editobjects.getMarkedComplexText(symbolText,xText);
}

void ObjectManager::getMarkedComplexTextColored(vector <string> & symbolText, vector <string> & xText)
{
  editobjects.getMarkedComplexTextColored(symbolText,xText);
}

void ObjectManager::changeMarkedComplexTextColored(const vector<string>& symbolText, const vector<string>& xText)
{
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    editobjects.changeMarkedComplexTextColored(symbolText,xText);
    editPostOperation();
  }
}
void ObjectManager::changeMarkedMultilineText(const vector<string>& symbolText)
{
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    editobjects.changeMarkedMultilineText(symbolText);
    editPostOperation();
  }
}

void ObjectManager::changeMarkedComplexText(const vector<string>& symbolText, const vector<string>& xText)
{
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    editobjects.changeMarkedComplexText(symbolText,xText);
    editPostOperation();
  }
}

bool ObjectManager::inTextMode(){
  return editobjects.inTextMode();
}


bool ObjectManager::inComplexTextColorMode(){
  return editobjects.inComplexTextColorMode();
}

bool ObjectManager::inComplexTextMode(){
  return editobjects.inComplexTextMode();
}

bool ObjectManager::inEditTextMode(){
  return editobjects.inEditTextMode();
}

void ObjectManager::getCurrentComplexText(vector<string> & symbolText, vector<string> & xText)
{
  WeatherSymbol::getCurrentComplexText(symbolText,xText);
}

void ObjectManager::setCurrentComplexText(const vector<string>& symbolText, const vector<string> & xText)
{
  WeatherSymbol::setCurrentComplexText(symbolText,xText);
}

void ObjectManager::initCurrentComplexText()
{
  editobjects.initCurrentComplexText();
}

set<string> ObjectManager::getComplexList()
{
  return WeatherSymbol::getComplexList();
}



// ---------------------------------------------------------------
// ---------------------------------------------------------------
// ---------------------------------------------------------------



void ObjectManager::autoJoinToggled(bool on){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("autoJoinToggled is called");
#endif
  editobjects.setAutoJoinOn(on);
  if (autoJoinOn()) editobjects.editJoinFronts(true,true,false);
}


bool ObjectManager::autoJoinOn(){
  return editobjects.isAutoJoinOn();
}

bool ObjectManager::inDrawing(){
  if (mapmode==draw_mode)
    return editobjects.inDrawing;
  else if (mapmode==combine_mode)
    return combiningobjects.inDrawing;

  return false;
}

void ObjectManager::createNewObject()
{
  // create the object at first position,
  // avoiding a lot of empty objects, and the possiblity
  // to make the dialog work properly
  // (type as editmode and edittool)
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::createNewObject");
#endif
  editStopDrawing();
  if (mapmode==draw_mode)
    editobjects.createNewObject();
  else if (mapmode==combine_mode)
    combiningobjects.createNewObject();
}



void ObjectManager::editTestFront(){
  editobjects.editTestFront();
}



void ObjectManager::editSplitFront(const float x, const float y){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::editSplitFront");
#endif
  if (mapmode==draw_mode){
    editPrepareChange(SplitFronts);
    if (editobjects.editSplitFront(x,y)){
      if (autoJoinOn()) editobjects.editJoinFronts(false,true,false);
    } else
      objectsChanged= false;
    setAllPassive();
    editPostOperation();
  }

}


void ObjectManager::editResumeDrawing(const float x, const float y) {
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::editResumeDrawing");
#endif
  if (mapmode==draw_mode){

    editPrepareChange(ResumeDrawing);
    objectsChanged = editobjects.editResumeDrawing(x,y);
    editPostOperation();
  } else if (mapmode == combine_mode)
    doCombine=combiningobjects.editResumeDrawing(x,y);
}



bool ObjectManager::editCheckPosition(const float x, const float y){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::editCheckPosition");
#endif
  bool changed=false;

  //returns true if marked nodepoints changed

  if (mapmode == draw_mode){
    changed = editobjects.editCheckPosition(x,y);
  }else if (mapmode == combine_mode){
    changed=combiningobjects.editCheckPosition(x,y);
  }else {
    editobjects.unmarkAllPoints();
    setRubber(false,0,0);
    combiningobjects.unmarkAllPoints();
    changed=true;
  }

  return changed;
}


void ObjectManager::setAllPassive(){
  //Sets current status of the objects to passive
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::setAllPassive()");
#endif
  editobjects.setAllPassive();
  combiningobjects.setAllPassive();
  setRubber(false,0,0);
}

bool ObjectManager::setRubber(bool rubber, const float x, const float y){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::setRubber called");
#endif
  return editobjects.setRubber(rubber,x,y);
}


// -------------------------------------------------------
// --------- -------------- editcommands -----------------
// -------------------------------------------------------
void ObjectManager::editPrepareChange(const operation op)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("objm::editPrepareChange");
#endif

  //temporary undo buffer, in case changes occur
  undoTemp = new UndoFront( );
  if (mapmode!=combine_mode){
    objectsChanged = editobjects.saveCurrentFronts(op, undoTemp);
  }else
    objectsChanged=false;
}

void ObjectManager::editMouseRelease(bool moved)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("objm::editMouserRelease");
#endif
  if (moved)objectsChanged = true;
  editPostOperation();
  if (moved && autoJoinOn()) editobjects.editJoinFronts(false,true,false);

}


void ObjectManager::editPostOperation(){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("!! editPostOperation");
#endif
  if (objectsChanged && undoTemp!=0){
    editobjects.newUndoCurrent(undoTemp);
    objectsSaved = false;
  } else if (undoTemp!=0){
    delete undoTemp;
  }
  undoTemp = 0;
}

void ObjectManager::editNewObjectsAdded(int edSize){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("editNewObjectsAdded");
#endif

  //for undo buffer
  int diffedSize=editobjects.getSize()-edSize;
  undoTemp = new UndoFront();
  if (editobjects.saveCurrentFronts(diffedSize,undoTemp))
    editobjects.newUndoCurrent(undoTemp);
  else{
    delete undoTemp;
    undoTemp = NULL;
  }
}

void ObjectManager::editCommandJoinFronts(bool joinAll,bool movePoints,bool joinOnLine){
  if(mapmode== draw_mode){
    editPrepareChange(JoinFronts);
    objectsChanged=editobjects.editJoinFronts(joinAll,movePoints,joinOnLine);
    editPostOperation();
  }
}

void ObjectManager::setEditMode(const mapMode mmode,
				const int emode,
				const std::string etool){
  mapmode= mmode;

  editobjects.setEditMode(mmode,emode,etool);
  combiningobjects.setEditMode(mmode,emode,etool);

}



// -------------------------------------------------------
// -------------------------------------------------------
//---------------------------------------------------------

void ObjectManager::editStopDrawing(){
  if (mapmode==draw_mode){
        setRubber(false,0,0);
    if (autoJoinOn()) editobjects.editJoinFronts(false,true,false);
  }
  setAllPassive();
}


void ObjectManager::editDeleteMarkedPoints(){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::editDeleteMarkedPoints");
#endif

  if (mapmode==draw_mode){

    editPrepareChange(DeleteMarkedPoints);
    editobjects.editDeleteMarkedPoints();
    editPostOperation();
    //remove fronts with only one point...
    cleanUp();
    if (autoJoinOn()) editobjects.editJoinFronts(false,true,false);

  } else if (mapmode==combine_mode)
    doCombine = combiningobjects.editDeleteMarkedPoints();

}


void ObjectManager::editAddPoint(const float x, const float y){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::editAddPoint");
#endif

  if(mapmode== draw_mode){
    editPrepareChange(AddPoint);
    editobjects.editAddPoint(x,y);
    editPostOperation();
  } else if (mapmode==combine_mode)
    doCombine=combiningobjects.editAddPoint(x,y);
}


void ObjectManager::editStayMarked(){
  editobjects.editStayMarked();

}

void ObjectManager::editNotMarked(){
  if(mapmode== draw_mode){
    editobjects.editNotMarked();
  } else if(mapmode== combine_mode){
    combiningobjects.editNotMarked();
  }
  setAllPassive();
}


void
ObjectManager::editMergeFronts(bool mergeAll){
  //input parameters
  //mergeAll = true ->all fronts are joined
  //        = false->only marked or active fronts are joined
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("+++EditManager::editMergeFronts");
#endif
  if(mapmode== draw_mode){
    editPrepareChange(JoinFronts);
    objectsChanged =editobjects.editMergeFronts(mergeAll);
    editPostOperation();
  }
}


void ObjectManager::editUnJoinPoints(){

  if (mapmode==draw_mode){
    editobjects.editUnJoinPoints();
    checkJoinPoints();
  }
}


bool ObjectManager::editMoveMarkedPoints(const float x, const float y)
{
  bool changed = false;
  if (mapmode==draw_mode){
    changed = editobjects.editMoveMarkedPoints(x,y);
    if (changed){
      checkJoinPoints();
      //Join points but don't move
      if (autoJoinOn()) editobjects.editJoinFronts(false,false,false);
    }
  } else if (mapmode == combine_mode){
    changed = combiningobjects.editMoveMarkedPoints(x,y);
    if (changed) doCombine = true;
  }
  return changed;
}

bool ObjectManager::editRotateLine(const float x, const float y){
  bool changed = false;
  if (mapmode==draw_mode){
    changed = editobjects.editRotateLine(x,y);
    if (changed){
      checkJoinPoints();
      //Join points but don't move
      if (autoJoinOn()) editobjects.editJoinFronts(false,false,false);
    }
  } else if (mapmode == combine_mode){
    changed = combiningobjects.editRotateLine(x,y);
    if (changed) doCombine = true;
  }
  return changed;
}


void ObjectManager::editCopyObjects(){
  if (mapmode==draw_mode){
    editobjects.editCopyObjects();
  }
}


void ObjectManager::editPasteObjects(){
  if (mapmode==draw_mode){
    editPrepareChange(PasteObjects);
    editobjects.editPasteObjects();
    editPostOperation();
  }
}


void ObjectManager::editFlipObjects(){
  if (mapmode==draw_mode){
    editPrepareChange(FlipObjects);
    editobjects.editFlipObjects();
    editPostOperation();
  }
}


void ObjectManager::editUnmarkAllPoints(){
  editobjects.unmarkAllPoints();
}

void ObjectManager::editIncreaseSize(float val){
  if (mapmode==draw_mode){
    editPrepareChange(IncreaseSize);
    editobjects.editIncreaseSize(val);
    editPostOperation();
  } else if (mapmode==combine_mode){
    bool changed = combiningobjects.editIncreaseSize(val);
    if (changed) doCombine = true;
  }
}

void ObjectManager::editDefaultSize(){
  if (mapmode==draw_mode){
    editPrepareChange(DefaultSize);
    editobjects.editDefaultSize();
    editPostOperation();
  }  else if (mapmode==combine_mode){
    combiningobjects.editDefaultSize();
  }
}


void ObjectManager::editDefaultSizeAll(){
  if (mapmode==draw_mode){
    editPrepareChange(DefaultSizeAll);
    editobjects.editDefaultSizeAll();
    editPostOperation();
  } else if (mapmode == combine_mode){
    combiningobjects.editDefaultSizeAll();
  }
}



void ObjectManager::editRotateObjects(float val){
  if (mapmode==draw_mode){
    editPrepareChange(RotateObjects);
    editobjects.editRotateObjects(val);
    editPostOperation();
  }
}

void ObjectManager::editHideBox(){
  if (mapmode==draw_mode){
    editPrepareChange(HideBox);
    editobjects.editHideBox();
    editPostOperation();
  }
}

void ObjectManager::editHideAll(){
    editobjects.editHideAll();
}


void ObjectManager::editHideCombineObjects(int ir){
  editobjects.editHideCombineObjects(ir);
}

void ObjectManager::editUnHideAll(){
  editobjects.editUnHideAll();
}


void ObjectManager::editHideCombining(){
  if (mapmode == combine_mode) return;
  combiningobjects.editHideAll();
}

void ObjectManager::editUnHideCombining(){
  if (mapmode == combine_mode) return;
  combiningobjects.editUnHideAll();
}


void ObjectManager::editChangeObjectType(int val){
  if (mapmode == draw_mode){
    editPrepareChange(ChangeObjectType);
    editobjects.editChangeObjectType(val);
    editPostOperation();
  } else if (mapmode == combine_mode)
    doCombine=combiningobjects.editChangeObjectType(val);
}

void ObjectManager::cleanUp(){
  editPrepareChange(CleanUp);
  editobjects.cleanUp();
  editPostOperation();
}


void ObjectManager::checkJoinPoints(){
  editobjects.checkJoinPoints();
}


bool ObjectManager::redofront(){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::redofront");
#endif
  return editobjects.redofront();
}


bool ObjectManager::undofront(){
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::undofront");
#endif
  return editobjects.undofront();
}

void ObjectManager::undofrontClear(){
  editobjects.undofrontClear();
}


map <std::string,bool> ObjectManager::decodeTypeString(std::string token)
{
  return WeatherObjects::decodeTypeString(token);
}


std::string ObjectManager::stringFromTime(const miTime& t,bool addMinutes){
  int yyyy= t.year();
  int mm  = t.month();
  int dd  = t.day();
  int hh  = t.hour();
  int mn  = t.min();

  ostringstream ostr;
  ostr << setw(4) << setfill('0') << yyyy
       << setw(2) << setfill('0') << mm
       << setw(2) << setfill('0') << dd
       << setw(2) << setfill('0') << hh;
  if (addMinutes)
       ostr << setw(2) << setfill('0') << mn;

  std::string timestring = ostr.str();
  return timestring;
}



bool ObjectManager::_isafile(const std::string name){
  FILE *fp;
  if ((fp=fopen(name.c_str(),"r"))){
    fclose(fp);
    return true;
  } else return false;
}


bool ObjectManager::checkFileName(std::string &fileName){
  if(!_isafile(fileName)) {
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ObjectManager::checkFileName");
  METLIBS_LOG_DEBUG("filename =  " << fileName);
#endif
    //find time of file
    miTime time = timeFileName(fileName);
    //old filename style
    fileName = "ANAdraw."+stringFromTime(time,false);
    if(!_isafile(fileName)) return false;
  }
  return true;
}
