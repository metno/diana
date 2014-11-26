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

#include <diPlotModule.h>
#include <diObsPlot.h>

#include <diFieldPlot.h>
#include <diMapPlot.h>
#include <diTrajectoryPlot.h>
#include <diMeasurementsPlot.h>

#include <diObsManager.h>
#include <diSatManager.h>
#include <diStationManager.h>
#include <diObjectManager.h>
#include <diEditManager.h>
#include <diWeatherArea.h>
#include <diStationPlot.h>
#include <diMapManager.h>
#include <diManager.h>
#include "diUtilities.h"

#include <diField/diFieldManager.h>
#include <diField/FieldSpecTranslation.h>
#include <diFieldPlotManager.h>
#include <puDatatypes/miCoordinates.h>
#include <puTools/miStringFunctions.h>

#include <GL/gl.h>

#include <QKeyEvent>
#include <QMouseEvent>

//#define DEBUGPRINT
//#define DEBUGREDRAW
#define MILOGGER_CATEGORY "diana.PlotModule"
#include <miLogger/miLogging.h>

using namespace miutil;
using namespace std;

namespace diutil {

void was_enabled::save(const Plot* plot, const std::string& key)
{
  key_enabled[key] = plot->isEnabled();
}

void was_enabled::restore(Plot* plot, const std::string& key) const
{
  key_enabled_t::const_iterator it = key_enabled.find(key);
  if (it != key_enabled.end())
    plot->setEnabled(it->second);
};

} // namespace diutil

// static class members
GridConverter PlotModule::gc; // Projection-converter

PlotModule *PlotModule::self = 0;

PlotModule::PlotModule() :
               plotw(0.),ploth(0.),
               showanno(true), staticPlot_(new StaticPlot()), hardcopy(false),
               dorubberband(false),
               dopanning(false), keepcurrentarea(true), obsnr(0)
{
  self = this;
  oldx = newx = oldy = newy = startx = starty = 0;
  mapdefined = false;
  mapDefinedByUser = false;
  mapDefinedByData = false;
  mapDefinedByView = false;

  // used to detect map area changes
  Projection p;
  Rectangle r(0., 0., 0., 0.);
  previousrequestedarea = Area(p, r);
  requestedarea = Area(p, r);
  staticPlot_->setRequestedarea(requestedarea);
  areaIndex = -1;
  areaSaved = false;
}

PlotModule::~PlotModule()
{
  cleanup();
}

void PlotModule::preparePlots(const vector<string>& vpi)
{
  METLIBS_LOG_SCOPE();
  // reset flags
  mapDefinedByUser = false;

  // split up input into separate products
  vector<string> fieldpi, obspi, areapi, mappi, satpi, statpi, objectpi, trajectorypi,
  labelpi, editfieldpi;

  typedef std::map<std::string, std::vector<std::string> > manager_pi_t;
  manager_pi_t manager_pi;

  // merge PlotInfo's for same type
  for (size_t i = 0; i < vpi.size(); i++) {
    vector<string> tokens = miutil::split(vpi[i], 1);
    if (!tokens.empty()) {
      std::string type = miutil::to_upper(tokens[0]);
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
      else if (managers.find(type) != managers.end())
        manager_pi[type].push_back(vpi[i]);
      else
        METLIBS_LOG_WARN("unknown type '" << type << "'");
    }
  }

  // call prepare methods
  prepareArea(areapi);
  prepareMap(mappi);
  prepareFields(fieldpi);
  prepareObs(obspi);
  satm->prepareSat(satpi);
  prepareStations(statpi);
  objm->prepareObjects(objectpi);
  prepareTrajectory(trajectorypi);
  prepareAnnotation(labelpi);

  if (editm->isInEdit() and not editfieldpi.empty()) {
    std::string plotName;
    vector<FieldRequest> vfieldrequest;
    fieldplotm->parsePin(editfieldpi[0],vfieldrequest,plotName);
    editm->prepareEditFields(plotName,editfieldpi);
  }

  // Send the commands to the other managers.
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    Manager *manager = it->second;

    // send any plot commands to the relevant manager.
    const manager_pi_t::const_iterator itp = manager_pi.find(it->first);
    if (itp != manager_pi.end())
      manager->processInput(itp->second);
    else
      manager->processInput(vector<std::string>());
  }
}

void PlotModule::prepareArea(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  MapManager mapm;

  if (!inp.size())
    return;
  if (inp.size() > 1)
    METLIBS_LOG_DEBUG("More AREA definitions, using: " <<inp[0]);

  const std::string key_name=  "name";
  const std::string key_areaname=  "areaname"; //old syntax
  const std::string key_proj=  "proj4string";
  const std::string key_rectangle=  "rectangle";
  const std::string key_xypart=  "xypart";

  Projection proj;
  Rectangle rect;

  const vector<std::string> tokens= miutil::split_protected(inp[0], '"','"'," ",true);
  for (size_t i=0; i<tokens.size(); i++){
    const vector<std::string> stokens= miutil::split(tokens[i], 1, "=");
    if (stokens.size() > 1) {
      const std::string key= miutil::to_lower(stokens[0]);

      if (key==key_name || key==key_areaname){
        if ( !mapm.getMapAreaByName(stokens[1], requestedarea) ) {
          METLIBS_LOG_WARN("Unknown AREA definition: "<< inp[0]);
        }
      } else if (key==key_proj){
        if ( proj.set_proj_definition(stokens[1]) ) {
          requestedarea.setP(proj);
        } else {
          METLIBS_LOG_WARN("Unknown proj definition: "<< stokens[1]);
        }
      } else if (key==key_rectangle){
        if ( rect.setRectangle(stokens[1],false) ) {
          requestedarea.setR(rect);
        } else {
          METLIBS_LOG_WARN("Unknown rectangle definition: "<< stokens[1]);
        }
      }
    }
  }
}

void PlotModule::prepareMap(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  staticPlot_->xyClear();

  // init inuse array
  const size_t nm = vmp.size();
  vector<bool> inuse(nm, false);

  // keep requested areas
  Area rarea = requestedarea;
  bool arearequested = requestedarea.P().isDefined();

  std::vector<MapPlot*> new_vmp; // new vector of map plots

  for (size_t k = 0; k < inp.size(); k++) { // loop through all plotinfo's
    bool isok = false;
    for (size_t j = 0; j < nm; j++) {
      if (!inuse[j]) { // not already taken
        if (vmp[j]->prepare(inp[k], rarea, true)) {
          inuse[j] = true;
          isok = true;
          arearequested |= vmp[j]->requestedArea(rarea);
          new_vmp.push_back(vmp[j]);
          break;
        }
      }
    }
    if (isok)
      continue;

    // make new mapPlot object and push it on the list
    MapPlot *mp = new MapPlot();
    if (!mp->prepare(inp[k], rarea, false)) {
      delete mp;
    } else {
      arearequested |= mp->requestedArea(rarea);
      new_vmp.push_back(mp);
    }
  } // end plotinfo loop

  // delete unwanted mapplots
  for (size_t i = 0; i < nm; i++) {
    if (!inuse[i])
      delete vmp[i];
  }
  vmp = new_vmp;

  // remove filled maps not used (free memory)
  if (MapPlot::checkFiles(true)) {
    for (size_t i = 0; i < vmp.size(); i++)
      vmp[i]->markFiles();
    MapPlot::checkFiles(false);
  }

  // check area
  if (!mapDefinedByUser && arearequested) {
    mapDefinedByUser = (rarea.P().isDefined());
    requestedarea = rarea;
    staticPlot_->setRequestedarea(requestedarea);
  }
}

