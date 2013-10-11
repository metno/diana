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

#define MILOGGER_CATEGORY "diana.PlotModule"
#include <miLogger/miLogging.h>

#include <diPlotModule.h>
#include <diObsPlot.h>

#include <diFieldPlot.h>
#include <diSatPlot.h>
#include <diMapPlot.h>
#include <diTrajectoryPlot.h>
#include <diMeasurementsPlot.h>

#include <diObsManager.h>
#include <diSatManager.h>
#include <diStationManager.h>
#include <diObjectManager.h>
#include <diEditManager.h>
#include <diGridAreaManager.h>
#include <diAnnotationPlot.h>
#include <diWeatherArea.h>
#include <diStationPlot.h>
#include <diMapManager.h>
#include <diManager.h>

#include <diField/diFieldManager.h>
#include <diField/FieldSpecTranslation.h>
#include <diFieldPlotManager.h>
#include <puDatatypes/miCoordinates.h>

#include <GL/gl.h>
#include <sstream>

#include <QKeyEvent>
#include <QMouseEvent>
//#define DEBUGPRINT
//#define DEBUGREDRAW

using namespace miutil;
using namespace milogger;

// static class members
GridConverter PlotModule::gc; // Projection-converter

PlotModule *PlotModule::self = 0;

// Default constructor
PlotModule::PlotModule() :
           apEditmessage(0),plotw(0.),ploth(0.),resizezoom(true),
           showanno(true),hardcopy(false),bgcolourname("midnightBlue"), inEdit(false),
           mapmode(normal_mode), prodtimedefined(false),dorubberband(false),
           dopanning(false), keepcurrentarea(true), obsnr(0)
{
  self = this;
  oldx = newx = oldy = newy = 0;
  mapdefined = false;
  mapDefinedByUser = false;
  mapDefinedByData = false;
  mapDefinedByView = false;

  // used to detect map area changes
  Projection p;
  Rectangle r(0., 0., 0., 0.);
  previousrequestedarea = Area(p, r);
  requestedarea = Area(p, r);
  splot.setRequestedarea(requestedarea);
  areaIndex = -1;
  areaSaved = false;
}

// Destructor
PlotModule::~PlotModule()
{
  cleanup();
}

void PlotModule::preparePlots(const vector<miString>& vpi)
{
  METLIBS_LOG_DEBUG("++ PlotModule::preparePlots ++");
  // reset flags
  mapDefinedByUser = false;

  levelSpecified.clear();
  levelCurrent.clear();
  idnumSpecified.clear();
  idnumCurrent.clear();

  // split up input into separate products
  vector<miString> fieldpi, obspi, areapi, mappi, satpi, statpi, objectpi, trajectorypi,
  labelpi, editfieldpi;

  int n = vpi.size();
  // merge PlotInfo's for same type
  for (int i = 0; i < n; i++) {
    vector<miString> tokens = vpi[i].split(1);
    if (!tokens.empty()) {
      miString type = tokens[0].upcase();
      METLIBS_LOG_INFO(vpi[i]);
      if (type == "FIELD")
        fieldpi.push_back(vpi[i]);
      else if (type == "OBS")
        obspi.push_back(vpi[i]);
      else if (type == "MAP")
        mappi.push_back(vpi[i]);
      else if (type == "AREA")
        areapi.push_back(vpi[i]);
      else if (type == "SAT")
        satpi.push_back(vpi[i]);
      else if (type == "STATION")
        statpi.push_back(vpi[i]);
      else if (type == "OBJECTS")
        objectpi.push_back(vpi[i]);
      else if (type == "TRAJECTORY")
        trajectorypi.push_back(vpi[i]);
      else if (type == "LABEL")
        labelpi.push_back(vpi[i]);
      else if (type == "EDITFIELD")
        editfieldpi.push_back(vpi[i]);
    }
  }

  // call prepare methods
  prepareArea(areapi);
  prepareMap(mappi);
  prepareFields(fieldpi);
  prepareObs(obspi);
  prepareSat(satpi);
  prepareStations(statpi);
  prepareObjects(objectpi);
  prepareTrajectory(trajectorypi);
  prepareAnnotation(labelpi);

  if (inEdit & editfieldpi.size()) {
    std::string plotName;
    vector<FieldRequest> vfieldrequest;
    fieldplotm->parsePin(editfieldpi[0],vfieldrequest,plotName);
    editm->prepareEditFields(plotName,editfieldpi);
  }

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ Returning from PlotModule::preparePlots ++");
#endif
}

void PlotModule::prepareArea(const vector<miutil::miString>& inp)
{
  MI_LOG & log = MI_LOG::getInstance("diana.PlotModule.prepareArea");
  log.debugStream() << "++ PlotModule::prepareArea ++";

  MapManager mapm;

  if ( !inp.size() ) return;
  if ( inp.size() > 1 )
    COMMON_LOG::getInstance("common").debugStream() << "More AREA definitions, using: " <<inp[0];

  const miString key_name=  "name";
  const miString key_areaname=  "areaname"; //old syntax
  const miString key_proj=  "proj4string";
  const miString key_rectangle=  "rectangle";
  const miString key_xypart=  "xypart";

  Projection proj;
  Rectangle rect;

  vector<miString> tokens= inp[0].split('"','"'," ",true);

  int n = tokens.size();
  for (int i=0; i<n; i++){
    vector<miString> stokens= tokens[i].split(1,'=');
    if (stokens.size() > 1) {
      miString key= stokens[0].downcase();

      if (key==key_name || key==key_areaname){
        if ( !mapm.getMapAreaByName(stokens[1], requestedarea) ) {
          COMMON_LOG::getInstance("common").warnStream() << "Unknown AREA definition: "<< inp[0];
        }
      } else if (key==key_proj){
        if ( proj.set_proj_definition(stokens[1]) ) {
          requestedarea.setP(proj);
        } else {
          COMMON_LOG::getInstance("common").warnStream() << "Unknown proj definition: "<< stokens[1];
        }
      } else if (key==key_rectangle){
        if ( rect.setRectangle(stokens[1],false) ) {
          requestedarea.setR(rect);
        } else {
          COMMON_LOG::getInstance("common").warnStream() << "Unknown rectangle definition: "<< stokens[1];
        }
      }
    }
  }


}

void PlotModule::prepareMap(const vector<miString>& inp)
{
  METLIBS_LOG_DEBUG("++ PlotModule::prepareMap ++");

  splot.xyClear();

  // init inuse array
  vector<bool> inuse;
  int nm = vmp.size();
  if (nm > 0)
    inuse.insert(inuse.begin(), nm, false);

  int n = inp.size();

  // keep requested areas
  Area rarea = requestedarea;
  bool arearequested = requestedarea.P().isDefined();

  bool isok;
  for (int k = 0; k < n; k++) { // loop through all plotinfo's
    isok = false;
    if (nm > 0) { // mapPlots exists
      for (int j = 0; j < nm; j++) {
        if (!inuse[j]) { // not already taken
          if (vmp[j]->prepare(inp[k], rarea, true)) {
            inuse[j] = true;
            isok = true;
            arearequested |= vmp[j]->requestedArea(rarea);
            vmp.push_back(vmp[j]);
            break;
          }
        }
      }
    }
    if (isok)
      continue;

    // make new mapPlot object and push it on the list
    int nnm = vmp.size();
    MapPlot *mp;
    vmp.push_back(mp);
    vmp[nnm] = new MapPlot();
    if (!vmp[nnm]->prepare(inp[k], rarea, false)) {
      delete vmp[nnm];
      vmp.pop_back();
    } else {
      arearequested |= vmp[nnm]->requestedArea(rarea);
    }
  } // end plotinfo loop

  // delete unwanted mapplots
  if (nm > 0) {
    for (int i = 0; i < nm; i++) {
      if (!inuse[i]) {
        delete vmp[i];
      }
    }
    vmp.erase(vmp.begin(), vmp.begin() + nm);
  }

  // remove filled maps not used (free memory)
  if (MapPlot::checkFiles(true)) {
    int n = vmp.size();
    for (int i = 0; i < n; i++)
      vmp[i]->markFiles();
    MapPlot::checkFiles(false);
  }

  // check area
  if (!mapDefinedByUser && arearequested) {
    mapDefinedByUser = (rarea.P().isDefined());
    requestedarea = rarea;
    splot.setRequestedarea(requestedarea);
  }
}

void PlotModule::prepareFields(const vector<miString>& inp)
{
  METLIBS_LOG_DEBUG("++ PlotModule::prepareFields ++");

  int npi = inp.size();

  miString str;
  map<miString, bool> plotenabled;

  // for now -- erase all fieldplots
  for (unsigned int i = 0; i < vfp.size(); i++) {
    // keep enable flag
    str = vfp[i]->getPlotInfo("model,plot,parameter,reftime");
    plotenabled[str] = vfp[i]->Enabled();
    // free old fields
    freeFields(vfp[i]);
    delete vfp[i];
  }
  vfp.clear();

  int n;
  for (int i = 0; i < npi; i++) {
    FieldPlot *fp;
    n = vfp.size();
    vfp.push_back(fp);
    vfp[n] = new FieldPlot();

    std::string plotName;
    vector<FieldRequest> vfieldrequest;
    std::string inpstr = std::string(inp[i]);
    fieldplotm->parsePin(inpstr,vfieldrequest,plotName);
    if (!vfp[n]->prepare(plotName, inp[i])) {
      delete vfp[n];
      vfp.pop_back();
    } else {
      str = vfp[n]->getPlotInfo("model,plot,parameter,reftime");
      if (plotenabled.count(str))
        vfp[n]->enable(plotenabled[str]);
      else
        vfp[n]->enable(vfp[n]->Enabled());


    }
  }

  METLIBS_LOG_DEBUG("++ Returning from PlotModule::prepareFields ++");
}

void PlotModule::prepareObs(const vector<miString>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule::prepareObs ++");
#endif

  int npi = inp.size();

  // keep enable flag
  miString str;
  map<miString, bool> plotenabled;
  for (unsigned int i = 0; i < vop.size(); i++) {
    str = vop[i]->getPlotInfo(3);
    plotenabled[str] = vop[i]->Enabled();
  }

  // for now -- erase all obsplots etc..
  //first log stations plotted
  int nvop = vop.size();
  for (int i = 0; i < nvop; i++)
    vop[i]->logStations();
  for (unsigned int i = 0; i < vobsTimes.size(); i++) {
    for (unsigned int j = 0; j < vobsTimes[i].vobsOneTime.size(); j++)
      delete vobsTimes[i].vobsOneTime[j];
    vobsTimes[i].vobsOneTime.clear();
  }
  vobsTimes.clear();

  //   for (int i=1; i<vop.size(); i++) // vop[0] has already been deleted (vobsTimes)
  //     delete vop[i];
  vop.clear();

  obsnr = 0;
  int n;
  ObsPlot *op;
  for (int i = 0; i < npi; i++) {
    n = vop.size();
    vop.push_back(op);
    vop[n] = new ObsPlot();
    if (!obsm->init(vop[n], inp[i])) {
      delete vop[n];
      vop.pop_back();
    } else {
      str = vop[n]->getPlotInfo(3);
      if (plotenabled.count(str) == 0)
        plotenabled[str] = true;
      vop[n]->enable(plotenabled[str] && vop[n]->Enabled());

      if (vobsTimes.size() == 0) {
        obsOneTime ot;
        vobsTimes.push_back(ot);
      }

      vobsTimes[0].vobsOneTime.push_back(op);
      vobsTimes[0].vobsOneTime[n] = vop[i];//forsiktig!!!!

    }
  }
  obsnr = 0;

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ Returning from PlotModule::prepareObs ++");
#endif
}

