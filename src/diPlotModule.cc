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

#include <diPlotModule.h>
#include <diObsPlot.h>

#include <diFieldPlot.h>
#include <diSatPlot.h>
#include <diMapPlot.h>
#include <diTrajectoryPlot.h>
#include <diRadarEchoPlot.h>

#include <diObsManager.h>
#include <diSatManager.h>
#include <diObjectManager.h>
#include <diEditManager.h>
#include <diGridAreaManager.h>
#include <diAnnotationPlot.h>
#include <diWeatherArea.h>
#include <diStationPlot.h>
#include <diMapManager.h>
#include <diFieldManager.h>
#include <diFieldPlotManager.h>

#include <GL/gl.h>
#include <sstream>

// static class members
GridConverter PlotModule::gc;    // Projection-converter


// Default constructor
PlotModule::PlotModule()
  :plotw(0), ploth(0), resizezoom(true), hardcopy(false),
   bgcolourname("midnightBlue"),
   inEdit(false), apEditmessage(0),
   dorubberband(false), dopanning(false),
   keepcurrentarea(true), prodtimedefined(false),showanno(true),
   obsnr(0)
{
  oldx=newx=oldy=newy=0;
  mapdefined=false;
  mapDefinedByUser=false;
  mapDefinedByData=false;
  mapDefinedByNeed=false;
  mapDefinedByView=false;

  // used to detect map area changes
  float gs[Projection::speclen] = { 0., 0., 0., 0., 0., 0. };
  Projection p(Projection::undefined_projection, gs);
  Rectangle  r(0.,0.,0.,0.);
  previousrequestedarea= Area(p,r);
  requestedarea= Area(p,r);
  splot.setRequestedarea(requestedarea);
  areaIndex=-1;
  areaSaved=false;
}


// Destructor
PlotModule::~PlotModule(){
  cleanup();
}


void PlotModule::preparePlots(const vector<miString>& vpi)
{
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::preparePlots ++" << endl;
#endif
  // reset flags
  mapDefinedByUser= false;

  levelSpecified.clear();
  levelCurrent.clear();
  idnumSpecified.clear();
  idnumCurrent.clear();

  // split up input into separate products
  vector<miString> fieldpi,obspi,mappi,satpi,objectpi,trajectorypi,labelpi,editfieldpi;

  int n= vpi.size();
  // merge PlotInfo's for same type
  for (int i=0; i<n; i++) {
    vector<miString> tokens= vpi[i].split(1);
    if (!tokens.empty()) {
      miString type= tokens[0].upcase();
      if      (type=="FIELD")      fieldpi.push_back(vpi[i]);
      else if (type=="OBS")        obspi.push_back(vpi[i]);
      else if (type=="MAP")        mappi.push_back(vpi[i]);
      else if (type=="SAT")        satpi.push_back(vpi[i]);
      else if (type=="OBJECTS")	   objectpi.push_back(vpi[i]);
      else if (type=="TRAJECTORY") trajectorypi.push_back(vpi[i]);
      else if (type=="LABEL")	   labelpi.push_back(vpi[i]);
      else if (type=="EDITFIELD")  editfieldpi.push_back(vpi[i]);
    }
  }

  if (mappi.size()==0){
    // just for the initial map, use first map from setup
    mappi.push_back("MAP map=default");
  }

  // call prepare methods
  prepareFields(fieldpi);
  prepareObs(obspi);
  prepareSat(satpi);
  prepareMap(mappi);
  prepareObjects(objectpi);
  prepareTrajectory(trajectorypi);
  prepareAnnotation(labelpi);

  if (inEdit) editm->prepareEditFields(editfieldpi);

#ifdef DEBUGPRINT
  cerr << "++ Returning from PlotModule::preparePlots ++" << endl;
#endif
}


void PlotModule::prepareMap(const vector<miString>& inp){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::prepareMap ++" << endl;
#endif

  splot.xyClear();

  SetupParser setupParser;

  // init inuse array
  vector<bool> inuse;
  int nm= vmp.size();
  if (nm>0)
    inuse.insert(inuse.begin(), nm, false);

  int n= inp.size();

  // keep requested areas
  Area rarea;
  bool arearequested= false;

  bool isok;
  for (int k=0; k<n; k++){ // loop through all plotinfo's
    isok= false;
    if (nm>0) { // mapPlots exists
      for (int j=0; j<nm; j++){
	if (!inuse[j]){ // not already taken
	  if (vmp[j]->prepare(inp[k],true)){
	    inuse[j]= true;
	    isok= true;
	    arearequested |= vmp[j]->requestedArea(rarea);
	    vmp.push_back(vmp[j]);
	    break;
	  }
	}
      }
    }
    if (isok) continue;

    // make new mapPlot object and push it on the list
    int nnm= vmp.size();
    MapPlot *mp;
    vmp.push_back(mp);
    vmp[nnm]= new MapPlot();
    if (!vmp[nnm]->prepare(inp[k],false)){
      delete vmp[nnm];
      vmp.pop_back();
    } else {
      arearequested |= vmp[nnm]->requestedArea(rarea);
    }
  } // end plotinfo loop

  // delete unwanted mapplots
  if (nm>0){
    for (int i=0; i<nm; i++){
      if (!inuse[i]){
	delete vmp[i];
      }
    }
    vmp.erase(vmp.begin(),vmp.begin()+nm);
  }

  // remove filled maps not used (free memory)
  if (MapPlot::checkFiles(true)) {
    int n= vmp.size();
    for (int i=0; i<n; i++)
      vmp[i]->markFiles();
    MapPlot::checkFiles(false);
  }

  // check area
  if (!mapDefinedByUser && arearequested){
    mapDefinedByUser= (rarea.P().Gridtype()!=Projection::undefined_projection);
    requestedarea= rarea;
    splot.setRequestedarea(requestedarea);
  }
}


void PlotModule::prepareFields(const vector<miString>& inp){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::prepareFields ++" << endl;
#endif

  int npi= inp.size();

  miString str;
  map<miString,bool> plotenabled;

  // for now -- erase all fieldplots
  for (int i=0; i<vfp.size(); i++){
    // keep enable flag
    str= vfp[i]->getPlotInfo(3);
    plotenabled[str]= vfp[i]->Enabled();
    // free old fields
    fieldm->fieldcache->freeFields(vfp[i]->getFields());
    delete vfp[i];
  }
  vfp.clear();

  int n;
  for (int i=0; i<npi; i++){
    FieldPlot *fp;
    n= vfp.size();
    vfp.push_back(fp);
    vfp[n]= new FieldPlot();

    if (inp[i].contains(" ( ") && 
	inp[i].contains(" - ") && 
	inp[i].contains(" ) ")) {
      size_t p1= inp[i].find(" ( ",0);
      size_t p2= inp[i].find(" - ",p1+3);
      size_t p3= inp[i].find(" ) ",p2+3);
      if (p1!=string::npos && p2!=string::npos && p3!=string::npos) {
        miString fspec1= inp[i].substr(0,p1) + inp[i].substr(p1+2,p2-p1-2);
        miString fspec2= inp[i].substr(0,p1) + inp[i].substr(p2+2,p3-p2-2);
        vfp[n]->setDifference(fspec1,fspec2);
      }
    }

    if (!vfp[n]->prepare(inp[i])){
      delete vfp[n];
      vfp.pop_back();
    } else {
      str= vfp[n]->getPlotInfo(3);
      if (plotenabled.count(str)==0) plotenabled[str]= true;
      vfp[n]->enable(plotenabled[str] && vfp[n]->Enabled());
    }
  }

#ifdef DEBUGPRINT
  cerr << "++ Returning from PlotModule::prepareFields ++" << endl;
#endif
}


void PlotModule::prepareObs(const vector<miString>& inp){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::prepareObs ++" << endl;
#endif

  int npi= inp.size();

  // keep enable flag
  miString str;
  map<miString,bool> plotenabled;
  for (int i=0; i<vop.size(); i++){
    str= vop[i]->getPlotInfo(3);
    plotenabled[str]= vop[i]->Enabled();
  }

  // for now -- erase all obsplots etc..
  //first log stations plotted
  int nvop=vop.size();
  for (int i=0; i<nvop; i++)
    vop[i]->logStations();
  for (int i=0; i<vobsTimes.size(); i++){
    for (int j=0; j<vobsTimes[i].vobsOneTime.size(); j++)
      delete vobsTimes[i].vobsOneTime[j];
    vobsTimes[i].vobsOneTime.clear();
  }
  vobsTimes.clear();

//   for (int i=1; i<vop.size(); i++) // vop[0] has already been deleted (vobsTimes)
//     delete vop[i];
  vop.clear();

  obsnr =0;
  int n;
  ObsPlot *op;
  for (int i=0; i<npi; i++){
    n= vop.size();
    vop.push_back(op);
    vop[n]= new ObsPlot();
    if( !obsm->init(vop[n],inp[i])){
      delete vop[n];
      vop.pop_back();
    } else {
      str= vop[n]->getPlotInfo(3);
      if (plotenabled.count(str)==0) plotenabled[str]= true;
      vop[n]->enable(plotenabled[str] && vop[n]->Enabled());

      if(vobsTimes.size()==0){
	obsOneTime ot;
	vobsTimes.push_back(ot);
      }

      vobsTimes[0].vobsOneTime.push_back(op);
      vobsTimes[0].vobsOneTime[n]=vop[i];//forsiktig!!!!

    }
   }
  obsnr = 0;

#ifdef DEBUGPRINT
  cerr << "++ Returning from PlotModule::prepareObs ++" << endl;
#endif
}


void PlotModule::prepareSat(const vector<miString>& inp){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::prepareSat ++" << endl;
#endif

  // keep enable flag
  miString str;
  map<miString,bool> plotenabled;
  for (int i=0; i<vsp.size(); i++){
    str= vsp[i]->getPlotInfo(4);
    plotenabled[str]= vsp[i]->Enabled();
  }

  if (!satm->init(vsp,inp)){
    cerr << "PlotModule::prepareSat.  init returned false" << endl;
  }

  for (int i=0; i<vsp.size(); i++){
    str= vsp[i]->getPlotInfo(4);
    if (plotenabled.count(str)==0) plotenabled[str]= true;
    vsp[i]->enable(plotenabled[str] && vsp[i]->Enabled());
  }
}


void PlotModule::prepareAnnotation(const vector<miString>& inp){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::prepareAnnotation ++" << endl;
#endif

  // for now -- erase all annotationplots
  for (int i=0; i<vap.size(); i++)
    delete vap[i];
  vap.clear();

  if (inp.size()==0) return;

  annotationStrings = inp;
}


void PlotModule::prepareObjects(const vector<miString>& inp){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::prepareObjects ++" << endl;
#endif

  int npi= inp.size();

  miString str;
  map<miString,bool> plotenabled;

  // keep enable flag
  str= objects.getPlotInfo();
  plotenabled[str]= objects.isEnabled();

  objects.init();

  for (int i=0; i<npi; i++){
    objects.define(inp[i]);
  }

  str= objects.getPlotInfo();
  if (plotenabled.find(str)==plotenabled.end())
    //new plot
    objects.enable(true);
  else if (plotenabled.count(str)>0)
    objects.enable(plotenabled[str]);
}

void PlotModule::prepareTrajectory(const vector<miString>& inp){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::prepareTrajectory ++" << endl;
#endif
//   vtp.push_back(new TrajectoryPlot());

}


