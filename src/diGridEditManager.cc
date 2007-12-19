#include <diGridEditManager.h>
#include <iostream>
#include <fstream>
#include <AbstractablePolygon.h>
#include <diGridArea.h>
#include <diGridAreaManager.h>
#include <diPlotModule.h>
#include <diObjectManager.h>


GridEditManager::GridEditManager(PlotModule* pm, FieldManager* fm)
  : plotm(pm), fieldm(fm), mapmode(normal_mode), 
    firsttime(true),autoexecute(true) 
{
#ifndef NOLOG4CXX
  logger = log4cxx::Logger::getLogger("diana.GridEditManager");
#endif 
 
  if (plotm==0 || fieldm==0){
    LOG4CXX_ERROR(logger, "Catastrophic error: plotm or fieldm == 0");
  }

  executor.setFieldManager(fieldm);
}

GridEditManager::~GridEditManager(){
}

/**
 * Has to be called from PlotModule::plot()
 */


bool GridEditManager::plot(bool under){
  bool b = true;
  if ( under ){
    int n = toPlot.size();
    for(int i=0; i<n; i++){
      editMap[toPlot[i].parameter][toPlot[i].time].fp->plot();
    }

  } else {

    b = areaManager.plot();
  }

  return b;
}



void GridEditManager::sendMouseEvent(const mouseEvent& me, EventResult& res){
	res.savebackground= true;
	res.background= false;
	res.repaint= false;
	res.newcursor = edit_cursor;
	float newx,newy;
	plotm->PhysToMap(me.x,me.y,newx,newy);
	areaManager.sendMouseEvent(me,res,newx,newy);
}


void GridEditManager::sendKeyboardEvent(const keyboardEvent& me, EventResult& res){
	areaManager.sendKeyboardEvent(me,res);
}

GridAreaManager *GridEditManager::getGridAreaManager(){
	return &areaManager;
}


void GridEditManager::outputExecuteResponce( vector<fetCodeExecutor::responce> & rl)
{
  ostringstream ost;
  for ( int i=0; i<rl.size(); i++ ){
    if ( rl[i].level != fetCodeExecutor::INFO )
      ost << ( rl[i].level == fetCodeExecutor::ERROR  
	       ? "ERROR:" : "WARNING:") 
	  << rl[i].message << ", line " << rl[i].linenum
	  << " in finished object-code" << endl;
  }
  LOG4CXX_ERROR(logger, ost.str() );
}


/*
  ======================================
  Convenience function for loading fetBaseObjects
  from file
  ======================================
*/
bool loadandpushbaseobjects( const miString& fn, vector<fetBaseObject>& vbo )
{
  ifstream file(fn.c_str());
  if ( !file ){
    return false;
  }
  
  ostringstream ost;
  char c;
  while ( file.get(c) ) ost.put(c);

  miString objectstring = ost.str();

  fetBaseObject foo(objectstring);
  if ( !foo.name().exists() )
    return false;

  vbo.push_back(foo);
  return true;
}


/*
  ==============================================
  initSession called at PROFET startup
  ==============================================
*/