void PlotModule::prepareSat(const vector<miString>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule::prepareSat ++");
#endif

  // keep enable flag
  miString str;
  map<miString, bool> plotenabled;
  for (unsigned int i = 0; i < vsp.size(); i++) {
    str = vsp[i]->getPlotInfo(4);
    plotenabled[str] = vsp[i]->Enabled();
  }

  if (!satm->init(vsp, inp)) {
    METLIBS_LOG_WARN("PlotModule::prepareSat.  init returned false");
  }

  for (unsigned int i = 0; i < vsp.size(); i++) {
    str = vsp[i]->getPlotInfo(4);
    if (plotenabled.count(str) == 0)
      plotenabled[str] = true;
    vsp[i]->enable(plotenabled[str] && vsp[i]->Enabled());
  }
}

void PlotModule::prepareStations(const vector<miString>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule::prepareStations ++");
#endif

  if (!stam->init(inp)) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("PlotModule::prepareStations.  init returned false");
#endif
  }
}

void PlotModule::prepareAnnotation(const vector<miString>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule::prepareAnnotation ++");
#endif

  // for now -- erase all annotationplots
  int n = vap.size();
  for (int i = 0; i < n; i++)
  {
    if (vap[i] != 0)
      delete vap[i];
  }
  vap.clear();

  if (inp.size() == 0)
    return;

  annotationStrings = inp;
}

void PlotModule::prepareObjects(const vector<miString>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule::prepareObjects ++");
#endif

  int npi = inp.size();

  miString str;
  map<miString, bool> plotenabled;

  // keep enable flag
  str = objects.getPlotInfo();
  plotenabled[str] = objects.isEnabled();

  objects.init();

  for (int i = 0; i < npi; i++) {
    objects.define(inp[i]);
  }

  str = objects.getPlotInfo();
  if (plotenabled.find(str) == plotenabled.end())
    //new plot
    objects.enable(true);
  else if (plotenabled.count(str) > 0)
    objects.enable(plotenabled[str]);
}

void PlotModule::prepareTrajectory(const vector<miString>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule::prepareTrajectory ++");
#endif
  //   vtp.push_back(new TrajectoryPlot());

}

vector<PlotElement>& PlotModule::getPlotElements()
{
  //  METLIBS_LOG_DEBUG("PlotModule::getPlotElements()");
  static vector<PlotElement> pel;
  pel.clear();

  int m;
  miString str;

  // get field names
  m = vfp.size();
  for (int j = 0; j < m; j++) {
    vfp[j]->getPlotName(str);
    str += "# " + miString(j);
    bool enabled = vfp[j]->Enabled();
    // add plotelement
    pel.push_back(PlotElement("FIELD", str, "FIELD", enabled));
  }

  // get obs names
  m = vop.size();
  for (int j = 0; j < m; j++) {
    vop[j]->getPlotName(str);
    str += "# " + miString(j);
    bool enabled = vop[j]->Enabled();
    // add plotelement
    pel.push_back(PlotElement("OBS", str, "OBS", enabled));
  }

  // get sat names
  m = vsp.size();
  for (int j = 0; j < m; j++) {
    vsp[j]->getPlotName(str);
    str += "# " + miString(j);
    bool enabled = vsp[j]->Enabled();
    // add plotelement
    pel.push_back(PlotElement("RASTER", str, "RASTER", enabled));
  }

  // get obj names
  objects.getPlotName(str);
  if (str.exists()) {
    str += "# " + miString("0");
    bool enabled = objects.isEnabled();
    // add plotelement
    pel.push_back(PlotElement("OBJECTS", str, "OBJECTS", enabled));
  }

  // get trajectory names
  m = vtp.size();
  for (int j = 0; j < m; j++) {
    vtp[j]->getPlotName(str);
    if (str.exists()) {
      str += "# " + miString(j);
      bool enabled = vtp[j]->Enabled();
      // add plotelement
      pel.push_back(PlotElement("TRAJECTORY", str, "TRAJECTORY", enabled));
    }
  }

  // get stationPlot names
  m = stam->plots().size();
  for (int j = 0; j < m; j++) {
    if (!stam->plots()[j]->isVisible())
      continue;
    stam->plots()[j]->getPlotName(str);
    if (str.exists()) {
      str += "# " + miString(j);
      bool enabled = stam->plots()[j]->Enabled();
      miString icon = stam->plots()[j]->getIcon();
      if (icon.empty())
        icon = "STATION";
      miString ptype = "STATION";
      // add plotelement
      pel.push_back(PlotElement(ptype, str, icon, enabled));
    }
  }

  // get area objects names
  int n = vareaobjects.size();
  for (int j = 0; j < n; j++) {
    vareaobjects[j].getPlotName(str);
    if (str.exists()) {
      str += "# " + miString(j);
      bool enabled = vareaobjects[j].isEnabled();
      miString icon = vareaobjects[j].getIcon();
      // add plotelement
      pel.push_back(PlotElement("AREAOBJECTS", str, icon, enabled));
    }
  }

  // get locationPlot annotations
  m = locationPlots.size();
  for (int j = 0; j < m; j++) {
    locationPlots[j]->getPlotName(str);
    if (str.exists()) {
      str += "# " + miString(j);
      bool enabled = locationPlots[j]->Enabled();
      // add plotelement
      pel.push_back(PlotElement("LOCATION", str, "LOCATION", enabled));
    }
  }

  return pel;
}