vector<PlotElement>& PlotModule::getPlotElements()
{
  //  cerr <<"PlotModule::getPlotElements()"<<endl;
  static vector<PlotElement> pel;
  pel.clear();

  int m;
  miString str;

  // get field names
  m= vfp.size();
  for (int j=0; j<m; j++){
    vfp[j]->getPlotName(str);
    str+= "# " + miString(j);
    bool enabled= vfp[j]->Enabled();
    // add plotelement
    pel.push_back(PlotElement("FIELD",str,"FIELD",enabled));
  }

  // get obs names
  m= vop.size();
  for (int j=0; j<m; j++){
    vop[j]->getPlotName(str);
    str+= "# " + miString(j);
    bool enabled= vop[j]->Enabled();
    // add plotelement
    pel.push_back(PlotElement("OBS",str,"OBS",enabled));
  }

  // get sat names
  m= vsp.size();
  for (int j=0; j<m; j++){
    vsp[j]->getPlotName(str);
    str+= "# " + miString(j);
    bool enabled= vsp[j]->Enabled();
    // add plotelement
    pel.push_back(PlotElement("RASTER",str,"RASTER",enabled));
  }

  // get obj names
  objects.getPlotName(str);
  if (str.exists()){
    bool enabled= objects.isEnabled();
    // add plotelement
    pel.push_back(PlotElement("OBJECTS",str,"OBJECTS",enabled));
  }

  // get trajectory names
  m= vtp.size();
  for (int j=0; j<m; j++){
    vtp[j]->getPlotName(str);
    if (str.exists()){
      str+= "# " + miString(j);
      bool enabled= vtp[j]->Enabled();
      // add plotelement
      pel.push_back(PlotElement("TRAJECTORY",str,"TRAJECTORY",enabled));
    }
  }

  // get stationPlot names
  m= stationPlots.size();
  for (int j=0; j<m; j++){
    if (!stationPlots[j]->isVisible())
      continue;
    stationPlots[j]->getPlotName(str);
    if (str.exists()){
      str+= "# " + miString(j);
      bool enabled = stationPlots[j]->Enabled();
      miString icon = stationPlots[j]->getIcon();
      miString ptype= "STATION";
      // add plotelement
      pel.push_back(PlotElement(ptype,str,icon,enabled));
    }
  }

  // get area objects names
  int n=vareaobjects.size();
  for (int j=0; j<n; j++){
    vareaobjects[j].getPlotName(str);
    if (str.exists()){
      str+= "# " + miString(j);
      bool enabled= vareaobjects[j].isEnabled();
      miString icon = vareaobjects[j].getIcon();
      // add plotelement
      pel.push_back(PlotElement("AREAOBJECTS",str,icon,enabled));
    }
  }


  // get locationPlot annotations
  m= locationPlots.size();
  for (int j=0; j<m; j++){
    locationPlots[j]->getPlotName(str);
    if (str.exists()){
      str+= "# " + miString(j);
      bool enabled= locationPlots[j]->Enabled();
      // add plotelement
      pel.push_back(PlotElement("LOCATION",str,"LOCATION",enabled));
    }
  }

  return pel;
}

void PlotModule::enablePlotElement(const PlotElement& pe)
{
  miString str;
  if (pe.type=="FIELD"){
    for (int i=0; i<vfp.size(); i++){
      vfp[i]->getPlotName(str);
      str+= "# " + miString(i);
      if (str==pe.str){
	vfp[i]->enable(pe.enabled);
	break;
      }
    }
  } else if (pe.type=="RASTER"){
    for (int i=0; i<vsp.size(); i++){
      vsp[i]->getPlotName(str);
      str+= "# " + miString(i);
      if (str==pe.str){
	vsp[i]->enable(pe.enabled);
	break;
      }
    }
  } else if (pe.type=="OBS"){
    for (int i=0; i<vop.size(); i++){
      vop[i]->getPlotName(str);
      str+= "# " + miString(i);
      if (str==pe.str){
	vop[i]->enable(pe.enabled);
	break;
      }
    }
  } else if (pe.type=="OBJECTS"){
    objects.getPlotName(str);
    if (str==pe.str)
      objects.enable(pe.enabled);
  } else if (pe.type=="TRAJECTORY"){
    for (int i=0; i<vtp.size(); i++){
      vtp[i]->getPlotName(str);
      str+= "# " + miString(i);
      if (str==pe.str){
	vtp[i]->enable(pe.enabled);
	break;
      }
    }
  } else if (pe.type=="STATION"){
    for (int i=0; i<stationPlots.size(); i++){
      stationPlots[i]->getPlotName(str);
      str+= "# " + miString(i);
      if (str==pe.str){
	stationPlots[i]->enable(pe.enabled);
	break;
      }
    }
  } else if (pe.type=="AREAOBJECTS"){
    int n=vareaobjects.size();
    for (int i=0; i<n; i++){
      vareaobjects[i].getPlotName(str);
      if (str.exists()){
      str+= "# " + miString(i);
	if (str==pe.str){
	  vareaobjects[i].enable(pe.enabled);
	  break;
	}
      }
    }
  } else if (pe.type=="LOCATION"){
    for (int i=0; i<locationPlots.size(); i++){
      locationPlots[i]->getPlotName(str);
      str+= "# " + miString(i);
      if (str==pe.str){
	locationPlots[i]->enable(pe.enabled);
	break;
      }
    }
  } else {
    // unknown
    return;
  }

  // get annotations from all plots
  setAnnotations();
}


void PlotModule::setAnnotations(){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule::setAnnotations ++" << endl;
#endif

  int n= vap.size();
  for (int i=0; i<n; i++) delete vap[i];
  vap.clear();

  int npi= annotationStrings.size();

  AnnotationPlot* ap;

  for (int i=0; i<npi; i++){
    n= vap.size();
    vap.push_back(ap);
    vap[n]= new AnnotationPlot();
    if (!vap[n]->prepare(annotationStrings[i])){
      delete vap[n];
      vap.pop_back();
    }
  }

  //Annotations from setup, qmenu, etc.
  int m;
  n= vap.size();

  // set annotation-data
  Colour col;
  miString str;
  vector<AnnotationPlot::Annotation> annotations;
  AnnotationPlot::Annotation ann;

  vector<miTime> fieldAnalysisTime;

  // get field annotations
  m= vfp.size();
  for (int j=0; j<m; j++){
    fieldAnalysisTime.push_back( vfp[j]->getAnalysisTime() );

    if (!vfp[j]->Enabled()) continue;
    vfp[j]->getFieldAnnotation(str,col);
    ann.str= str;
    ann.col= col;
    annotations.push_back(ann);
  }

  // get sat annotations
  m= vsp.size();
  for (int j=0; j<m; j++){
    if (!vsp[j]->Enabled()) continue;
    vsp[j]->getSatAnnotation(str,col);
    ann.str= str;
    ann.col= col;
    //ann.col= getContrastColour();
    annotations.push_back(ann);
  }

  // get obj annotations
  if (objects.isEnabled()){
    objects.getObjAnnotation(str,col);
    ann.str= str;
    ann.col= col;
    if (str.exists()){
      annotations.push_back(ann);
    }
  }
  // get obs annotations
  m= vop.size();
  for (int j=0; j<m; j++){
    if (!vop[j]->Enabled()) continue;
    vop[j]->getObsAnnotation(str,col);
    ann.str= str;
    ann.col= col;
    annotations.push_back(ann);
  }

  // get trajectory annotations
  m= vtp.size();
  for (int j=0; j<m; j++){
    if (!vtp[j]->Enabled()) continue;
    vtp[j]->getTrajectoryAnnotation(str,col);
    // empty string if data plot is off
    if (str.exists()) {
      ann.str= str;
      ann.col= col;
      annotations.push_back(ann);
    }
  }

  // get radar echo annotations
  m= vrp.size();
  for (int j=0; j<m; j++){
    if (!vrp[j]->Enabled()) continue;
    vrp[j]->getRadarEchoAnnotation(str,col);
    // empty string if data plot is off
    if (str.exists()) {
      ann.str= str;
      ann.col= col;
      annotations.push_back(ann);
    }
  }
  
  
  // get stationPlot annotations
  m= stationPlots.size();
  for (int j=0; j<m; j++){
    if (!stationPlots[j]->Enabled()) continue;
    stationPlots[j]->getStationPlotAnnotation(str,col);
    ann.str= str;
    ann.col= col;
    annotations.push_back(ann);
  }

  // get locationPlot annotations
  m= locationPlots.size();
  for (int j=0; j<m; j++){
    if (!locationPlots[j]->Enabled()) continue;
    locationPlots[j]->getAnnotation(str,col);
    ann.str= str;
    ann.col= col;
    annotations.push_back(ann);
  }

  for (int i=0; i<n; i++){
    vap[i]->setData(annotations,fieldAnalysisTime);
    vap[i]->setfillcolour(splot.getBgColour());
  }

  //annotations from data

  //get field and sat annotations
  for (int i=0; i<n; i++){
    vector< vector<miString> > vvstr = vap[i]->getAnnotationStrings();
    int nn=vvstr.size();
    for (int k=0; k<nn; k++){
      m= vfp.size();
      for (int j=0; j<m; j++){
	vfp[j]->getAnnotations(vvstr[k]);
      }
      m= vsp.size();
      for (int j=0; j<m; j++){
	vsp[j]->getAnnotations(vvstr[k]);
      }
      editm->getAnnotations(vvstr[k]);
      objects.getAnnotations(vvstr[k]);
    }
    vap[i]->setAnnotationStrings(vvstr);
  }



  //get obs annotations
  n= vop.size();
  for(int i=0; i<n; i++){
    if (!vop[i]->Enabled()) continue;
    vector<miString> obsinfo = vop[i]->getObsExtraAnnotations();
    int npi= obsinfo.size();
    for (int j=0; j<npi; j++){
      int nn= vap.size();
      AnnotationPlot* ap = new AnnotationPlot(obsinfo[i]);;
      vap.push_back(ap);
    }
  }

  //objects
  vector <miString> objLabelstring=objects.getObjectLabels();
  n= objLabelstring.size();
  for (int i = 0;i<n;i++){
    AnnotationPlot* ap = new AnnotationPlot(objLabelstring[i]);
    vap.push_back(ap);
  }

}


void PlotModule::updateLevel(const miString& levelSpec, const miString& levelSet)
{
  levelSpecified= levelSpec;
  levelCurrent=   levelSet;
  miString pin;
  vector<Field*> fv;
  int i,n,vectorIndex;
  miTime t= splot.getTime();

  n= vfp.size();
  for (i=0; i<n; i++){
    if (vfp[i]->updateLevelNeeded(levelSpec,pin)) {
      bool res;
      if (vfp[i]->isDifference()) {
        miString fspec1,fspec2;
        vfp[i]->getDifference(fspec1,fspec2,vectorIndex);
        res= fieldplotm->makeDifferenceField(fspec1,fspec2,t,fv,
					 levelSpecified,levelCurrent,
					 idnumSpecified,idnumCurrent,
			                 vectorIndex);
      } else {
        res= fieldplotm->makeFields(pin,t,fv,
				levelSpecified,levelCurrent,
				idnumSpecified,idnumCurrent);
      }
      //free old fields
      fieldm->fieldcache->freeFields(vfp[i]->getFields());
      //set new fields
      vfp[i]->setData(fv,t);
    }
  }

  if (fv.size() && fv[0]->oceanDepth>=0 && vop.size()>0)
    splot.setOceanDepth(int(fv[0]->oceanDepth));

  if (fv.size() && fv[0]->pressureLevel>=0 && vop.size()>0)
    splot.setPressureLevel(int(fv[0]->pressureLevel));

  n= vop.size();
  for (i=0; i<n; i++) {
    if (vop[i]->LevelAsField()) {
      if (!obsm->prepare(vop[i],t))
	cerr << "updateLevel: ObsManager returned false from prepare" << endl;
    }
  }

  // get annotations from all plots
  setAnnotations();
}