bool GridEditManager::initSession(vector<miTime>&         timeLabel,
				  vector<miString>&       parameterLabel,
				  vector< vector<bool> >& mark,
				  miString& sessionHeader)
{
  if(firsttime) {
    
//     profetParameter.push_back(fetParameter("MSLP",1));
//     profetParameter.push_back(fetParameter("T.2M",1));
//     profetParameter.push_back(fetParameter("VIND.10M",1));
//     profetParameter.push_back(fetParameter("TOTALT.SKYDEKKE",1));
//     profetParameter.push_back(fetParameter("NEDBØR.3T",1));
    
//     profetSession = fetSession(12,"3:6:9:12:15:18:21:24:27:30:33:36:39:42:45:48");

//     profetModel = fetModel("2007-06-15 12:00:00","HIRLAM.4KM.00");

//     loadandpushbaseobjects("objects/Modifiser_nedbør.poc", baseObject);
//     loadandpushbaseobjects("objects/TT-updown.poc", baseObject);
    

    puSQLwarning w;
    // first version hack... run "startdb"
    //if(!gate.open("profet","Localhost","audunc","",w,3306)){
    if(!gate.open("profet","describe ","juergens","",w,19100)){
      w.warnout();
      return false;
    } else {
      
      vector<fetSession>   fets;
      vector<fetModel>     fetm;
      
      int hh=miTime::nowTime().hour();
	
      gate.select(profetParameter,w);
      
      LOG4CXX_INFO(logger,"got : " << profetParameter.size() << " parameters");
      
      miString query="where session >="+miString(hh)+" order by session";
      
      if(gate.select(fets,w,query)){
	if(!fets.empty()) {
	  profetSession=fets[0];
	  
	  query="where session=\'"+profetSession.session().isoTime()
	    +"\'";
	  
	  if(gate.select(fetm,w,query))
	    if(!fetm.empty()) 
	      profetModel=fetm[0];
	  
	}
      }

      gate.select(baseObject,w);
    }
    w.warnout();
  }
  
  
  vtime=profetSession.progs();
  
  ostringstream ost;
  ost << "<b> Session: </b> " << profetSession.session() << "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b> Model: </b> " << profetModel.model();
  sessionHeader=ost.str();
  
  //test
  currentTime=vtime[1];
  
  int nParameter = profetParameter.size();
  int nTime = vtime.size();

  //  vector<fetBaseObject> baseObjects; //= ask dB
  
  for( int i=0; i<nParameter; i++){
    
    miString pin = "FIELD " + profetModel.model()+ " " + profetParameter[i].name();
    vector<bool> timeMark;
    for( int j=0; j<nTime; j++){
      EditField editField;
      editField.fp = new FieldPlot;
      fieldm->makeFields(pin,vtime[j],editField.origField);
      timeMark.push_back( editField.object.size() );
      editMap[profetParameter[i].name()][vtime[j]]=editField;
      
    }
    mark.push_back(timeMark);
  }
  
  
  
  timeLabel = vtime;
  
  parameterLabel.clear();

  for(int i=0;i<profetParameter.size();i++)
    parameterLabel.push_back(profetParameter[i].name());

  //set Area (projection)
  if( profetParameter.size() && vtime.size() &&
      editMap.count(profetParameter[0].name()) &&
      editMap[profetParameter[0].name()].count(vtime[0]) && 
      editMap[profetParameter[0].name()][vtime[0]].origField.size())
    area = editMap[profetParameter[0].name()][vtime[0]].origField[0]->area;

  firsttime=false;

  return true;
  
}

void GridEditManager::refresh()
{
  cout << "refresh baseobjects..." << endl;
  puSQLwarning w;
  gate.select(baseObject,w);
  w.warnout();
}



/*
  ==============================================
  setCurrent called to change current editfield
  IN: field parameter-name and time
  ==============================================
*/
void GridEditManager::setCurrent(miString p, miTime t)
{

  currentParameter = p;
  currentTime = t;
  if( editMap[p][t].object.size() ){
    rawObject =  editMap[p][t].object.begin()->second;
  } else {
    rawObject = fetObject();
  }
  
  currentObject = rawObject;
  
}


/*
  =====================================================
  Prepare called from PlotModule in responce to "Apply"
  =====================================================
*/
bool GridEditManager::prepare(const miString& pin)
{
  LOG4CXX_INFO(logger,"GridEditManager::prepare: " << pin);
  vector<miString> tokens= pin.split('"','"');
  int n= tokens.size();

  // at least FIELD <modelName> <fieldName>
  if (n<3) return false;

  miString modelName= tokens[1];
  miString fieldName= tokens[2];
  if( !("Profet" == modelName && editMap.count(fieldName)) ) 
    return false; //no edit field

  parameter_time pt;
  pt.parameter=fieldName;
  pt.pin=pin;
  toPlot.push_back(pt);
  //  vFieldname.push_back(fieldName); //should keep pin
  //update all fieldPlots with this parameter
  map<miTime,EditField>::iterator ptime;

  for( ptime=editMap[fieldName].begin(); ptime!=editMap[fieldName].end(); ptime++)
    ptime->second.fp->prepare(pin);

}