void PlotModule::enablePlotElement(const PlotElement& pe)
{
  miString str;
  if (pe.type == "FIELD") {
    for (unsigned int i = 0; i < vfp.size(); i++) {
      vfp[i]->getPlotName(str);
      str += "# " + miString(int(i));
      if (str == pe.str) {
        vfp[i]->enable(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "RASTER") {
    for (unsigned int i = 0; i < vsp.size(); i++) {
      vsp[i]->getPlotName(str);
      str += "# " + miString(int(i));
      if (str == pe.str) {
        vsp[i]->enable(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "OBS") {
    for (unsigned int i = 0; i < vop.size(); i++) {
      vop[i]->getPlotName(str);
      str += "# " + miString(int(i));
      if (str == pe.str) {
        vop[i]->enable(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "OBJECTS") {
    objects.getPlotName(str);
    str += "# 0" ;
    if (str == pe.str) {
      objects.enable(pe.enabled);
    }
  } else if (pe.type == "TRAJECTORY") {
    for (unsigned int i = 0; i < vtp.size(); i++) {
      vtp[i]->getPlotName(str);
      str += "# " + miString(int(i));
      if (str == pe.str) {
        vtp[i]->enable(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "STATION") {
    for (unsigned int i = 0; i < stam->plots().size(); i++) {
      stam->plots()[i]->getPlotName(str);
      str += "# " + miString(int(i));
      if (str == pe.str) {
        stam->plots()[i]->enable(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "AREAOBJECTS") {
    int n = vareaobjects.size();
    for (int i = 0; i < n; i++) {
      vareaobjects[i].getPlotName(str);
      if (str.exists()) {
        str += "# " + miString(int(i));
        if (str == pe.str) {
          vareaobjects[i].enable(pe.enabled);
          break;
        }
      }
    }
  } else if (pe.type == "LOCATION") {
    for (unsigned int i = 0; i < locationPlots.size(); i++) {
      locationPlots[i]->getPlotName(str);
      str += "# " + miString(int(i));
      if (str == pe.str) {
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

void PlotModule::setAnnotations()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule::setAnnotations ++");
#endif

  int n = vap.size();
  for (int i = 0; i < n; i++)
  {
    if (vap[i] != 0)
      delete vap[i];
  }
  vap.clear();

  int npi = annotationStrings.size();

  for (int i = 0; i < npi; i++) {
    AnnotationPlot* ap= new AnnotationPlot();
    // Dont add an invalid object to vector
    if (!ap->prepare(annotationStrings[i]))
    {
      delete ap;
    }
    else
    {
      // Add to vector
      vap.push_back(ap);
    }
  }

  //Annotations from setup, qmenu, etc.
  int m;
  n = vap.size();

  // set annotation-data
  Colour col;
  miString str;
  vector<AnnotationPlot::Annotation> annotations;
  AnnotationPlot::Annotation ann;

  vector<miTime> fieldAnalysisTime;

  // get field annotations
  m = vfp.size();
  for (int j = 0; j < m; j++) {
    fieldAnalysisTime.push_back(vfp[j]->getAnalysisTime());

    if (!vfp[j]->Enabled())
      continue;
    vfp[j]->getFieldAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // get sat annotations
  m = vsp.size();
  for (int j = 0; j < m; j++) {
    if (!vsp[j]->Enabled())
      continue;
    vsp[j]->getSatAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    //ann.col= getContrastColour();
    annotations.push_back(ann);
  }

  // get obj annotations
  if (objects.isEnabled()) {
    objects.getObjAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    if (str.exists()) {
      annotations.push_back(ann);
    }
  }
  // get obs annotations
  m = vop.size();
  for (int j = 0; j < m; j++) {
    if (!vop[j]->Enabled())
      continue;
    vop[j]->getObsAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // get trajectory annotations
  m = vtp.size();
  for (int j = 0; j < m; j++) {
    if (!vtp[j]->Enabled())
      continue;
    vtp[j]->getTrajectoryAnnotation(str, col);
    // empty string if data plot is off
    if (str.exists()) {
      ann.str = str;
      ann.col = col;
      annotations.push_back(ann);
    }
  }

  // get stationPlot annotations
  m = stam->plots().size();
  for (int j = 0; j < m; j++) {
    if (!stam->plots()[j]->Enabled())
      continue;
    stam->plots()[j]->getStationPlotAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // get locationPlot annotations
  m = locationPlots.size();
  for (int j = 0; j < m; j++) {
    if (!locationPlots[j]->Enabled())
      continue;
    locationPlots[j]->getAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  for (int i = 0; i < n; i++) {
    vap[i]->setData(annotations, fieldAnalysisTime);
    vap[i]->setfillcolour(splot.getBgColour());
  }

  //annotations from data

  //get field and sat annotations
  for (int i = 0; i < n; i++) {
    vector<vector<miString> > vvstr = vap[i]->getAnnotationStrings();
    int nn = vvstr.size();
    for (int k = 0; k < nn; k++) {
      m = vfp.size();
      for (int j = 0; j < m; j++) {
        vfp[j]->getAnnotations(vvstr[k]);
      }
      m = vsp.size();
      for (int j = 0; j < m; j++) {
        vsp[j]->getAnnotations(vvstr[k]);
      }
      editm->getAnnotations(vvstr[k]);
      objects.getAnnotations(vvstr[k]);
    }
    vap[i]->setAnnotationStrings(vvstr);
  }

  //get obs annotations
  n = vop.size();
  for (int i = 0; i < n; i++) {
    if (!vop[i]->Enabled())
      continue;
    vector<miString> obsinfo = vop[i]->getObsExtraAnnotations();
    int npi = obsinfo.size();
    for (int j = 0; j < npi; j++) {
      AnnotationPlot* ap = new AnnotationPlot(obsinfo[j]);
      vap.push_back(ap);
    }
  }

  //objects
  vector<miString> objLabelstring = objects.getObjectLabels();
  n = objLabelstring.size();
  for (int i = 0; i < n; i++) {
    AnnotationPlot* ap = new AnnotationPlot(objLabelstring[i]);
    vap.push_back(ap);
  }

}


bool PlotModule::updateFieldPlot(const vector<miString>& pin)
{
  vector<Field*> fv;
  int i, n;
  miTime t = splot.getTime();

  n = vfp.size();
  for (i = 0; i < n; i++) {
    if (vfp[i]->updatePinNeeded(pin[i])) {
      // Make the updated fields or return false.
      if (!fieldplotm->makeFields(pin[i], t, fv))
        return false;
      //free old fields
      freeFields(vfp[i]);
      //set new fields
      vfp[i]->setData(fv, t);
    }
  }

  if (fv.size() && fv[0]->oceanDepth >= 0 && vop.size() > 0)
    splot.setOceanDepth(int(fv[0]->oceanDepth));

  if (fv.size() && fv[0]->pressureLevel >= 0 && vop.size() > 0)
    splot.setPressureLevel(int(fv[0]->pressureLevel));

  n = vop.size();
  for (i = 0; i < n; i++) {
    if (vop[i]->LevelAsField()) {
      if (!obsm->prepare(vop[i], t))
        METLIBS_LOG_WARN("updateLevel: ObsManager returned false from prepare");
    }
  }

  // get annotations from all plots
  setAnnotations();

  // Successful update
  return true;
}


// update plots
bool PlotModule::updatePlots(bool failOnMissingData)
{
  METLIBS_LOG_DEBUG("++ PlotModule::updatePlots ++");

  miString pin;
  vector<Field*> fv;
  int i, n;
  miTime t = splot.getTime();
  Area plotarea, newarea;

  bool nodata = !vmp.size(); // false when data are found

  // prepare data for field plots
  n = vfp.size();
  for (i = 0; i < n; i++) {
    if (vfp[i]->updateNeeded(pin)) {
      if (fieldplotm->makeFields(pin, t, fv)) {
        nodata = false;
      }
      //free old fields
      freeFields(vfp[i]);
      //set new fields
      vfp[i]->setData(fv, t);
    }
  }

  if (fv.size()) {
    // level for P-level observations "as field"
    splot.setPressureLevel(int(fv[0]->pressureLevel));
    // depth for ocean depth observations "as field"
    splot.setOceanDepth(int(fv[0]->oceanDepth));
  }

  // prepare data for satellite plots
  n = vsp.size();
  for (i = 0; i < n; i++) {
    if (!satm->setData(vsp[i])) {
#ifdef DEBUGPRINT
      COMMON_LOG::getInstance("common").debugStream() << "SatManager returned false from setData";
#endif
    } else {
      nodata = false;
    }

  }

  // set maparea from map spec., sat or fields

  //######################################################################
  //  Area aa;
  //  METLIBS_LOG_DEBUG("----------------------------------------------------");
  //  aa=previousrequestedarea;
  //  METLIBS_LOG_DEBUG("previousrequestedarea " << previousrequestedarea);
  //  aa=requestedarea;
  //  METLIBS_LOG_DEBUG("requestedarea         " <<requestedarea.Name()<<" : "<<requestedarea);
  //  METLIBS_LOG_DEBUG("mapDefinedByUser= " << mapDefinedByUser);
  //  METLIBS_LOG_DEBUG("mapDefinedByData= " << mapDefinedByData);
  //  METLIBS_LOG_DEBUG("mapDefinedByView= " << mapDefinedByView);
  //  METLIBS_LOG_DEBUG("mapdefined=       " << mapdefined);
  //  METLIBS_LOG_DEBUG("keepcurrentarea=  " << keepcurrentarea);
  //  METLIBS_LOG_DEBUG("----------------------------------------------------");
  //######################################################################
  mapdefined = false;

  if (mapDefinedByUser) {     // area != "modell/sat-omr."

    plotarea = splot.getMapArea();

    if (!keepcurrentarea ){ // show def. area
      mapdefined = mapDefinedByUser = splot.setMapArea(requestedarea,
          keepcurrentarea);
    } else if( plotarea.P() != requestedarea.P() || // or user just selected new area
        previousrequestedarea.R() != requestedarea.R()) {
      newarea = splot.findBestMatch(requestedarea);
      mapdefined = mapDefinedByUser = splot.setMapArea(newarea, keepcurrentarea);
    } else {
      mapdefined = true;
    }

  } else if (keepcurrentarea && previousrequestedarea != requestedarea) {
    // change from specified area to model/sat area
    mapDefinedByData = false;
  }

  previousrequestedarea = requestedarea;

  if (!mapdefined && keepcurrentarea && mapDefinedByData)
    mapdefined = true;

  if (!mapdefined && vsp.size() > 0) {
    Area a;
    // set area equal to first EXISTING sat-area
    a = vsp[0]->getSatArea();
    if (keepcurrentarea)
      newarea = splot.findBestMatch(a);
    else
      newarea = a;
    splot.setMapArea(newarea, keepcurrentarea);
    mapdefined = mapDefinedByData = true;
  }

  if (!mapdefined && inEdit) {
    // set area equal to editfield-area
    Area a;
    if (editm->getFieldArea(a)) {
      splot.setMapArea(a, true);
      mapdefined = mapDefinedByData = true;
    }
  }

  if (!mapdefined && vfp.size() > 0) {
    // set area equal to first EXISTING field-area ("all timesteps"...)
    n = vfp.size();
    i = 0;
    Area a;
    while (i < n && !vfp[i]->getRealFieldArea(a))
      i++;
    if (i < n) {
      if (keepcurrentarea)
        newarea = splot.findBestMatch(a);
      else
        newarea = a;
      splot.setMapArea(newarea, keepcurrentarea);
      mapdefined = mapDefinedByData = true;
    }
  }

  // moved here ------------------------
  if (!mapdefined && keepcurrentarea && mapDefinedByView)
    mapdefined = true;

  if (!mapdefined) {
    // no data on initial map ... change to "Hirlam.50km" projection and area
    //    miString areaString = "proj=spherical_rot grid=-46.5:-36.5:0.5:0.5:0:65 area=1:188:1:152";
    Area a;
    a.setDefault();
    //    a.setAreaFromLog(areaString);
    splot.setMapArea(a, keepcurrentarea);
    mapdefined = mapDefinedByView = true;
  }
  // ----------------------------------

  // prepare data for observation plots
  n = vop.size();
  for (int i = 0; i < n; i++) {
    vop[i]->logStations();
    vop[i] = vobsTimes[0].vobsOneTime[i];
  }

  if (obsnr > 0)
    obsnr = 0;

  int m = vobsTimes.size();
  for (i = m - 1; i > 0; i--) {
    int l = vobsTimes[i].vobsOneTime.size();
    for (int j = 0; j < l; j++) {
      delete vobsTimes[i].vobsOneTime[j];
      vobsTimes[i].vobsOneTime.pop_back();
    }
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }
  for (i = 0; i < n; i++) {
    if (!obsm->prepare(vop[i], splot.getTime())){
#ifdef DEBUGPRINT
      COMMON_LOG::getInstance("common").debugStream() << "ObsManager returned false from prepare";
#endif
    } else {
      nodata = false;
    }

  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  // prepare met-objects
  if ( objects.defined ) {
    if ( objm->prepareObjects(t, splot.getMapArea(), objects) ) {
      nodata = false;
    }
  }

  // prepare item stored in miscellaneous managers
  map<string,Manager*>::iterator it = managers.begin();
  while (it != managers.end()) {
    it->second->prepare(splot.getTime());
    ++it;
  }

  // prepare editobjects (projection etc.)
  editobjects.changeProjection(splot.getMapArea());
  combiningobjects.changeProjection(splot.getMapArea());

  n = stam->plots().size();
  for (int i = 0; i < n; i++) {
    stam->plots()[i]->changeProjection();
    nodata = false;
  }

  // Prepare/compute trajectories - change projection
  if (vtp.size() > 0) {
    vtp[0]->prepare();

    if (vtp[0]->inComputation()) {
      miString fieldname = vtp[0]->getFieldName().downcase();
      int i = 0, n = vfp.size();
      while (i < n && vfp[i]->getTrajectoryFieldName().downcase() != fieldname)
        i++;
      if (i < n) {
        vector<Field*> vf = vfp[i]->getFields();
        // may have 2 or 3 fields (the 3rd a colour-setting field)
        if (vf.size() >= 2) {
          vtp[0]->compute(vf);
        }
      }
    }
    nodata = false;
  }

  // Prepare measurement positions - change projection
  if (vMeasurementsPlot.size() > 0) {
    vMeasurementsPlot[0]->prepare();
  }

  // get annotations from all plots
  setAnnotations();

  PlotAreaSetup();

  // Update drawing items - this needs to be after the PlotAreaSetup call
  // because we need to reproject the items to screen coordinates.
  it = managers.begin();
  while (it != managers.end()) {
    it->second->changeProjection(splot.getMapArea());
    ++it;
  }

  // Successful update
  return !(failOnMissingData && nodata);
}

// start hardcopy plot
void PlotModule::startHardcopy(const printOptions& po)
{
  if (hardcopy) {
    // if hardcopy in progress, and same filename: make new page
    if (po.fname == printoptions.fname) {
      splot.startPSnewpage();
      return;
    }
    // different filename: end current output and start a new
    splot.endPSoutput();
  }
  hardcopy = true;
  printoptions = po;
  // postscript output
  splot.startPSoutput(printoptions);
}

// end hardcopy plot
void PlotModule::endHardcopy()
{
  if (hardcopy)
    splot.endPSoutput();
  hardcopy = false;
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
  METLIBS_LOG_DEBUG("++ PlotModule.plot() ++");
#endif
#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("++++++PlotModule::plot  under,over: "<<under<<" "<<over);
#endif

  Colour cback(splot.getBgColour().c_str());

  // make background colour and a suitable contrast colour available
  splot.setBackgroundColour(cback);
  splot.setBackContrastColour(getContrastColour());

  //if plotarea has changed, calculate great circle distance...
  if (splot.getDirty()) {
    float lat1, lat2, lon1, lon2, lat3, lon3;
    float width, height;
    splot.getPhysSize(width, height);
    //     PhysToGeo(0,0,lat1,lon1);
    //     PhysToGeo(width,height,lat2,lon2);
    //     PhysToGeo(width/2,height/2,lat3,lon3);
    //##############################################################
    // lat3,lon3, point where ratio between window scale and geographical scale
    // is computed, set to Oslo coordinates, can be changed according to area
    lat3 = 60;
    lon3 = 10;
    lat1 = lat3 - 10;
    lat2 = lat3 + 10;
    lon1 = lon3 - 10;
    lon2 = lon3 + 10;
    //###############################################################
    //gcd is distance between lower left and upper right corners
    float gcd = GreatCircleDistance(lat1, lat2, lon1, lon2);
    float x1, y1, x2, y2;
    GeoToPhys(lat1, lon1, x1, y1);
    GeoToPhys(lat2, lon2, x2, y2);
    float distGeoSq = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
    float distWindowSq = width * width + height * height;
    float ratio = sqrtf(distWindowSq / distGeoSq);
    gcd = gcd * ratio;
    //float gcd2 = GreatCircleDistance(lat1,lat3,lon1,lon3);
    //float earthCircumference = 40005041; // meters
    //if (gcd<gcd2) gcd  = earthCircumference-gcd;
    splot.setGcd(gcd);
  }

  if (under) {
    plotUnder();
  }

  if (over) {
    plotOver();
  }

#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("++++++finished PlotModule::plot  under,over: "<<under<<" "<<over);
#endif
}

// plot underlay ---------------------------------------
void PlotModule::plotUnder()
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("++++++PlotModule::plotUnder  under,over: ");
#endif
  int i, n, m;

  Rectangle plotr = splot.getPlotSize();

  Colour cback(splot.getBgColour().c_str());

  // set correct worldcoordinates
  glLoadIdentity();
  glOrtho(plotr.x1, plotr.x2, plotr.y1, plotr.y2, -1, 1);

  if (hardcopy)
    splot.addHCScissor(plotr.x1 + 0.0001, plotr.y1 + 0.0001, plotr.x2
        - plotr.x1 - 0.0002, plotr.y2 - plotr.y1 - 0.0002);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set the default stencil buffer value.
  glClearStencil(0);

  glClearColor(cback.fR(), cback.fG(), cback.fB(), cback.fA());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // draw background (for hardcopy)
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glColor4f(cback.fR(), cback.fG(), cback.fB(), cback.fA());
  const float d = 0;
  glRectf(plotr.x1 + d, plotr.y1 + d, plotr.x2 - d, plotr.y2 - d);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_BLEND);

  // plot map-elements for lowest zorder
  n = vmp.size();
  for (i = 0; i < n; i++) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Kaller plot til mapplot number:" << i);
#endif
    vmp[i]->plot(0);
  }

  // plot satellite images
  n = vsp.size();
  for (i = 0; i < n; i++) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Kaller plot til satplot number:" << i);
#endif
    vsp[i]->plot();
  }

  // mark undefined areas/values in field (before map)
  n = vfp.size();
  for (i = 0; i < n; i++) {
    if (vfp[i]->getUndefinedPlot()) {
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("Kaller plotUndefined til fieldplot number:" << i);
#endif
      vfp[i]->plotUndefined();
    }
  }

  // plot fields (shaded fields etc. before map)
  n = vfp.size();
  for (i = 0; i < n; i++) {
    if (vfp[i]->getShadePlot()) {
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("Kaller plot til fieldplot number:" << i);
#endif
      vfp[i]->plot();
    }
  }

  // plot map-elements for auto zorder
  n = vmp.size();
  for (i = 0; i < n; i++) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Kaller plot til mapplot number:" << i);
#endif
    vmp[i]->plot(1);
  }

  // plot locationPlots (vcross,...)
  n = locationPlots.size();
  for (i = 0; i < n; i++)
    locationPlots[i]->plot();

  // plot fields (isolines, vectors etc. after map)
  n = vfp.size();
  for (i = 0; i < n; i++) {
    if (!vfp[i]->getShadePlot() && !vfp[i]->overlayBuffer()) {
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("Kaller plot til fieldplot number:" << i);
#endif
      vfp[i]->plot();
    }
  }

  objects.changeProjection(splot.getMapArea());
  objects.plot();

  n = vareaobjects.size();
  for (i = 0; i < n; i++) {
    vareaobjects[i].changeProjection(splot.getMapArea());
    vareaobjects[i].plot();
  }

  // plot station plots
  n = stam->plots().size();
  for (i = 0; i < n; i++) {
    stam->plots()[i]->plot();
  }

  // plot inactive edit fields/objects under observations
  if (inEdit) {
    editm->plot(true, false);
  }

  // plot drawing items
  map<string,Manager*>::iterator it = managers.begin();
  while (it != managers.end()) {
    if (it->second->isEnabled())
      it->second->plot(true, false);
    ++it;
  }

  // if "PPPP-mslp", calc. values and plot observations,
  //if inEdit use editField, if not use first "MSLP"-field
  if (obsm->obs_mslp()) {
    if (inEdit && mapmode != fedit_mode && mapmode != combine_mode) {
      // in underlay while not changing the field
      if (editm->obs_mslp(obsm->getObsPositions())) {
        obsm->calc_obs_mslp(vop);
      }
    } else if (!inEdit) {
      for (unsigned int i = 0; i < vfp.size(); i++) {
        if (vfp[i]->obs_mslp(obsm->getObsPositions())) {
          obsm->calc_obs_mslp(vop);
          break;
        }
      }
    }
  }

  // plot observations
  if (!inEdit || !obsm->obs_mslp()) {
    ObsPlot::clearPos();
    m = vop.size();
    for (i = 0; i < m; i++)
      vop[i]->plot();
  }

  int nanno = vap.size();
  for (int l = 0; l < nanno; l++) {
    vector<vector<miString> > vvstr = vap[l]->getAnnotationStrings();
    int nn = vvstr.size();
    for (int k = 0; k < nn; k++) {
      n = vfp.size();
      for (int j = 0; j < n; j++) {
        vfp[j]->getDataAnnotations(vvstr[k]);
      }
      n = vop.size();
      for (int j = 0; j < n; j++) {
        vop[j]->getDataAnnotations(vvstr[k]);
      }
    }
    vap[l]->setAnnotationStrings(vvstr);
  }

  //plot trajectories
  m = vtp.size();
  for (i = 0; i < m; i++)
    vtp[i]->plot();

  m = vMeasurementsPlot.size();
  for (i = 0; i < m; i++)
    vMeasurementsPlot[i]->plot();

  if (showanno && !inEdit) {
    // plot Annotations
    n = vap.size();
    for (i = 0; i < n; i++) {
      //	METLIBS_LOG_DEBUG("i:"<<i);
      vap[i]->plot();
    }
  }

  if (hardcopy)
    splot.removeHCClipping();
}

// plot overlay ---------------------------------------
void PlotModule::plotOver()
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_DEBUG("++++++PlotModule::plotOver  under,over: ");
#endif

  int i, n;

  Rectangle plotr = splot.getPlotSize();

  // plot GridAreas (polygons)
  if (aream)
    aream->plot();

  // Check this!!!
  n = vfp.size();
  for (i = 0; i < n; i++) {
    if (vfp[i]->overlayBuffer() && !vfp[i]->getShadePlot()) {
      vfp[i]->plot();
    }
  }

  // plot active draw- and editobjects here
  if (inEdit) {

    if (apEditmessage)
      apEditmessage->plot();

    editm->plot(false, true);

    // if PPPP-mslp, calc. values and plot observations,
    // in overlay while changing the field
    if (obsm->obs_mslp() && (mapmode == fedit_mode || mapmode == combine_mode)) {
      if (editm->obs_mslp(obsm->getObsPositions())) {
        obsm->calc_obs_mslp(vop);
      }
    }

    // Annotations
    if (showanno) {
      n = vap.size();
      for (i = 0; i < n; i++)
        vap[i]->plot();
    }
    n = editVap.size();
    for (i = 0; i < n; i++)
      editVap[i]->plot();

  } // if inEdit

  map<string,Manager*>::iterator it = managers.begin();
  while (it != managers.end()) {
    if (it->second->isEnabled())
      it->second->plot(false, true);
    ++it;
  }

  if (hardcopy)
    splot.addHCScissor(plotr.x1 + 0.0001, plotr.y1 + 0.0001, plotr.x2
        - plotr.x1 - 0.0002, plotr.y2 - plotr.y1 - 0.0002);

  // plot map-elements for highest zorder
  n = vmp.size();
  for (i = 0; i < n; i++) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Kaller plot til mapplot number:" << i);
#endif
    vmp[i]->plot(2);
  }

  splot.UpdateOutput();
  splot.setDirty(false);

  // frame (not needed if maprect==fullrect)
  Rectangle mr = splot.getMapSize();
  Rectangle fr = splot.getPlotSize();
  if (mr != fr || hardcopy) {
    glShadeModel(GL_FLAT);
    glColor3f(0.0, 0.0, 0.0);
    glLineWidth(1.0);
    mr.x1 += 0.0001;
    mr.y1 += 0.0001;
    mr.x2 -= 0.0001;
    mr.y2 -= 0.0001;
    glBegin(GL_LINES);
    glVertex2f(mr.x1, mr.y1);
    glVertex2f(mr.x2, mr.y1);
    glVertex2f(mr.x2, mr.y1);
    glVertex2f(mr.x2, mr.y2);
    glVertex2f(mr.x2, mr.y2);
    glVertex2f(mr.x1, mr.y2);
    glVertex2f(mr.x1, mr.y2);
    glVertex2f(mr.x1, mr.y1);
    glEnd();
  }

  splot.UpdateOutput();
  // plot rubberbox
  if (dorubberband) {
#ifdef DEBUGREDRAW
    METLIBS_LOG_DEBUG("PlotModule::plot rubberband oldx,oldy,newx,newy: "
        <<oldx<<" "<<oldy<<" "<<newx<<" "<<newy);
#endif
    Rectangle fullr = splot.getPlotSize();
    float x1 = fullr.x1 + fullr.width() * oldx / plotw;
    float y1 = fullr.y1 + fullr.height() * oldy / ploth;
    float x2 = fullr.x1 + fullr.width() * newx / plotw;
    float y2 = fullr.y1 + fullr.height() * newy / ploth;

    Colour c = getContrastColour();
    glColor4ubv(c.RGBA());
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0);
    //glRectf(x1,y1,x2,y2); // Mesa problems ?
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
  }

  if (hardcopy)
    splot.removeHCClipping();

}

vector<AnnotationPlot*> PlotModule::getAnnotations()
{
  return vap;
}

vector<Rectangle> PlotModule::plotAnnotations()
{
  Rectangle plotr = splot.getPlotSize();

  // set correct worldcoordinates
  glLoadIdentity();
  glOrtho(plotr.x1, plotr.x2, plotr.y1, plotr.y2, -1, 1);

  Colour cback(splot.getBgColour().c_str());

  glClearColor(cback.fR(), cback.fG(), cback.fB(), cback.fA());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  vector<Rectangle> rectangles;

  unsigned int n = vap.size();
  for (unsigned int i = 0; i < n; i++) {
    //	METLIBS_LOG_DEBUG("i:"<<i);
    vap[i]->plot();
    rectangles.push_back(vap[i]->getBoundingBox());
  }

  return rectangles;
}

void PlotModule::PlotAreaSetup()
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule.PlotAreaSetup() ++");
#endif

  if (plotw < 1 || ploth < 1)
    return;
  float waspr = plotw / ploth;

  Area ma = splot.getMapArea();
  Rectangle mapr = ma.R();

  //   float d, del, delta = 0.01;
  //   del = delta;
  //   while (mapr.width() < delta) {
  //     d = (del - mapr.width()) * 0.5;
  //     mapr.x1 -= d;
  //     mapr.x2 += d;
  //     del = del * 2.;
  //   }
  //   del = delta;
  //   while (mapr.height() < delta) {
  //     d = (del - mapr.height()) * 0.5;
  //     mapr.y1 -= d;
  //     mapr.y2 += d;
  //     del = del * 2.;
  //   }

  float maspr = mapr.width() / mapr.height();

  float dwid = 0, dhei = 0;
  if (waspr > maspr) {// increase map width
    dwid = waspr * mapr.height() - mapr.width();
  } else { // increase map height
    dhei = mapr.width() / waspr - mapr.height();
  }

  // update map area
  Rectangle mr = mapr;
  mr.x1 -= (dwid) / 2.0;
  mr.y1 -= (dhei) / 2.0;
  mr.x2 += (dwid) / 2.0;
  mr.y2 += (dhei) / 2.0;

  // add border
  //   float border= mr.width()*0.03/2.0;
  float border = 0.0;
  // update full plot area
  Rectangle fr = mr;
  fr.x1 -= border;
  fr.y1 -= border;
  fr.x2 += border;
  fr.y2 += border;

  splot.setMapSize(mr);
  splot.setPlotSize(fr);

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("============ After PlotAreaSetup ======");
  METLIBS_LOG_DEBUG("Fullplotarea:" << fr);
  METLIBS_LOG_DEBUG("plotarea:" << mr);
  METLIBS_LOG_DEBUG("=======================================");
#endif
}

void PlotModule::setPlotWindow(const int& w, const int& h)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ PlotModule.setPlotWindow() ++" <<
      " w=" << w << " h=" << h);
#endif

  plotw = float(w);
  ploth = float(h);

  splot.setPhysSize(plotw, ploth);

  PlotAreaSetup();

  if (hardcopy)
    splot.resetPage();
}

void PlotModule::freeFields(FieldPlot* fp)
{
  vector<Field*> v = fp->getFields();
  for (unsigned int i = 0; i < v.size(); i++) {
    fieldm->fieldcache->freeField(v[i]);
    v[i] = NULL;
  }
  fp->clearFields();
}

void PlotModule::cleanup()
{

  int i, n;
  n = vmp.size();
  for (i = 0; i < n; i++)
    delete vmp[i];
  vmp.clear();

  n = vfp.size();

  // Field deletion at the end is done in the cache. The cache destructor is called by
  // FieldPlotManagers destructor, which comes before this destructor. Basically we try to
  // destroy something in a dead pointer here....
  for (i = 0; i < n; i++) {
    freeFields(vfp[i]);
    delete vfp[i];
  }
  vfp.clear();

  n = vsp.size();
  for (i = 0; i < n; i++)
    delete vsp[i];
  vsp.clear();

  n = stam->plots().size();
  for (i = 0; i < n; i++)
    delete stam->plots()[i];
  stam->plots().clear();

  n = vobsTimes.size();
  for (i = n - 1; i > -1; i--) {
    int m = vobsTimes[i].vobsOneTime.size();
    for (int j = 0; j < m; j++)
      delete vobsTimes[i].vobsOneTime[j];
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }
  vobsTimes.clear();
  vop.clear();

  objects.clear();

  n = vtp.size();
  for (i = 0; i < n; i++)
    delete vtp[i];
  vtp.clear();

  n = vMeasurementsPlot.size();
  for (i = 0; i < n; i++)
    delete vMeasurementsPlot[i];
  vMeasurementsPlot.clear();

  annotationStrings.clear();

  n = vap.size();
  for (i = 0; i < n; i++)
  {
    if (vap[i] != 0)
      delete vap[i];
  }
  vap.clear();
  if (apEditmessage)
    delete apEditmessage;
  apEditmessage = 0;

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ Returning from PlotModule::cleanup ++");
#endif
}

void PlotModule::PixelArea(const Rectangle r)
{
  if (!plotw || !ploth)
    return;
  // full plot
  Rectangle fullr = splot.getPlotSize();
  // minus border etc..
  Rectangle plotr = splot.getMapSize();
  // map-area
  Area ma = splot.getMapArea();
  //Rectangle mapr= ma.R();

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Plotw:" << plotw);
  METLIBS_LOG_DEBUG("Ploth:" << ploth);
  METLIBS_LOG_DEBUG("Selected rectangle:" << r);
#endif

  // map to grid-coordinates
  Rectangle newr = fullr;
  newr.x1 = fullr.x1 + fullr.width() * r.x1 / plotw;
  newr.y1 = fullr.y1 + fullr.height() * r.y1 / ploth;
  newr.x2 = fullr.x1 + fullr.width() * r.x2 / plotw;
  newr.y2 = fullr.y1 + fullr.height() * r.y2 / ploth;

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Fullplotarea:" << fullr);
  METLIBS_LOG_DEBUG("plotarea:" << plotr);
  //METLIBS_LOG_DEBUG("maparea:" << mapr);
  METLIBS_LOG_DEBUG("new maparea:" << newr);
#endif

  // keep selection inside plotarea

  //   newr.x1+= mapr.width()*r.x1/plotw;
  //   newr.y1+= mapr.height()*r.y1/ploth;
  //   newr.x2-= mapr.width()*(plotw-r.x2)/plotw;
  //   newr.y2-= mapr.height()*(ploth-r.y2)/ploth;

  ma.setR(newr);
  splot.setMapArea(ma, keepcurrentarea);

  PlotAreaSetup();
}

bool PlotModule::PhysToGeo(const float x, const float y, float& lat, float& lon)
{
  bool ret=false;

  if (mapdefined && plotw > 0 && ploth > 0) {
    GridConverter gc;
    Area area = splot.getMapArea();

    Rectangle r = splot.getPlotSize();
    int npos = 1;
    float gx = r.x1 + r.width() / plotw * x;
    float gy = r.y1 + r.height() / ploth * y;

    // convert point to correct projection
    ret = gc.xy2geo(area, npos, &gx, &gy);

    lon = gx;
    lat = gy;
  }

  return ret;
}

bool PlotModule::PhysToGeo(const float x, const float y, float& lat, float& lon, Area area, Rectangle r)
{
  bool ret=false;

  if (mapdefined && plotw > 0 && ploth > 0) {
    GridConverter gc;
    int npos = 1;
    float gx = r.x1 + r.width() / plotw * x;
    float gy = r.y1 + r.height() / ploth * y;

    // convert point to correct projection
    ret = gc.xy2geo(area, npos, &gx, &gy);

    lon = gx;
    lat = gy;
  }

  return ret;
}

bool PlotModule::GeoToPhys(const float lat, const float lon, float& x, float& y)
{
  bool ret=false;

  if (mapdefined && plotw > 0 && ploth > 0) {
    GridConverter gc;
    Area area = splot.getMapArea();

    Rectangle r = splot.getPlotSize();
    int npos = 1;

    float yy = lat;
    float xx = lon;

    // convert point to correct projection
    ret = gc.geo2xy(area, npos, &xx, &yy);

    x = (xx - r.x1) * plotw / r.width();
    y = (yy - r.y1) * ploth / r.height();

  }
  return ret;
}

bool PlotModule::GeoToPhys(const float lat, const float lon, float& x, float& y, Area area, Rectangle r)
{
  bool ret=false;

  if (mapdefined && plotw > 0 && ploth > 0) {
    GridConverter gc;
    int npos = 1;

    float yy = lat;
    float xx = lon;

    // convert point to correct projection
    ret = gc.geo2xy(area, npos, &xx, &yy);

    x = (xx - r.x1) * plotw / r.width();
    y = (yy - r.y1) * ploth / r.height();

  }
  return ret;
}

void PlotModule::PhysToMap(const float x, const float y, float& xmap,
    float& ymap)
{
  if (mapdefined && plotw > 0 && ploth > 0) {
    Rectangle r = splot.getPlotSize();
    xmap = r.x1 + r.width() / plotw * x;
    ymap = r.y1 + r.height() / ploth * y;
  }

}

/// return field grid x,y from map x,y if field defined and map proj = field proj
bool PlotModule::MapToGrid(const float xmap, const float ymap,
    float& gridx, float& gridy){

  if (vsp.size()>0) {
    if (splot.getMapArea().P() == vsp[0]->getSatArea().P()) {
      gridx = xmap/vsp[0]->getGridResolutionX();
      gridy = ymap/vsp[0]->getGridResolutionY();
      return true;
    }
  }

  if (vfp.size()>0) {
    if (splot.getMapArea().P() == vfp[0]->getFieldArea().P()) {
      vector<Field*> ff = vfp[0]->getFields();
      if (ff.size()>0) {
        gridx = xmap/ff[0]->gridResolutionX;
        gridy = ymap/ff[0]->gridResolutionY;
        return true;
      }
    }
  }

  return false;

}

float PlotModule::GreatCircleDistance(float lat1, float lat2, float lon1, float lon2)
{
  return LonLat::fromDegrees(lon1, lat1).distanceTo(LonLat::fromDegrees(lon2, lat2));
}

// set managers
void PlotModule::setManagers(FieldManager* fm, FieldPlotManager* fpm,
    ObsManager* om, SatManager* sm, StationManager* stm, ObjectManager* obm, EditManager* edm,
    GridAreaManager* gam)
{
  fieldm = fm;
  fieldplotm = fpm;
  obsm = om;
  satm = sm;
  stam = stm;
  objm = obm;
  editm = edm;
  aream = gam;

  if (!fieldm)
    METLIBS_LOG_ERROR("PlotModule::ERROR fieldmanager==0");
  if (!fieldplotm)
    METLIBS_LOG_ERROR("PlotModule::ERROR fieldplotmanager==0");
  if (!obsm)
    METLIBS_LOG_ERROR("PlotModule::ERROR obsmanager==0");
  if (!satm)
    METLIBS_LOG_ERROR("PlotModule::ERROR satmanager==0");
  if (!stam)
    METLIBS_LOG_ERROR("PlotModule::ERROR stationmanager==0");
  if (!objm)
    METLIBS_LOG_ERROR("PlotModule::ERROR objectmanager==0");
  if (!editm)
    METLIBS_LOG_ERROR("PlotModule::ERROR editmanager==0");
  if (!aream)
    METLIBS_LOG_ERROR("PlotModule::ERROR gridareamanager==0");
}

// return current plottime
void PlotModule::getPlotTime(miString& s)
{
  miTime t = splot.getTime();

  s = t.isoTime();
}

void PlotModule::getPlotTime(miTime& t)
{
  t = splot.getTime();
}

void PlotModule::getPlotTimes(map<string,vector<miutil::miTime> >& times,
    bool updateSources)
{
  times.clear();

  // edit product proper time
  if (prodtimedefined) {
    times["products"].push_back(producttime);
  }

  vector<miString> pinfos;
  int n = vfp.size();
  for (int i = 0; i < n; i++) {
    pinfos.push_back(vfp[i]->getPlotInfo());
    METLIBS_LOG_DEBUG("Field plotinfo:" << vfp[i]->getPlotInfo());
  }
  if (pinfos.size() > 0) {
    bool constT;
    times["fields"] = fieldplotm->getFieldTime(pinfos, constT, updateSources);
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("--- Found fieldtimes:");
  for (unsigned int i=0; i<fieldtimes.size(); i++)
    METLIBS_LOG_DEBUG(fieldtimes[i]);
#endif

  n = vsp.size();
  pinfos.clear();
  for (int i = 0; i < n; i++)
    pinfos.push_back(vsp[i]->getPlotInfo());
  if (pinfos.size() > 0) {
    times["satellites"] = satm->getSatTimes(pinfos);
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("--- Found sattimes:");
  for (unsigned int i=0; i<sattimes.size(); i++)
    METLIBS_LOG_DEBUG(sattimes[i]);
#endif

  n = vop.size();
  pinfos.clear();
  for (int i = 0; i < n; i++)
    pinfos.push_back(vop[i]->getPlotInfo());
  if (pinfos.size() > 0) {
    times["observations"] = obsm->getObsTimes(pinfos);
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("--- Found obstimes:");
  for (unsigned int i=0; i<obstimes.size(); i++)
    METLIBS_LOG_DEBUG(obstimes[i]);
#endif

  pinfos.clear();
  pinfos.push_back(objects.getPlotInfo());
  if (pinfos.size() > 0) {
    times["objects"] = objm->getObjectTimes(pinfos);
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("--- Found objtimes:");
  for (unsigned int i=0; i<objtimes.size(); i++)
    METLIBS_LOG_DEBUG(objtimes[i]);
#endif

  map<string,Manager*>::iterator it = managers.begin();
  while (it != managers.end()) {
    times[it->first] = it->second->getTimes();
    ++it;
  }
}

//returns union or intersection of plot times from all pinfos
void PlotModule::getCapabilitiesTime(set<miTime>& okTimes,
    set<miTime>& constTimes, const vector<miString>& pinfos,
    bool allTimes, bool updateSources)
{
  vector<miTime> normalTimes;
  miTime constTime;
  int timediff;
  int n = pinfos.size();
  bool normalTimesFound = false;
  bool moreTimes = true;
  for (int i = 0; i < n; i++) {
    vector<miString> tokens = pinfos[i].split(1);
    if (!tokens.empty()) {
      miString type = tokens[0].upcase();
      if (type == "FIELD")
        fieldplotm->getCapabilitiesTime(normalTimes, constTime, timediff,
            pinfos[i], updateSources);
      else if (type == "SAT")
        satm->getCapabilitiesTime(normalTimes, constTime, timediff, pinfos[i]);
      else if (type == "OBS")
        obsm->getCapabilitiesTime(normalTimes, constTime, timediff, pinfos[i]);
      else if (type == "OBJECTS")
        objm->getCapabilitiesTime(normalTimes, constTime, timediff, pinfos[i]);
    }

    if (!constTime.undef()) { //insert constTime

      METLIBS_LOG_INFO("constTime:" << constTime.isoTime());
      constTimes.insert(constTime);

    } else if (moreTimes) { //insert okTimes

      if ((!normalTimesFound && normalTimes.size()))
        normalTimesFound = true;

      //if intersection and no common times, no need to look for more times
      if ((!allTimes && normalTimesFound && !normalTimes.size()))
        moreTimes = false;

      int nTimes = normalTimes.size();

      if (allTimes || okTimes.size() == 0) { // union or first el. of intersection

        for (int k = 0; k < nTimes; k++) {
          okTimes.insert(normalTimes[k]);
        }

      } else { //intersection

        set<miTime> tmptimes;
        set<miTime>::iterator p = okTimes.begin();
        for (; p != okTimes.end(); p++) {
          int k = 0;
          while (k < nTimes && abs(miTime::minDiff(*p, normalTimes[k]))
          > timediff)
            k++;
          if (k < nTimes)
            tmptimes.insert(*p); //time ok
        }
        okTimes = tmptimes;

      }
    } // if neither normalTimes nor constatTime, product is ignored
    normalTimes.clear();
    constTime = miTime();
  }

}

// set plottime
bool PlotModule::setPlotTime(miTime& t)
{
  splot.setTime(t);
  //  updatePlots();
  return true;
}

void PlotModule::updateObs()
{

  // Update ObsPlots if data files have changed

  //delete vobsTimes
  int nvop = vop.size();
  for (int i = 0; i < nvop; i++)
    vop[i] = vobsTimes[0].vobsOneTime[i];

  int n = vobsTimes.size();
  for (int i = n - 1; i > 0; i--) {
    int m = vobsTimes[i].vobsOneTime.size();
    for (int j = 0; j < m; j++)
      delete vobsTimes[i].vobsOneTime[j];
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }
  obsnr = 0;

  // if time of current vop[0] != splot.getTime() or  files have changed,
  // read files from disk
  for (int i = 0; i < nvop; i++) {
    if (vop[i]->updateObs()) {
      if (!obsm->prepare(vop[i], splot.getTime()))
        METLIBS_LOG_WARN("ObsManager returned false from prepare");
    }
  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  // get annotations from all plots
  setAnnotations();

}

bool PlotModule::findObs(int x, int y)
{

  bool found = false;
  int n = vop.size();

  for (int i = 0; i < n; i++)
    if (vop[i]->findObs(x, y))
      found = true;

  return found;
}

bool PlotModule::getObsName(int x, int y, miString& name)
{

  int n = vop.size();

  for (int i = 0; i < n; i++)
    if (vop[i]->getObsName(x, y, name))
      return true;

  return false;
}

//areas
void PlotModule::makeAreas(miString name, miString areastring, int id)
{
  int n = vareaobjects.size();
  //   METLIBS_LOG_DEBUG("makeAreas:"<<n);
  //   METLIBS_LOG_DEBUG("name:"<<name);
  //   METLIBS_LOG_DEBUG("areastring:"<<areastring);
  //name can be name:icon
  vector<miString> tokens = name.split(":");
  miString icon;
  if (tokens.size() > 1) {
    icon = tokens[1];
    name = tokens[0];
  }

  //check if dataset with this id/name already exist
  int i = 0;
  while (i < n && (id != vareaobjects[i].getId() || name
      != vareaobjects[i].getName()))
    i++;

  if (i < n) { //add new areas and replace old areas
    vareaobjects[i].makeAreas(name, icon, areastring, id, splot.getMapArea());
    return;
  }

  //make new dataset
  AreaObjects new_areaobjects;
  new_areaobjects.makeAreas(name, icon, areastring, id, splot.getMapArea());
  vareaobjects.push_back(new_areaobjects);

}

void PlotModule::areaCommand(const miString& command, const miString& dataSet,
    const miString& data, int id)
{
  //   METLIBS_LOG_DEBUG("PlotModule::areaCommand");
  //   METLIBS_LOG_DEBUG("id=" << id);
  //   METLIBS_LOG_DEBUG("command=" << command);
  //   METLIBS_LOG_DEBUG("data="<<data);

  int n = vareaobjects.size();
  for (int i = 0; i < n && i > -1; i++) {
    if ((id == -1 || id == vareaobjects[i].getId()) && (dataSet == "all"
        || dataSet == vareaobjects[i].getName())) {
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
}

vector<selectArea> PlotModule::findAreas(int x, int y, bool newArea)
{
  //METLIBS_LOG_DEBUG("PlotModule::findAreas"  << x << " " << y);
  float xm = 0, ym = 0;
  PhysToMap(x, y, xm, ym);
  vector<selectArea> vsA;
  int n = vareaobjects.size();
  for (int i = 0; i < n; i++) {
    if (!vareaobjects[i].isEnabled())
      continue;
    vector<selectArea> sub_vsA;
    sub_vsA = vareaobjects[i].findAreas(xm, ym, newArea);
    vsA.insert(vsA.end(), sub_vsA.begin(), sub_vsA.end());
  }
  return vsA;
}

//********** plotting and selecting locationPlots on the map *************

void PlotModule::putLocation(const LocationData& locationdata)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("PlotModule::putLocation");
#endif
  bool found = false;
  int n = locationPlots.size();
  miString name = locationdata.name;
  for (int i = 0; i < n; i++) {
    if (name == locationPlots[i]->getName()) {
      bool visible = locationPlots[i]->isVisible();
      delete locationPlots[i];
      locationPlots[i] = new LocationPlot();
      locationPlots[i]->setData(locationdata);
      if (!visible)
        locationPlots[i]->hide();
      found = true;
    }
  }
  if (!found) {
    int i = locationPlots.size();
    locationPlots.push_back(new LocationPlot());
    locationPlots[i]->setData(locationdata);
  }
  setAnnotations();
}

void PlotModule::updateLocation(const LocationData& locationdata)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("PlotModule::updateLocation");
#endif
  int n = locationPlots.size();
  miString name = locationdata.name;
  for (int i = 0; i < n; i++) {
    if (name == locationPlots[i]->getName()) {
      locationPlots[i]->updateOptions(locationdata);
    }
  }
}

void PlotModule::deleteLocation(const miString& name)
{
  vector<LocationPlot*>::iterator p = locationPlots.begin();
  vector<LocationPlot*>::iterator pend = locationPlots.end();

  while (p != pend && name != (*p)->getName())
    p++;
  if (p != pend) {
    delete (*p);
    locationPlots.erase(p);
    setAnnotations();
  }
}

void PlotModule::setSelectedLocation(const miString& name,
    const miString& elementname)
{
  int n = locationPlots.size();
  for (int i = 0; i < n; i++) {
    if (name == locationPlots[i]->getName())
      locationPlots[i]->setSelected(elementname);
  }
}

miString PlotModule::findLocation(int x, int y, const miString& name)
{

  int n = locationPlots.size();
  for (int i = 0; i < n; i++) {
    if (name == locationPlots[i]->getName())
      return locationPlots[i]->find(x, y);
  }
  return miString();
}

//****************************************************

Colour PlotModule::getContrastColour()
{

  Colour c(splot.getBgColour());
  int sum = c.R() + c.G() + c.B();
  if (sum > 255 * 3 / 2)
    c.set(0, 0, 0);
  else
    c.set(255, 255, 255);

  return c;
}

void PlotModule::nextObs(bool next)
{

  int n = vop.size();
  for (int i = 0; i < n; i++)
    vop[i]->nextObs(next);

}

void PlotModule::obsTime(QKeyEvent* ke, EventResult& res)
{
  // This function changes the observation time one hour,
  // and leaves the rest (fields, images etc.) unchanged.
  // It saves the obsPlot object in the vector vobsTimes.
  // This only works for vop[0], which is the only one used at the moment.
  // This only works in edit modus
  // vobsTimes is deleted when anything else are changed or edit modus are left

  if (vop.size() == 0)
    return;
  if (!inEdit)
    return;

  if (obsnr == 0 && ke->key() == Qt::Key_Right)
    return;
  if (obsnr > 20 && ke->key() == Qt::Key_Left)
    return;

  obsm->clearObsPositions();

  miTime newTime = splot.getTime();
  if (ke->key() == Qt::Key_Left) {
    obsnr++;
  } else {
    obsnr--;
  }
  newTime.addHour(-1 * obsTimeStep * obsnr);

  int nvop = vop.size();
  int n = vobsTimes.size();

  //log old stations
  for (int i = 0; i < nvop; i++)
    vop[i]->logStations();

  //Make new obsPlot object
  if (obsnr == n) {

    obsOneTime ot;
    for (int i = 0; i < nvop; i++) {
      ObsPlot *op;
      op = new ObsPlot();
      miString pin = vop[i]->getInfoStr();
      if (!obsm->init(op, pin)) {
        delete op;
        op = NULL;
      } else if (!obsm->prepare(op, newTime))
        METLIBS_LOG_WARN("ObsManager returned false from prepare");

      ot.vobsOneTime.push_back(op);
    }
    vobsTimes.push_back(ot);

  } else {
    for (int i = 0; i < nvop; i++)
      vobsTimes[obsnr].vobsOneTime[i]->readStations();
  }

  //ask last plot object which stations was plotted,
  //and tell this plot object
  for (int i = 0; i < nvop; i++) {
    vop[i] = vobsTimes[obsnr].vobsOneTime[i];
  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  miString labelstr;
  if (obsnr != 0) {
    miString timer(obsnr * obsTimeStep);
    labelstr = "LABEL text=\"OBS -" + timer;
    labelstr += "\" tcolour=black bcolour=red fcolour=red:150 ";
    labelstr += "polystyle=both halign=center valign=top fontsize=18";
  }
  if (vop.size() > 0) {
    vop[0]->setLabel(labelstr);
  }

  setAnnotations();

}

void PlotModule::obsStepChanged(int step)
{

  obsTimeStep = step;

  int n = vop.size();
  for (int i = 0; i < n; i++)
    vop[i] = vobsTimes[0].vobsOneTime[i];

  int m = vobsTimes.size();
  for (int i = m - 1; i > 0; i--) {
    int l = vobsTimes[i].vobsOneTime.size();
    for (int j = 0; j < l; j++) {
      delete vobsTimes[i].vobsOneTime[j];
      vobsTimes[i].vobsOneTime.pop_back();
    }
    vobsTimes[i].vobsOneTime.clear();
    vobsTimes.pop_back();
  }

  if (obsnr > 0)
    obsnr = 0;

  setAnnotations();

}

void PlotModule::trajPos(vector<miString>& vstr)
{
  int n = vtp.size();

  //if vstr starts with "quit", delete all trajectoryPlot objects
  int m = vstr.size();
  for (int j = 0; j < m; j++) {
    if (vstr[j].substr(0, 4) == "quit") {
      for (int i = 0; i < n; i++)
        delete vtp[i];
      vtp.clear();
      setAnnotations(); // will remove tarjectoryPlot annotation
      return;
    }
  }

  //if no trajectoryPlot object, make one
  if (n == 0)
    vtp.push_back(new TrajectoryPlot());

  //there are never more than one trajectoryPlot object (so far..)
  int action = vtp[0]->trajPos(vstr);

  if (action == 1) {
    // trajectories cleared, reset annotations
    setAnnotations(); // will remove tarjectoryPlot annotation
  }
}

void PlotModule::measurementsPos(vector<miString>& vstr)
{
  int n = vMeasurementsPlot.size();
  //if vstr starts with "quit", delete all MeasurementsPlot objects
  int m = vstr.size();
  for (int j = 0; j < m; j++) {
    if (vstr[j].substr(0, 4) == "quit") {
      for (int i = 0; i < n; i++)
        delete vMeasurementsPlot[i];
      vMeasurementsPlot.clear();
      return;
    }
  }

  //if no MeasurementsPlot object, make one
  if (n == 0) {
    vMeasurementsPlot.push_back(new MeasurementsPlot());
  }
  //there are never more than one MeasurementsPlot object (so far..)
  vMeasurementsPlot[0]->measurementsPos(vstr);

}

vector<miString> PlotModule::getCalibChannels()
{

  vector<miString> channels;

  int n = vsp.size();
  for (int i = 0; i < n; i++) {
    if (vsp[i]->Enabled())
      vsp[i]->getCalibChannels(channels); //add channels
  }

  return channels;
}

vector<SatValues> PlotModule::showValues(float x, float y)
{

  // return values of current channels (with calibration)

  int n = vsp.size();
  vector<SatValues> satval;

  for (int i = 0; i < n; i++) {
    if (vsp[i]->Enabled()) {
      vsp[i]->values(x, y, satval);
    }
  }

  return satval;

}

vector<miString> PlotModule::getSatnames()
{
  vector<miString> satnames;
  miString str;
  // get sat names
  int m = vsp.size();
  for (int j = 0; j < m; j++) {
    vsp[j]->getSatName(str);
    if (!str.empty())
      satnames.push_back(str);
  }
  return satnames;
}

bool PlotModule::markAnnotationPlot(int x, int y)
{
  int m = editVap.size();
  bool marked = false;
  for (int j = 0; j < m; j++)
    if (editVap[j]->markAnnotationPlot(x, y))
      marked = true;
  return marked;
}

miString PlotModule::getMarkedAnnotation()
{
  int m = editVap.size();
  miString annotext;
  for (int j = 0; j < m; j++) {
    miString text = editVap[j]->getMarkedAnnotation();
    if (!text.empty())
      annotext = text;
  }
  return annotext;
}

void PlotModule::changeMarkedAnnotation(miString text, int cursor, int sel1,
    int sel2)
{
  int m = editVap.size();
  for (int j = 0; j < m; j++)
    editVap[j]->changeMarkedAnnotation(text, cursor, sel1, sel2);
}

void PlotModule::DeleteMarkedAnnotation()
{
  int m = editVap.size();
  for (int j = 0; j < m; j++)
    editVap[j]->DeleteMarkedAnnotation();
}

void PlotModule::startEditAnnotation()
{
  int m = editVap.size();
  for (int j = 0; j < m; j++)
    editVap[j]->startEditAnnotation();
}

void PlotModule::stopEditAnnotation()
{
  int m = editVap.size();
  for (int j = 0; j < m; j++)
    editVap[j]->stopEditAnnotation();
}

void PlotModule::editNextAnnoElement()
{
  int m = editVap.size();
  for (int j = 0; j < m; j++) {
    editVap[j]->editNextAnnoElement();
  }
}

void PlotModule::editLastAnnoElement()
{
  int m = editVap.size();
  for (int j = 0; j < m; j++) {
    editVap[j]->editLastAnnoElement();
  }
}

vector<miString> PlotModule::writeAnnotations(miString prodname)
{
  vector<miString> annostrings;
  int m = editVap.size();
  for (int j = 0; j < m; j++) {
    miString str = editVap[j]->writeAnnotation(prodname);
    if (!str.empty())
      annostrings.push_back(str);
  }
  return annostrings;
}

void PlotModule::updateEditLabels(vector<miString> productLabelstrings,
    miString productName, bool newProduct)
{
  METLIBS_LOG_DEBUG("diPlotModule::updateEditLabels");
  int n;
  vector<AnnotationPlot*> oldVap; //display object labels
  //read the old labels...

  vector<miString> objLabelstrings = editobjects.getObjectLabels();
  n = objLabelstrings.size();
  for (int i = 0; i < n; i++) {
    AnnotationPlot* ap = new AnnotationPlot(objLabelstrings[i]);
    oldVap.push_back(ap);
  }

  int m = productLabelstrings.size();
  n = oldVap.size();
  for (int j = 0; j < m; j++) {
    AnnotationPlot* ap = new AnnotationPlot(productLabelstrings[j]);
    ap->setProductName(productName);

    vector<vector<miString> > vvstr = ap->getAnnotationStrings();
    int nn = vvstr.size();
    for (int k = 0; k < nn; k++) {
      int mm = vfp.size();
      for (int j = 0; j < mm; j++) {
        vfp[j]->getAnnotations(vvstr[k]);
      }
    }
    ap->setAnnotationStrings(vvstr);

    // here we compare the labels, take input from oldVap
    for (int i = 0; i < n; i++)
      ap->updateInputLabels(oldVap[i], newProduct);

    editVap.push_back(ap);
  }

  for (int i = 0; i < n; i++)
    delete oldVap[i];
  oldVap.clear();
}

//autoFile
void PlotModule::setSatAuto(bool autoFile, const miString& satellite,
    const miString& file)
{
  int m = vsp.size();
  for (int j = 0; j < m; j++)
    vsp[j]->setSatAuto(autoFile, satellite, file);
}

void PlotModule::setObjAuto(bool autoF)
{
  objects.autoFile = autoF;
}

void PlotModule::areaInsert(Area a, bool newArea)
{

  if (newArea && areaSaved) {
    areaSaved = false;
    return;
  }

  if(areaIndex>-1){
    areaQ.erase(areaQ.begin() + areaIndex + 1, areaQ.end());
  }
  if (areaQ.size() > 20)
    areaQ.pop_front();
  else
    areaIndex++;

  areaQ.push_back(a);

}

void PlotModule::changeArea(QKeyEvent* ke)
{
  Area a;
  MapManager mapm;

  // define your own area
  if (ke->key() == Qt::Key_F2 && ke->modifiers() & Qt::ShiftModifier) {
    myArea = splot.getMapArea();
    return;
  }

  if (ke->key() == Qt::Key_F3 || ke->key() == Qt::Key_F4) { // go to previous or next area
    //if last area is not saved, save it
    if (!areaSaved) {
      areaInsert(splot.getMapArea(), false);
      areaSaved = true;
    }
    if (ke->key() == Qt::Key_F3) { // go to previous area
      if (areaIndex < 1)
        return;
      areaIndex--;
      a = areaQ[areaIndex];

    } else if (ke->key() == Qt::Key_F4) { //go to next area
      if (areaIndex + 2 > int(areaQ.size()))
        return;
      areaIndex++;
      a = areaQ[areaIndex];
    }

  } else if (ke->key() == Qt::Key_F2) { //get your own area
    areaInsert(splot.getMapArea(), true);//save last area
    a = myArea;
  } else if (ke->key() == Qt::Key_F5) { //get predefined areas
    areaInsert(splot.getMapArea(), true);
    mapm.getMapAreaByFkey("F5", a);
  } else if (ke->key() == Qt::Key_F6) {
    areaInsert(splot.getMapArea(), true);
    mapm.getMapAreaByFkey("F6", a);
  } else if (ke->key() == Qt::Key_F7) {
    areaInsert(splot.getMapArea(), true);
    mapm.getMapAreaByFkey("F7", a);
  } else if (ke->key() == Qt::Key_F8) {
    areaInsert(splot.getMapArea(), true);
    mapm.getMapAreaByFkey("F8", a);
  }

  Area temp = splot.getMapArea();
  if (temp.P() == a.P()) {
    splot.setMapArea(a, keepcurrentarea);
    PlotAreaSetup();
  } else { //if projection has changed, updatePlots must be called
    splot.setMapArea(a, keepcurrentarea);
    PlotAreaSetup();
    updatePlots();
  }
}

void PlotModule::zoomTo(const Rectangle& rectangle)
{
  Area a = splot.getMapArea();
  a.setR(rectangle);
  splot.setMapArea(a, false);
  PlotAreaSetup();
  updatePlots();
}

void PlotModule::zoomOut()
{
  float scale = 1.3;
  float wd = ((plotw * scale - plotw) / 2.);
  float hd = ((ploth * scale - ploth) / 2.);
  float x1 = -wd;
  float y1 = -hd;
  float x2 = plotw + wd;
  float y2 = ploth + hd;
  //define new plotarea, first save the old one
  areaInsert(splot.getMapArea(), true);
  Rectangle r(x1, y1, x2, y2);
  PixelArea(r);

  // change the projection for drawing items
  map<string,Manager*>::iterator it = managers.begin();
  while (it != managers.end()) {
    it->second->changeProjection(splot.getMapArea());
    ++it;
  }
}

// keyboard/mouse events
void PlotModule::sendMouseEvent(QMouseEvent* me, EventResult& res)
{
  newx = me->x();
  newy = me->y();

  // ** mousepress
  if (me->type() == QEvent::MouseButtonPress) {
    oldx = me->x();
    oldy = me->y();

    if (me->button() == Qt::LeftButton) {
      if (aream)
        dorubberband = !aream->overrideMouseEvent;
      else
        dorubberband = true;
      res.savebackground = true;
      res.background = true;
      res.repaint = true;

      return;

    } else if (me->button() == Qt::MiddleButton) {
      areaInsert(splot.getMapArea(), true); // Save last area
      dopanning = true;
      splot.panPlot(true);
      res.newcursor = paint_move_cursor;
      return;
    }

    else if (me->button() == Qt::RightButton) {
      res.action = rightclick;
    }

    return;
  }
  // ** mousemove
  else if (me->type() == QEvent::MouseMove) {

    res.action = browsing;

    if (dorubberband) {
      res.action = quick_browsing;
      res.background = false;
      res.repaint = true;
      return;

    } else if (dopanning) {
      float x1, y1, x2, y2;
      float wd = me->x() - oldx;
      float hd = me->y() - oldy;
      x1 = -wd;
      y1 = -hd;
      x2 = plotw - wd;
      y2 = ploth - hd;

      Rectangle r(x1, y1, x2, y2);
      PixelArea(r);
      oldx = me->x();
      oldy = me->y();

      res.action = quick_browsing;
      res.background = true;
      res.repaint = true;
      res.newcursor = paint_move_cursor;
      return;
    }

  }
  // ** mouserelease
  else if (me->type() == QEvent::MouseButtonRelease) {

    bool plotnew = false;

    res.savebackground = false;

    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    // minimum rubberband size for zooming (in pixels)
    const float rubberlimit = 15.;

    if (me->button() == Qt::RightButton) { // zoom out

      //end of popup
      //res.action= rightclick;

    } else if (me->button() == Qt::LeftButton) {

      x1 = oldx;
      y1 = oldy;
      x2 = me->x();
      y2 = me->y();

      if (oldx > x2) {
        x1 = x2;
        x2 = oldx;
      }
      if (oldy > y2) {
        y1 = y2;
        y2 = oldy;
      }
      if (fabsf(x2 - x1) > rubberlimit && fabsf(y2 - y1) > rubberlimit) {
        if (dorubberband)
          plotnew = true;
      } else {
        res.action = pointclick;
      }

      dorubberband = false;

    } else if (me->button() == Qt::MiddleButton) {
      dopanning = false;
      splot.panPlot(false);
      res.repaint = true;
      res.background = true;
      return;
    }
    if (plotnew) {
      //define new plotarea, first save the old one
      areaInsert(splot.getMapArea(), true);
      Rectangle r(x1, y1, x2, y2);
      PixelArea(r);

      // update the projection for drawing items
      map<string,Manager*>::iterator it = managers.begin();
      while (it != managers.end()) {
        it->second->changeProjection(splot.getMapArea());
        ++it;
      }
      res.repaint = true;
      res.background = true;
    }

    return;
  }
  // ** mousedoubleclick
  else if (me->type() == QEvent::MouseButtonDblClick) {
    res.action = doubleclick;
  }
}

void PlotModule::sendKeyboardEvent(QKeyEvent* ke, EventResult& res)
{
  static int arrowKeyDirection = 1;

  float dx = 0, dy = 0;
  float zoom = 0.;

  if (ke->type() == QEvent::KeyPress) {

    if (ke->key() == Qt::Key_Home) {
      keepcurrentarea = false;
      updatePlots();
      keepcurrentarea = true;
      return;
    }

    if (ke->key() == Qt::Key_R) {
      if (arrowKeyDirection > 0)
        arrowKeyDirection = -1;
      else
        arrowKeyDirection = 1;
      return;
    }

    if (ke->key() == Qt::Key_Left)
      dx = -plotw / 8;
    else if (ke->key() == Qt::Key_Right)
      dx = plotw / 8;
    else if (ke->key() == Qt::Key_Down)
      dy = -ploth / 8;
    else if (ke->key() == Qt::Key_Up)
      dy = ploth / 8;
    //     else if (ke->key()==Qt::Key_A)     dx= -plotw/8;
    //     else if (ke->key()==Qt::Key_D)     dx=  plotw/8;
    //     else if (ke->key()==Qt::Key_S)     dy= -ploth/8;
    //     else if (ke->key()==Qt::Key_W)     dy=  ploth/8;
    else if (ke->key() == Qt::Key_Z && ke->modifiers() & Qt::ShiftModifier)
      zoom = 1.3;
    else if (ke->key() == Qt::Key_Z)
      zoom = 1. / 1.3;
    else if (ke->key() == Qt::Key_X)
      zoom = 1.3;

    if (dx != 0 || dy != 0) {
      dx *= arrowKeyDirection;
      dy *= arrowKeyDirection;
      //define new plotarea, first save the old one
      areaInsert(splot.getMapArea(), true);
      Rectangle r(dx, dy, plotw + dx, ploth + dy);
      PixelArea(r);
    } else if (zoom > 0.) {
      //define new plotarea, first save the old one
      areaInsert(splot.getMapArea(), true);
      dx = plotw - (plotw * zoom);
      dy = ploth - (ploth * zoom);
      Rectangle r(dx, dy, plotw - dx, ploth - dy);
      PixelArea(r);
    }

    // update the projection for drawing items
    map<string,Manager*>::iterator it = managers.begin();
    while (it != managers.end()) {
      it->second->changeProjection(splot.getMapArea());
      ++it;
    }
    res.repaint = true;
    res.background = true;
  }

}

void PlotModule::setEditMessage(const miString& str)
{

  // set or remove (if empty string) an edit message

  if (apEditmessage) {
    delete apEditmessage;
    apEditmessage = 0;
  }

  if (str.exists()) {
    miString labelstr;
    labelstr = "LABEL text=\"" + str + "\"";
    labelstr += " tcolour=blue bcolour=red fcolour=red:128";
    labelstr += " polystyle=both halign=left valign=top";
    labelstr += " xoffset=0.01 yoffset=0.1 fontsize=30";
    apEditmessage = new AnnotationPlot();
    if (!apEditmessage->prepare(labelstr)) {
      delete apEditmessage;
      apEditmessage = 0;
    }
  }
}

vector<miString> PlotModule::getFieldModels()
{
  vector<miString> vstr;
  set<miString> unique;
  int n = vfp.size();
  for (int i = 0; i < n; i++) {
    miString fname = vfp[i]->getModelName();
    if (fname.exists())
      unique.insert(fname);
  }
  set<miString>::iterator p = unique.begin(), pend = unique.end();
  for (; p != pend; p++)
    vstr.push_back(*p);

  return vstr;
}

vector<miString> PlotModule::getTrajectoryFields()
{
  vector<miString> vstr;
  int n = vfp.size();
  for (int i = 0; i < n; i++) {
    miString fname = vfp[i]->getTrajectoryFieldName();
    if (fname.exists())
      vstr.push_back(fname);
  }
  return vstr;
}

bool PlotModule::startTrajectoryComputation()
{
  if (vtp.size() < 1)
    return false;

  miString fieldname = vtp[0]->getFieldName().downcase();

  int i = 0, n = vfp.size();

  while (i < n && vfp[i]->getTrajectoryFieldName().downcase() != fieldname)
    i++;
  if (i == n)
    return false;

  vector<Field*> vf = vfp[i]->getFields();
  if (vf.size() < 2)
    return false;

  return vtp[0]->startComputation(vf);
}

void PlotModule::stopTrajectoryComputation()
{
  if (vtp.size() > 0)
    vtp[0]->stopComputation();
}

// write trajectory positions to file
bool PlotModule::printTrajectoryPositions(const miString& filename)
{
  if (vtp.size() > 0)
    return vtp[0]->printTrajectoryPositions(filename);

  return false;
}

/********************* reading and writing log file *******************/

vector<miString> PlotModule::writeLog()
{

  //put last area in areaQ
  areaInsert(splot.getMapArea(), true);

  vector<miString> vstr;

  //Write self-defined area (F2)
  miString aa = "name=F2 " + myArea.getAreaString();
  vstr.push_back(aa);

  //Write all araes in list (areaQ)
  int n = areaQ.size();
  for (int i = 0; i < n; i++) {
    aa = "name=" + miString(i) + " " + areaQ[i].getAreaString();
    vstr.push_back(aa);
  }

  return vstr;
}

void PlotModule::readLog(const vector<miString>& vstr,
    const miString& thisVersion, const miString& logVersion)
{

  areaQ.clear();
  Area area;
  int n = vstr.size();
  for (int i = 0; i < n; i++) {

    if(!area.setAreaFromString(vstr[i])) {
      if(!area.setAreaFromLog(vstr[i])) { // try obsolete syntax
        continue;
      }
    }
    if (area.Name() == "F2") {
      myArea = area;
    } else {
      areaQ.push_back(area);
    }
  }

  areaIndex = areaQ.size() - 1;

}

// Miscellaneous get methods
vector<SatPlot*> PlotModule::getSatellitePlots() const
{
  return vsp;
}

vector<FieldPlot*> PlotModule::getFieldPlots() const
{
  return vfp;
}

vector<ObsPlot*> PlotModule::getObsPlots() const
{
  return vop;
}

void PlotModule::getPlotWindow(int &width, int &height)
{
  width = plotw;
  height = ploth;
}