void PlotModule::updateIdnum(const miString& idnumSpec, const miString& idnumSet)
{
  idnumSpecified= idnumSpec;
  idnumCurrent=   idnumSet;

  miString pin;
  vector<Field*> fv;
  int i,n,vectorIndex;
  miTime t= splot.getTime();

  n= vfp.size();
  for (i=0; i<n; i++){
    if (vfp[i]->updateIdnumNeeded(idnumSpec,pin)) {
      bool res;
      if (vfp[i]->isDifference()) {
        miString fspec1,fspec2;
        vfp[i]->getDifference(fspec1,fspec2,vectorIndex);
        res= fieldplotm->makeDifferenceField(fspec1,fspec2,t,fv,
					     levelSpecified,levelCurrent,
					     idnumSpecified,idnumCurrent,
					     vectorIndex);
      } else {
        res= fieldplotm->makeFields(pin,t,fv,
				    levelSpecified,levelCurrent,
				    idnumSpecified,idnumCurrent);
      }
      //free old fields
      fieldm->fieldcache->freeFields(vfp[i]->getFields());
      //set new fields
      vfp[i]->setData(fv,t);
    }
  }

  // get annotations from all plots
  setAnnotations();
}


// update plots
void PlotModule::updatePlots()
{
#ifdef DEBUGREDRAW
  cerr <<"PlotModule::updatePlots  keepcurrentarea="<<keepcurrentarea<<endl;
#endif
  miString pin;
  vector<Field*> fv;
  int i,n,vectorIndex;
  miTime t= splot.getTime();
  bool satOK= false;
  Area satarea, plotarea, newarea;

  // prepare data for field plots
  n= vfp.size();
  for (i=0; i<n; i++){
    if (vfp[i]->updateNeeded(pin)) {
      bool res;
      if (vfp[i]->isDifference()) {
        miString fspec1,fspec2;
        vfp[i]->getDifference(fspec1,fspec2,vectorIndex);
        res= fieldplotm->makeDifferenceField(fspec1,fspec2,t,fv,
					     levelSpecified,levelCurrent,
					     idnumSpecified,idnumCurrent,
					     vectorIndex);
      } else {
        res= fieldplotm->makeFields(pin,t,fv,
				    levelSpecified,levelCurrent,
				    idnumSpecified,idnumCurrent);
      }
      //free old fields
      fieldm->fieldcache->freeFields(vfp[i]->getFields());
      //set new fields
      vfp[i]->setData(fv,t);
    }
  }

  if (fv.size() ){
    // level for P-level observations "as field"
    splot.setPressureLevel(int(fv[0]->pressureLevel));
    // depth for ocean depth observations "as field"
    splot.setOceanDepth(int(fv[0]->oceanDepth));
  }

  // prepare data for satellite plots
  n= vsp.size();
  for (i=0; i<n; i++){
    if (satm->setData(vsp[i])){
      if (!satOK) {
        satarea=vsp[i]->getSatArea();
        satOK= true;
      }
#ifdef DEBUGPRINT
    } else {
      cerr << "SatManager returned false from setData" << endl;
#endif
    }
  }

  // set maparea from sat, map spec. or fields

  //######################################################################
  //int gt;
  //float gs[6];
  //Area aa;
  //cerr << "----------------------------------------------------" << endl;
  //aa=previousrequestedarea;
  //gt= aa.P().Gridtype();
  //aa.P().Gridspec(gs);
  //cerr << "previousrequestedarea " << gt;
  //for (i=0; i<6; i++) cerr << " " << gs[i];
  //cerr << "    " << aa.R().x1 << " " << aa.R().x2 << " "
  //               << aa.R().y1 << " " << aa.R().y2 << endl;
  //aa=requestedarea;
  //gt= aa.P().Gridtype();
  //aa.P().Gridspec(gs);
  //cerr << "requestedarea         " << gt;
  //for (i=0; i<6; i++) cerr << " " << gs[i];
  //cerr << "    " << aa.R().x1 << " " << aa.R().x2 << " "
  //               << aa.R().y1 << " " << aa.R().y2 << endl;
  //cerr << "mapDefinedByUser= " << mapDefinedByUser << endl;
  //cerr << "mapDefinedByData= " << mapDefinedByData << endl;
  //cerr << "mapDefinedByNeed= " << mapDefinedByNeed << endl;
  //cerr << "mapDefinedByView= " << mapDefinedByView << endl;
  //cerr << "mapdefined=       " << mapdefined << endl;
  //cerr << "keepcurrentarea=  " << keepcurrentarea << endl;
  //cerr << "----------------------------------------------------" << endl;
  //######################################################################
  mapdefined= false;

  if (satOK){ // we have a raster image
    plotarea= splot.getMapArea(); // current area on display

    bool previousOK = false;  // previous area ok to use
    bool requestedOK= false;  // requested area ok to use

    // if first plot: getMapArea returns undefined area
    if (plotarea.P().Gridtype()==Projection::undefined_projection){
      if (mapDefinedByUser)
	plotarea= requestedarea; // choose requested-area if existing
      else
	plotarea= satarea; // left with sat-area
    }

    // check if previous area ok to use
    if (satarea.P()==plotarea.P())
      previousOK= true;
    else
      gc.checkAreaSimilarity(satarea, plotarea, previousOK);

    // check if requested area (if any) ok to use
    if (mapDefinedByUser){
      if (satarea.P()==requestedarea.P())
	requestedOK= true;
      else
	gc.checkAreaSimilarity(satarea, requestedarea, requestedOK);
    }


      // FIRST: forced change!
    if (!keepcurrentarea || !previousOK){
      if (keepcurrentarea) // but not similar enough
	newarea= splot.findBestMatch(satarea);
      // do not keep current area
      else if (requestedOK) // change to requested
	newarea= requestedarea;
      else // change to satarea
	newarea= satarea;
      splot.setMapArea(newarea,keepcurrentarea);

      // THEN: selected a new MAP-area
      // (keepcurrentarea=previousOK=TRUE)
    } else if (previousrequestedarea!=requestedarea){
      if (requestedOK) // try to use requested area
	newarea= requestedarea;
      else { // find best match from satarea and requestedarea
	splot.setMapArea(requestedarea,keepcurrentarea);
	newarea= splot.findBestMatch(satarea);
      }
      splot.setMapArea(newarea,keepcurrentarea);
    }// ELSE: no change needed

    mapdefined= mapDefinedByData= true;

    // ----- NO raster images
  } else if (mapDefinedByUser) { // area != "modell/sat-omr."
    plotarea= splot.getMapArea();
    if (!keepcurrentarea || // show def. area
	plotarea.P()!=requestedarea.P() || // or user just selected new area
	previousrequestedarea.R()!=requestedarea.R()){
      mapdefined= mapDefinedByUser=
	splot.setMapArea(requestedarea,keepcurrentarea);
    } else {
      mapdefined= true;
    }
  } else if (keepcurrentarea && previousrequestedarea!=requestedarea) {
    // change from specified area to model/sat area
    mapDefinedByData= false;
  }

  previousrequestedarea= requestedarea;

  if (!mapdefined && keepcurrentarea && mapDefinedByData)
    mapdefined= true;

  if (!mapdefined && inEdit) {
    // set area equal to editfield-area
    Area a;
    if (editm->getFieldArea(a)) {
      splot.setMapArea(a,true);
      mapdefined= mapDefinedByData= true;
    }
  }

  if (!mapdefined && vfp.size()>0) {
    // set area equal to first EXISTING field-area ("all timesteps"...)
    n= vfp.size();
    i= 0;
    Area a;
    while (i<n && !vfp[i]->getRealFieldArea(a)) i++;
    if (i<n) {
      // AC: should we find best match here???
      if (keepcurrentarea) newarea= splot.findBestMatch(a);
      else newarea= a;
      splot.setMapArea(newarea,keepcurrentarea);
      mapdefined= mapDefinedByData= true;
    }
  }

  if (!mapdefined && keepcurrentarea && mapDefinedByNeed)
    mapdefined= true;

  if (!mapdefined && (vop.size()>0 || objects.defined
				   || locationPlots.size()>0)) {
    // obs or objects on initial map ... change to "Atlant" area
    float gs[Projection::speclen] = { 54.,52., 79., 0., 0., 0. };
    float rx1=20., rx2=76., ry1=9., ry2=59.;
    Projection p(Projection::polarstereographic_60, gs);
    Rectangle  r(rx1,ry1,rx2,ry2);
    Area       a(p,r);
    splot.setMapArea(a,keepcurrentarea);
    mapdefined= mapDefinedByNeed= true;
  }

  // moved here ------------------------
  if (!mapdefined && keepcurrentarea && mapDefinedByView)
    mapdefined= true;

  if (!mapdefined) {
    // no data on initial map ... change to "Hirlam.50km" projection and area
    float gs[Projection::speclen] = { -46.5, -36.5, 0.5, 0.5, 0., 65. };
    float rx1=0., rx2=187., ry1=0., ry2=151.;
    Projection p(Projection::spherical_rotated, gs);
    Rectangle  r(rx1,ry1,rx2,ry2);
    Area       a(p,r);
    splot.setMapArea(a,keepcurrentarea);
    mapdefined= mapDefinedByView= true;
  }
  // ----------------------------------

  // prepare data for observation plots
  n= vop.size();
  for(int i=0; i<n; i++){
    vop[i]->logStations();
    vop[i]=vobsTimes[0].vobsOneTime[i];
  }

  if(obsnr > 0) obsnr=0;

  int m = vobsTimes.size();
  for(i=m-1; i>0; i--){
    int l = vobsTimes[i].vobsOneTime.size();
    for( int j=0;j<l; j++){
      delete vobsTimes[i].vobsOneTime[j];
      vobsTimes[i].vobsOneTime.pop_back();
    }
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }
  for (i=0; i<n; i++){
    if (!obsm->prepare(vop[i],splot.getTime()))
      cerr << "ObsManager returned false from prepare" << endl;
  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);


  // prepare met-objects
  objm->prepareObjects(t,splot.getMapArea(),objects);




  // prepare editobjects (projection etc.)
  editobjects.changeProjection(splot.getMapArea());
  combiningobjects.changeProjection(splot.getMapArea());

  n=stationPlots.size();
  for (int i=0; i<n; i++){
    stationPlots[i]->changeProjection();
  }



  // Prepare/compute trajectories - change projection
  if (vtp.size()>0) {
    vtp[0]->prepare();

    if (vtp[0]->inComputation()) {
      miString fieldname= vtp[0]->getFieldName();
      int i=0, n= vfp.size();
      while (i<n && vfp[i]->getTrajectoryFieldName()!=fieldname) i++;
      if (i<n) {
	vector<Field*> vf= vfp[i]->getFields();
	// may have 2 or 3 fields (the 3rd a colour-setting field)
	if (vf.size()>=2) {
	  vtp[0]->compute(vf);
	}
      }
    }
  }

  // get annotations from all plots
  setAnnotations();

  PlotAreaSetup();
}


// update projection used...minimum update of data
void PlotModule::updateProjection(){
  //cerr << "++++++++ CALLING updateProjection" << endl;
  miTime t= splot.getTime();
  bool satOK= false;
  Area satarea, plotarea;

  if (vsp.size()>0){
    satarea=vsp[0]->getSatArea();
    satOK= true;
  }

  // set maparea from sat or fields
  if (keepcurrentarea) {
    mapdefined= true;
  }
  if (satOK){
    plotarea= splot.getMapArea();
    if (!(plotarea.P()==satarea.P())){
      splot.setMapArea(satarea,keepcurrentarea);
    }
    mapdefined= true;
  }
  if (!mapdefined && inEdit) {
    // set area equal to editfield-area
    Area a;
    if (editm->getFieldArea(a)) {
      splot.setMapArea(a,true);
      mapdefined= true;
    }
  }
  if (!mapdefined && vfp.size()>0) {
    // else set area equal to first field-area
    Area a=vfp[0]->getFieldArea();
    splot.setMapArea(a,keepcurrentarea);
    mapdefined= true;
  }

  // prepare met-objects
  objm->prepareObjects(t,splot.getMapArea(),objects);


  // prepare editobjects (projection etc.)
   editobjects.changeProjection(splot.getMapArea());
   combiningobjects.changeProjection(splot.getMapArea());

  int n=stationPlots.size();
  for (int i=0; i<n; i++){
    stationPlots[i]->changeProjection();
  }

  PlotAreaSetup();
}