void PlotModule::prepareFields(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  diutil::was_enabled plotenabled;

  // for now -- erase all fieldplots
  for (unsigned int i = 0; i < vfp.size(); i++) {
    FieldPlot* fp = vfp[i];
    plotenabled.save(fp, fp->getPlotInfo("model,plot,parameter,reftime"));
    // free old fields
    freeFields(fp);
    delete fp;
  }
  vfp.clear();

  // NOTE: If we use the fieldCache, we must clear it here
  // to avoid memory consumption!
  if (inp.empty()) {
    // No fields will be used any more...
    fieldm->fieldcache->flush();
    return;
  }

  for (size_t i=0; i < inp.size(); i++) {
    FieldPlot *fp = new FieldPlot();

    std::string plotName;
    vector<FieldRequest> vfieldrequest;
    std::string inpstr = inp[i];
    fieldplotm->parsePin(inpstr,vfieldrequest,plotName);
    if (!fp->prepare(plotName, inp[i])) {
      delete fp;
    } else {
      plotenabled.restore(fp, fp->getPlotInfo("model,plot,parameter,reftime"));
      vfp.push_back(fp);
    }
  }
}

void PlotModule::prepareObs(const vector<string>& inp)
{
  METLIBS_LOG_SCOPE();

  diutil::was_enabled plotenabled;
  for (unsigned int i = 0; i < vop.size(); i++)
    plotenabled.save(vop[i], vop[i]->getPlotInfo(3));

  // for now -- erase all obsplots etc..
  //first log stations plotted
  for (size_t i = 0; i < vop.size(); i++)
    vop[i]->logStations();
  vop.clear();
  for (size_t i = 0; i < vobsTimes.size(); i++)
    diutil::delete_all_and_clear(vobsTimes[i].vobsOneTime);
  vobsTimes.clear();

  for (size_t i = 0; i < inp.size(); i++) {
    ObsPlot *op = obsm->createObsPlot(inp[i]);
    if (op) {
      plotenabled.restore(op, op->getPlotInfo(3));

      if (vobsTimes.empty()) {
        obsOneTime ot;
        vobsTimes.push_back(ot);
      }

      vobsTimes[0].vobsOneTime.push_back(op);
      // vobsTimes[0].vobsOneTime[n] = vop[i];//forsiktig!!!!
      //     FIXME alexanderb vop[i] may be undefined; assuming that vop[i] == op anyhow, therefore commented out

      vop.push_back(op);
    }
  }
  obsnr = 0;
}

void PlotModule::prepareStations(const vector<string>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  if (!stam->init(inp)) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("init returned false");
#endif
  }
}

void PlotModule::prepareAnnotation(const vector<string>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  // for now -- erase all annotationplots
  diutil::delete_all_and_clear(vap);

  if (not inp.empty())
    // FIXME this seems suspicious, why not overwrite if empty?
    annotationStrings = inp;
}

void PlotModule::prepareTrajectory(const vector<string>& inp)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  // vtp.push_back(new TrajectoryPlot());
}

vector<PlotElement> PlotModule::getPlotElements()
{
  //  METLIBS_LOG_SCOPE();
  std::vector<PlotElement> pel;

  // get field names
  for (size_t j = 0; j < vfp.size(); j++) {
    std::string str = vfp[j]->getPlotName() + "# " + miutil::from_number(int(j));
    bool enabled = vfp[j]->isEnabled();
    pel.push_back(PlotElement("FIELD", str, "FIELD", enabled));
  }

  // get obs names
  for (size_t j = 0; j < vop.size(); j++) {
    std::string str = vop[j]->getPlotName() + "# " + miutil::from_number(int(j));
    bool enabled = vop[j]->isEnabled();
    pel.push_back(PlotElement("OBS", str, "OBS", enabled));
  }

  satm->addPlotElements(pel);
  objm->addPlotElements(pel);

  // get trajectory names
  for (size_t j = 0; j < vtp.size(); j++) {
    std::string str = vtp[j]->getPlotName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = vtp[j]->isEnabled();
      pel.push_back(PlotElement("TRAJECTORY", str, "TRAJECTORY", enabled));
    }
  }

  // get stationPlot names; vector is built on each call to stam->plots()
  const std::vector<StationPlot*> stam_plots(stam->plots());
  for (size_t j = 0; j < stam_plots.size(); j++) {
    StationPlot* sp = stam_plots[j];
    if (!sp->isVisible())
      continue;
    std::string str = sp->getPlotName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = sp->isEnabled();
      std::string icon = sp->getIcon();
      if (icon.empty())
        icon = "STATION";
      std::string ptype = "STATION";
      pel.push_back(PlotElement(ptype, str, icon, enabled));
    }
  }

  // get area objects names
  for (size_t j = 0; j < vareaobjects.size(); j++) {
    std::string str = vareaobjects[j].getName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = vareaobjects[j].isEnabled();
      std::string icon = vareaobjects[j].getIcon();
      pel.push_back(PlotElement("AREAOBJECTS", str, icon, enabled));
    }
  }

  // get locationPlot annotations
  for (size_t j = 0; j < locationPlots.size(); j++) {
    std::string str = locationPlots[j]->getPlotName();
    if (not str.empty()) {
      str += "# " + miutil::from_number(int(j));
      bool enabled = locationPlots[j]->isEnabled();
      pel.push_back(PlotElement("LOCATION", str, "LOCATION", enabled));
    }
  }

  return pel;
}