/*
  =================================================
  Called from PlotModule when time changes
  =================================================
*/
bool GridEditManager::setData(const miTime& time)
{

  LOG4CXX_INFO(logger,"GridEditManager::setData for time: "<<time.isoTime()
	       <<" toPlot.size()= "<<toPlot.size());

  int n = toPlot.size();
  if(n==0) return false;

  polygonMap.clear();

  for(int i=0; i<n; i++){
    toPlot[i].time = time;
    //  
    //    if(!editMap[toPlot[i].parameter][time].fp->fieldsOK()){
    //If field is not read
      if(!editMap[toPlot[i].parameter][time].origField.size()){
	//	cerr <<"makeFields:"<<toPlot[i].pin<<endl;
        fieldm->makeFields(toPlot[i].pin,time,
			   editMap[toPlot[i].parameter][time].origField);
      }

      //make copy of orig fields
      int nn= editMap[toPlot[i].parameter][time].origField.size();
      vector<Field*> tmp;
      for(int k=0;k<nn;k++){
	Field* ttmp = new Field(*editMap[toPlot[i].parameter][time].origField[k]);
	tmp.push_back(ttmp);
      }
      
      LOG4CXX_INFO(logger,"setData running all objects" );
      //execute object on origField
      vector<fetCodeExecutor::responce> resplist;
      map<miString,fetObject>::iterator itr 
	= editMap[toPlot[i].parameter][time].object.begin();
      map<miString,fetObject>::iterator end 
	= editMap[toPlot[i].parameter][time].object.end();
      for ( ; itr!=end; itr++ ){ 
	LOG4CXX_INFO(logger,"Running object " << itr->second.name() << "/"
		     << itr->second.id() );
	bool b = executor.execute( itr->second,tmp, resplist);
	if ( !b ){
	  outputExecuteResponce(resplist);
	  resplist.clear();
	}
	Polygon pp = itr->second.polygon();
	polygonMap[itr->second.id()] = pp;
	LOG4CXX_INFO(logger,"Adding polygon to map:" << itr->second.id());
      }	
      editMap[toPlot[i].parameter][time].fp->setData(tmp,time);
    }
  //  }

  cerr <<"  LOG4CXX_INFO(logger,"<<"set current POLYGON:"<<"currentObject.id());"<<endl;
  areaManager.setGridAreas(polygonMap, area);
  areaManager.setCurrentArea(currentObject.id());
}



/*
  =================================================
  Info to FieldDialog 
  =================================================
*/
bool GridEditManager::getFieldGroups(const miString& modelNameRequest, 
				     miString& modelName, 
				     vector<FieldGroupInfo>& vfgi)
{
  vector<miString> vparameter;
  for(int i=0;i<profetParameter.size();i++)
    vparameter.push_back(profetParameter[i].name());
  
  if(modelNameRequest!="Profet") return false;
  vfgi.clear();
  FieldGroupInfo fgi;
  modelName=modelNameRequest;
  fgi.modelName=modelNameRequest;
  fgi.groupName="HAHAHA";
  fgi.fieldNames = vparameter;
   vfgi.push_back(fgi);
  return true;
}




/*
  =================================================
  =================================================
*/
bool GridEditManager::isEditField(const vector<FieldTimeRequest>& request)
{
  //  cerr <<"isEditField"<<request.size()<<endl;
  if (request.size()>0 && request[0].modelName == "Profet") return true;
  return false;
}


/// return available times for the requested models and fields
vector<miTime> GridEditManager::getFieldTime(const vector<FieldTimeRequest>& request,
			      bool allTimeSteps)
{
  // cerr <<"GridEditManager::getFieldTime"<<endl;
  return vtime;
}

/*
  =================================================
  Called from PlotModule 
  =================================================
*/

bool GridEditManager::getPlotElements(vector<PlotElement>& pel)
{
  int m= toPlot.size();
  miString str;
  for (int i=0; i<m; i++){
    editMap[toPlot[i].parameter][currentTime].fp->getPlotName(str);
    str+= "# " + miString(i);
    bool enabled= editMap[toPlot[i].parameter][currentTime].fp->Enabled();
    // add plotelement
    pel.push_back(PlotElement("EDITFIELD",str,"FIELD",enabled));
  }
}

bool GridEditManager::enablePlot(const PlotElement& pe)
{
  int m= toPlot.size();
  miString str;
  for (int i=0; i<m; i++){
    editMap[toPlot[i].parameter][currentTime].fp->getPlotName(str);
    str+= "# " + miString(i);
    if (str==pe.str){
      map<miString,fetObject>::iterator itr 
	= editMap[toPlot[i].parameter][currentTime].object.begin();
      map<miString,fetObject>::iterator end 
	= editMap[toPlot[i].parameter][currentTime].object.end();
      for ( ; itr!=end; itr++ ){ 
	areaManager.setEnabled
	  (itr->second.id(),pe.enabled);
      }
	editMap[toPlot[i].parameter][currentTime].fp->enable(pe.enabled);
      break;
    }
  }
}

void GridEditManager::setCurrentObject(miString id)
{
  LOG4CXX_INFO(logger,"setCurrentObject for id:" << id);
  
  if ( !editMap[currentParameter][currentTime].object.count(id) ){
    LOG4CXX_ERROR(logger,"setCurrentObject, id unknown:" << id );
    return;
  }

  setObject(editMap[currentParameter][currentTime].object[id]);

  if(!areaManager.setCurrentArea(id)) {
    LOG4CXX_ERROR(logger,"setCurrentObject - area "<<id<<" couldn't be set");
  }

}