// start hardcopy plot
void PlotModule::startHardcopy(const printOptions& po){
  if (hardcopy){
    // if hardcopy in progress, and same filename: make new page
    if (po.fname == printoptions.fname){
      splot.startPSnewpage();
      return;
    }
    // different filename: end current output and start a new
    splot.endPSoutput();
  }
  hardcopy= true;
  printoptions= po;
  // postscript output
  splot.startPSoutput(printoptions);
}

// end hardcopy plot
void PlotModule::endHardcopy(){
  if (hardcopy) splot.endPSoutput();
  hardcopy= false;
}


// -------------------------------------------------------------------------
// Master plot-routine
// The under/over toggles are used for speedy editing/drawing
// under: plot underlay part of image (static fields, sat.pict., obs. etc.)
// over:  plot overlay part (editfield, objects etc.)
//--------------------------------------------------------------------------
void PlotModule::plot(bool under, bool over)
{
	
	
  	
#ifdef DEBUGPRINT
  cerr << "++ PlotModule.plot() ++" << endl;
#endif
#ifdef DEBUGREDRAW
  cerr<<"PlotModule::plot  under,over: "<<under<<" "<<over<<endl;
#endif

  Colour cback(splot.getBgColour().cStr());

  // make background colour and a suitable contrast colour available
  splot.setBackgroundColour(cback);
  splot.setBackContrastColour(getContrastColour());

  //if plotarea has changed, calculate great circle distance...
  if (splot.getDirty()){
    float lat1,lat2,lon1,lon2,lat3,lon3;
    float width,height;
    splot.getPhysSize(width,height);
    //     PhysToGeo(0,0,lat1,lon1);
    //     PhysToGeo(width,height,lat2,lon2);
    //     PhysToGeo(width/2,height/2,lat3,lon3);
    //##############################################################
    // lat3,lon3, point where ratio between window scale and geographical scale
    // is computed, set to Oslo coordinates, can be changed according to area
    lat3=60;
    lon3=10;
    lat1=lat3-10;
    lat2=lat3+10;
    lon1=lon3-10;
    lon2=lon3+10;
    //###############################################################
    //gcd is distance between lower left and upper right corners
    float gcd = GreatCircleDistance(lat1,lat2,lon1,lon2);
    float x1,y1,x2,y2;
    GeoToPhys(lat1,lon1,x1,y1);
    GeoToPhys(lat2,lon2,x2,y2);
    float distGeoSq=(x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
    float distWindowSq=width*width+height*height;
    float ratio=sqrtf(distWindowSq/distGeoSq);
    gcd=gcd*ratio;
    //float gcd2 = GreatCircleDistance(lat1,lat3,lon1,lon3);
    //float earthCircumference = 40005041; // meters
    //if (gcd<gcd2) gcd  = earthCircumference-gcd;
    splot.setGcd(gcd);
  }

  if(under){
    plotUnder();
  }

  if(over){
    plotOver();
  }


}



// plot underlay ---------------------------------------
void PlotModule::plotUnder()
{
  int i,n,m;

  Rectangle plotr= splot.getPlotSize();

  Colour cback(splot.getBgColour().cStr());

  // set correct worldcoordinates
  glLoadIdentity();
  glOrtho(plotr.x1,plotr.x2,plotr.y1,plotr.y2,-1,1);
  
  if (hardcopy) splot.addHCScissor(plotr.x1+0.0001,plotr.y1+0.0001,
				   plotr.x2-plotr.x1-0.0002,
				   plotr.y2-plotr.y1-0.0002);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  glClearColor(cback.fR(),cback.fG(),cback.fB(),cback.fA());
  glClear( GL_COLOR_BUFFER_BIT |
	   GL_DEPTH_BUFFER_BIT |
	   GL_STENCIL_BUFFER_BIT);
  
  // draw background (for hardcopy)
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glColor4f(cback.fR(),cback.fG(),cback.fB(),cback.fA());
  const float d= 0;
  glRectf(plotr.x1+d,plotr.y1+d,plotr.x2-d,plotr.y2-d);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_BLEND);
  
  // plot map-elements for lowest zorder
  n= vmp.size();
  for (i=0; i<n; i++){
#ifdef DEBUGPRINT
    cerr << "Kaller plot til mapplot number:" << i << endl;
#endif
    vmp[i]->plot(0);
  }
  
  // plot satellite images
  n= vsp.size();
  for (i=0; i<n; i++){
#ifdef DEBUGPRINT
    cerr << "Kaller plot til satplot number:" << i << endl;
#endif
    vsp[i]->plot();
  }

  // mark undefined areas/values in field (before map)
  n= vfp.size();
  for (i=0; i<n; i++){
    if (vfp[i]->getUndefinedPlot()){
#ifdef DEBUGPRINT
      cerr << "Kaller plotUndefined til fieldplot number:" << i << endl;
#endif
      vfp[i]->plotUndefined();
    }
  }

  // plot fields (shaded fields etc. before map)
  n= vfp.size();
  for (i=0; i<n; i++){
    if (vfp[i]->getShadePlot()){
#ifdef DEBUGPRINT
      cerr << "Kaller plot til fieldplot number:" << i << endl;
#endif
      vfp[i]->plot();
    }
  }

  // plot map-elements for auto zorder
  n= vmp.size();
  for (i=0; i<n; i++){
#ifdef DEBUGPRINT
    cerr << "Kaller plot til mapplot number:" << i << endl;
#endif
    vmp[i]->plot(1);
  }

  // plot locationPlots (vcross,...)
  n= locationPlots.size();
  for (i=0; i<n; i++)
    locationPlots[i]->plot();

  // plot fields (isolines, vectors etc. after map)
  n= vfp.size();
  for (i=0; i<n; i++){
    if (!vfp[i]->getShadePlot() && !vfp[i]->overlayBuffer()){
#ifdef DEBUGPRINT
      cerr << "Kaller plot til fieldplot number:" << i << endl;
#endif
      vfp[i]->plot();
    }
  }

  objects.changeProjection(splot.getMapArea());
  objects.plot();

  n=vareaobjects.size();
  for (i=0; i<n; i++){
    vareaobjects[i].changeProjection(splot.getMapArea());
    vareaobjects[i].plot();
  }

  n=stationPlots.size();
  for (i=0; i<n; i++){
    stationPlots[i]->plot();
  }

  // plot inactive edit fields/objects under observations
  if (inEdit){
    editm->plot(true, false);
  }

  // if "PPPP-mslp", calc. values and plot observations,
  //if inEdit use editField, if not use first "MSLP"-field
  if ( obsm->obs_mslp() ){
    if( inEdit && mapmode!=fedit_mode && mapmode!=combine_mode) {
      // in underlay while not changing the field
      if (editm->obs_mslp(obsm->getObsPositions())) {
	obsm->calc_obs_mslp(vop);
      }
    } else if (!inEdit) {
      for(int i=0; i<vfp.size();i++){
	if (vfp[i]->obs_mslp(obsm->getObsPositions())) {
	  obsm->calc_obs_mslp(vop);
	  break;
	}
      }
    }
  }


  // plot observations
  if (!inEdit || !obsm->obs_mslp()){
    ObsPlot::clearPos();
    m= vop.size();
    for (i=0; i<m; i++)
      vop[i]->plot();
  }

  int nanno=vap.size();
  for (int l=0; l<nanno; l++){
    vector< vector<miString> > vvstr = vap[l]->getAnnotationStrings();
    int nn=vvstr.size();
    for (int k=0; k<nn; k++){
      n= vfp.size();
      for (int j=0; j<n; j++){
	vfp[j]->getDataAnnotations(vvstr[k]);
      }
      n= vop.size();
      for (int j=0; j<n; j++){
	vop[j]->getDataAnnotations(vvstr[k]);
      }
    }
    vap[l]->setAnnotationStrings(vvstr);
  }

  //plot trajectories
  m= vtp.size();
  for (i=0; i<m; i++)
    vtp[i]->plot();

  if (showanno && !inEdit){
    // plot Annotations
    n= vap.size();
    for (i=0; i<n; i++){
      //	cerr <<"i:"<<i<<endl;
      vap[i]->plot();
    }
  }

  if (hardcopy) splot.removeHCClipping();
}

  
// plot overlay ---------------------------------------
void PlotModule::plotOver()
{

  int i,n,m;

  Rectangle plotr= splot.getPlotSize();

  // plot GridAreas (polygons)
  if(aream) aream->plot();

// Check this!!!  
    n= vfp.size();
    for (i=0; i<n; i++){
      if (vfp[i]->overlayBuffer() && !vfp[i]->getShadePlot()){
	vfp[i]->plot();
      }
    }

  // plot active draw- and editobjects here
  if (inEdit){

    if (apEditmessage) apEditmessage->plot();

    editm->plot(false, true);

    // if PPPP-mslp, calc. values and plot observations,
    // in overlay while changing the field
    if ( obsm->obs_mslp() &&
	(mapmode==fedit_mode || mapmode==combine_mode) ) {
      if (editm->obs_mslp(obsm->getObsPositions())) {
	obsm->calc_obs_mslp(vop);
      }
    }

    // Annotations
    if (showanno){
      n= vap.size();
      for (i=0; i<n; i++)
	vap[i]->plot();
    }
    n= editVap.size();
    for (i=0; i<n; i++)
      editVap[i]->plot();

  } // if inEdit


  if (hardcopy) splot.addHCScissor(plotr.x1+0.0001,plotr.y1+0.0001,
				   plotr.x2-plotr.x1-0.0002,
				   plotr.y2-plotr.y1-0.0002);

  // plot map-elements for highest zorder
  n= vmp.size();
  for (i=0; i<n; i++){
#ifdef DEBUGPRINT
    cerr << "Kaller plot til mapplot number:" << i << endl;
#endif
    vmp[i]->plot(2);
  }
  
  splot.UpdateOutput();
  splot.setDirty(false);
  
  // frame (not needed if maprect==fullrect)
  Rectangle mr= splot.getMapSize();
  Rectangle fr= splot.getPlotSize();
  if (mr!=fr || hardcopy) {
    glShadeModel(GL_FLAT);
    glColor3f(0.0,0.0,0.0);
    glLineWidth(1.0);
    mr.x1 += 0.0001;
    mr.y1 += 0.0001;
    mr.x2 -= 0.0001;
    mr.y2 -= 0.0001;
    glBegin(GL_LINES);
    glVertex2f(mr.x1,mr.y1);
    glVertex2f(mr.x2,mr.y1);
    glVertex2f(mr.x2,mr.y1);
    glVertex2f(mr.x2,mr.y2);
    glVertex2f(mr.x2,mr.y2);
    glVertex2f(mr.x1,mr.y2);
    glVertex2f(mr.x1,mr.y2);
    glVertex2f(mr.x1,mr.y1);
    glEnd();
  }
  
  splot.UpdateOutput();
  // plot rubberbox
  if (dorubberband){
#ifdef DEBUGREDRAW
    cerr<<"PlotModule::plot rubberband oldx,oldy,newx,newy: "
	<<oldx<<" "<<oldy<<" "<<newx<<" "<<newy<<endl;
#endif
    Rectangle fullr= splot.getPlotSize();
    float x1= fullr.x1 + fullr.width()*oldx/float(plotw);
    float y1= fullr.y1 + fullr.height()*oldy/float(ploth);
    float x2= fullr.x1 + fullr.width()*newx/float(plotw);
    float y2= fullr.y1 + fullr.height()*newy/float(ploth);
    
    Colour c= getContrastColour();
    glColor4ubv(c.RGBA());
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0);
    //glRectf(x1,y1,x2,y2); // Mesa problems ?
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1,y1);
    glVertex2f(x2,y1);
    glVertex2f(x2,y2);
    glVertex2f(x1,y2);
    glEnd();
  }

  if (hardcopy) splot.removeHCClipping();

} 