void PlotModule::enablePlotElement(const PlotElement& pe)
{
  if (pe.type == "FIELD") {
    for (unsigned int i = 0; i < vfp.size(); i++) {
      std::string str = vfp[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        vfp[i]->setEnabled(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "RASTER") {
    satm->enablePlotElement(pe);
  } else if (pe.type == "OBS") {
    for (unsigned int i = 0; i < vop.size(); i++) {
      std::string str = vop[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        vop[i]->setEnabled(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "OBJECTS") {
    objm->enablePlotElement(pe);
  } else if (pe.type == "TRAJECTORY") {
    for (unsigned int i = 0; i < vtp.size(); i++) {
      std::string str = vtp[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        vtp[i]->setEnabled(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "STATION") {
    const std::vector<StationPlot*> stam_plots(stam->plots());
    for (size_t i = 0; i < stam_plots.size(); i++) {
      StationPlot* sp = stam_plots[i];
      std::string str = sp->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        sp->setEnabled(pe.enabled);
        break;
      }
    }
  } else if (pe.type == "AREAOBJECTS") {
    for (size_t i = 0; i < vareaobjects.size(); i++) {
      std::string str = vareaobjects[i].getName();
      if (not str.empty()) {
        str += "# " + miutil::from_number(int(i));
        if (str == pe.str) {
          vareaobjects[i].enable(pe.enabled);
          break;
        }
      }
    }
  } else if (pe.type == "LOCATION") {
    for (unsigned int i = 0; i < locationPlots.size(); i++) {
      std::string str = locationPlots[i]->getPlotName() + "# " + miutil::from_number(int(i));
      if (str == pe.str) {
        locationPlots[i]->setEnabled(pe.enabled);
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
  METLIBS_LOG_SCOPE();
#endif

  diutil::delete_all_and_clear(vap);

  for (size_t i = 0; i < annotationStrings.size(); i++) {
    AnnotationPlot* ap= new AnnotationPlot();
    // Dont add an invalid object to vector
    if (!ap->prepare(annotationStrings[i]))
      delete ap;
    else
      vap.push_back(ap);
  }

  //Annotations from setup, qmenu, etc.

  // set annotation-data
  Colour col;
  std::string str;
  vector<AnnotationPlot::Annotation> annotations;
  AnnotationPlot::Annotation ann;

  vector<miTime> fieldAnalysisTime;

  // get field annotations
  for (size_t j = 0; j < vfp.size(); j++) {
    fieldAnalysisTime.push_back(vfp[j]->getAnalysisTime());

    if (!vfp[j]->isEnabled())
      continue;
    vfp[j]->getFieldAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  satm->addSatAnnotations(annotations);

  { // get obj annotations
    objm->getObjAnnotation(ann.str, ann.col);
    if (not ann.str.empty())
      annotations.push_back(ann);
  }
  // get obs annotations
  for (size_t j = 0; j < vop.size(); j++) {
    if (!vop[j]->isEnabled())
      continue;
    vop[j]->getObsAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // get trajectory annotations
  for (size_t j = 0; j < vtp.size(); j++) {
    if (!vtp[j]->isEnabled())
      continue;
    vtp[j]->getTrajectoryAnnotation(str, col);
    // empty string if data plot is off
    if (not str.empty()) {
      ann.str = str;
      ann.col = col;
      annotations.push_back(ann);
    }
  }

  // get stationPlot annotations
  const std::vector<StationPlot*> stam_plots(stam->plots());
  for (size_t j = 0; j < stam_plots.size(); j++) {
    StationPlot* sp = stam_plots[j];
    if (!sp->isEnabled())
      continue;
    sp->getStationPlotAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // get locationPlot annotations
  for (size_t j = 0; j < locationPlots.size(); j++) {
    if (!locationPlots[j]->isEnabled())
      continue;
    locationPlots[j]->getAnnotation(str, col);
    ann.str = str;
    ann.col = col;
    annotations.push_back(ann);
  }

  // Miscellaneous managers
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    // Obtain the annotations for enabled managers.
    if (it->second->isEnabled()) {
      vector<string> plotAnnotations = it->second->getAnnotations();
      vector<string>::const_iterator iti;
      for (iti = plotAnnotations.begin(); iti != plotAnnotations.end(); ++iti) {
        ann.str = *iti;
        ann.col = Colour(0, 0, 0);
        annotations.push_back(ann);
      }
    }
  }

  for (size_t i = 0; i < vap.size(); i++) {
    vap[i]->setData(annotations, fieldAnalysisTime);
    vap[i]->setfillcolour(staticPlot_->getBgColour());
  }

  //annotations from data

  //get field and sat annotations
  for (size_t i = 0; i < vap.size(); i++) {
    vector<vector<string> > vvstr = vap[i]->getAnnotationStrings();
    for (size_t k = 0; k < vvstr.size(); k++) {
      for (size_t j = 0; j < vfp.size(); j++) {
        vfp[j]->getDataAnnotations(vvstr[k]);
      }
      for (size_t j = 0; j < vop.size(); j++) {
        vop[j]->getDataAnnotations(vvstr[k]);
      }
      satm->getSatAnnotations(vvstr[k]);
      editm->getAnnotations(vvstr[k]);
      objm->getObjectsAnnotations(vvstr[k]);
    }
    vap[i]->setAnnotationStrings(vvstr);
  }

  //get obs annotations
  for (size_t i = 0; i < vop.size(); i++) {
    if (!vop[i]->isEnabled())
      continue;
    vector<std::string> obsinfo = vop[i]->getObsExtraAnnotations();
    for (size_t j = 0; j < obsinfo.size(); j++) {
      AnnotationPlot* ap = new AnnotationPlot(obsinfo[j]);
      vap.push_back(ap);
    }
  }

  //objects
  vector<std::string> objLabelstring = objm->getObjectLabels();
  for (size_t i = 0; i < objLabelstring.size(); i++) {
    AnnotationPlot* ap = new AnnotationPlot(objLabelstring[i]);
    vap.push_back(ap);
  }
}


bool PlotModule::updateFieldPlot(const vector<std::string>& pin)
{
  vector<Field*> fv;
  const miTime& t = staticPlot_->getTime();

  for (size_t i = 0; i < vfp.size(); i++) {
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
    staticPlot_->setOceanDepth(int(fv[0]->oceanDepth));

  if (fv.size() && fv[0]->pressureLevel >= 0 && vop.size() > 0)
    staticPlot_->setPressureLevel(int(fv[0]->pressureLevel));

  for (size_t i = 0; i < vop.size(); i++) {
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
  METLIBS_LOG_SCOPE();

  vector<Field*> fv;
  const miTime& t = staticPlot_->getTime();
  Area plotarea, newarea;

  bool nodata = vmp.empty(); // false when data are found

  // prepare data for field plots
  for (size_t i = 0; i < vfp.size(); i++) {
    std::string pin;
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
    staticPlot_->setPressureLevel(int(fv[0]->pressureLevel));
    // depth for ocean depth observations "as field"
    staticPlot_->setOceanDepth(int(fv[0]->oceanDepth));
  }

  // prepare data for satellite plots
  if (satm->setData())
    nodata = false;
  else
    METLIBS_LOG_DEBUG("SatManager returned false from setData");

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

    plotarea = staticPlot_->getMapArea();

    if (!keepcurrentarea ){ // show def. area
      mapdefined = mapDefinedByUser = staticPlot_->setMapArea(requestedarea,
          keepcurrentarea);
    } else if( plotarea.P() != requestedarea.P() || // or user just selected new area
        previousrequestedarea.R() != requestedarea.R()) {
      newarea = staticPlot_->findBestMatch(requestedarea);
      mapdefined = mapDefinedByUser = staticPlot_->setMapArea(newarea, keepcurrentarea);
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

  if (!mapdefined) {
    Area a;
    if (satm->getSatArea(a)) {
      // set area equal to first EXISTING sat-area
      if (keepcurrentarea)
        newarea = staticPlot_->findBestMatch(a);
      else
        newarea = a;
      staticPlot_->setMapArea(newarea, keepcurrentarea);
      mapdefined = mapDefinedByData = true;
    }
  }

  if (!mapdefined && editm->isInEdit()) {
    // set area equal to editfield-area
    Area a;
    if (editm->getFieldArea(a)) {
      staticPlot_->setMapArea(a, true);
      mapdefined = mapDefinedByData = true;
    }
  }

  int i, n;
  if (!mapdefined && vfp.size() > 0) {
    // set area equal to first EXISTING field-area ("all timesteps"...)
    n = vfp.size();
    i = 0;
    Area a;
    while (i < n && !vfp[i]->getRealFieldArea(a))
      i++;
    if (i < n) {
      if (keepcurrentarea)
        newarea = staticPlot_->findBestMatch(a);
      else
        newarea = a;
      staticPlot_->setMapArea(newarea, keepcurrentarea);
      mapdefined = mapDefinedByData = true;
    }
  }

  // moved here ------------------------
  if (!mapdefined && keepcurrentarea && mapDefinedByView)
    mapdefined = true;

  if (!mapdefined) {
    // no data on initial map ... change to "Hirlam.50km" projection and area
    //    std::string areaString = "proj=spherical_rot grid=-46.5:-36.5:0.5:0.5:0:65 area=1:188:1:152";
    Area a;
    a.setDefault();
    //    a.setAreaFromLog(areaString);
    staticPlot_->setMapArea(a, keepcurrentarea);
    mapdefined = mapDefinedByView = true;
  }
  // ----------------------------------

  // prepare data for observation plots
  obsnr = 0;
  for (size_t i = 0; i < vop.size(); i++) {
    vop[i]->logStations();
    vop[i] = vobsTimes[0].vobsOneTime[i];
  }
  for (; vobsTimes.size() > 1; vobsTimes.pop_back())
    diutil::delete_all_and_clear(vobsTimes.back().vobsOneTime);
  for (size_t i = 0; i < vop.size(); i++) {
    if (!obsm->prepare(vop[i], staticPlot_->getTime())){
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("ObsManager returned false from prepare");
#endif
    } else {
      nodata = false;
    }
  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  // prepare met-objects
  if (objm->prepareObjects(t, staticPlot_->getMapArea()))
    nodata = false;

  // prepare item stored in miscellaneous managers
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    // If the preparation fails then return false to indicate an error.
    if (it->second->isEnabled() && !it->second->prepare(staticPlot_->getTime()))
      nodata = false;
  }

  // prepare editobjects (projection etc.)
  objm->changeProjection(staticPlot_->getMapArea());

  const std::vector<StationPlot*> stam_plots(stam->plots());
  for (size_t i = 0; i < stam_plots.size(); i++) {
    stam_plots[i]->changeProjection();
    nodata = false;
  }

  // Prepare/compute trajectories - change projection
  if (vtp.size() > 0) {
    vtp[0]->prepare();

    if (vtp[0]->inComputation()) {
      std::string fieldname = miutil::to_lower(vtp[0]->getFieldName());
      int i = 0, n = vfp.size();
      while (i < n && miutil::to_lower(vfp[i]->getTrajectoryFieldName()) != fieldname)
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

  setAnnotations();

  PlotAreaSetup();

  // Update drawing items - this needs to be after the PlotAreaSetup call
  // because we need to reproject the items to screen coordinates.
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it)
    it->second->changeProjection(staticPlot_->getMapArea());




  // Successful update
  return !(failOnMissingData && nodata);
}

// start hardcopy plot
void PlotModule::startHardcopy(const printOptions& po)
{
  if (hardcopy) {
    // if hardcopy in progress, and same filename: make new page
    if (po.fname == printoptions.fname) {
      staticPlot_->startPSnewpage();
      return;
    }
    // different filename: end current output and start a new
    staticPlot_->endPSoutput();
  }
  hardcopy = true;
  printoptions = po;
  // postscript output
  staticPlot_->startPSoutput(printoptions);
}

// end hardcopy plot
void PlotModule::endHardcopy()
{
  if (hardcopy)
    staticPlot_->endPSoutput();
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
#if defined(DEBUGPRINT) || defined(DEBUGREDRAW)
  METLIBS_LOG_SCOPE(LOGVAL(under) << LOGVAL(over));
#endif

  Colour cback(staticPlot_->getBgColour().c_str());

  // make background colour and a suitable contrast colour available
  staticPlot_->setBackgroundColour(cback);
  staticPlot_->setBackContrastColour(getContrastColour());

  //if plotarea has changed, calculate great circle distance...
  if (staticPlot_->getDirty()) {
    float lat1, lat2, lon1, lon2, lat3, lon3;
    float width, height;
    staticPlot_->getPhysSize(width, height);
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
    staticPlot_->setGcd(gcd);
  }

  if (under)
    plotUnder();

  if (over)
    plotOver();
}

// plot underlay ---------------------------------------
void PlotModule::plotUnder()
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_SCOPE();
#endif

  const Rectangle& plotr = staticPlot_->getPlotSize();

  Colour cback(staticPlot_->getBgColour().c_str());

  // set correct worldcoordinates
  glLoadIdentity();
  glOrtho(plotr.x1, plotr.x2, plotr.y1, plotr.y2, -1, 1);

  if (hardcopy)
    staticPlot_->addHCScissor(plotr.x1 + 0.0001, plotr.y1 + 0.0001, plotr.x2
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
  for (size_t i = 0; i < vmp.size(); i++) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Kaller plot til mapplot number:" << i);
#endif
    vmp[i]->plot(0);
  }

  // plot satellite images
  satm->plot();

  // mark undefined areas/values in field (before map)
  for (size_t i = 0; i < vfp.size(); i++) {
    if (vfp[i]->getUndefinedPlot()) {
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("Kaller plotUndefined til fieldplot number:" << i);
#endif
      vfp[i]->plotUndefined();
    }
  }

  // plot fields (shaded fields etc. before map)
  for (size_t i = 0; i < vfp.size(); i++) {
    if (vfp[i]->getShadePlot()) {
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("Kaller plot til fieldplot number:" << i);
#endif
      vfp[i]->plot();
    }
  }

  // plot map-elements for auto zorder
  for (size_t i = 0; i < vmp.size(); i++) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Kaller plot til mapplot number:" << i);
#endif
    vmp[i]->plot(1);
  }

  // plot locationPlots (vcross,...)
  for (size_t i = 0; i < locationPlots.size(); i++)
    locationPlots[i]->plot();

  // plot fields (isolines, vectors etc. after map)
  for (size_t i = 0; i < vfp.size(); i++) {
    if (!vfp[i]->getShadePlot() && !vfp[i]->overlayBuffer()) {
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("Kaller plot til fieldplot number:" << i);
#endif
      vfp[i]->plot();
    }
  }

  objm->plotObjects();

  for (size_t i = 0; i < vareaobjects.size(); i++) {
    vareaobjects[i].changeProjection(staticPlot_->getMapArea());
    vareaobjects[i].plot();
  }

  // plot station plots
  const std::vector<StationPlot*> stam_plots(stam->plots());
  for (size_t j = 0; j < stam_plots.size(); j++)
    stam_plots[j]->plot();

  // plot inactive edit fields/objects under observations
  if (editm->isInEdit()) {
    editm->plot(true, false);
  }

  // plot other objects, including drawing items
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    if (it->second->isEnabled()) {
      it->second->changeProjection(staticPlot_->getMapArea());
      it->second->plot(true, false);
    }
  }

  // plot observations (if in fieldEditMode  and the option obs_mslp is true, plot observations in overlay)
  if ( !( editm->isInEdit()&& (editm->getMapMode() == fedit_mode || editm->getMapMode() == combine_mode)  && obsm->obs_mslp())) {
    ObsPlot::clearPos();
    for (size_t i = 0; i < vop.size(); i++)
      vop[i]->plot();
  }

  //plot trajectories
  for (size_t i = 0; i < vtp.size(); i++)
    vtp[i]->plot();

  for (size_t i = 0; i < vMeasurementsPlot.size(); i++)
    vMeasurementsPlot[i]->plot();

  if (showanno && !editm->isInEdit()) {
    // plot Annotations
    for (size_t i = 0; i < vap.size(); i++)
      vap[i]->plot();
  }

  if (hardcopy)
    staticPlot_->removeHCClipping();
}

// plot overlay ---------------------------------------
void PlotModule::plotOver()
{
#ifdef DEBUGREDRAW
  METLIBS_LOG_SCOPE();
#endif

  const Rectangle& plotr = staticPlot_->getPlotSize();

  // Check this!!!
  for (size_t i = 0; i < vfp.size(); i++) {
    if (vfp[i]->overlayBuffer() && !vfp[i]->getShadePlot()) {
      vfp[i]->plot();
    }
  }

  // plot active draw- and editobjects here
  if (editm->isInEdit()) {

    editm->plot(false, true);

    // if PPPP-mslp, calc. values and plot observations,
    // in overlay while changing the field
    if (obsm->obs_mslp() && (editm->getMapMode() == fedit_mode || editm->getMapMode() == combine_mode)) {
      if (editm->obs_mslp(obsm->getObsPositions())) {
        obsm->calc_obs_mslp(vop);
      }
    }

    // Annotations
    if (showanno) {
      for (size_t i = 0; i < vap.size(); i++)
        vap[i]->plot();
    }
    for (size_t i = 0; i < editVap.size(); i++)
      editVap[i]->plot();

  } // if editm->isInEdit()

  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it) {
    if (it->second->isEnabled()) {
      it->second->changeProjection(staticPlot_->getMapArea());
      it->second->plot(false, true);
    }
  }

  if (hardcopy)
    staticPlot_->addHCScissor(plotr.x1 + 0.0001, plotr.y1 + 0.0001, plotr.x2
        - plotr.x1 - 0.0002, plotr.y2 - plotr.y1 - 0.0002);

  // plot map-elements for highest zorder
  for (size_t i = 0; i < vmp.size(); i++) {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("Kaller plot til mapplot number:" << i);
#endif
    vmp[i]->plot(2);
  }

  staticPlot_->UpdateOutput();
  staticPlot_->setDirty(false);

  // frame (not needed if maprect==fullrect)
  Rectangle mr = staticPlot_->getMapSize();
  const Rectangle& fr = staticPlot_->getPlotSize();
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

  staticPlot_->UpdateOutput();
  // plot rubberbox
  if (dorubberband) {
#ifdef DEBUGREDRAW
    METLIBS_LOG_DEBUG("PlotModule::plot rubberband oldx,oldy,newx,newy: "
        <<oldx<<" "<<oldy<<" "<<newx<<" "<<newy);
#endif
    const Rectangle& fullr = staticPlot_->getPlotSize();
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
    staticPlot_->removeHCClipping();
}

const vector<AnnotationPlot*>& PlotModule::getAnnotations()
{
  return vap;
}

vector<Rectangle> PlotModule::plotAnnotations()
{
  const Rectangle& plotr = staticPlot_->getPlotSize();

  // set correct worldcoordinates
  glLoadIdentity();
  glOrtho(plotr.x1, plotr.x2, plotr.y1, plotr.y2, -1, 1);

  Colour cback(staticPlot_->getBgColour().c_str());

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
  METLIBS_LOG_SCOPE();
#endif

  if (plotw < 1 || ploth < 1)
    return;
  float waspr = plotw / ploth;

  Area ma = staticPlot_->getMapArea();
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

  staticPlot_->setMapSize(mr);
  staticPlot_->setPlotSize(fr);

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
  METLIBS_LOG_SCOPE(LOGVAL(w) << LOGVAL(h));
#endif

  plotw = float(w);
  ploth = float(h);

  staticPlot_->setPhysSize(plotw, ploth);

  PlotAreaSetup();

  if (hardcopy)
    staticPlot_->resetPage();
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
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  diutil::delete_all_and_clear(vmp);

  // Field deletion at the end is done in the cache. The cache destructor is called by
  // FieldPlotManagers destructor, which comes before this destructor. Basically we try to
  // destroy something in a dead pointer here....
  for (size_t i = 0; i < vfp.size(); i++) {
    freeFields(vfp[i]);
    delete vfp[i];
  }
  vfp.clear();

  satm->clear();

  std::vector<StationPlot*> stam_plots(stam->plots());
  diutil::delete_all_and_clear(stam_plots); // FIXME this does not clear anything in StationManager

  for (; not vobsTimes.empty(); vobsTimes.pop_back())
    diutil::delete_all_and_clear(vobsTimes.back().vobsOneTime);
  vop.clear();

  objm->clearObjects();

  diutil::delete_all_and_clear(vtp);

  diutil::delete_all_and_clear(vMeasurementsPlot);

  annotationStrings.clear();

  diutil::delete_all_and_clear(vap);
}

void PlotModule::PixelArea(const Rectangle r)
{
  if (!plotw || !ploth)
    return;
  // full plot
  const Rectangle& fullr = staticPlot_->getPlotSize();

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

  Area ma = staticPlot_->getMapArea();
  ma.setR(newr);
  staticPlot_->setMapArea(ma, keepcurrentarea);

  PlotAreaSetup();
}

bool PlotModule::PhysToGeo(const float x, const float y, float& lat, float& lon)
{
  bool ret=false;

  if (mapdefined && plotw > 0 && ploth > 0) {
    GridConverter gc;
    Area area = staticPlot_->getMapArea();

    Rectangle r = staticPlot_->getPlotSize();
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
    Area area = staticPlot_->getMapArea();

    const Rectangle r = staticPlot_->getPlotSize();
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
    const Rectangle& r = staticPlot_->getPlotSize();
    xmap = r.x1 + r.width() / plotw * x;
    ymap = r.y1 + r.height() / ploth * y;
  }

}

/// return field grid x,y from map x,y if field defined and map proj = field proj
bool PlotModule::MapToGrid(const float xmap, const float ymap,
    float& gridx, float& gridy)
{
  Area a;
  if (satm->getSatArea(a) and staticPlot_->getMapArea().P() == a.P()) {
    float rx=0, ry=0;
    if (satm->getGridResolution(rx, ry)) {
      gridx = xmap/rx;
      gridy = ymap/ry;
      return true;
    }
  }

  if (vfp.size()>0) {
    if (staticPlot_->getMapArea().P() == vfp[0]->getFieldArea().P()) {
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

double PlotModule::getEntireWindowDistances(const bool horizontal){
  int x1,  y2, x2 = 0, y1 = 0;
  getPlotWindow(x1, y2);

  float flat1, flat3, flat4, flon1, flon3, flon4;
  PhysToGeo(x1, y1, flat1, flon1);
  PhysToGeo(x1, y2, flat3, flon3);
  PhysToGeo(x2, y2, flat4, flon4);
  double dist;
  if(horizontal){
    dist = GreatCircleDistance(flat3,flat4,flon3 ,flon4);
  } else {
    dist = GreatCircleDistance(flat1,flat3,flon1 ,flon3);
  }
  return dist;
}

double PlotModule::getWindowDistances(const float& x, const float& y, const bool horizontal){

  if ( !dorubberband )
    return getEntireWindowDistances(horizontal);

  float flat1, flat3, flat4, flon1, flon3, flon4;
  PhysToGeo(startx, starty, flat1, flon1);
  PhysToGeo(startx, y, flat3, flon3);
  PhysToGeo(x, y, flat4, flon4);
  double dist;
  if(horizontal){
    dist = GreatCircleDistance(flat3,flat4,flon3 ,flon4);
  } else {
    dist = GreatCircleDistance(flat1,flat3,flon1 ,flon3);
  }
  return dist;
}

double PlotModule::getMarkedArea(const float& x, const float& y){

  if ( !dorubberband) return 0.;

  float flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4;
  PhysToGeo(startx, starty, flat1, flon1);
  PhysToGeo(x, starty, flat2, flon2);
  PhysToGeo(startx, y, flat3, flon3);
  PhysToGeo(x, y, flat4, flon4);
  return getArea(flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4);
}

double PlotModule::getWindowArea(){
  int x1,  y2, x2 = 0, y1 = 0;
  getPlotWindow(x1, y2);
  float flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4;
  PhysToGeo(x1, y1, flat1, flon1);
  PhysToGeo(x2, y1, flat2, flon2);
  PhysToGeo(x1, y2, flat3, flon3);
  PhysToGeo(x2, y2, flat4, flon4);
  return getArea(flat1, flat2, flat3, flat4, flon1, flon2, flon3, flon4);
}

double PlotModule::calculateArea(double hLSide, double hUSide, double vLSide, double vRSide, double diag){
  /*Calculates the area as two triangles
   * Each triangle is calculated with Herons formula*/
  // Returns the calculated area as m2
  double calcArea = 0;
  double p1 = ((hLSide + vLSide + diag)/2);
  double p2 = ((hUSide + vRSide + diag)/2);
  double nonsqrt1, nonsqrt2;
  nonsqrt1 = p1*(p1 - hLSide)*(p1 - vLSide)*(p1 - diag);
  nonsqrt2 = p2*(p1 - hUSide)*(p1 - vRSide)*(p1 - diag);
  calcArea = sqrt(nonsqrt1)+sqrt(nonsqrt2);
  return calcArea;
}

double PlotModule::getArea(const float& flat1,
    const float& flat2,
    const float& flat3,
    const float& flat4,
    const float& flon1,
    const float& flon2,
    const float& flon3,
    const float& flon4){
  //Calculate distance vertical left side with earth radius in mind
  double vLSide = GreatCircleDistance(flat1,flat3,flon1,flon3);
  //Calculate distance horizontal lower side with earth radius in mind
  double hLSide = GreatCircleDistance(flat3,flat4,flon3,flon4);
  //Calculate distance vertical right side with earth radius in mind
  double vRSide = GreatCircleDistance(flat2,flat4,flon2,flon4);
  //Calculate distance horizontal upper side with earth radius in mind
  double hUSide = GreatCircleDistance(flat1,flat2,flon1,flon2);
  //Calculate distance diagonal with earth radius in mind
  double diagonal = GreatCircleDistance(flat1,flat4,flon1,flon4);
  //Calculate area as addition of two triangles
  return calculateArea(hLSide, hUSide, vLSide, vRSide, diagonal);
}


// static
float PlotModule::GreatCircleDistance(float lat1, float lat2, float lon1, float lon2)
{
  return LonLat::fromDegrees(lon1, lat1).distanceTo(LonLat::fromDegrees(lon2, lat2));
}

// set managers
void PlotModule::setManagers(FieldManager* fm, FieldPlotManager* fpm,
    ObsManager* om, SatManager* sm, StationManager* stm, ObjectManager* obm, EditManager* edm)
{
  fieldm = fm;
  fieldplotm = fpm;
  obsm = om;
  satm = sm;
  stam = stm;
  objm = obm;
  editm = edm;

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
}

// return current plottime
void PlotModule::getPlotTime(std::string& s)
{
  const miTime& t = staticPlot_->getTime();

  s = t.isoTime();
}

void PlotModule::getPlotTime(miTime& t)
{
  t = staticPlot_->getTime();
}

void PlotModule::getPlotTimes(map<string,vector<miutil::miTime> >& times,
    bool updateSources)
{
  times.clear();

  { // edit product proper time
    miutil::miTime pt;;
    if (editm->getProductTime(pt))
      times["products"].push_back(pt);
  }

  vector<std::string> pinfos;
  for (size_t i = 0; i < vfp.size(); i++) {
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

  { std::vector<miTime> sattimes = satm->getSatTimes();
  if (not sattimes.empty())
    times["satellites"] = sattimes;
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("--- Found sattimes:");
  for (unsigned int i=0; i<sattimes.size(); i++)
    METLIBS_LOG_DEBUG(sattimes[i]);
#endif
  }

  pinfos.clear();
  for (size_t i = 0; i < vop.size(); i++)
    pinfos.push_back(vop[i]->getPlotInfo());
  if (pinfos.size() > 0) {
    times["observations"] = obsm->getObsTimes(pinfos);
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("--- Found obstimes:");
  for (unsigned int i=0; i<obstimes.size(); i++)
    METLIBS_LOG_DEBUG(obstimes[i]);
#endif

  times["objects"] = objm->getObjectTimes();

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("--- Found objtimes:");
  for (unsigned int i=0; i<objtimes.size(); i++)
    METLIBS_LOG_DEBUG(objtimes[i]);
#endif

  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it)
    times[it->first] = it->second->getTimes();
}

//returns union or intersection of plot times from all pinfos
void PlotModule::getCapabilitiesTime(set<miTime>& okTimes,
    set<miTime>& constTimes, const vector<std::string>& pinfos,
    bool allTimes, bool updateSources)
{
  vector<miTime> normalTimes;
  miTime constTime;
  int timediff;
  bool normalTimesFound = false;
  bool moreTimes = true;
  for (size_t i = 0; i < pinfos.size(); i++) {
    vector<std::string> tokens = miutil::split(pinfos[i], 1);
    if (!tokens.empty()) {
      std::string type = miutil::to_upper(tokens[0]);
      if (type == "FIELD")
        fieldplotm->getCapabilitiesTime(normalTimes, constTime, timediff, pinfos[i], updateSources);
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

      if (allTimes || okTimes.empty()) { // union or first el. of intersection
        okTimes.insert(normalTimes.begin(), normalTimes.end());

      } else { //intersection

        set<miTime> tmptimes;
        for (set<miTime>::const_iterator p = okTimes.begin(); p != okTimes.end(); p++) {
          vector<miTime>::const_iterator itn = normalTimes.begin();
          while (itn != normalTimes.end() and abs(miTime::minDiff(*p, *itn)) > timediff)
            ++itn;
          if (itn != normalTimes.end())
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
  staticPlot_->setTime(t);
  //  updatePlots();
  return true;
}

void PlotModule::updateObs()
{
  // Update ObsPlots if data files have changed

  //delete vobsTimes
  for (size_t i = 0; i < vop.size(); i++)
    vop[i] = vobsTimes[0].vobsOneTime[i];

  for (; vobsTimes.size() > 1; vobsTimes.pop_back())
    diutil::delete_all_and_clear(vobsTimes.back().vobsOneTime);
  obsnr = 0;

  // if time of current vop[0] != splot.getTime() or  files have changed,
  // read files from disk
  for (size_t i = 0; i < vop.size(); i++) {
    if (vop[i]->updateObs()) {
      if (!obsm->prepare(vop[i], staticPlot_->getTime()))
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

  for (size_t i = 0; i < vop.size(); i++)
    if (vop[i]->showpos_findObs(x, y))
      found = true;

  return found;
}

bool PlotModule::getObsName(int x, int y, std::string& name)
{
  for (size_t i = 0; i < vop.size(); i++)
    if (vop[i]->getObsName(x, y, name))
      return true;

  return false;
}
std::string PlotModule::getObsPopupText(int x, int y)
{
  int n = vop.size();
  std::string obsText = "";
 
  for (size_t i = 0; i < n; i++)
    if (vop[i]->getObsPopupText(x, y, obsText))
      return obsText;

  return obsText;
}


//areas
void PlotModule::makeAreas(std::string name, std::string areastring, int id)
{
  //   METLIBS_LOG_DEBUG("makeAreas:"<<n);
  //   METLIBS_LOG_DEBUG("name:"<<name);
  //   METLIBS_LOG_DEBUG("areastring:"<<areastring);
  //name can be name:icon
  vector<std::string> tokens = miutil::split(name, ":");
  std::string icon;
  if (tokens.size() > 1) {
    icon = tokens[1];
    name = tokens[0];
  }

  //check if dataset with this id/name already exist
  areaobjects_v::iterator it = vareaobjects.begin();
  while (it != vareaobjects.end() && (id != it->getId() || name != it->getName()))
    ++it;
  if (it != vareaobjects.end()) { //add new areas and replace old areas
    it->makeAreas(name, icon, areastring, id, staticPlot_->getMapArea());
    return;
  }

  //make new dataset
  AreaObjects new_areaobjects;
  new_areaobjects.makeAreas(name, icon, areastring, id, staticPlot_->getMapArea());
  vareaobjects.push_back(new_areaobjects);
}

void PlotModule::areaCommand(const std::string& command, const std::string& dataSet,
    const std::string& data, int id)
{
  //   METLIBS_LOG_DEBUG("PlotModule::areaCommand");
  //   METLIBS_LOG_DEBUG("id=" << id);
  //   METLIBS_LOG_DEBUG("command=" << command);
  //   METLIBS_LOG_DEBUG("data="<<data);

  int n = vareaobjects.size();
  for (int i = 0; i < n && i > -1; i++) {
    if ((id == -1 || id == vareaobjects[i].getId()) && (dataSet == "all"
        || dataSet == vareaobjects[i].getName())) {
      if (command == "delete" && (data == "all" || data.empty())) {
        vareaobjects.erase(vareaobjects.begin() + i);
        i--;
        n = vareaobjects.size();
      } else {
        vareaobjects[i].areaCommand(command, data);
        //zoom to selected area
        if (command == "select" && vareaobjects[i].autoZoom()) {
          vector<std::string> token = miutil::split(data, ":");
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
  METLIBS_LOG_SCOPE();
#endif
  bool found = false;
  for (size_t i = 0; i < locationPlots.size(); i++) {
    if (locationdata.name == locationPlots[i]->getName()) {
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
    LocationPlot* lp = new LocationPlot();
    lp->setData(locationdata);
    locationPlots.push_back(lp);
  }
  setAnnotations();
}

void PlotModule::updateLocation(const LocationData& locationdata)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  const std::string& name = locationdata.name;
  for (size_t i = 0; i < locationPlots.size(); i++) {
    if (name == locationPlots[i]->getName()) {
      locationPlots[i]->updateOptions(locationdata);
    }
  }
}

void PlotModule::deleteLocation(const std::string& name)
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

void PlotModule::setSelectedLocation(const std::string& name,
    const std::string& elementname)
{
  int n = locationPlots.size();
  for (int i = 0; i < n; i++) {
    if (name == locationPlots[i]->getName())
      locationPlots[i]->setSelected(elementname);
  }
}

std::string PlotModule::findLocation(int x, int y, const std::string& name)
{

  int n = locationPlots.size();
  for (int i = 0; i < n; i++) {
    if (name == locationPlots[i]->getName())
      return locationPlots[i]->find(x, y);
  }
  return std::string();
}

//****************************************************

Colour PlotModule::getContrastColour()
{
  return Colour(staticPlot_->getBgColour()).contrastColour();
}

void PlotModule::nextObs(bool next)
{
  for (size_t i = 0; i < vop.size(); i++)
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

  if (vop.empty())
    return;
  if (!editm->isInEdit())
    return;

  if (obsnr == 0 && ke->key() == Qt::Key_Right)
    return;
  if (obsnr > 20 && ke->key() == Qt::Key_Left)
    return;

  obsm->clearObsPositions();

  miTime newTime = staticPlot_->getTime();
  if (ke->key() == Qt::Key_Left) {
    obsnr++;
  } else {
    obsnr--;
  }
  newTime.addHour(-1 * obsTimeStep * obsnr);

  //log old stations
  for (size_t i = 0; i < vop.size(); i++)
    vop[i]->logStations();

  //Make new obsPlot object
  if (obsnr == int(vobsTimes.size())) {

    obsOneTime ot;
    for (size_t i = 0; i < vop.size(); i++) {
      const std::string& pin = vop[i]->getInfoStr();
      ObsPlot *op = obsm->createObsPlot(pin);
      if (op) {
        if (!obsm->prepare(op, newTime))
          METLIBS_LOG_WARN("ObsManager returned false from prepare");
      }
      ot.vobsOneTime.push_back(op);
    }
    vobsTimes.push_back(ot);

  } else {
    for (size_t i = 0; i < vop.size(); i++)
      vobsTimes[obsnr].vobsOneTime[i]->readStations();
  }

  //ask last plot object which stations was plotted,
  //and tell this plot object
  for (size_t i = 0; i < vop.size(); i++) {
    vop[i] = vobsTimes[obsnr].vobsOneTime[i];
  }

  //update list of positions ( used in "PPPP-mslp")
  obsm->updateObsPositions(vop);

  std::string labelstr;
  if (obsnr != 0) {
    std::string timer = miutil::from_number(obsnr * obsTimeStep);
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

void PlotModule::trajPos(vector<std::string>& vstr)
{
  const int n = vtp.size();

  //if vstr starts with "quit", delete all trajectoryPlot objects
  for (size_t j = 0; j < vstr.size(); j++) {
    if (vstr[j].substr(0, 4) == "quit") {
      diutil::delete_all_and_clear(vtp);
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

void PlotModule::measurementsPos(vector<std::string>& vstr)
{
  const int n = vMeasurementsPlot.size();
  //if vstr starts with "quit", delete all MeasurementsPlot objects
  for (size_t j = 0; j < vstr.size(); j++) {
    if (vstr[j].substr(0, 4) == "quit") {
      diutil::delete_all_and_clear(vMeasurementsPlot);
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

bool PlotModule::markAnnotationPlot(int x, int y)
{
  bool marked = false;
  for (size_t j = 0; j < editVap.size(); j++)
    if (editVap[j]->markAnnotationPlot(x, y))
      marked = true;
  return marked;
}

std::string PlotModule::getMarkedAnnotation()
{
  std::string annotext;
  for (size_t j = 0; j < editVap.size(); j++) {
    std::string text = editVap[j]->getMarkedAnnotation();
    if (!text.empty())
      annotext = text;
  }
  return annotext;
}

void PlotModule::changeMarkedAnnotation(std::string text, int cursor, int sel1,
    int sel2)
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->changeMarkedAnnotation(text, cursor, sel1, sel2);
}

void PlotModule::DeleteMarkedAnnotation()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->DeleteMarkedAnnotation();
}

void PlotModule::startEditAnnotation()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->startEditAnnotation();
}

void PlotModule::stopEditAnnotation()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->stopEditAnnotation();
}

void PlotModule::editNextAnnoElement()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->editNextAnnoElement();
}

void PlotModule::editLastAnnoElement()
{
  for (size_t j = 0; j < editVap.size(); j++)
    editVap[j]->editLastAnnoElement();
}

vector<string> PlotModule::writeAnnotations(const string& prodname)
{
  vector<std::string> annostrings;
  for (size_t j = 0; j < editVap.size(); j++) {
    string str = editVap[j]->writeAnnotation(prodname);
    if (not str.empty())
      annostrings.push_back(str);
  }
  return annostrings;
}

void PlotModule::updateEditLabels(const vector<std::string>& productLabelstrings,
    const std::string& productName, bool newProduct)
{
  METLIBS_LOG_SCOPE();
  vector<AnnotationPlot*> oldVap; //display object labels
  //read the old labels...

  vector<std::string> objLabelstrings = objm->getEditObjects().getObjectLabels();
  for (size_t i = 0; i < objLabelstrings.size(); i++) {
    AnnotationPlot* ap = new AnnotationPlot(objLabelstrings[i]);
    oldVap.push_back(ap);
  }

  for (size_t j = 0; j < productLabelstrings.size(); j++) {
    AnnotationPlot* ap = new AnnotationPlot(productLabelstrings[j]);
    ap->setProductName(productName);

    vector<vector<string> > vvstr = ap->getAnnotationStrings();
    for (size_t k = 0; k < vvstr.size(); k++) {
      for (size_t j = 0; j < vfp.size(); j++)
        vfp[j]->getDataAnnotations(vvstr[k]);
    }
    ap->setAnnotationStrings(vvstr);

    // here we compare the labels, take input from oldVap
    for (size_t i = 0; i < oldVap.size(); i++)
      ap->updateInputLabels(oldVap[i], newProduct);

    editVap.push_back(ap);
  }

  diutil::delete_all_and_clear(oldVap);
}

void PlotModule::deleteAllEditAnnotations()
{
  diutil::delete_all_and_clear(editVap);
}

void PlotModule::setObjAuto(bool autoF)
{
  objm->setObjAuto(autoF);
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
    myArea = staticPlot_->getMapArea();
    return;
  }

  if (ke->key() == Qt::Key_F3 || ke->key() == Qt::Key_F4) { // go to previous or next area
    //if last area is not saved, save it
    if (!areaSaved) {
      areaInsert(staticPlot_->getMapArea(), false);
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
    areaInsert(staticPlot_->getMapArea(), true);//save last area
    a = myArea;
  } else if (ke->key() == Qt::Key_F5) { //get predefined areas
    areaInsert(staticPlot_->getMapArea(), true);
    mapm.getMapAreaByFkey("F5", a);
  } else if (ke->key() == Qt::Key_F6) {
    areaInsert(staticPlot_->getMapArea(), true);
    mapm.getMapAreaByFkey("F6", a);
  } else if (ke->key() == Qt::Key_F7) {
    areaInsert(staticPlot_->getMapArea(), true);
    mapm.getMapAreaByFkey("F7", a);
  } else if (ke->key() == Qt::Key_F8) {
    areaInsert(staticPlot_->getMapArea(), true);
    mapm.getMapAreaByFkey("F8", a);
  }

  const bool projChanged = (staticPlot_->getMapArea().P() != a.P());
  staticPlot_->setMapArea(a, keepcurrentarea); // ### only when projChanged == true ?
  PlotAreaSetup();
  if (projChanged) {
    updatePlots();
  } else {
    // reproject items to screen coordinates
    for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it)
      it->second->changeProjection(staticPlot_->getMapArea());
  }
}

void PlotModule::zoomTo(const Rectangle& rectangle)
{
  Area a = staticPlot_->getMapArea();
  a.setR(rectangle);
  staticPlot_->setMapArea(a, false);
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
  areaInsert(staticPlot_->getMapArea(), true);
  Rectangle r(x1, y1, x2, y2);
  PixelArea(r);

  // change the projection for drawing items
  for (managers_t::iterator it = managers.begin(); it != managers.end(); ++it)
    it->second->changeProjection(staticPlot_->getMapArea());
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
      dorubberband = true;
      res.savebackground = true;
      res.background = true;
      res.repaint = true;
      startx = me->x();
      starty = me->y();
      return;

    } else if (me->button() == Qt::MidButton) {
      areaInsert(staticPlot_->getMapArea(), true); // Save last area
      dopanning = true;
      staticPlot_->panPlot(true);
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
      startx = starty = 0;

    } else if (me->button() == Qt::MidButton) {
      dopanning = false;
      staticPlot_->panPlot(false);
      res.repaint = true;
      res.background = true;
      return;
    }
    if (plotnew) {
      //define new plotarea, first save the old one
      areaInsert(staticPlot_->getMapArea(), true);
      Rectangle r(x1, y1, x2, y2);
      PixelArea(r);

      // update the projection for drawing items
      map<string,Manager*>::iterator it = managers.begin();
      while (it != managers.end()) {
        it->second->changeProjection(staticPlot_->getMapArea());
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
      areaInsert(staticPlot_->getMapArea(), true);
      Rectangle r(dx, dy, plotw + dx, ploth + dy);
      PixelArea(r);
    } else if (zoom > 0.) {
      //define new plotarea, first save the old one
      areaInsert(staticPlot_->getMapArea(), true);
      dx = plotw - (plotw * zoom);
      dy = ploth - (ploth * zoom);
      Rectangle r(dx, dy, plotw - dx, ploth - dy);
      PixelArea(r);
    }

    // update the projection for drawing items
    map<string,Manager*>::iterator it = managers.begin();
    while (it != managers.end()) {
      it->second->changeProjection(staticPlot_->getMapArea());
      ++it;
    }
    res.repaint = true;
    res.background = true;
  }
}

vector<std::string> PlotModule::getFieldModels()
{
  std::set<std::string> unique;
  for (size_t i = 0; i < vfp.size(); i++) {
    const std::string& fname = vfp[i]->getModelName();
    if (not fname.empty())
      unique.insert(fname);
  }
  return std::vector<std::string>(unique.begin(), unique.end());
}

vector<std::string> PlotModule::getTrajectoryFields()
{
  vector<std::string> vstr;
  for (size_t i = 0; i < vfp.size(); i++) {
    std::string fname = vfp[i]->getTrajectoryFieldName();
    if (not fname.empty())
      vstr.push_back(fname);
  }
  return vstr;
}

bool PlotModule::startTrajectoryComputation()
{
  if (vtp.size() < 1)
    return false;

  std::string fieldname = miutil::to_lower(vtp[0]->getFieldName());

  int i = 0, n = vfp.size();

  while (i < n && miutil::to_lower(vfp[i]->getTrajectoryFieldName()) != fieldname)
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
bool PlotModule::printTrajectoryPositions(const std::string& filename)
{
  if (vtp.size() > 0)
    return vtp[0]->printTrajectoryPositions(filename);

  return false;
}

/********************* reading and writing log file *******************/

vector<std::string> PlotModule::writeLog()
{
  //put last area in areaQ
  areaInsert(staticPlot_->getMapArea(), true);

  vector<std::string> vstr;

  //Write self-defined area (F2)
  std::string aa = "name=F2 " + myArea.getAreaString();
  vstr.push_back(aa);

  //Write all araes in list (areaQ)
  for (size_t i = 0; i < areaQ.size(); i++) {
    aa = "name=" + miutil::from_number(int(i)) + " " + areaQ[i].getAreaString();
    vstr.push_back(aa);
  }

  return vstr;
}

void PlotModule::readLog(const vector<std::string>& vstr,
    const std::string& thisVersion, const std::string& logVersion)
{
  areaQ.clear();
  Area area;
  for (size_t i = 0; i < vstr.size(); i++) {

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
const vector<FieldPlot*>& PlotModule::getFieldPlots() const
{
  return vfp;
}

const vector<ObsPlot*>& PlotModule::getObsPlots() const
{
  return vop;
}

void PlotModule::getPlotWindow(int &width, int &height)
{
  width = plotw;
  height = ploth;
}