//
void GridEditManager::setObject(const fetObject& fobj)
{
  LOG4CXX_INFO(logger,"setObject(fetObject) for id="<< fobj.id());

  fetBaseObject  baseobject = fobj.baseObject();
  vector<fetDynamicGui::GuiComponent> components;
  map<miString,miString> guikeys = fobj.guiElements();
  bool onlygui = false;

//   map<miString,miString>::iterator itr=guikeys.begin();
//   for(;itr!=guikeys.end(); itr++)
//     cerr << itr->first << " = " << itr->second << endl;
  
  // compile and fetch gui components from code
  bool ok = executor.compile( baseobject, responcel, components, guikeys, onlygui );
  if ( !ok ){
    outputExecuteResponce(responcel);
    responcel.clear();
  }

  rawObject = fobj;
  rawObject.setAlgorithm(executor.cleanCode());
  prepareObject(components);
}

vector<fetDynamicGui::GuiComponent>
GridEditManager::setObject(const fetBaseObject& fobj,
			   bool onlygui)

// Called when areaManager reports changes in active Polygon AND a new object has 
// been requested
{
  LOG4CXX_INFO(logger,"setObject(fetBaseObject) onlygui=" <<(onlygui?"TRUE":"FALSE"));

  vector<fetDynamicGui::GuiComponent> components;
  map<miString,miString> guikeys;

  // compile and fetch gui components from code
  bool ok = executor.compile( fobj, responcel, components, guikeys, onlygui );

  if (onlygui) return components;
  
  if(areaManager.isAreaSelected() ){
    int nx =  editMap[currentParameter][currentTime].origField[0]->nx;
    int ny =  editMap[currentParameter][currentTime].origField[0]->ny;

    //Set area
    ProjectablePolygon polygon = areaManager.getCurrentPolygon();

    Area area;    // !!
    proMask mask; // !!
    fetObject ob;
    ob.setFromBaseObject(fobj,
			 "Derfor",polygon,mask,nx,ny,area,executor.cleanCode(),"me",
			 miTime::nowTime(),miTime::nowTime(),"PARAMETER",
			 guikeys,"");
    
    rawObject = ob;
    prepareObject(components);

    areaManager.changeAreaId("newPolygon",rawObject.id());
    areaManager.setCurrentArea(rawObject.id());
    polygonMap[rawObject.id()]= polygon;
  
  }

  return components;
}

/*
  =================================================
  prepare fetObject code with Gui components
  =================================================
 */
void GridEditManager::prepareObject( const vector<fetDynamicGui::GuiComponent>& components )
{
  LOG4CXX_INFO(logger,"prepareObject with components");

  guicomponents = components;
  currentObject = rawObject;
  bool ok = executor.prepareCode(currentObject,components);

  if(currentObject.id().exists()){
    editMap[currentParameter][currentTime].object[currentObject.id()] 
      = currentObject;
  }
}


/*
  =================================================
  Executes fetObject "currentObject"
  Prerequisites:
  - object prepared with GuiComponents 
  =================================================
*/
bool GridEditManager::execute(bool apply)
{
  if(!autoexecute && !apply) return false;
  LOG4CXX_INFO(logger,"execute");

  bool ok;
  
  int nm= editMap[currentParameter][currentTime].origField.size();
  vector<Field*> tmpfield;
  for(int k=0;k<nm;k++){
    Field* ttmp = new Field(*editMap[currentParameter][currentTime].origField[k]);
    tmpfield.push_back(ttmp);
  }

  //run old objects
  LOG4CXX_INFO(logger,"RUN OLD OBJECTS:"
	       << editMap[currentParameter][currentTime].object.size()-1);
  map<miString,fetObject>::iterator itr 
    = editMap[currentParameter][currentTime].object.begin();
  map<miString,fetObject>::iterator end 
    = editMap[currentParameter][currentTime].object.end();
  for ( ; itr!=end; itr++ ){ 
    if(itr->first == currentObject.id() ) continue; //do not run current yet
    vector<fetCodeExecutor::responce> resplist;
    bool b = executor.execute( itr->second,tmpfield, resplist);
    if ( !b ){
      outputExecuteResponce(resplist);
      resplist.clear();
      continue;
    }
//     Polygon pp = itr->second.polygon();
//     polygonMap[itr->second.id()] = pp;
  }	

  if(currentObject.exists()){

    LOG4CXX_INFO(logger,"RUN ACTIVE OBJECT:" << currentObject.name()
		 << "/" << currentObject.id());
    
    
    ok = executor.execute( currentObject, tmpfield, responcel ); 
    
    if ( !ok ){
      outputExecuteResponce(responcel);
      responcel.clear();
      return false;
    }
  }
  
  editMap[currentParameter][currentTime].fp->setData(tmpfield,currentTime);
  
  return ok;
}