void PlotModule::PlotAreaSetup(){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule.PlotAreaSetup() ++" << endl;
#endif

  if (plotw<1 || ploth<1) return;
  float waspr= float(plotw)/float(ploth);

  Area ma= splot.getMapArea();
  Rectangle mapr= ma.R();

  float d, del, delta = 0.01;
  del = delta;
  while (mapr.width()<delta) {
      d = (del - mapr.width())*0.5;
      mapr.x1 -= d;
      mapr.x2 += d;
      del = del*2.;
  }
  del = delta;
  while (mapr.height()<delta) {
      d = (del - mapr.height())*0.5;
      mapr.y1 -= d;
      mapr.y2 += d;
      del = del*2.;
  }

  float maspr= mapr.width()/mapr.height();

  float dwid=0, dhei=0;
  if (waspr>maspr) {// increase map width
    dwid= waspr*mapr.height() - mapr.width();
  } else { // increase map height
    dhei= mapr.width()/waspr - mapr.height();
  }

  // update map area
  Rectangle mr= mapr;
  mr.x1-= (dwid)/2.0;
  mr.y1-= (dhei)/2.0;
  mr.x2+= (dwid)/2.0;
  mr.y2+= (dhei)/2.0;

  // add border
//   float border= mr.width()*0.03/2.0;
  float border= 0.0;
  // update full plot area
  Rectangle fr= mr;
  fr.x1-= border;
  fr.y1-= border;
  fr.x2+= border;
  fr.y2+= border;

  splot.setMapSize(mr);
  splot.setPlotSize(fr);

#ifdef DEBUGPRINT
  cerr << "============ After PlotAreaSetup ======" << endl;
  cerr << "Fullplotarea:" << fr << endl;
  cerr << "plotarea:" << mr << endl;
  cerr << "=======================================" << endl;
#endif
}


void PlotModule::setPlotWindow(const int& w, const int& h){
#ifdef DEBUGPRINT
  cerr << "++ PlotModule.setPlotWindow() ++" <<
    " w=" << w << " h=" << h << endl;
#endif

  plotw= w;
  ploth= h;

  splot.setPhysSize(float(plotw),float(ploth));

  PlotAreaSetup();

  if (hardcopy) splot.resetPage();
}


void PlotModule::cleanup(){

  int i,n;
  n= vmp.size();
  for (i=0; i<n; i++) delete vmp[i];
  vmp.clear();

  n= vfp.size();
  
  // Field deletion at the end is done in the cache. The cache destructor is called by
  // FieldPlotManagers destructor, which comes before this destructor. Basocally we try to
  // destroy something in a dead pointer here....
  for (i=0; i<n; i++){
    vector<Field*> v=vfp[i]->getFields();
    fieldm->fieldcache->freeFields(v);
    delete vfp[i];
  }
  vfp.clear();

  n= vsp.size();
  for (i=0; i<n; i++) delete vsp[i];
  vsp.clear();

  n= vobsTimes.size();
  for (i=n-1; i>-1; i--) {
    int m = vobsTimes[i].vobsOneTime.size();
    for( int j=0; j<m; j++)
      delete vobsTimes[i].vobsOneTime[j];
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }
  vobsTimes.clear();
  vop.clear();
  //BAD!!!!!!obsm->clear();

  objects.clear();
  //BAD!!!!!!!!!!!!objm->clear();

  n= vtp.size();
  for (i=0; i<n; i++) delete vtp[i];
  vtp.clear();

  annotationStrings.clear();

  n= vap.size();
  for (i=0; i<n; i++) delete vap[i];
  vap.clear();
  if (apEditmessage) delete apEditmessage;
  apEditmessage= 0;

#ifdef DEBUGPRINT
  cerr << "++ Returning from PlotModule::cleanup ++" << endl;
#endif
}


void PlotModule::PixelArea(const Rectangle r){
  if (!plotw || !ploth) return;
  // full plot
  Rectangle fullr= splot.getPlotSize();
  // minus border etc..
  Rectangle plotr= splot.getMapSize();
  // map-area
  Area ma= splot.getMapArea();
//Rectangle mapr= ma.R();

#ifdef DEBUGPRINT
  cerr << "Plotw:" << plotw << endl;
  cerr << "Ploth:" << ploth << endl;
  cerr << "Selected rectangle:" << r << endl;
#endif

  // map to grid-coordinates
  Rectangle newr= fullr;
  newr.x1= fullr.x1 + fullr.width()*r.x1/float(plotw);
  newr.y1= fullr.y1 + fullr.height()*r.y1/float(ploth);
  newr.x2= fullr.x1 + fullr.width()*r.x2/float(plotw);
  newr.y2= fullr.y1 + fullr.height()*r.y2/float(ploth);

#ifdef DEBUGPRINT
  cerr << "Fullplotarea:" << fullr << endl;
  cerr << "plotarea:" << plotr << endl;
//cerr << "maparea:" << mapr << endl;
  cerr << "new maparea:" << newr << endl;
#endif

  // keep selection inside plotarea

//   newr.x1+= mapr.width()*r.x1/plotw;
//   newr.y1+= mapr.height()*r.y1/ploth;
//   newr.x2-= mapr.width()*(plotw-r.x2)/plotw;
//   newr.y2-= mapr.height()*(ploth-r.y2)/ploth;

  ma.setR(newr);
  splot.setMapArea(ma,keepcurrentarea);

  PlotAreaSetup();
}


void PlotModule::PhysToGeo(const float x, const float y,
			   float& lat, float& lon){
  if (mapdefined && plotw>0 && ploth>0){
    GridConverter gc;
    Area area= splot.getMapArea();

    Rectangle r= splot.getPlotSize();
    int npos=1;
    float gx= r.x1 + r.width()/plotw*x;
    float gy= r.y1 + r.height()/ploth*y;

    // convert point to correct projection
    gc.xy2geo(area,npos,&gx,&gy);

    lon= gx;
    lat= gy;
  }
}

void PlotModule::GeoToPhys(const float lat, const float lon,
			   float& x, float& y){
  if (mapdefined && plotw>0 && ploth>0){
    GridConverter gc;
    Area area= splot.getMapArea();

    Rectangle r= splot.getPlotSize();
    int npos=1;

    float yy = lat;
    float xx = lon;

    // convert point to correct projection
    gc.geo2xy(area,npos,&xx,&yy);

    x= (xx-r.x1)*plotw/r.width();
    y= (yy-r.y1)*ploth/r.height();

  }
}


void PlotModule::PhysToMap(const float x, const float y,
			   float& xmap, float& ymap){
  if (mapdefined && plotw>0 && ploth>0){
    Rectangle r= splot.getPlotSize();
    xmap= r.x1 + r.width()/plotw*x;
    ymap= r.y1 + r.height()/ploth*y;
  }
}


float PlotModule::GreatCircleDistance(float lat1, float lat2,
				      float lon1, float lon2)
{
  const float degToRad = 0.0174533; //conversion factor
  const float earthRadius =  6367000; //earth radius in meters
  lat1 = lat1*degToRad;
  lat2 = lat2*degToRad;
  lon1 = lon1*degToRad;
  lon2 = lon2*degToRad;
  float sinlat = sin((lat2-lat1)/2);
  float sinlon = sin((lon2-lon1)/2);
  float a = sinlat*sinlat + cos(lat1)*cos(lat2)*sinlon*sinlon;
  float c = 2*atan2(sqrt(a),sqrt(1-a));
  float dist = earthRadius*c;
  return dist;
}


// set managers
void PlotModule::setManagers(FieldManager* fm,
			     FieldPlotManager* fpm,
			     ObsManager* om,
			     SatManager* sm,
			     ObjectManager* obm,
			     EditManager* edm,
			     GridAreaManager* gam){
  fieldm= fm;
  fieldplotm= fpm;
  obsm=   om;
  satm=   sm;
  objm=   obm;
  editm=  edm;
  aream=  gam;

  if (!fieldm) cerr << "PlotModule::ERROR fieldmanager==0" << endl;
  if (!fieldplotm) cerr << "PlotModule::ERROR fieldplotmanager==0" << endl;
  if (!obsm)   cerr << "PlotModule::ERROR obsmanager==0" << endl;
  if (!satm)   cerr << "PlotModule::ERROR satmanager==0" << endl;
  if (!objm)   cerr << "PlotModule::ERROR objectmanager==0" << endl;
  if (!editm)  cerr << "PlotModule::ERROR editmanager==0" << endl;
  if (!aream)  cerr << "PlotModule::ERROR gridareamanager==0" << endl;
}


// return current plottime
void PlotModule::getPlotTime(miString& s){
  miTime t= splot.getTime();

  s= t.isoTime();
}


void PlotModule::getPlotTime(miTime& t){
  t= splot.getTime();
}


void PlotModule::getPlotTimes(vector<miTime>& fieldtimes,
			      vector<miTime>& sattimes,
			      vector<miTime>& obstimes,
			      vector<miTime>& objtimes,
			      vector<miTime>& ptimes)
{
  
  fieldtimes.clear();
  sattimes.clear();
  obstimes.clear();
  objtimes.clear();
  ptimes.clear();

  // edit product proper time
  if (prodtimedefined){
    ptimes.push_back(producttime);
  }

  vector<miString> pinfos;
  int n= vfp.size();
  for (int i=0; i<n; i++){
    pinfos.push_back(vfp[i]->getPlotInfo());
    cerr <<"Field plotinfo:"<<vfp[i]->getPlotInfo()<<endl;
  }
  if (pinfos.size()>0){
    bool constT;
    fieldtimes= fieldplotm->getFieldTime(pinfos,constT);
  }
#ifdef DEBUGPRINT
  cerr << "--- Found fieldtimes:" << endl;
  for (int i=0; i<fieldtimes.size(); i++)
    cerr << fieldtimes[i] << endl;
#endif

  n= vsp.size();
  pinfos.clear();
  for (int i=0; i<n; i++)
    pinfos.push_back(vsp[i]->getPlotInfo());
  if (pinfos.size()>0){
    sattimes= satm->getSatTimes(pinfos);
  }
#ifdef DEBUGPRINT
  cerr << "--- Found sattimes:" << endl;
  for (int i=0; i<sattimes.size(); i++)
    cerr << sattimes[i] << endl;
#endif

  n= vop.size();
  pinfos.clear();
  for (int i=0; i<n; i++)
    pinfos.push_back(vop[i]->getPlotInfo());
  if (pinfos.size()>0){
    obstimes= obsm->getObsTimes(pinfos);
  }
#ifdef DEBUGPRINT
  cerr << "--- Found obstimes:" << endl;
  for (int i=0; i<obstimes.size(); i++)
    cerr << obstimes[i] << endl;
#endif

  pinfos.clear();
  pinfos.push_back(objects.getPlotInfo());
  if (pinfos.size()>0){
    objtimes= objm->getObjectTimes(pinfos);
  }
#ifdef DEBUGPRINT
  cerr << "--- Found objtimes:" << endl;
  for (int i=0; i<objtimes.size(); i++)
    cerr << objtimes[i] << endl;
#endif


}


