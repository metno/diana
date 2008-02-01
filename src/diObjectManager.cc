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
#include <fstream>
#include <diObjectManager.h>
#include <diPlotModule.h>
#include <diDrawingTypes.h>
#include <diObjectPlot.h>
#include <diWeatherObjects.h>
#include <diEditObjects.h>
#include <diDisplayObjects.h>
#include <diWeatherSymbol.h>
#include <glob.h>
#include <stdio.h>

ObjectManager::ObjectManager(PlotModule* pl)
  : plotm(pl)
{
#ifdef DEBUGPRINT
  cerr << "ObjectManager constructor" << endl;
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


bool ObjectManager::parseSetup(SetupParser& sp) {

  miString section="OBJECTS";
  vector<miString> vstr;

  if (!sp.getSection(section,vstr)){
    cerr << "No " << section << " section in setupfile, ok." << endl;
    return true;
  }

  miString key,value,error;
  int i,n,nv,nvstr=vstr.size();

  for (nv=0; nv<nvstr; nv++) {
//#####################################################################
//  cerr << "ObjectManager::parseSetup: " << vstr[nv] << endl;
//#####################################################################
    vector<miString> tokens= vstr[nv].split('\"','\"'," ",true);
    n= tokens.size();
    miString name;
    ObjectList olist;
    olist.updated= false;
    bool ok= true;

    for (i=0; i<n; i++) {
      sp.splitKeyValue(tokens[i],key,value);
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
    if (name.exists() && olist.filename.exists()){
      if (objectFiles.find(name)==objectFiles.end()) {     //add new name
	objectNames.push_back(name);
      } 
      objectFiles[name]= olist;       //add or replace object
    } else {
      ok= false;
    }
    if (!ok) {
      error= "Bad object";
      sp.errorMsg(section,nv,error);
    }
  }

  return true;
}


vector<miString> ObjectManager::getObjectNames(bool archive) {
  //return objectnames. with/without archive
  vector<miString> objNames;
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

vector<miTime> ObjectManager::getObjectTimes(const vector<miString>& pinfos)
//  * PURPOSE:   return times for list of PlotInfo's
{
#ifdef DEBUGPRINT
   cerr<<"ObjectManager----> getTimes "<<endl;
#endif

  set<miTime> timeset;

  int nn= pinfos.size();
  for (int i=0; i<nn; i++){
    vector<miTime> tv = getObjectTimes(pinfos[i]);
    for (int j=0; j<tv.size(); j++){
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

vector<miTime> ObjectManager::getObjectTimes(const miString& pinfo)
//  * PURPOSE:   return times for list of PlotInfo's
{

  vector<miTime> timevec;

  vector<miString> tokens= pinfo.split('"','"');
  int m= tokens.size();
  for (int j=0; j<m; j++){
    miString key,value;
    SetupParser::splitKeyValue(tokens[j],key,value);
    if (key=="name"){
      vector<ObjFileInfo> ofi= getObjectFiles(value,true);
      for (int k=0; k<ofi.size(); k++){
	timevec.push_back(ofi[k].time);
      }
      break;
    }
  }

  return timevec;
}

void ObjectManager::getCapabilitiesTime(vector<miTime>& normalTimes,
					miTime& constTime,
					int& timediff,
					const miString& pinfo)
{
  //Finding times from pinfo
  //If pinfo contains "file=", return constTime

  miString fileName;
  miString objectname;
  timediff=0;

  vector<miString> tokens= pinfo.split('"','"');
  int m= tokens.size();
  for (int j=0; j<m; j++){ 
    miString key,value;
    SetupParser::splitKeyValue(tokens[j],key,value);
    if (key=="name"){
      objectname = value;
    } else if( key=="timediff"){
      timediff = value.toInt();
    } else if( key=="file"){ //Product with no time
      fileName=value;
    }
  }

  if(fileName.exists() ){ //Product with const time
    if(objectFiles.count(objectname)){
      constTime = timeFilterFileName(fileName,objectFiles[objectname].filter);
    }
  } else { //Product with prog times
    vector<ObjFileInfo> ofi= getObjectFiles(objectname,true);
    int nfinfo=ofi.size();
    for (int k=0; k<nfinfo; k++){
      normalTimes.push_back(ofi[k].time);
    }
  }

}


PlotOptions ObjectManager::getPlotOptions(miString objectName){
  return objectFiles[objectName].poptions;
}


bool ObjectManager::insertObjectName(const miString & name,
				     const miString & file){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::insertObjectName " << name << " " << file << endl;
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

bool ObjectManager::prepareObjects(const miTime& t,
				   const Area& area,
				   DisplayObjects& wObjects){

  //are objects defined (in objects.define() if not return false)
    if (!wObjects.defined) return false;

  //if autoFile or wrong time, set correct time
  if (wObjects.autoFile || wObjects.getTime() ==ztime)
    wObjects.setTime(t);

  if (wObjects.autoFile || wObjects.filename.empty()){
    //not approved, we have to read new objects
    wObjects.approved=false;
    //clear objects !
    wObjects.clear();
    //get new filename
    if (!getFileName(wObjects)) return false;
  }

  if (!wObjects.approved){
    if (!wObjects.readEditDrawFile(wObjects.filename,area))
      return false;
  }


  return wObjects.prepareObjects();

}


vector<ObjFileInfo> ObjectManager::getObjectFiles(const miString objectname,
						  bool refresh){
  // called from ObjectDialog
#ifdef DEBUGPRINT
  cerr << "ObjectManager::getObjectFiles" << endl;
#endif

  if (refresh) {
    map<miString,ObjectList>::iterator p,pend= objectFiles.end();
    for (p=objectFiles.begin(); p!=pend; p++)
      p->second.updated= false;
  }

  vector<ObjFileInfo> files;

  map<miString,ObjectList>::iterator po= objectFiles.find(objectname);
  if (po==objectFiles.end()) return files;

  if (!po->second.updated || !po->second.files.size()) {
    po->second.files= listFiles(po->second);
    po->second.updated= true;
  }

  return po->second.files;
}


vector<ObjFileInfo> ObjectManager::listFiles(ObjectList & ol) {

  glob_t globBuf;
  miString fileString= ol.filename + "*";
#ifdef DEBUGPRINT
  cerr <<"ObjectManager::listFiles, search string " << fileString << endl;
#endif

  vector<ObjFileInfo> files;

  glob(fileString.c_str(),0,0,&globBuf);

  for (int i=0; i<globBuf.gl_pathc; i++) {
    ObjFileInfo info;
    miString name = globBuf.gl_pathv[i];
    miTime time = timeFilterFileName(name,ol.filter);
    if(!time.undef() && time!=ztime){
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

  globfree(&globBuf);

  return files;
}


miString ObjectManager::prefixFileName(miString fileName)
{
  //get prefix from a file with name  /.../../prefix_*.yyyymmddhh
    vector <miString> parts= fileName.split('/');
    miString prefix = parts.back();
    vector <miString> sparts= prefix.split('_');
    prefix=sparts[0];
    return prefix;
}

miTime ObjectManager::timeFilterFileName(miString fileName,TimeFilter filter)
{
  if (filter.ok()){
    miTime t;
    filter.getTime(fileName,t);
    return t;
  }
  else
    return timeFileName(fileName);
}


miTime ObjectManager::timeFileName(miString fileName)
{
  //get time from a file with name *.yyyymmddhh
  vector <miString> parts= fileName.split('.');
  if (parts.size() != 2) return ztime ;
  if (parts[1].length() < 10) return ztime;
  return timeFromString(parts[1]);
}

miTime ObjectManager::timeFromString(miString timeString)
{
  //get time from a string with yyyymmddhhmm
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



bool ObjectManager::getFileName(DisplayObjects& wObjects){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::getFileName" << endl;
#endif


  if (wObjects.objectname.empty() || wObjects.getTime()==ztime)
    return false;


  map<miString,ObjectList>::iterator po;
  po= objectFiles.find(wObjects.objectname);
  if (po==objectFiles.end()) return false;

  if (!po->second.updated || !po->second.files.size()) {
    po->second.files= listFiles(po->second);
    po->second.updated= true;
  }

  int n= po->second.files.size();

  int fileno=-1;
  int d, diff=wObjects.timeDiff+1;

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

/*----------------------------------------------------------------------
-----------  end of methods for finding and showing objectfiles ---------
 -----------------------------------------------------------------------*/
bool ObjectManager::editCommandReadCommentFile(const miString filename)
{
#ifdef DEBUGPRINT
  cerr << "ObjectManager::editCommandReadCommentfile" << endl;
#endif

  //read file with comments
  readEditCommentFile(filename,plotm->editobjects);

  return true;
}


bool ObjectManager::readEditCommentFile(const miString filename,
				     WeatherObjects& wObjects){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::readEditCommentFile" << endl;
#endif

  return wObjects.readEditCommentFile(filename);
}

void ObjectManager::putCommentStartLines(miString name,miString prefix){
  //return the startline of the comments file
#ifdef DEBUGPRINT
  cerr << "ObjectManager::putCommentStartLines" << endl;
#endif

  plotm->editobjects.putCommentStartLines(name,prefix);
}

miString ObjectManager::getComments(){
  //return the comments
  return plotm->editobjects.getComments();
}

void ObjectManager::putComments(const miString & comments){
  plotm->editobjects.putComments(comments);
}

miString ObjectManager::readComments(bool inEditSession){
  if (inEditSession)
    return plotm->editobjects.readComments();
  else
    return plotm->objects.readComments();

}

/*----------------------------------------------------------------------
----------- end of methods for reading and writing comments -------------
 -----------------------------------------------------------------------*/

bool ObjectManager::editCommandReadDrawFile(const miString filename)
{
#ifdef DEBUGPRINT
  cerr << "ObjectManager::editCommandReadDrawfile" << endl;
#endif

  //size of objects to start with
  int edSize = plotm->editobjects.getSize();

  readEditDrawFile(filename,plotm->getMapArea(),plotm->editobjects);

  editNewObjectsAdded(edSize);

  // set symbol default size to size of last read object !
  plotm->editobjects.changeDefaultSize();

  if (autoJoinOn()) plotm->editobjects.editJoinFronts(true, true,false);

  setAllPassive();

  return true;
}


bool ObjectManager::readEditDrawFile(const miString filename,
				     const Area& area,
				     WeatherObjects& wObjects){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::readEditDrawFile(2)" << endl;
#endif


  miString fileName = filename;


  //check if filename exists, if not look for other files
  //with same time.
  //HK ??? to solve temporary problem with file names with/without mins
  if (!checkFileName(fileName)){
    cerr << "FILE " << fileName << " does not exist !" << endl;
    return false;
  }


  return wObjects.readEditDrawFile(fileName,area);
}

miString ObjectManager::writeEditDrawString(const miTime& t,
					WeatherObjects& wObjects){
  return wObjects.writeEditDrawString(t);
}



bool ObjectManager::writeEditDrawFile(const miString filename,
				      const miString outputString){

#ifdef DEBUGPRINT
  cerr << "ObjectManager::writeEditDrawFile" << endl;
#endif

  if (outputString.empty()) return false;

  // open filestream
  ofstream file(filename.cStr());
  if (!file){
    cerr << "ERROR OPEN (WRITE) " << filename << endl;
    return false;
  }

  file << outputString;

  file.close();

#ifdef DEBUGPRINT
  cerr << "File " << filename   << " closed " << endl;
#endif

  return true;
}


/************************************************
 *  Methods for reading and writing text  *
 *************************************************/

void ObjectManager::setCurrentText(const miString & newText){
  WeatherSymbol::setCurrentText(newText);
}

void ObjectManager::setCurrentColour(const Colour::ColourInfo & newColour){
  WeatherSymbol::setCurrentColour(newColour);
}


miString ObjectManager::getCurrentText(){
  return WeatherSymbol::getCurrentText();
}

Colour::ColourInfo ObjectManager::getCurrentColour(){
  return WeatherSymbol::getCurrentColour();
}


miString ObjectManager::getMarkedText(){
  return plotm->editobjects.getMarkedText();
}

Colour::ColourInfo ObjectManager::getMarkedColour(){
  return plotm->editobjects.getMarkedColour();
}


void ObjectManager::changeMarkedText(const miString & newText){
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    plotm->editobjects.changeMarkedText(newText);
    editPostOperation();
  }
}


void ObjectManager::changeMarkedColour(const Colour::ColourInfo & newColour){
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    plotm->editobjects.changeMarkedColour(newColour);
    editPostOperation();
  }
}



set <miString> ObjectManager::getTextList(){
  return WeatherSymbol::getTextList();
}


void ObjectManager::getMarkedComplexText(vector <miString> & symbolText, vector <miString> & xText){
  plotm->editobjects.getMarkedComplexText(symbolText,xText);
}

void ObjectManager::changeMarkedComplexText(const vector <miString> & symbolText, const vector <miString> & xText){
  if (mapmode==draw_mode){
    editPrepareChange(ChangeText);
    plotm->editobjects.changeMarkedComplexText(symbolText,xText);
    editPostOperation();
  }
}

bool ObjectManager::inTextMode(){
  return plotm->editobjects.inTextMode();
}


bool ObjectManager::inComplexTextMode(){
  return plotm->editobjects.inComplexTextMode();
}


void ObjectManager::getCurrentComplexText(vector <miString> & symbolText,
					  vector <miString> & xText){
  WeatherSymbol::getCurrentComplexText(symbolText,xText);
}

void ObjectManager::setCurrentComplexText(const vector <miString> &
symbolText, const vector <miString> & xText){
  WeatherSymbol::setCurrentComplexText(symbolText,xText);
}


void ObjectManager::initCurrentComplexText(){
  plotm->editobjects.initCurrentComplexText();
}



set <miString> ObjectManager::getComplexList(){
  return WeatherSymbol::getComplexList();
}







// ---------------------------------------------------------------
// ---------------------------------------------------------------
// ---------------------------------------------------------------




void ObjectManager::autoJoinToggled(bool on){
#ifdef DEBUGPRINT
  cerr << "autoJoinToggled is called" << endl;
#endif
  plotm->editobjects.setAutoJoinOn(on);
  if (autoJoinOn()) plotm->editobjects.editJoinFronts(true,true,false);
}


bool ObjectManager::autoJoinOn(){
  return plotm->editobjects.isAutoJoinOn();
}

bool ObjectManager::inDrawing(){
  if (mapmode==draw_mode)
    return plotm->editobjects.inDrawing;
  else if (mapmode==combine_mode)
    return plotm->combiningobjects.inDrawing;

}

void ObjectManager::createNewObject()
{
  // create the object at first position,
  // avoiding a lot of empty objects, and the possiblity
  // to make the dialog work properly
  // (type as editmode and edittool)
#ifdef DEBUGPRINT
  cerr << "ObjectManager::createNewObject" << endl;
#endif
  editStopDrawing();
  if (mapmode==draw_mode)
    plotm->editobjects.createNewObject();
  else if (mapmode==combine_mode)
    plotm->combiningobjects.createNewObject();
}



void ObjectManager::editTestFront(){
  plotm->editobjects.editTestFront();
}



void ObjectManager::editSplitFront(const float x, const float y){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::editSplitFront" << endl;
#endif
  if (mapmode==draw_mode){
    editPrepareChange(SplitFronts);
    if (plotm->editobjects.editSplitFront(x,y)){
      if (autoJoinOn()) plotm->editobjects.editJoinFronts(false,true,false);
    } else
      objectsChanged= false;
    setAllPassive();
    editPostOperation();
  }

}


void ObjectManager::editResumeDrawing(const float x, const float y) {
#ifdef DEBUGPRINT
  cerr << "ObjectManager::editResumeDrawing" << endl;
#endif
  if (mapmode==draw_mode){

    editPrepareChange(ResumeDrawing);
    objectsChanged = plotm->editobjects.editResumeDrawing(x,y);
    editPostOperation();
  } else if (mapmode == combine_mode)
    doCombine=plotm->combiningobjects.editResumeDrawing(x,y);
}



bool ObjectManager::editCheckPosition(const float x, const float y){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::editCheckPosition" << endl;
#endif
  bool changed=false;

  //returns true if marked nodepoints changed

  if (mapmode == draw_mode){
    changed = plotm->editobjects.editCheckPosition(x,y);
  }else if (mapmode == combine_mode){
    changed=plotm->combiningobjects.editCheckPosition(x,y);
  }else {
    plotm->editobjects.unmarkAllPoints();
    setRubber(false,0,0);
    plotm->combiningobjects.unmarkAllPoints();
    changed=true;
  }

  return changed;
}


void ObjectManager::setAllPassive(){
  //Sets current status of the objects to passive
#ifdef DEBUGPRINT
  cerr <<"ObjectManager::setAllPassive()"<< endl;
#endif
  plotm->editobjects.setAllPassive();
  plotm->combiningobjects.setAllPassive();
  setRubber(false,0,0);
}

bool ObjectManager::setRubber(bool rubber, const float x, const float y){
#ifdef DEBUGPRINT
  cerr <<"ObjectManager::setRubber called" << endl;
#endif
  return plotm->editobjects.setRubber(rubber,x,y);
}


// -------------------------------------------------------
// --------- -------------- editcommands -----------------
// -------------------------------------------------------
void ObjectManager::editPrepareChange(const operation op)
{
#ifdef DEBUGPRINT
  cerr << "objm::editPrepareChange" << endl;
#endif

  //temporary undo buffer, in case changes occur
  undoTemp = new UndoFront( );
  if (mapmode!=combine_mode){
    objectsChanged = plotm->editobjects.saveCurrentFronts(op, undoTemp);
  }else
    objectsChanged=false;
}

void ObjectManager::editMouseRelease(bool moved)
{
#ifdef DEBUGPRINT
  cerr << "objm::editMouserRelease" << endl;
#endif
  if (moved)objectsChanged = true;
  editPostOperation();
  if (moved && autoJoinOn()) plotm->editobjects.editJoinFronts(false,true,false);

}


void ObjectManager::editPostOperation(){
#ifdef DEBUGPRINT
  cerr << "!! editPostOperation" << endl;
#endif
  if (objectsChanged && undoTemp!=0){
    plotm->editobjects.newUndoCurrent(undoTemp);
    objectsSaved = false;
  } else if (undoTemp!=0){
    delete undoTemp;
  }
  undoTemp = 0;
}

void ObjectManager::editNewObjectsAdded(int edSize){
#ifdef DEBUGPRINT
  cerr << "editNewObjectsAdded" << endl;
#endif

  //for undo buffer
  int diffedSize=plotm->editobjects.getSize()-edSize;
  undoTemp = new UndoFront();
  if (plotm->editobjects.saveCurrentFronts(diffedSize,undoTemp))
    plotm->editobjects.newUndoCurrent(undoTemp);
  else{
    delete undoTemp;
    undoTemp = NULL;
  }
}

void ObjectManager::editCommandJoinFronts(bool joinAll,bool movePoints,bool joinOnLine){
  if(mapmode== draw_mode){
    editPrepareChange(JoinFronts);
    objectsChanged=plotm->editobjects.editJoinFronts(joinAll,movePoints,joinOnLine);
    editPostOperation();
  }
}

void ObjectManager::setEditMode(const mapMode mmode,
				const int emode,
				const miString etool){
  mapmode= mmode;

  plotm->editobjects.setEditMode(mmode,emode,etool);
  plotm->combiningobjects.setEditMode(mmode,emode,etool);

}



// -------------------------------------------------------
// -------------------------------------------------------
//---------------------------------------------------------

void ObjectManager::editStopDrawing(){
  if (mapmode==draw_mode){
        setRubber(false,0,0);
    if (autoJoinOn()) plotm->editobjects.editJoinFronts(false,true,false);
  }
  setAllPassive();
}


void ObjectManager::editDeleteMarkedPoints(){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::editDeleteMarkedPoints" << endl;
#endif

  if (mapmode==draw_mode){

    editPrepareChange(DeleteMarkedPoints);
    plotm->editobjects.editDeleteMarkedPoints();
    editPostOperation();
    //remove fronts with only one point...
    cleanUp();
    if (autoJoinOn()) plotm->editobjects.editJoinFronts(false,true,false);

  } else if (mapmode==combine_mode)
    doCombine = plotm->combiningobjects.editDeleteMarkedPoints();

}


void ObjectManager::editAddPoint(const float x, const float y){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::editAddPoint" << endl;
#endif

  if(mapmode== draw_mode){
    editPrepareChange(AddPoint);
    plotm->editobjects.editAddPoint(x,y);
    editPostOperation();
  } else if (mapmode==combine_mode)
    doCombine=plotm->combiningobjects.editAddPoint(x,y);
}


void ObjectManager::editStayMarked(){
  plotm->editobjects.editStayMarked();

}

void ObjectManager::editNotMarked(){
  if(mapmode== draw_mode){
    plotm->editobjects.editNotMarked();
  } else if(mapmode== combine_mode){
    plotm->combiningobjects.editNotMarked();
  }
  setAllPassive();
}


void
ObjectManager::editMergeFronts(bool mergeAll){
  //input parameters
  //mergeAll = true ->all fronts are joined
  //        = false->only marked or active fronts are joined
#ifdef DEBUGPRINT
  cerr << "+++EditManager::editMergeFronts" << endl;
#endif
  if(mapmode== draw_mode){
    editPrepareChange(JoinFronts);
    objectsChanged =plotm->editobjects.editMergeFronts(mergeAll);
    editPostOperation();
  }
}


void ObjectManager::editUnJoinPoints(){

  if (mapmode==draw_mode){
    plotm->editobjects.editUnJoinPoints();
    checkJoinPoints();
  }
}


bool ObjectManager::editMoveMarkedPoints(const float x, const float y)
{
  bool changed = false;
  if (mapmode==draw_mode){
    changed = plotm->editobjects.editMoveMarkedPoints(x,y);
    if (changed){
      checkJoinPoints();
      //Join points but don't move
      if (autoJoinOn()) plotm->editobjects.editJoinFronts(false,false,false);
    }
  } else if (mapmode == combine_mode){
    changed = plotm->combiningobjects.editMoveMarkedPoints(x,y);
    if (changed) doCombine = true;
  }
  return changed;
}

bool ObjectManager::editRotateLine(const float x, const float y){
  bool changed = false;
  if (mapmode==draw_mode){
    changed = plotm->editobjects.editRotateLine(x,y);
    if (changed){
      checkJoinPoints();
      //Join points but don't move
      if (autoJoinOn()) plotm->editobjects.editJoinFronts(false,false,false);
    }
  } else if (mapmode == combine_mode){
    changed = plotm->combiningobjects.editRotateLine(x,y);
    if (changed) doCombine = true;
  }
  return changed;
}


void ObjectManager::editCopyObjects(){
  if (mapmode==draw_mode){
    plotm->editobjects.editCopyObjects();
  }
}


void ObjectManager::editPasteObjects(){
  if (mapmode==draw_mode){
    editPrepareChange(PasteObjects);
    plotm->editobjects.editPasteObjects();
    editPostOperation();
  }
}


void ObjectManager::editFlipObjects(){
  if (mapmode==draw_mode){
    editPrepareChange(FlipObjects);
    plotm->editobjects.editFlipObjects();
    editPostOperation();
  }
}


void ObjectManager::editUnmarkAllPoints(){
  plotm->editobjects.unmarkAllPoints();
}

void ObjectManager::editIncreaseSize(float val){
  if (mapmode==draw_mode){
    editPrepareChange(IncreaseSize);
    plotm->editobjects.editIncreaseSize(val);
    editPostOperation();
  } else if (mapmode==combine_mode){
    bool changed = plotm->combiningobjects.editIncreaseSize(val);
    if (changed) doCombine = true;
  }
}

void ObjectManager::editDefaultSize(){
  if (mapmode==draw_mode){
    editPrepareChange(DefaultSize);
    plotm->editobjects.editDefaultSize();
    editPostOperation();
  }  else if (mapmode==combine_mode){
    plotm->combiningobjects.editDefaultSize();
  }
}


void ObjectManager::editDefaultSizeAll(){
  if (mapmode==draw_mode){
    editPrepareChange(DefaultSizeAll);
    plotm->editobjects.editDefaultSizeAll();
    editPostOperation();
  } else if (mapmode == combine_mode){
    plotm->combiningobjects.editDefaultSizeAll();
  }
}



void ObjectManager::editRotateObjects(float val){
  if (mapmode==draw_mode){
    editPrepareChange(RotateObjects);
    plotm->editobjects.editRotateObjects(val);
    editPostOperation();
  }
}

void ObjectManager::editHideBox(){
  if (mapmode==draw_mode){
    editPrepareChange(HideBox);
    plotm->editobjects.editHideBox();
    editPostOperation();
  }
}

void ObjectManager::editHideAll(){
    plotm->editobjects.editHideAll();
}


void ObjectManager::editHideCombineObjects(int ir){
  plotm->editobjects.editHideCombineObjects(ir);
}

void ObjectManager::editUnHideAll(){
  plotm->editobjects.editUnHideAll();
}


void ObjectManager::editHideCombining(){
  if (mapmode == combine_mode) return;
  plotm->combiningobjects.editHideAll();
}

void ObjectManager::editUnHideCombining(){
  if (mapmode == combine_mode) return;
  plotm->combiningobjects.editUnHideAll();
}


void ObjectManager::editChangeObjectType(int val){
  if (mapmode == draw_mode){
    editPrepareChange(ChangeObjectType);
    plotm->editobjects.editChangeObjectType(val);
    editPostOperation();
  } else if (mapmode == combine_mode)
    doCombine=plotm->combiningobjects.editChangeObjectType(val);
}

void ObjectManager::cleanUp(){
  editPrepareChange(CleanUp);
  plotm->editobjects.cleanUp();
  editPostOperation();
}


void ObjectManager::checkJoinPoints(){
  plotm->editobjects.checkJoinPoints();
}


bool ObjectManager::redofront(){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::redofront" << endl;
#endif
  return plotm->editobjects.redofront();
}


bool ObjectManager::undofront(){
#ifdef DEBUGPRINT
  cerr << "ObjectManager::undofront" << endl;
#endif
  return plotm->editobjects.undofront();
}

void ObjectManager::undofrontClear(){
  plotm->editobjects.undofrontClear();
}


map <miString,bool> ObjectManager::decodeTypeString(miString token){
  return WeatherObjects::decodeTypeString(token);
}


miString ObjectManager::stringFromTime(const miTime& t,bool addMinutes){
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

  miString timestring = ostr.str();
  return timestring;
}



bool ObjectManager::_isafile(const miString name){
  FILE *fp;
  if (fp=fopen(name.cStr(),"r")){
    fclose(fp);
    return true;
  } else return false;
}


bool ObjectManager::checkFileName(miString &fileName){
  if(!_isafile(fileName)) {
    //find time of file
    miTime time = timeFileName(fileName);
    //old filename style
    fileName = "ANAdraw."+stringFromTime(time,false);
    if(!_isafile(fileName)) return false;
  }
  return true;
}