vector<fetBaseObject> GridEditManager::getBaseObjects()
{

  vector<fetBaseObject> fbo;
  int n = baseObject.size();
  for(int i=0; i<n; i++ ){
    if(baseObject[i].validinput().contains(currentParameter))
      fbo.push_back(baseObject[i]);
  }

  return fbo;

}

map<miString,fetObject> GridEditManager::getObjects()
{
  LOG4CXX_INFO(logger,"getObjects():"<<currentParameter
	       <<"  :  "<<currentTime.isoTime()<<"  :  "
	       <<editMap[currentParameter][currentTime].object.size());
  
  return editMap[currentParameter][currentTime].object;
}


bool GridEditManager::changeArea()
{
  miString id = areaManager.getCurrentId();
  LOG4CXX_INFO(logger,"GridEditManager::changeArea id=: " << id
	       << " rawObject has id:"<< rawObject.id());

  if(id == "newPolygon") return false;

  if(rawObject.id() == id){
    // Changes in Polygon for current Object
    LOG4CXX_INFO(logger,"GridEditManager::changeArea SAME ID ");
    rawObject.setPolygon(areaManager.getCurrentPolygon());
    // Check that these guicomponents are correct!!!!
    prepareObject(guicomponents);
    execute(); // AC
    return false;

  } else {
    LOG4CXX_INFO(logger,"GridEditManager::changeArea DIFFERENT ID ");
    //find and set parameter, time, object
    int m= toPlot.size();
    for (int i=0; i<m; i++){
      map<miString,fetObject>::iterator itr 
	= editMap[toPlot[i].parameter][currentTime].object.begin();
      map<miString,fetObject>::iterator end 
	= editMap[toPlot[i].parameter][currentTime].object.end();
      for ( ; itr!=end; itr++ ){
	if(itr->first == id){
	  currentParameter = toPlot[i].parameter;
	  return true;
	}
      }
    }
  }
  return true;
}


void GridEditManager::addArea(bool newArea)
{

  LOG4CXX_INFO(logger,"addArea, newArea=" << (newArea?"TRUE":"FALSE"));

  if( newArea) { // create a new area
    if(!areaManager.addArea("newPolygon")) {
      LOG4CXX_WARN(logger,"AreaManager::addArea returned false");
    }
    return; // AC!!!!!!!!!
  } 
  
  // create a copy of current selected area
  ProjectablePolygon pp = areaManager.getCurrentPolygon();
  areaManager.addArea("newPolygon",pp,true);
  

}

void GridEditManager::deleteObject()
{
  map<miString,fetObject>::iterator p
    =editMap[currentParameter][currentTime].object.find(currentObject.id());
  if(p!=editMap[currentParameter][currentTime].object.end()){
    areaManager.removeArea(p->second.id());
    editMap[currentParameter][currentTime].object.erase(p);
  }
  
  //set current
  if(editMap[currentParameter][currentTime].object.size()){
    rawObject = editMap[currentParameter][currentTime].object.begin()->second;
    areaManager.setCurrentArea(rawObject.id());
  } else {
    rawObject = fetObject();
  }
  currentObject = rawObject;

  execute();

}

void GridEditManager::quit()
{

  toPlot.clear();
  baseObject.clear();
  vtime.clear();
  profetParameter.clear();
  areaManager.clear();

  map<miString, map<miTime,EditField> >::iterator p   =editMap.begin();
  map<miString, map<miTime,EditField> >::iterator pend=editMap.end();
  map<miTime,EditField>::iterator q;
  map<miTime,EditField>::iterator qend;
  for(;p!=pend;p++){
    q    = editMap[p->first].begin();
    qend = editMap[p->first].end();
    for(;q!=qend;q++){
      delete q->second.fp;
      int n = q->second.origField.size();
      for( int i=0; i<n; i++){
	q->second.origField[i]->cleanup();
      }
      q->second.origField.clear();
    }
  }
  
  editMap.clear();
  firsttime = true;

}