//returns union or intersection of plot times from all pinfos
void PlotModule::getCapabilitiesTime(set<miTime>& okTimes,
				     set<miTime>& constTimes,
				     const vector<miString>& pinfos,
				     bool allTimes)
{
  vector<miTime> normalTimes;
  miTime constTime;
  int timediff;
  int n = pinfos.size();
  bool normalTimesFound = false;
  bool moreTimes=true;
  for (int i=0; i<n; i++){
    vector<miString> tokens= pinfos[i].split(1);
    if (!tokens.empty()) {
      miString type= tokens[0].upcase();
      if(type=="FIELD") 
	fieldplotm->getCapabilitiesTime(normalTimes,constTime,timediff,pinfos[i]);
      else if (type=="SAT")      
	satm->getCapabilitiesTime(normalTimes,constTime,timediff,pinfos[i]);
      else if (type=="OBS")     
	obsm->getCapabilitiesTime(normalTimes,constTime,timediff,pinfos[i]);
      else if (type=="OBJECTS")	
	objm->getCapabilitiesTime(normalTimes,constTime,timediff,pinfos[i]);
    }

    if( !constTime.undef() ) {     //insert constTime
      
      cerr <<"constTime:"<<constTime.isoTime()<<endl;
      constTimes.insert(constTime);

    } else if (moreTimes) {     //insert okTimes

      if(( !normalTimesFound && normalTimes.size())) normalTimesFound=true;

      //if intersection and no common times, no need to look for more times
      if(( !allTimes && normalTimesFound && !normalTimes.size())) moreTimes=false;
      
      int nTimes = normalTimes.size();

      if(allTimes || okTimes.size()==0){ // union or first el. of intersection
	
	for (int k=0; k<nTimes; k++){
	  okTimes.insert(normalTimes[k]);
	}
	
      } else {  //intersection 
	
	set<miTime> tmptimes;
	set<miTime>::iterator p= okTimes.begin();
	for(; p!=okTimes.end(); p++){
	  int k=0;
	  while( k<nTimes && 
		 abs(miTime::minDiff(*p,normalTimes[k])) >timediff ) k++; 
	  if(k<nTimes) tmptimes.insert(*p); //time ok
	}
	okTimes = tmptimes;
      
      }
    } // if neither normalTimes nor constatTime, product is ignored
    normalTimes.clear();
    constTime = miTime();
  }

}

// set plottime
bool PlotModule::setPlotTime(miTime& t){
  splot.setTime(t);
  //  updatePlots();
  return true;
}


void PlotModule::updateObs()
{

  // Update ObsPlots if data files have changed

  //delete vobsTimes
  int nvop=vop.size();
  for( int i=0; i<nvop; i++)
    vop[i] = vobsTimes[0].vobsOneTime[i];

  int n= vobsTimes.size();
  for (int i=n-1; i>0; i--) {
    int m = vobsTimes[i].vobsOneTime.size();
    for( int j=0; j<m; j++)
      delete vobsTimes[i].vobsOneTime[j];
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }
  obsnr=0;


  // if time of current vop[0] != splot.getTime() or  files have changed,
  // read files from disk
  for (int i=0; i<nvop; i++){
    if( vop[i]->updateObs() ){
      if (!obsm->prepare(vop[i],splot.getTime()))
	cerr << "ObsManager returned false from prepare" << endl;
    }
  }


  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  // get annotations from all plots
  setAnnotations();

}


bool PlotModule::findObs(int x,int y)
{

  bool found= false;
  int n = vop.size();

  for (int i=0; i<n; i++)
    if (vop[i]->findObs(x,y))
      found= true;

  return found;
}

bool PlotModule::getObsName(int x,int y, miString& name)
{

  int n = vop.size();

  for (int i=0; i<n; i++)
    if (vop[i]->getObsName(x,y,name))
      return  true;

  return false;
}


//********** plotting and selecting stations on the map***************

void PlotModule::putStations(StationPlot* stationPlot){
#ifdef DEBUGPRINT
  cerr << "PlotModule::putStations"<< endl;
#endif

  miString name=stationPlot->getName();
  vector<StationPlot*>::iterator p=    stationPlots.begin();
  vector<StationPlot*>::iterator pend= stationPlots.end();

  //delete old stationPlot
  while (p!=pend && name!=(*p)->getName()) p++;
  if (p!=pend) {
    if(!((*p)->isVisible())) stationPlot->hide();
    stationPlot->enable((*p)->Enabled());
    delete (*p);
    stationPlots.erase(p);
  }

  //put new stationPlot into vector (sorted by priority and number of stations)
  p = stationPlots.begin();
  pend= stationPlots.end();

  int pri = stationPlot->getPriority();
  while(p!=pend && pri>(*p)->getPriority()) p++;
  if(p!=pend && pri == (*p)->getPriority())
    while(p!=pend && (*stationPlot)<(**p)) p++;
  stationPlots.insert(p,stationPlot);

  setAnnotations();
}


void PlotModule::makeStationPlot(const miString& commondesc,
				 const miString& common,
				 const miString& description,
				 int from,
				 const vector<miString>& data)
{
  StationPlot* stationPlot =
    new StationPlot(commondesc,common,description,from,data);
  putStations(stationPlot);

}


miString PlotModule::findStation(int x,int y, miString name, int id){

//################  if(dopanning || dorubberband ) return miString();

  int n=stationPlots.size();
  for (int i =0;i<n;i++){
    if ((id == -1 || id == stationPlots[i]->getId())
	&& (name==stationPlots[i]->getName()) ){
      vector<miString> st=stationPlots[i]->findStation(x,y);
      if(st.size()>0)
	return st[0];
    }
  }
  return miString();
}

void PlotModule::findStations(int x,int y, bool add, vector<miString>& name,
			      vector<int>& id, vector<miString>& station){

  int n=stationPlots.size();
  int ii;
  for (int i =0;i<n;i++){
    vector<miString> st = stationPlots[i]->findStation(x,y,add);
    if( (ii=stationPlots[i]->getId())>-1){
      for(int j=0; j<st.size();j++){
	name.push_back(stationPlots[i]->getName());
	id.push_back(ii);
	station.push_back(st[j]);
      }
    }
  }

}


void PlotModule::getEditStation(int step,miString& name,
				int& id, vector<miString>& stations){

  bool updateArea=false;
  int n=stationPlots.size();
  int i=0;
  while(i<n
	&& !stationPlots[i]->getEditStation(step,name,id,stations,updateArea))
    i++;

  if(updateArea)
    PlotAreaSetup();

}

void PlotModule::stationCommand(const miString& command,
			        vector<miString>& data,
			        const miString& name, int id,
			        const miString& misc)
{

  int n=stationPlots.size();
  if(command == "selected"){
    for (int i =0;i<n;i++){
      data.push_back(stationPlots[i]->stationRequest(command));
    }
  } else { // use stationPlot with name and id
    for (int i =0;i<n;i++){
      if ((id == -1 || id == stationPlots[i]->getId())
	  && (name==stationPlots[i]->getName() || !name.exists())){
	stationPlots[i]->stationCommand(command,data,misc);
	break;
      }
    }
  }

  if(command == "annotation")
    setAnnotations();

}

void PlotModule::stationCommand(const miString& command,
			        const miString& name, int id)
{
  if(command == "delete"){
    vector<StationPlot*>::iterator p=stationPlots.begin();

    while(p!=stationPlots.end()){
      if( (name == "all" && (*p)->getId() != -1) ||
	  (id == (*p)->getId() &&
	   (name==(*p)->getName() || !name.exists()))){
	delete (*p);
	stationPlots.erase(p);
      } else{
	p++;
      }
    }


  } else {
    int n=stationPlots.size();
    for (int i =0;i<n;i++){
      if ((id == -1 || id == stationPlots[i]->getId())
	  && (name==stationPlots[i]->getName() || !name.exists()) )
	stationPlots[i]->stationCommand(command);
    }
  }

  setAnnotations();

}


//areas
void PlotModule::makeAreas(miString name,miString areastring, int id){
  int n=vareaobjects.size();
//   cerr <<"makeAreas:"<<n<<endl;
//   cerr <<"name:"<<name<<endl;
//   cerr <<"areastring:"<<areastring<<endl;
 //name can be name:icon
  vector<miString> tokens = name.split(":");
  miString icon;
  if(tokens.size()>1){
    icon=tokens[1];
    name = tokens[0];
  }

 //check if dataset with this id/name already exist
  int i=0;
  while (i<n &&
	 (id!=vareaobjects[i].getId() || name!=vareaobjects[i].getName()))
    i++;

  if(i<n){    //add new areas and replace old areas
    vareaobjects[i].makeAreas(name,icon,areastring,id,splot.getMapArea());
    return;
  }

  //make new dataset
  AreaObjects new_areaobjects;
  new_areaobjects.makeAreas(name,icon,areastring,id,splot.getMapArea());
  vareaobjects.push_back(new_areaobjects);

}

void PlotModule::areaCommand(const miString& command, const miString& dataSet,
    const miString& data, int id)
{
  //   cerr << "PlotModule::areaCommand" << endl;
  //   cerr << "id=" << id << endl;
  //   cerr << "command=" << command << endl;
  //   cerr <<"data="<<data<<endl;

  int n = vareaobjects.size();
  for (int i = 0; i < n && i > -1; i++) {
    if ((id == -1 || id == vareaobjects[i].getId()) && (dataSet == "all"
        || dataSet == vareaobjects[i].getName()))
      if (command == "delete" && (data == "all" || !data.exists())) {
        vareaobjects.erase(vareaobjects.begin() + i);
        i--;
        n = vareaobjects.size();
      } else {
        vareaobjects[i].areaCommand(command, data);
        //zoom to selected area
        if (command == "select" && vareaobjects[i].autoZoom()) {
          vector<miString> token = data.split(":");
          if (token.size() == 2 && token[1] == "on") {
            zoomTo(vareaobjects[i].getBoundBox(token[0]));
          }
        }
      }
  }
}

vector <selectArea> PlotModule::findAreas(int x, int y, bool newArea){
  //cerr << "PlotModule::findAreas"  << x << " " << y << endl;
  float xm=0,ym=0;
  PhysToMap(x,y,xm,ym);
  vector <selectArea> vsA;
  int n=vareaobjects.size();
  for (int i=0; i<n; i++){
    if (!vareaobjects[i].isEnabled()) continue;
    vector <selectArea> sub_vsA;
    sub_vsA=vareaobjects[i].findAreas(xm,ym,newArea);
    vsA.insert(vsA.end(),sub_vsA.begin(),sub_vsA.end());
  }
  return vsA;
}

//********** plotting and selecting locationPlots on the map *************

void PlotModule::putLocation(const LocationData& locationdata)
{
#ifdef DEBUGPRINT
  cerr << "PlotModule::putLocation"<< endl;
#endif
  bool found= false;
  int n= locationPlots.size();
  miString name= locationdata.name;
  for (int i=0; i<n; i++){
    if (name==locationPlots[i]->getName()) {
      bool visible= locationPlots[i]->isVisible();
      delete locationPlots[i];
      locationPlots[i]= new LocationPlot();
      locationPlots[i]->setData(locationdata);
      if (!visible) locationPlots[i]->hide();
      found= true;
    }
  }
  if (!found) {
    int i= locationPlots.size();
    locationPlots.push_back(new LocationPlot());
    locationPlots[i]->setData(locationdata);
  }
  setAnnotations();
}


void PlotModule::updateLocation(const LocationData& locationdata)
{
#ifdef DEBUGPRINT
  cerr << "PlotModule::updateLocation"<< endl;
#endif
  int n= locationPlots.size();
  miString name= locationdata.name;
  for (int i=0; i<n; i++){
    if (name==locationPlots[i]->getName()) {
      locationPlots[i]->updateOptions(locationdata);
    }
  }
}


void PlotModule::deleteLocation(const miString& name)
{
  vector<LocationPlot*>::iterator p=    locationPlots.begin();
  vector<LocationPlot*>::iterator pend= locationPlots.end();

  while (p!=pend && name!=(*p)->getName()) p++;
  if (p!=pend) {
    delete (*p);
    locationPlots.erase(p);
    setAnnotations();
  }
}


void PlotModule::setSelectedLocation(const miString& name,
				     const miString& elementname)
{
  int n= locationPlots.size();
  for (int i=0; i<n; i++) {
    if (name==locationPlots[i]->getName())
      locationPlots[i]->setSelected(elementname);
  }
}


miString PlotModule::findLocation(int x, int y, const miString& name)
{

//################  if(dopanning || dorubberband ) return miString();

  int n= locationPlots.size();
  for (int i=0; i<n; i++) {
     if (name==locationPlots[i]->getName())
       return locationPlots[i]->find(x,y);
  }
  return miString();
}


//****************************************************

Colour PlotModule::getContrastColour(){

    Colour c(splot.getBgColour());
    int sum  = c.R() + c.G() + c.B();
    if(sum>255*3/2)
      c.set(0,0,0);
    else
      c.set(255,255,255);

    return  c;
}


void PlotModule::nextObs(bool next){

  int n = vop.size();
  for( int i=0;i<n; i++)
    vop[i]->nextObs(next);

}

void PlotModule::obsTime(const keyboardEvent& me, EventResult& res)
{
  // This function changes the observation time one hour,
  // and leaves the rest (fields, images etc.) unchanged.
  // It saves the obsPlot object in the vector vobsTimes.
  // This only works for vop[0], which is the only one used at the moment.
  // This only works in edit modus
  // vobsTimes is deleted when anything else are changed or edit modus are left

  if(vop.size()==0) return;
  if(!inEdit) return;

  if( obsnr == 0 && me.key==key_Right) return;
  if( obsnr > 20 && me.key==key_Left) return;

  obsm->clearObsPositions();

  miTime newTime =splot.getTime();
  if(me.key==key_Left){
    obsnr++;
  } else {
    obsnr--;
  }
  newTime.addHour(-1*obsTimeStep*obsnr);

  int nvop = vop.size();
  int n= vobsTimes.size();

  //log old stations
  for( int i=0; i<nvop; i++)
    vop[i]->logStations();

  //Make new obsPlot object
  if( obsnr == n ){

    obsOneTime ot;
    for( int i=0; i<nvop; i++){
      ObsPlot *op;
      op= new ObsPlot();
      miString pin = vop[i]->getInfoStr();
      if( !obsm->init(op,pin)){
	delete op;
	op = NULL;
      } else if (!obsm->prepare(op,newTime))
	cerr << "ObsManager returned false from prepare" << endl;

      ot.vobsOneTime.push_back(op);
    }
    vobsTimes.push_back(ot);

  } else {
    for( int i=0; i<nvop; i++)
      vobsTimes[obsnr].vobsOneTime[i]->readStations();
  }

    //ask last plot object which stations was plotted,
    //and tell this plot object
  for( int i=0; i<nvop; i++){
      vop[i]=vobsTimes[obsnr].vobsOneTime[i];
    }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  miString labelstr;
  if (obsnr != 0 ){
    miString timer(obsnr*obsTimeStep);
    labelstr = "LABEL text=\"OBS -" +timer;
    labelstr+= "\" tcolour=black bcolour=red fcolour=red:150 ";
    labelstr+= "polystyle=both halign=center valign=top fontsize=18";
  }
  if(vop.size()>0){
    vop[0]->setLabel(labelstr);
  }

  setAnnotations();

}

void PlotModule::obsStepChanged(int step){

  obsTimeStep = step;

  int n=vop.size();
  for(int i=0; i<n; i++)
    vop[i]=vobsTimes[0].vobsOneTime[i];

  int m = vobsTimes.size();
  for(int i=m-1; i>0; i--){
    int l = vobsTimes[i].vobsOneTime.size();
    for( int j=0;j<l; j++){
      delete vobsTimes[i].vobsOneTime[j];
      vobsTimes[i].vobsOneTime.pop_back();
    }
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }

  if(obsnr > 0) obsnr=0;

  setAnnotations();

}

void PlotModule::trajPos(vector<miString>& vstr)
{  
  int n = vtp.size();

  //if vstr starts with "quit", delete all trajectoryPlot objects
  int m = vstr.size();
  for(int j=0;j<m;j++){
    if( vstr[j].substr(0,4) == "quit"){
      for (int i=0; i<n; i++) delete vtp[i];
      vtp.clear();
      setAnnotations();  // will remove tarjectoryPlot annotation
      return;
    }
  }

  //if no trajectoryPlot object, make one
  if(n==0)
    vtp.push_back(new TrajectoryPlot());

  //there are never more than one trajectoryPlot object (so far..)
  int action= vtp[0]->trajPos(vstr);

  if (action==1) {
    // trajectories cleared, reset annotations
    setAnnotations();  // will remove tarjectoryPlot annotation
  } 
}

void PlotModule::radePos(vector<miString>& vstr)
{      
  int n = vrp.size();

  //if vstr starts with "quit", delete all trajectoryPlot objects
  int m = vstr.size();
  for(int j=0;j<m;j++){
    if( vstr[j].substr(0,4) == "quit"){
      for (int i=0; i<n; i++) delete vrp[i];
      vrp.clear();
      setAnnotations();  // will remove tarjectoryPlot annotation
      return;
    }
  }

  //if no trajectoryPlot object, make one
  if(n==0)
    vrp.push_back(new RadarEchoPlot());

  //there are never more than one trajectoryPlot object (so far..)
  int action= vrp[0]->radePos(vstr);

  if (action==1) {
    // trajectories cleared, reset annotations
    setAnnotations();  // will remove tarjectoryPlot annotation
  }
}


vector<miString> PlotModule::getCalibChannels()
{

  vector<miString> channels;

  int n = vsp.size();
  for( int i=0; i<n; i++){
    if( vsp[i]->Enabled())
      vsp[i]->getCalibChannels(channels); //add channels
  } 

  return channels;
}

vector<SatValues> PlotModule::showValues(int x,int y){

// return values of current channels (with calibration) 

  int n = vsp.size();
  vector<SatValues> satval;

  for( int i=0; i<n; i++){
    if( vsp[i]->Enabled()){
      vsp[i]->values(x,y,satval);
    }
  }

  return satval;

}

vector <miString> PlotModule::getSatnames(){
  vector <miString> satnames;
  miString str;
  // get sat names
  int m= vsp.size();
  for (int j=0; j<m; j++){
    vsp[j]->getSatName(str);
    if (!str.empty()) satnames.push_back(str);
  }
  return satnames;
}


bool PlotModule::markAnnotationPlot(int x, int y){
  int m= editVap.size();
  bool marked=false;
  for (int j=0; j<m; j++)
    if (editVap[j]->markAnnotationPlot(x,y))
      marked=true;
  return marked;
}

miString PlotModule::getMarkedAnnotation(){
  int m= editVap.size();
  miString annotext;
  for (int j=0; j<m; j++){
    miString text=editVap[j]->getMarkedAnnotation();
    if (!text.empty()) annotext=text;
  }
  return annotext;
}

void PlotModule::changeMarkedAnnotation(miString text, int cursor,
					int sel1, int sel2)
{
  int m= editVap.size();
  for (int j=0; j<m; j++)
   editVap[j]->changeMarkedAnnotation(text,cursor,sel1,sel2);
}

void PlotModule::DeleteMarkedAnnotation()
{
  int m= editVap.size();
  for (int j=0; j<m; j++)
    editVap[j]->DeleteMarkedAnnotation();
}



void PlotModule::startEditAnnotation(){
  int m= editVap.size();
  for (int j=0; j<m; j++)
    editVap[j]->startEditAnnotation();
}


void PlotModule::stopEditAnnotation(){
  int m= editVap.size();
  for (int j=0; j<m; j++)
    editVap[j]->stopEditAnnotation();
}

void PlotModule::editNextAnnoElement(){
  int m= editVap.size();
  for (int j=0; j<m; j++){
    editVap[j]->editNextAnnoElement();
  }
}


void PlotModule::editLastAnnoElement(){
  int m= editVap.size();
  for (int j=0; j<m; j++){
    editVap[j]->editLastAnnoElement();
  }
}


vector <miString> PlotModule::writeAnnotations(miString prodname){
  vector <miString> annostrings;
  int m= editVap.size();
  for (int j=0; j<m; j++){
    miString str=editVap[j]->writeAnnotation(prodname);
    if (!str.empty())
      annostrings.push_back(str);
  }
  return annostrings;
}


void PlotModule::updateEditLabels(vector <miString> productLabelstrings,
			          miString productName, bool newProduct){
  cerr <<"diPlotModule::updateEditLabels"<<endl;
  int n;
  vector <AnnotationPlot*> oldVap; //display object labels
  //read the old labels...

  vector <miString> objLabelstrings=editobjects.getObjectLabels();
  n= objLabelstrings.size();
  for (int i = 0; i<n; i++){
    AnnotationPlot* ap = new AnnotationPlot(objLabelstrings[i]);
    oldVap.push_back(ap);
  }

  int m = productLabelstrings.size();
  n= oldVap.size();
  for (int j=0; j<m; j++){
    AnnotationPlot* ap = new AnnotationPlot(productLabelstrings[j]);
    ap->setProductName(productName);


    vector< vector<miString> > vvstr = ap->getAnnotationStrings();
    int nn=vvstr.size();
    for (int k=0; k<nn; k++){
      int mm= vfp.size();
      for (int j=0; j<mm; j++){
	vfp[j]->getAnnotations(vvstr[k]);
      }
    }
    ap->setAnnotationStrings(vvstr);
    
    // here we compare the labels, take input from oldVap
    for (int i = 0;i<n;i++)
      ap->updateInputLabels(oldVap[i],newProduct);
    
    
    editVap.push_back(ap);
  }

  for (int i = 0;i<n;i++)
    delete oldVap[i];
  oldVap.clear();
}




//autoFile
void PlotModule::setSatAuto(bool autoFile,const miString& satellite,
			    const miString& file){
  int m= vsp.size();
  for (int j=0; j<m; j++)
    vsp[j]->setSatAuto(autoFile,satellite,file);
}


void PlotModule::setObjAuto(bool autoF){
  objects.autoFile=autoF;
}




void PlotModule::areaInsert(Area a, bool newArea){

  if(newArea && areaSaved){
    areaSaved=false;
    return;
    }

  areaQ.erase(areaQ.begin()+areaIndex+1,areaQ.end());

  if(areaQ.size() > 20)
    areaQ.pop_front();
  else
    areaIndex++;

  areaQ.push_back(a);

}


void PlotModule::changeArea(const keyboardEvent& me){

  Area a;
  MapManager mapm;

  // define your own area
  if(me.key==key_F2 && me.modifier==key_Shift){
    myArea=splot.getMapArea();
    return;
  }

  if (me.key==key_F3 || me.key==key_F4){ // go to previous or next area
    //if last area is not saved, save it
    if(!areaSaved){
      areaInsert(splot.getMapArea(),false);
      areaSaved=true;
    }
    if (me.key==key_F3){            // go to previous area
      if(areaIndex < 1)
	return;
      areaIndex--;
      a=areaQ[areaIndex];

    } else if (me.key==key_F4){  //go to next area
      if(areaIndex+2 > areaQ.size())
	return;
      areaIndex++;
      a=areaQ[areaIndex];
    }

  } else if (me.key==key_F2){ //get your own area
    areaInsert(splot.getMapArea(),true);//save last area
    a = myArea;
  } else if (me.key==key_F5){          //get predefined areas
    areaInsert(splot.getMapArea(),true);
    mapm.getMapAreaByFkey("F5",a);
  } else if (me.key==key_F6){
    areaInsert(splot.getMapArea(),true);
    mapm.getMapAreaByFkey("F6",a);
  } else if (me.key==key_F7){
    areaInsert(splot.getMapArea(),true);
    mapm.getMapAreaByFkey("F7",a);
  } else if (me.key==key_F8){
    areaInsert(splot.getMapArea(),true);
    mapm.getMapAreaByFkey("F8",a);
  }

  Area temp = splot.getMapArea();
  if (temp.P() == a.P()){
    splot.setMapArea(a,keepcurrentarea);
    PlotAreaSetup();
  }
  else {  //if projection has changed, updatePlots must be called
    splot.setMapArea(a,keepcurrentarea);

    //If satellitte, use satellitte projection
    int n= vsp.size();
    if(n>0){
      Area satarea=vsp[0]->getSatArea();
      a= splot.findBestMatch(satarea);
      splot.setMapArea(a,keepcurrentarea);
    }
    PlotAreaSetup();
    updatePlots();
  }
}

void PlotModule::zoomTo(const Rectangle& rectangle) {
  Area a = splot.getMapArea();
  a.setR(rectangle);
  splot.setMapArea(a, false);
  PlotAreaSetup();
  updatePlots();
}

void PlotModule::zoomOut(){
  float scale = 1.3;
  int wd= int((plotw*scale-plotw)/2.);
  int hd= int((ploth*scale-ploth)/2.);
  int x1= -wd;
  int y1= -hd;
  int x2= plotw+wd;
  int y2= ploth+hd;
  //define new plotarea, first save the old one
  areaInsert(splot.getMapArea(),true);
  Rectangle r(x1,y1,x2,y2);
  PixelArea(r);
}



// keyboard/mouse events
void PlotModule::sendMouseEvent(const mouseEvent& me, EventResult& res)
{
  newx= me.x;
  newy= me.y;

  // ** mousepress
  if (me.type == mousepress){
    oldx= me.x;
    oldy= me.y;

    if (me.button == leftButton){
      if(aream) dorubberband = !aream->overrideMouseEvent;
      else dorubberband = true;
      res.savebackground= true;
      res.background= true;
      res.repaint= true;

      return;

    } else if (me.button == midButton){
      areaInsert(splot.getMapArea(),true); // Save last area
      dopanning= true;
      splot.panPlot(true);
      return;
    }

    else if (me.button ==rightButton)
      {
	res.action= rightclick;
      }


    return;
  }
  // ** mousemove
  else if (me.type == mousemove){

    res.action= browsing;

      if (dorubberband){
      res.action= quick_browsing;
      res.background= false;
      res.repaint= true;
      return;

    } else if (dopanning) {
      int x1,y1,x2,y2;
      int wd= me.x - oldx;
      int hd= me.y - oldy;
      x1= -wd; y1=-hd;
      x2= plotw - wd;
      y2= ploth - hd;

      Rectangle r(x1,y1,x2,y2);
      PixelArea(r);
      oldx= me.x; oldy= me.y;

      res.action= quick_browsing;
      res.background= true;
      res.repaint= true;
      return;
    }

  }
  // ** mouserelease
  else if (me.type == mouserelease){

    bool plotnew = false;

    res.savebackground= false;

    int x1=0,y1=0,x2=0,y2=0;
    // minimum rubberband size for zooming (in pixels)
    const float rubberlimit= 15.;


    if (me.button == rightButton){ // zoom out

      //end of popup
      //res.action= rightclick;

    } else if (me.button == leftButton){


	x1=oldx;
	y1=oldy;
	x2=me.x;
	y2=me.y;

	if (oldx>x2){
	  x1= x2;
	  x2= oldx;
	}
	if (oldy>y2){
	  y1= y2;
	  y2= oldy;
	}
	if (fabsf(x2-x1)>rubberlimit && fabsf(y2-y1)>rubberlimit){
	  if (dorubberband)
	    plotnew = true;
	}
	else
	  res.action= pointclick;
// 	moveClassified =false;
	dorubberband= false;

    } else if (me.button == midButton){
      dopanning= false;
      splot.panPlot(false);
      res.repaint= true;
      res.background= true;
      return;
    }
    if (plotnew){
      //define new plotarea, first save the old one
      areaInsert(splot.getMapArea(),true);
      Rectangle r(x1,y1,x2,y2);
      PixelArea(r);
      res.repaint= true;
      res.background= true;
    }

    return;
  }
  // ** mousedoubleclick
  else if (me.type == mousedoubleclick){
  }
}


void PlotModule::sendKeyboardEvent(const keyboardEvent& me, EventResult& res)
{
  static int arrowKeyDirection= 1;

  int dx=0, dy=0;
  float zoom=0.;

  if (me.type==keypress){

    if (me.key==key_Home) {
      keepcurrentarea= false;
      updatePlots();
      keepcurrentarea= true;
      return;
    }

    if (me.key==key_R) {
      if (arrowKeyDirection>0) arrowKeyDirection= -1;
      else                     arrowKeyDirection=  1;
      return;
    }

    if      (me.key==key_Left)  dx= -plotw/8;
    else if (me.key==key_Right) dx=  plotw/8;
    else if (me.key==key_Down)  dy= -ploth/8;
    else if (me.key==key_Up)    dy=  ploth/8;
//     else if (me.key==key_A)     dx= -plotw/8;
//     else if (me.key==key_D)     dx=  plotw/8;
//     else if (me.key==key_S)     dy= -ploth/8;
//     else if (me.key==key_W)     dy=  ploth/8;
    else if (me.key==key_Z && me.modifier==key_Shift) zoom=  1.3;
    else if (me.key==key_Z)     zoom=  1./1.3;
    else if (me.key==key_X)     zoom=  1.3;

    if (dx!=0 || dy!=0){
      dx*=arrowKeyDirection;
      dy*=arrowKeyDirection;
      //define new plotarea, first save the old one
      areaInsert(splot.getMapArea(),true);
      Rectangle r(dx,dy,plotw+dx,ploth+dy);
      PixelArea(r);
    } else if (zoom>0.){
      //define new plotarea, first save the old one
      areaInsert(splot.getMapArea(),true);
      dx= plotw - int(plotw*zoom);
      dy= ploth - int(ploth*zoom);
      Rectangle r(dx,dy,plotw-dx,ploth-dy);
      PixelArea(r);
    }

  }

}


void PlotModule::setEditMessage(const miString& str) {

  // set or remove (if empty string) an edit message

  if (apEditmessage) {
    delete apEditmessage;
    apEditmessage= 0;
  }

  if (str.exists()) {
    miString labelstr;
    labelstr = "LABEL text=\"" + str + "\"";
    labelstr+= " tcolour=blue bcolour=red fcolour=red:128";
    labelstr+= " polystyle=both halign=left valign=top";
    labelstr+= " xoffset=0.01 yoffset=0.1 fontsize=30";
    apEditmessage= new AnnotationPlot();
    if (!apEditmessage->prepare(labelstr)){
      delete apEditmessage;
      apEditmessage= 0;
    }
  }
}


vector<miString> PlotModule::getFieldModels()
{
  vector<miString> vstr;
  set<miString> unique;
  int n= vfp.size();
  for (int i=0; i<n; i++) {
    miString fname= vfp[i]->getModelName();
    if (fname.exists())
      unique.insert(fname);
  }
  set<miString>::iterator p=unique.begin(), pend=unique.end();
  for (; p!=pend; p++)
    vstr.push_back(*p);

  return vstr;
}


vector<miString> PlotModule::getTrajectoryFields()
{
  vector<miString> vstr;
  int n= vfp.size();
  for (int i=0; i<n; i++) {
    miString fname= vfp[i]->getTrajectoryFieldName();
    if (fname.exists())
      vstr.push_back(fname);
  }
  return vstr;
}

vector<miString> PlotModule::getRadarEchoFields()
{
  vector<miString> vstr;
  int n= vfp.size();
  for (int i=0; i<n; i++) {
    miString fname= vfp[i]->getRadarEchoFieldName();
    if (fname.exists())
      vstr.push_back(fname);
  }
  return vstr;
}


bool PlotModule::startTrajectoryComputation()
{
  if (vtp.size()<1)
    return false;

  miString fieldname= vtp[0]->getFieldName();

  int i=0, n= vfp.size();

  while (i<n && vfp[i]->getTrajectoryFieldName()!=fieldname) i++;
  if (i==n)
    return false;

  vector<Field*> vf= vfp[i]->getFields();
  if (vf.size()<2)
    return false;

  return vtp[0]->startComputation(vf);
}


void PlotModule::stopTrajectoryComputation()
{
  if (vtp.size()>0)
    vtp[0]->stopComputation();
}

// write trajectory positions to file
bool PlotModule::printTrajectoryPositions(const miString& filename )
{
  if (vtp.size()>0)
    return  vtp[0]->printTrajectoryPositions( filename );
}

/********************* reading and writing log file *******************/

vector<miString> PlotModule::writeLog(){

  //put last area in areaQ
  areaInsert(splot.getMapArea(),true);

  vector<miString> vstr;
  const int speclen=Projection::speclen;
  float gs[speclen];

  //Write self-defined area (F2)
  ostringstream ost;
  myArea.P().Gridspecstd(gs);
  ost << "name=F2";
  ost << ", proj=" << myArea.P().Gridtype();
  ost << ", grid=";
  for( int j=0; j<speclen-1; j++)
    ost << gs[j]<<":";
  if(speclen>0)
    ost << gs[speclen-1];

  //+1 to make coordinates  "fortran indexed"
  ost << ", area=";
  ost <<myArea.R().x1+1<<":";
  ost <<myArea.R().x2+1<<":";
  ost <<myArea.R().y1+1<<":";
  ost <<myArea.R().y2+1;
  vstr.push_back(ost.str());

  //Write all araes in list (areaQ)
  int n=areaQ.size();
  for(int i=0; i<n; i++){
    ostringstream ostr;
    areaQ[i].P().Gridspecstd(gs);
    ostr << "name="<<i;
    ostr << ", proj=" << areaQ[i].P().Gridtype();
    ostr << ", grid=";
    for( int j=0; j<speclen-1; j++)
      ostr << gs[j]<<":";
    if(speclen>0)
      ostr << gs[speclen-1];

    // +1 to make coordinates  "fortran indexed"
    ostr << ", area=";
    ostr <<areaQ[i].R().x1+1<<":";
    ostr <<areaQ[i].R().x2+1<<":";
    ostr <<areaQ[i].R().y1+1<<":";
    ostr <<areaQ[i].R().y2+1;
    vstr.push_back(ostr.str());

  }
  return vstr;
}

void PlotModule::readLog(const vector<miString>& vstr,
			 const miString& thisVersion,
			 const miString& logVersion)
{
  const miString key_name= "name";
  const miString key_proj= "proj";
  const miString key_grid= "grid";
  const miString key_area= "area";

  vector<miString> tokens,stokens,sstokens;
  miString key,value,name;
  int i,j,k,l,m,n,o;
  Area area;
  Projection proj;
  Rectangle rect;
  int projtype;
  float gridspec[Projection::speclen];
  float x1,y1,x2,y2;

  areaQ.clear();

  n= vstr.size();
  for (i=0; i<n; i++){
    projtype= Projection::undefined_projection;
    for (l=0; l<Projection::speclen; l++) gridspec[l]=0;
    name= "";
    x1=y1=x2=y2=0;

    tokens= vstr[i].split(',');
    m= tokens.size();
    for (j=0; j<m; j++){
      stokens= tokens[j].split('=');
      o= stokens.size();
      if (o>1) {
	key= stokens[0].downcase();
	value= stokens[1];

	if (key==key_name){
	  name= value;

	} else if (key==key_proj){
	  projtype=atoi(value.c_str());

	} else if (key==key_grid){
	  sstokens= value.split(':');
	  k= sstokens.size();
	  if (k<4) continue;
	  for (l=0; l<k; l++) gridspec[l]= atof(sstokens[l].cStr());

	} else if (key==key_area){
	  sstokens= value.split(':');
	  k= sstokens.size();
	  if (k<4) continue;
	  x1= atof(sstokens[0].cStr());
	  x2= atof(sstokens[1].cStr());
	  y1= atof(sstokens[2].cStr());
	  y2= atof(sstokens[3].cStr());
	  // assuming that coordinates are "fortran indexed"
	  // (as gridspec, gridspec changed when defining Projection)
	  x1-=1.;
	  x2-=1.;
	  y1-=1.;
	  y2-=1.;

	}
      }
    }
    if (name.exists()){
      rect = Rectangle(x1,y1,x2,y2);
      proj = Projection(projtype, gridspec);
      area = Area(name,proj,rect);
      if( name=="F2")
	myArea=area;
      else
	areaQ.push_back(area);
    }
  }

  areaIndex=areaQ.size()-1;
}
